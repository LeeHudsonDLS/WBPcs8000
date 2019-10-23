//
// Created by jjc62351 on 06/08/19.
//

#include "pcsController.h"
#include <asynOctetSyncIO.h>
#include <iocsh.h>
#include <epicsExport.h>



pcsController::pcsController(const char *portName, int lowLevelPortAddress, int numAxes,
                              double movingPollPeriod, double idlePollPeriod)
    : asynMotorController(portName, numAxes + 1, NUM_MOTOR_DRIVER_PARAMS + NUM_OF_PCS_PARAMS,
                          0,
                          0,
                              ASYN_CANBLOCK | ASYN_MULTIDEVICE,
                          1, // autoconnect
                              0,
                          0),commandConstructor(*this)
{
    size_t nwrite;
    asynStatus status;
    static const char *functionName = "pcsController::pcsController";
    createAsynParams();
    char buffer[1024];
    std::string temp;
    int a;
    int test;



    //Add portname suffix
    lowLevelPortName = (char*)malloc(strlen(portName)+strlen(MAIN_PORT_SUFFIX)+1);
    streamPortName = (char*)malloc(strlen(portName)+strlen(STREAMS_PORT_SUFFIX)+1);
    eventPortName = (char*)malloc(strlen(portName)+strlen(EVENT_PORT_SUFFIX)+1);

    sprintf(lowLevelPortName,"%s_CTRL",portName);
    sprintf(streamPortName,"%s_UDP",portName);
    sprintf(eventPortName,"%s_TCP",portName);


    /* Connect to pcsController controller */
    status = pasynOctetSyncIO->connect(lowLevelPortName, 0, &pasynUserController_, NULL);

    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s: cannot connect to pcs controller\n",functionName);
    }
    // Initial handshaking
    sprintf(outString_,"");
    writeReadController();

    printf("Debug1: %s\n",inString_);

    sprintf(outString_,"%s,%.2f,%d",NAME,VERSION,CODE);
    writeReadController();

    printf("Debug2: %04x\n",inString_[0]);


    if(strcmp(inString_,"OK"))
        status=asynError;

    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s: Handshake with controller failed\n",functionName);
    }

    //Parameters that do not require a value
    commandConstructor.addParameter(START_UDP_CMD,"udpxmit,start");
    commandConstructor.addParameter(CLEAR_UDP_CMD,"udpxmit,clear");

    //Parameters that require a value
    commandConstructor.addParameter(SEQ_CONTROL_PARAM,"sequencer,set,seq_state");       // Set to "program" to start sequence
    commandConstructor.addParameter(REGISTER_STREAM_PARAM, "udpxmit,register,stream");  // Set to stream required, eg, phys14
    commandConstructor.addParameter(SYS_STATE_PARAM, "maincontrol,set,sys_state");      // Set to "Ready" to enable drives

    commandConstructor.addInputParameter(POS_LIMIT_INPUT,GET_INPUT,2);
    commandConstructor.addInputParameter(NEG_LIMIT_INPUT,GET_INPUT,3);
    commandConstructor.addInputParameter(DRV_READY_INPUT,GET_INPUT,1);



    sprintf(outString_,commandConstructor.getXml(1,CLEAR_UDP_CMD).c_str());

    status = pasynOctetSyncIO->setInputEos(pasynUserController_,commandConstructor.getEos(CLEAR_UDP_CMD).c_str(),2);

    status=writeReadController();
    inString_[0]='\0';

    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s: writeReadController timeout\n",functionName);
    }
    sprintf(outString_,commandConstructor.getXml(1,REGISTER_STREAM_PARAM,"phys14").c_str());
    //pasynOctetSyncIO->setInputEos(pasynUserController_,commandConstructor.getEos(REGISTER_STREAM_PARAM).c_str(),2);
    status = pasynOctetSyncIO->setInputEos(pasynUserController_,"</>",2);
    //pasynOctetSyncIO->getInputEos(pasynUserController_,buffer,1024,&test);
    status=writeReadController();

    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s: writeReadController timeout\n",functionName);
    }



    sprintf(outString_,commandConstructor.getXml(1,START_UDP_CMD).c_str());
    pasynOctetSyncIO->setInputEos(pasynUserController_,commandConstructor.getEos(START_UDP_CMD).c_str(),2);
    printf("!!!!!!!!!!!!!!!!%s\n",outString_);
    status=writeReadController();

    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s: writeReadController timeout\n",functionName);
    }
    printf("Got %s \n",inString_);

    /*
    pasynUserUDPStream = pasynManager->createAsynUser(0, 0);
	status = pasynManager->connectDevice(pasynUserUDPStream, streamPortName, 0);

    pasynUserUDPStream->drvUser = (void *) this;
	if (status != asynSuccess) {
		asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s: cannot connect to UDP\n",functionName);
		return;
	}*/


    //commandConstructor.dumpXml();
    startPoller(movingPollPeriod, idlePollPeriod, 2);

}

pcsController::~pcsController() {}



asynStatus pcsController::poll() {
    size_t nbytes;
    int nreason;
    char buffer[1024];

    //pasynOctet->read(pasynUserUDPStream->drvUser,pasynUserUDPStream,buffer,1024,&nbytes,&nreason);

    return asynSuccess;
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