//
// Created by jjc62351 on 06/08/19.
//

#include "pcsController.h"
#include <asynOctetSyncIO.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <cantProceed.h>
#include <epicsString.h>


/* libxml error functions */
void structuredErrorFunc(void *userData, xmlErrorPtr error) {
}

void genericErrorFunc(void *ctx, const char * msg, ...) {
}

static void udpReadTaskC(void *drvPvt)
{
    pcsController *pPvt = (pcsController *)drvPvt;

    pPvt->udpReadTask();
}

static void eventListenerC(pcsController::portData *data)
{
    pcsController *pPvt = (pcsController *)data->pController;

    pPvt->eventListener(data);
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
                          scale(AXIS_SCALE_FACTOR),
                          clientsConnected(0),
                          axesInitialised(0)
{

    pcsAxis* pAxis;
    asynStatus status;
    static const char *functionName = "pcsController::pcsController";
    createAsynParams();

    driverName = "pcsController";

    /*Supress libxml2 error messages*/
    //xmlGenericErrorFunc handler = (xmlGenericErrorFunc)NULL;
    //initGenericErrorDefaultFunc(&handler);

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
    if (status!=asynSuccess) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s: cannot connect to pcs controller\n",functionName);
    }

    /* Supress libxml2 errors as we expect them*/
    xmlSetGenericErrorFunc(NULL, genericErrorFunc);
    xmlSetStructuredErrorFunc(NULL, structuredErrorFunc);

    // Initial handshaking
    sprintf(outString_,"");
    status = writeController();
    sprintf(outString_,"%s,%.2f,%d",NAME,VERSION,CODE);
    status = writeReadController();

    if(strcmp(inString_,"OK\r\n")) {
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


    // Configure asyn for tcp event stream
    status = configureServer(eventPortName, *&pEventPvt,
                             tcpClientConnectedCallback);
    if (status!=asynSuccess) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s: error configuring tcp event port\n",functionName);
    }

    /*
     * Wait for all TCP clients to be connected. Should be 1 client per axis/controller.
     */
    while(clientsConnected < numAxes){
    }

    // Configure asyn for udp sensor stream
    status = configureServer(streamPortName,*&pStreamPvt,NULL);
    if (status!=asynSuccess) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s: error configuring udp stream port\n",functionName);
    }


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

    startPoller(movingPollPeriod, idlePollPeriod, 2);

    // Start thread for UDP streams
    epicsThreadCreate("UDPStreamTask",
                      epicsThreadPriorityMedium,
                      epicsThreadGetStackSize(epicsThreadStackMedium),
                      udpReadTaskC, this);
    return;
}

pcsController::~pcsController() {}


asynStatus pcsController::poll() {
    callParamCallbacks();
    return asynSuccess;
}

void pcsController::createAsynParams(void){
    int index = 0;
    asynStatus status = asynSuccess;
    status = createParam(PCS_C_FirstParamString,asynParamInt32,&PCS_C_FirstParam);
    status = createParam(PCS_C_SeqStateString,asynParamInt32,&PCS_C_SeqState);
    status = createParam(PCS_C_XmlSequencerString,asynParamOctet,&PCS_C_XmlSequencer);

}

pcsAxis* pcsController::getAxis(asynUser *pasynUser)
{
    return static_cast<pcsAxis*>(asynMotorController::getAxis(pasynUser));
}

pcsAxis* pcsController::getAxis(int axisNo)
{
    return static_cast<pcsAxis*>(asynMotorController::getAxis(axisNo));
}


/*
 * Sets up asyn server and initialises the portData struct so the server can be accessed. Also registers interrupts if needed.
 * @param portname Name of the asyn port to be connected to.
 * @param pPvt Reference to a pointer to a portData structure. This is what will be used to access the port later on
 * @param callBackRoutine Function that will be called if and when registerInterruptUser is called. Intended for setting
 * up the TCP event server. Set to NULL if no callback required (UDP).
 * @return asynStatus Any asyn errors
 */
asynStatus pcsController::configureServer(const char *portname, portData *&pPvt, interruptCallbackOctet callBackRoutine) {
    asynStatus status;
    asynInterface *localAsynInterface;
    static const char *functionName = "pcsController::configureServer";

    pPvt = (portData *)callocMustSucceed(1, sizeof(portData), "ipEchoServer");
    pPvt->pController=this;
    pPvt->mutexId = epicsMutexCreate();
    pPvt->portName = epicsStrDup(portname);
    pPvt->pasynUser = pasynManager->createAsynUser(0,0);
    (pPvt->pasynUser)->userPvt = pPvt;
    status = pasynManager->connectDevice(pPvt->pasynUser,portname,0);
    if (status!=asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                  "%s: cannot connect server\n",functionName);
    }

    localAsynInterface = pasynManager->findInterface(
            pPvt->pasynUser,asynOctetType,1);
    if(!localAsynInterface) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,"%s driver not supported\n",asynOctetType);
        return asynError;
    }

    pPvt->readTimeout = -1.0;
    pPvt->pasynOctet = (asynOctet *)localAsynInterface->pinterface;
    pPvt->octetPvt = localAsynInterface->drvPvt;

    /*
     * If a callback routine has been specified it means we want to register an interrupt
     * and call that routine when an interrupt occurs (TCP client connecting).
     */
    if(callBackRoutine!=NULL) {
        status = pPvt->pasynOctet->registerInterruptUser(
                pPvt->octetPvt, pPvt->pasynUser,
                callBackRoutine, pPvt, &pPvt->registrarPvt);
    }
    if(status!=asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,"ipEchoServer devAsynOctet registerInterruptUser %s\n",
                  pPvt->pasynUser->errorMessage);
    }
    return status;
}

