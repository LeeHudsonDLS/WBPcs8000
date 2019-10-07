//
// Created by jjc62351 on 06/08/19.
//

#include "pcsController.h"
#include <asynOctetSyncIO.h>
#include <iocsh.h>
#include <epicsExport.h>

pcsController::pcsController(const char *portName, const char *lowLevelPortName, int lowLevelPortAddress, int numAxes,
                              double movingPollPeriod, double idlePollPeriod)
    : asynMotorController(portName, numAxes + 1, NUM_MOTOR_DRIVER_PARAMS + NUM_OF_PCS_PARAMS,
                              0,
                              0,
                              ASYN_CANBLOCK | ASYN_MULTIDEVICE,
                              1, // autoconnect
                              0,
                              0)
{
    asynStatus status;
    static const char *functionName = "pcsController::pcsController";
    createAsynParams();

    /* Connect to pcsController controller */
    status = pasynOctetSyncIO->connect(lowLevelPortName, 0, &pasynUserController_, NULL);
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
      "%s: cannot connect to pcs controller\n",functionName);
    }
    startPoller(movingPollPeriod, idlePollPeriod, 2);
}

pcsController::~pcsController() {}



asynStatus pcsController::poll() {

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