//
// Created by jjc62351 on 06/08/19.
//

#include "pcsController.h"
#include <asynOctetSyncIO.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <cantProceed.h>
#include <epicsString.h>

static void udpReadTaskC(void *drvPvt)
{
    pcsController *pPvt = (pcsController *)drvPvt;

    pPvt->udpReadTask();
}

pcsController::pcsController(const char *portName, int lowLevelPortAddress, int numAxes,
                              double movingPollPeriod, double idlePollPeriod)
    : asynMotorController(portName, numAxes + 1, NUM_MOTOR_DRIVER_PARAMS + NUM_OF_PCS_PARAMS,
                          0,
                          0,
                              ASYN_CANBLOCK | ASYN_MULTIDEVICE,
                          1, // autoconnect
                              0,
                          0),
                          commandConstructor(*this),
                          scale(AXIS_SCALE_FACTOR)
{

    pcsAxis* pAxis;
    asynStatus status;
    static const char *functionName = "pcsController::pcsController";
    createAsynParams();

    driverName = "pcsController";
    //Add portname suffix
    lowLevelPortName = (char*)malloc(strlen(portName)+strlen(MAIN_PORT_SUFFIX)+1);
    streamPortName = (char*)malloc(strlen(portName)+strlen(STREAMS_PORT_SUFFIX)+1);
    eventPortName = (char*)malloc(strlen(portName)+strlen(EVENT_PORT_SUFFIX)+1);
    //Set port names
    sprintf(lowLevelPortName,"%s_CTRL",portName);
    sprintf(streamPortName,"%s_UDP",portName);
    sprintf(eventPortName,"%s_TCP",portName);


    /* Connect to pcsController controller via main tcp port */
    status = pasynOctetSyncIO->connect(lowLevelPortName, 0, &pasynUserController_, NULL);
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s: cannot connect to pcs controller\n",functionName);
    }

    // Initial handshaking
    sprintf(outString_,"");
    writeReadController();
    sprintf(outString_,"%s,%.2f,%d",NAME,VERSION,CODE);
    writeReadController();

    if(strcmp(inString_,"OK")) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s: Handshake with controller failed\n", functionName);
        status = asynError;
    }

    //Construct XML commands to be used later on
    //Parameters that do not require a value
    commandConstructor.addParameter(START_UDP_CMD,"udpxmit,start");
    commandConstructor.addParameter(CLEAR_UDP_CMD,"udpxmit,clear");

    //Parameters that require a value
    commandConstructor.addParameter(SEQ_CONTROL_PARAM,"sequencer,set,seq_state");       // Set to "program" to start sequence
    commandConstructor.addParameter(REGISTER_STREAM_PARAM, "udpxmit,register,stream");  // Set to stream required, eg, phys14
    commandConstructor.addParameter(SYS_STATE_PARAM, "maincontrol,set,sys_state");      // Set to "Ready" to enable drives

    //Inputs where the value is the input pin number
    commandConstructor.addInputParameter(POS_LIMIT_INPUT,GET_INPUT,2);
    commandConstructor.addInputParameter(NEG_LIMIT_INPUT,GET_INPUT,3);
    commandConstructor.addInputParameter(DRV_READY_INPUT,GET_INPUT,1);


    //Configure UDP streams and generic parameters for all axes
    for(int i = 0; i < numAxes; i++) {
        status = sendXmlCommand(i+1,CLEAR_UDP_CMD);
        status = sendXmlCommand(i+1,REGISTER_STREAM_PARAM,"phys14");

        // Enable motor
        status = sendXmlCommand(i+1,SYS_STATE_PARAM,"Ready");
    }

    //Start UDP
    for(int i = 0; i < numAxes; i++) {
        status = sendXmlCommand(i+1,START_UDP_CMD);
    }


    // Configure asyn for udp sensor stream
    configureServer(streamPortName,*&pStreamPvt,*&pasynUserUDPStream,*&pasynInterface);

    // Configure asyn for tcp event stream
    configureServer(eventPortName,*&pEventPvt,*&pasynUserEventStream,*&pasynInterfaceEvent);

    // Register interrupts on tcp server to allow action to be taken when client connects
    status = pEventPvt->pasynOctet->registerInterruptUser(
            pEventPvt->octetPvt, pasynUserEventStream,
            connectionCallback,pEventPvt,&pEventPvt->registrarPvt);

    if(status!=asynSuccess) {
        printf("ipEchoServer devAsynOctet registerInterruptUser %s\n",
               pasynUserEventStream->errorMessage);
    }

    startPoller(movingPollPeriod, idlePollPeriod, 2);

    epicsThreadCreate("UDPStreamTask",
                      epicsThreadPriorityMedium,
                      epicsThreadGetStackSize(epicsThreadStackMedium),
                      udpReadTaskC, this);



    return;
}