/*
 * Callback method to be called when TCP clients connect. Creates a thread for each client.
 * See interruptCallbackOctet in asynOctet.h
 */
void pcsController::tcpClientConnectedCallback(void *drvPvt, asynUser *pasynUser, char *portName, size_t len, int eomReason) {

    portData  *pPvt = (portData *)drvPvt;
    portData *newPvt = (portData*)calloc(1, sizeof(portData));


    asynPrint(pPvt->pasynUser, ASYN_TRACE_FLOW,
              "ipEchoServer: tcpClientConnectedCallback, portName=%s\n", portName);
    epicsMutexLock(pPvt->mutexId);
    /* Make a copy of portData, with new portName */
    *newPvt = *pPvt;
    epicsMutexUnlock(pPvt->mutexId);
    newPvt->portName = epicsStrDup(portName);
    /* Create a new thread to communicate with this port */
    epicsThreadCreate(pPvt->portName,
                      epicsThreadPriorityMedium,
                      epicsThreadGetStackSize(epicsThreadStackSmall),
                      (EPICSTHREADFUNC) eventListenerC, newPvt);
}

/*
 * Thread method for each TCP client. Listens for events and sets relevant asyn parameters directly
 * @param pPvt Pointer to the portData structure to allow access to the client.
 */
void pcsController::eventListener(pcsController::portData *pPvt) {
    asynUser *pasynUser;
    char handShakeBuf[EVENT_CLIENT_HANDSHAKE_SIZE];
    char rxBuffer[EVENT_PACKET_SIZE];
    size_t nread, nwrite;
    int eomReason;
    int packetIndex;
    asynStatus status;
    eventTCPPacket PACKET;
    pcsAxis* pAxis;

    status = pasynOctetSyncIO->connect(pPvt->portName, 0, &pasynUser, NULL);
    if (status) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                  "eventListener: unable to connect to port %s\n",
                  pPvt->portName);
        return;
    }
    status = pasynOctetSyncIO->setInputEos(pasynUser, "\r\n", 2);
    if (status) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                  "eventListener: unable to set input EOS on %s: %s\n",
                  pPvt->portName, pasynUser->errorMessage);
        return;
    }
    status = pasynOctetSyncIO->setOutputEos(pasynUser, "\r\n", 2);
    if (status) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                  "eventListener: unable to set output EOS on %s: %s\n",
                  pPvt->portName, pasynUser->errorMessage);
        return;
    }

    // Give the client an awkward handshake
    status = pasynOctetSyncIO->write(pasynUser,"HELLO\n",sizeof("HELLO\n"),2,&nwrite);
    status = pasynOctetSyncIO->read(pasynUser,handShakeBuf,EVENT_CLIENT_HANDSHAKE_SIZE,1.0,&nread,&eomReason);
    if (status) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                  "eventListener: TCP port client didn't respond %s: %s\n",
                  pPvt->portName, pasynUser->errorMessage);
        return;
    }

    status = pasynOctetSyncIO->write(pasynUser,"OK\n",sizeof("OK\n"),2,&nwrite);
    printf("Got: %s\n",handShakeBuf);
    clientsConnected++;
    pasynUser->timeout=0.1;

    /*
     * Wait for axes to be initialised.
     */
    while(axesInitialised < numAxes_-1){
    }
    while(1) {
        packetIndex = 0;
        pasynOctetSyncIO->read(pasynUser,rxBuffer,EVENT_PACKET_SIZE,-1,&nread,&eomReason);
        if(nread>0) {
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
            printf("nread:%d\n",nread);
            printf("Server %u,%u,%llu,%u,%u\n",PACKET.ev_code,PACKET.slave,PACKET.ts,PACKET.ev_value,PACKET.num_data);

            lock();
            pAxis = getAxis(PACKET.slave + 1);

            switch(PACKET.ev_code){
                case SEQ_STATE_CHANGE_EVT:
                    if(PACKET.ev_value == 2) {
                        pAxis->setIntegerParam(motorStatusDone_, 1);
                        pAxis->setIntegerParam(PCS_C_SeqState,0);
                    }
                    if(PACKET.ev_value == 3) {
                        pAxis->setIntegerParam(motorStatusDone_, 0);
                        pAxis->setIntegerParam(PCS_C_SeqState,1);
                    }
                    break;
                case PLIM_STATE_CHANGE_EVT:
                    pAxis->setDoubleParam(motorHighLimit_,PACKET.ev_value^1);
                    printf("Setting PLIM to %d\n",PACKET.ev_value^1);
                    break;
                case NLIM_STATE_CHANGE_EVT:
                    pAxis->setDoubleParam(motorLowLimit_,PACKET.ev_value^1);
                    printf("Setting NLIM to %d\n",PACKET.ev_value^1);
                    break;
                default:
                    break;
            }


            unlock();
        }
    }
}

