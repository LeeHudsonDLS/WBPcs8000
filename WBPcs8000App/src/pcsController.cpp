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

}

pcsController::~pcsController() {}

asynStatus pcsController::writeFloat64(asynUser *pasynUser, epicsFloat64 value) {
    printf("writeFloat64 called\n");
}
asynStatus pcsController::writeInt32(asynUser *pasynUser, epicsInt32 value) {
    printf("writeInt32 called\n");
}

asynStatus pcsController::writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual) {
    printf("writeOctet called\n");
}
asynStatus pcsController::poll() {
    printf("poll called\n");
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

/** Axis configuration command, called directly or from iocsh.
  * \param[in] portName The name of the controller asyn port.
  * \param[in] axisNum The number of the axis, zero based.
  * \param[in] homeMode The homing mode of the axis
  */
extern "C" int pcsAxisConfig(const char *controllerName, int axisNum)
{
    int result = asynSuccess;
    pcsController* controller = (pcsController*)findAsynPortDriver(controllerName);
    if(controller == NULL)
    {
        printf("Could not find a GCS controller with this port name\n");
        result = asynError;
    }
    else
    {
        new pcsAxis(controller, axisNum);
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




/* Code for iocsh registration for pcsAxis*/
/*
 * Following parameters required for asynMotorController constructor:
 * name of associated asynMotorController char*
 * axis number int
 */

static const iocshArg pcsAxisConfigArg0 = {"Controller port name", iocshArgString};
static const iocshArg pcsAxisConfigArg1 = {"Axis number", iocshArgInt};

static const iocshArg *const pcsAxisConfigArgs[] = {&pcsAxisConfigArg0,
                                                     &pcsAxisConfigArg1};

static const iocshFuncDef configPcsAxis =
    {"pcsAxisConfig", 2, pcsAxisConfigArgs};

static void configPcsAxisCallFunc(const iocshArgBuf *args)
{
    pcsAxisConfig(args[0].sval, args[1].ival);
}


static void PcsRegister(void)
{
    iocshRegister(&configPcsController, configPcsControllerCallFunc);
    iocshRegister(&configPcsAxis, configPcsAxisCallFunc);
}


extern "C" { epicsExportRegistrar(PcsRegister); }