pcsController::~pcsController() {}

asynStatus pcsController::configureServer(const char *portname, myData *&pPvt, asynUser *&pasynUser, asynInterface *&pasynInterface) {
    asynStatus status;
    static const char *functionName = "pcsController::configureEventStream";


    pPvt = (myData *)callocMustSucceed(1, sizeof(myData), "ipEchoServer");
    pPvt->mutexId = epicsMutexCreate();
    pPvt->portName = epicsStrDup(portname);
    pasynUser = pasynManager->createAsynUser(0,0);
    (pasynUser)->userPvt = pPvt;
    status = pasynManager->connectDevice(pasynUser,portname,0);
    if(status!=asynSuccess) {
        printf("can't connect to port %s: %s\n", portname, pasynUser->errorMessage);
        return asynError;
    }
    pasynInterface = pasynManager->findInterface(
            pasynUser,asynOctetType,1);
    if(!pasynInterface) {
        printf("%s driver not supported\n",asynOctetType);
        return asynError;
    }

    pPvt->readTimeout = -1.0;

    pPvt->pasynOctet = (asynOctet *)pasynInterface->pinterface;
    pPvt->octetPvt = pasynInterface->drvPvt;
    return status;

}


void pcsController::echoListener(pcsController::myData *pPvt) {
    asynUser *pasynUser;
    char buffer[1024];
    char rxBuffer[1024];
    size_t nread, nwrite;
    int eomReason;
    int packetIndex;
    asynStatus status;
    eventPacket PACKET;
    asynOctet *pasynOctet;

    printf("asyn client registerd: %s\n",pPvt->portName);
    status = pasynOctetSyncIO->connect(pPvt->portName, 0, &pasynUser, NULL);
    if (status) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                  "echoListener: unable to connect to port %s\n",
                  pPvt->portName);
        return;
    }
    status = pasynOctetSyncIO->setInputEos(pasynUser, "\r\n", 2);
    if (status) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                  "echoListener: unable to set input EOS on %s: %s\n",
                  pPvt->portName, pasynUser->errorMessage);
        return;
    }
    status = pasynOctetSyncIO->setOutputEos(pasynUser, "\r\n", 2);
    if (status) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                  "echoListener: unable to set output EOS on %s: %s\n",
                  pPvt->portName, pasynUser->errorMessage);
        return;
    }

    pasynOctetSyncIO->write(pasynUser,"HELLO\n",sizeof("HELLO\n"),2,&nwrite);
    pasynOctetSyncIO->read(pasynUser,buffer,1024,2.0,&nread,&eomReason);
    pasynOctetSyncIO->write(pasynUser,"OK\n",sizeof("OK\n"),2,&nwrite);
    printf("Got: %s\n",buffer);


    pasynUser->timeout=0.1;

    while(1) {
        packetIndex = 0;
        pasynOctetSyncIO->read(pasynUser,rxBuffer,1024,0.1,&nread,&eomReason);
        //printf("%s : nread : %d\n",pPvt->portName,nread);

        if(nread>0) {
            printf("Event!! %d\n",nread);
            //Manually unpack datagram
            memcpy(&PACKET.ev_code, &rxBuffer[packetIndex], 4);
            packetIndex += 4;
            memcpy(&PACKET.slave, &rxBuffer[packetIndex], 4);
            packetIndex += 4;
            memcpy(&PACKET.ts, &rxBuffer[packetIndex], 8);
            packetIndex += 8;
            memcpy(&PACKET.ev_value, &rxBuffer[packetIndex], 4);
            packetIndex += 4;
            memcpy(&PACKET.num_data, &rxBuffer[packetIndex], 4);
            packetIndex += 4;
            printf("Server %u,%u,%llu,%u,%u\n",PACKET.ev_code,PACKET.slave,PACKET.ts,PACKET.ev_value,PACKET.num_data);
        }

    }

}