/*
 * Thread method to get information from the UDP stream port. Updates the asyn parameters directly but doesn't call
 * callParamCallbacks() as this stream is likely to be in the order of kHz, callParamCallbacks() is called in the
 * poll() method.
 */
void pcsController::udpReadTask() {

    char rxBuffer[UDP_PACKET_SIZE];
    asynStatus status = asynSuccess;
    static const char *functionName = "udpReadTask";
    streamUDPPacket PACKET;
    size_t nBytesIn;
    int eomReason;
    int packetIndex = 0;
    int scale;
    pcsAxis* pAxis;

    while(1){
        packetIndex = 0;

        //Read UDP Packet
        status = pStreamPvt->pasynOctet->read(pStreamPvt->octetPvt,pStreamPvt->pasynUser,rxBuffer,UDP_PACKET_SIZE,&nBytesIn,&eomReason);

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


asynStatus pcsController::writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual) {

    /*
     * Need method to determine if string resembles a sequencer
     */
    if(pasynUser->reason == PCS_C_XmlSequencer){

    }

    *nActual = strlen(value);
    return asynSuccess;
}


/*
 * Method to populate outString_ with an XML command and send to the controller.
 * Looks up the parameter in the commandConstructor member with the unique parameter key.
 * @param axisNo Axis number
 * @param parameter Unique parameter key
 * @param value The value of the parameter
 */
template <typename T>
asynStatus pcsController::sendXmlCommand(int axisNo, const std::string &parameter, T value) {

    asynStatus status = asynSuccess;
    sprintf(outString_, commandConstructor.getXml(axisNo, parameter,value).c_str());
    status = writeReadController();
    if (status != asynSuccess) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "sendXmlCommand: writeReadController error\n");
        return status;
    }
    return status;
}

/*
 * Method to populate outString_ with an XML command and send to the controller.
 * Looks up the parameter in the commandConstructor member with the unique parameter key.
 * This variant is for a parameter with no value, eg, <udpxmit><slave>0</slave><clear></clear></udpxmit>
 * @param axisNo Axis number
 * @param parameter Unique parameter key
 */
asynStatus pcsController::sendXmlCommand(int axisNo, const std::string &parameter) {

    asynStatus status = asynSuccess;
    sprintf(outString_, commandConstructor.getXml(axisNo, parameter).c_str());
    status = writeReadController();
    if (status != asynSuccess) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "sendXmlCommand: writeReadController error\n");
        return status;
    }
    return status;
}


asynStatus pcsController::writeReadController() {

    asynStatus status = asynSuccess;
    char* eos;
    char rxBuffer[2048];
    int xmlEosCounter,commandLength,eomReason,returnSize = 2048;
    size_t  nwrite,nread;
    size_t nbytesOut;
    size_t nbytesIn;
    commandLength = strlen(outString_);


    if(Sequencer::isStringXML(outString_)){

        xmlEosCounter = commandLength-1;
        /* Find the start of the last xml element */
        while(outString_[xmlEosCounter] != '<'){
            xmlEosCounter--;
        }
        /* EOS is the last element in the XML */
        eos=&outString_[xmlEosCounter];

    }else{
        /* Command is not XML so EOS is the usual \r\n */
        eos="\r\n";
    }

    status = pasynOctetSyncIO->writeRead(pasynUserController_,
                                         outString_,
                                         strlen((outString_)),
                                         rxBuffer,
                                         returnSize,
                                         2.0,
                                         &nbytesOut,
                                         &nbytesIn,
                                         &eomReason);

    if ( status != asynSuccess ) {
        asynPrint(pasynUserController_, ASYN_TRACE_ERROR,
                  "SendAndReceive error calling writeRead, output=%s status=%d, error=%s\n",
                  outString_, status, pasynUserController_->errorMessage);
    }
    asynPrint(pasynUserController_, ASYN_TRACEIO_DRIVER,
              "SendAndReceive, sent: '%s', received: '%s'\n",
              outString_, rxBuffer);
    nread = nbytesIn;
    /* Loop until we the response contains the eos or we get an error */
    while ((status==asynSuccess) &&
           (strcmp(rxBuffer + nread - strlen(eos), eos) != 0)) {
        status = pasynOctetSyncIO->read(pasynUserController_,
                                        &rxBuffer[nread],
                                        returnSize-nread,
                                        0.5,
                                        &nbytesIn,
                                        &eomReason);
        asynPrint(pasynUserController_, ASYN_TRACEIO_DRIVER,
              "SendAndReceive, received: nread=%d, returnSize-nread=%d, nbytesIn=%d\n",
              (int)nread, returnSize-nread, (int)nbytesIn);
        nread += nbytesIn;
    }


    sprintf(inString_,rxBuffer);
    return status;

}

void pcsController::_errorFunc(void *ctxt, char *msg, ...) {
    printf("Error func\n");
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