//
// Created by jjc62351 on 06/08/19.
//

#include "pcsController.h"
#include <asynOctetSyncIO.h>
#include <iocsh.h>
#include <epicsExport.h>

pcsController::pcsController(const char *portName, const char *lowLevelPortName, int lowLevelPortAddress, int numAxes,
                              double movingPollPeriod, double idlePollPeriod)
    : asynMotorController(portName, numAxes + 1, NUM_MOTOR_DRIVER_PARAMS +50,
                              asynEnumMask | asynInt32ArrayMask, // For user mode and velocity mode
                              asynEnumMask, // No addition interrupt interfaces
                              ASYN_CANBLOCK | ASYN_MULTIDEVICE,
                              1, // autoconnect
                              0, 50000)
{

    printf("pcsController created");
    pAxes_ = (pcsAxis **) (asynMotorController::pAxes_);
    createAsynParams();
}

pcsController::~pcsController() {}

asynStatus pcsController::writeFloat64(asynUser *pasynUser, epicsFloat64 value) {
    int function = pasynUser->reason;
    const char *name[128];
    bool status = true;
    pcsAxis *pAxis = NULL;

    getParamName(function, name);
    pAxis = this->getAxis(pasynUser);
    if (!pAxis) {
        return asynError;
    }
    printf("Axis number is :%d\n",pAxis->axisNo_);

    printf("writeFloat64 called with function : %s\n",*name);
    if (function == motorPosition_) printf("motorPosition_\n");
    if (function == motorLowLimit_) printf("motorLowLimit_\n");
    if (function == motorHighLimit_) printf("motorHighLimit_\n");

    /* Set the parameter and readback in the parameter library. */
    status = (pAxis->setDoubleParam(function, value) == asynSuccess) && status;

    /* Call base class for anything missed in this controller implementation*/
    status = (asynMotorController::writeFloat64(pasynUser, value) == asynSuccess) && status;

}
asynStatus pcsController::writeInt32(asynUser *pasynUser, epicsInt32 value) {

    int function = pasynUser->reason;
    const char *name[128];
    bool status = true;
    pcsAxis *pAxis = NULL;

    getParamName(function, name);
    pAxis = this->getAxis(pasynUser);
    if (!pAxis) {
        return asynError;
    }

    status = (pAxis->setIntegerParam(function, value) == asynSuccess) && status;
    printf("writeInt32 called\n");
}

asynStatus pcsController::writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual) {
    printf("writeOctet called\n");
}
asynStatus pcsController::poll() {
    printf("poll called\n");
}

void pcsController::createAsynParams(void){
  int index = 0;

  createParam(PCS_C_FirstParamString,asynParamInt32,&PCS_C_FirstParam);

}


pcsAxis *pcsController::getAxis(asynUser *pasynUser) {
    int axisNo = 0;

    getAddress(pasynUser, &axisNo);
    return getAxis(axisNo);
}

pcsAxis *pcsController::getAxis(int axisNo) {
    if((axisNo < 1) || (axisNo > numAxes_)){
        return NULL;
    }else{
        return pAxes_[axisNo];
    }
}












/** Configuration command, called directly or from iocsh.
  * \param[in] portName The name of this asyn port.
  * \param[in] serialPortName The name of the serial port connected to the device.
  * \param[in] serialPortAddress The address of the serial port (usually 0).
  */
extern "C" int pcsControllerConfig(const char *portName, const char *lowLevelPortName, int lowLevelPortAddress,
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
        new pcsController(portName, lowLevelPortName,
                          lowLevelPortAddress, numAxes,
                          movingPollPeriod / 1000.,
                          idlePollPeriod / 1000.);
    }
    return result;
}

/* Code for iocsh registration for pcsController*/
static const iocshArg pcsControllerConfigArg0 = {"Port name", iocshArgString};
static const iocshArg pcsControllerConfigArg1 = {"Low level port name", iocshArgString};
static const iocshArg pcsControllerConfigArg2 = {"Low level port address", iocshArgInt};
static const iocshArg pcsControllerConfigArg3 = {"Number of axes", iocshArgInt};
static const iocshArg pcsControllerConfigArg4 = {"Moving poll period (ms)", iocshArgInt};
static const iocshArg pcsControllerConfigArg5 = {"Idle poll period (ms)", iocshArgInt};

static const iocshArg* const pcsControllerConfigArgs[] =
    {&pcsControllerConfigArg0, &pcsControllerConfigArg1, &pcsControllerConfigArg2, &pcsControllerConfigArg3, &pcsControllerConfigArg4,
        &pcsControllerConfigArg5};

static const iocshFuncDef configPcsController =
    {"pcsControllerConfig", 6, pcsControllerConfigArgs};

static void configPcsControllerCallFunc(const iocshArgBuf *args)
{
    pcsControllerConfig(args[0].sval, args[1].sval, args[2].ival, args[3].ival,
        args[4].ival, args[5].ival);
}

static void PcsControllerRegister(void)
{
    iocshRegister(&configPcsController, configPcsControllerCallFunc);
}


extern "C" { epicsExportRegistrar(PcsControllerRegister); }