void pcsController::connectionCallback(void *drvPvt, asynUser *pasynUser, char *portName, size_t len, int eomReason) {

    myData  *pPvt = (myData *)drvPvt;
    myData *newPvt = (myData*)calloc(1, sizeof(myData));
    size_t bytes;
    int nreason;
    char buffer[10240];



    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "ipEchoServer: connectionCallback, portName=%s\n", portName);
    printf("TCP  connection callback");
    epicsMutexLock(pPvt->mutexId);
    /* Make a copy of myData, with new portName */
    *newPvt = *pPvt;
    epicsMutexUnlock(pPvt->mutexId);
    newPvt->portName = epicsStrDup(portName);
    /* Create a new thread to communicate with this port */
    epicsThreadCreate(pPvt->portName,
                      epicsThreadPriorityMedium,
                      epicsThreadGetStackSize(epicsThreadStackSmall),
                      (EPICSTHREADFUNC)echoListener, newPvt);
}


asynStatus pcsController::poll() {
    callParamCallbacks();
    asynStatus status;

}



void pcsController::createAsynParams(void){
  int index = 0;

  createParam(PCS_C_FirstParamString,asynParamInt32,&PCS_C_FirstParam);

}

/** Returns a pointer to an MCB4BAxis object.
  * Returns NULL if the axis number encoded in pasynUser is invalid.
  * \param[in] pasynUser asynUser structure that encodes the axis index number. */
pcsAxis* pcsController::getAxis(asynUser *pasynUser)
{
    return static_cast<pcsAxis*>(asynMotorController::getAxis(pasynUser));
}

/** Returns a pointer to an MCB4BAxis object.
  * Returns NULL if the axis number encoded in pasynUser is invalid.
  * \param[in] axisNo Axis index number. */
pcsAxis* pcsController::getAxis(int axisNo)
{
    return static_cast<pcsAxis*>(asynMotorController::getAxis(axisNo));
}


void pcsController::udpReadTask() {

    char rxBuffer[65535];
    asynStatus status = asynSuccess;
    static const char *functionName = "udpReadTask";
    udpPacket PACKET;
    size_t nBytesIn;
    int eomReason;
    int packetIndex = 0;
    int scale;
    pcsAxis* pAxis;

    while(1){
        packetIndex = 0;

        //Read UDP Packet
        status = pStreamPvt->pasynOctet->read(pStreamPvt->octetPvt,pasynUserUDPStream,rxBuffer,65535-1,&nBytesIn,&eomReason);

        if(nBytesIn>0) {
            //Manually unpack datagram
            memcpy(&PACKET.code, &rxBuffer[packetIndex], 4);
            packetIndex += 4;
            memcpy(&PACKET.slave, &rxBuffer[packetIndex], 4);
            packetIndex += 4;
            memcpy(&PACKET.nData, &rxBuffer[packetIndex], 4);
            packetIndex += 4;
            memcpy(&PACKET.pkgIndex, &rxBuffer[packetIndex], 8);
            packetIndex += 8;
            memcpy(&PACKET.ts, &rxBuffer[packetIndex], 8);
            packetIndex += 8;
            memcpy(&PACKET.minDrag, &rxBuffer[packetIndex], 4);
            packetIndex += 4;
            memcpy(&PACKET.maxDrag, &rxBuffer[packetIndex], 4);
            packetIndex += 4;
            memcpy(&PACKET.data, &rxBuffer[packetIndex], 4);
            packetIndex += 4;

            if (PACKET.code == POSITION_UDP_STREAM_CODE) {
                lock();
                pAxis = getAxis(PACKET.slave + 1);
                pAxis->setDoubleParam(motorPosition_, PACKET.data * pAxis->scale_);
                unlock();
            }
        }
    }

}




asynStatus pcsController::sendXmlCommand(const std::string& parameter) {
    asynStatus status;
    status = pasynOctetSyncIO->setInputEos(pasynUserController_, parameter.c_str(),2);
    if (status != asynSuccess) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "sendXmlCommand: error setting EOS\n");
        return status;
    }
    status = writeReadController();
    if (status != asynSuccess) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "sendXmlCommand: writeReadController error\n");
        return status;
    }
    return status;

}

template <typename T>
asynStatus pcsController::sendXmlCommand(int axisNo, const std::string &parameter, T value) {

    sprintf(outString_, commandConstructor.getXml(axisNo, parameter,value).c_str());
    return sendXmlCommand(commandConstructor.getEos(parameter));
}

asynStatus pcsController::sendXmlCommand(int axisNo, const std::string &parameter) {

    sprintf(outString_, commandConstructor.getXml(axisNo, parameter).c_str());
    return sendXmlCommand(commandConstructor.getEos(parameter));
}


/** Configuration command, called directly or from iocsh.
  * \param[in] portName The name of this asyn port.
  * \param[in] serialPortName The name of the serial port connected to the device.
  * \param[in] serialPortAddress The address of the serial port (usually 0).
  */
extern "C" int pcsControllerConfig(const char *portName, int lowLevelPortAddress,
                                   int numAxes, int movingPollPeriod, int idlePollPeriod)
{
    int result = asynSuccess;
    pcsController* existing = (pcsController*)findAsynPortDriver(portName);
    if(existing != NULL)
    {
        printf("A pcs controller already exists with this port name\n");
        result = asynError;
    }
    else
    {
        new pcsController(portName,
                          lowLevelPortAddress, numAxes,
                          movingPollPeriod / 1000.,
                          idlePollPeriod / 1000.);
    }
    return result;
}





/* Code for iocsh registration for pcsController*/
static const iocshArg pcsControllerConfigArg0 = {"Port name", iocshArgString};
static const iocshArg pcsControllerConfigArg1 = {"Low level port address", iocshArgInt};
static const iocshArg pcsControllerConfigArg2 = {"Number of axes", iocshArgInt};
static const iocshArg pcsControllerConfigArg3 = {"Moving poll period (ms)", iocshArgInt};
static const iocshArg pcsControllerConfigArg4 = {"Idle poll period (ms)", iocshArgInt};

static const iocshArg* const pcsControllerConfigArgs[] =
    {&pcsControllerConfigArg0, &pcsControllerConfigArg1, &pcsControllerConfigArg2, &pcsControllerConfigArg3, &pcsControllerConfigArg4};

static const iocshFuncDef configPcsController =
    {"pcsControllerConfig", 5, pcsControllerConfigArgs};

static void configPcsControllerCallFunc(const iocshArgBuf *args)
{
    pcsControllerConfig(args[0].sval, args[1].ival, args[2].ival,
        args[3].ival, args[4].ival);
}

static void PcsControllerRegister(void)
{
    iocshRegister(&configPcsController, configPcsControllerCallFunc);
}


extern "C" { epicsExportRegistrar(PcsControllerRegister); }