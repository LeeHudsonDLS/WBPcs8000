//
// Created by jjc62351 on 06/08/19.
//

#include "pcsAxis.h"
#include "pcsController.h"
#include <stdlib.h>
#include <epicsThread.h>
#include <sstream>
#include <iostream>
#include <iocsh.h>
#include <epicsExport.h>

#include <epicsExport.h>
pcsAxis::pcsAxis(pcsController *ctrl, int axisNo)
        :asynMotorAxis((asynMotorController *) ctrl, axisNo),
        ctrl_(ctrl){

    static const char *functionName = "pcsAxis::pcsAxis";
    asynPrint(ctrl_->pasynUserSelf, ASYN_TRACE_FLOW, "%s\n",functionName);

    pmacHeader[0] = '\x8B';
    pmacHeader[1] = '\x40';
    pmacHeader[2] = '\xBF';
    pmacHeader[3] = '\x00';
    pmacHeader[4] = '\x00';
    pmacHeader[5] = '\x00';
    pmacHeader[6] = '\x00';
    pmacHeader[7] = '\x00';
    //Initialize non-static data members
    velocity_ = 0.0;
    accel_ = 0.0;

    initialise(axisNo_);

    asynStatus status = ctrl_->writeReadController();

}

void pcsAxis::initialise(int axisNo) {
    static const char *functionName = "pcsAxis::initialise";
    printf("Axis %d created \n",axisNo);
}


asynStatus pcsAxis::move(double position, int relative, double minVelocity, double maxVelocity, double acceleration){

    double distance = 0;
    asynStatus status = asynError;
    static const char *functionName = "move";

    char commandBuffer[255] = {0};

    printf("pcsAxis::move() called\n");

    asynPrint(ctrl_->pasynUserSelf, ASYN_TRACE_FLOW, "%s\n", functionName);

    setIntegerParam(ctrl_->motorStatusMoving_, true);
    sprintf(commandBuffer, "#%dj=%f\n ", axisNo_, position);
    printf(commandBuffer);

    return asynSuccess;
}
asynStatus pcsAxis::moveVelocity(double minVelocity,double maxVelocity, double acceleration){
    printf("pcsAxis::moveVelocity() called\n");
    return asynSuccess;
}
asynStatus pcsAxis::home(double minVelocity,double maxVelocity, double acceleration, int forwards){
    printf("pcsAxis::home() called\n");
    return asynSuccess;
}
asynStatus pcsAxis::stop(double acceleration){
    printf("pcsAxis::stop() called\n");
     return asynSuccess;
}
asynStatus pcsAxis::setPosition(double position){
    printf("pcsAxis::setPosition() called\n");
    return asynSuccess;
}

asynStatus pcsAxis::poll(bool *moving) {
    sprintf(ctrl_->outString_,"%s%c#1p\n",pmacHeader,'\x03');
    printf("Output : %s\n",ctrl_->outString_);
    ctrl_->writeReadController();
    printf("inString%s\n",ctrl_->inString_);

}

pcsAxis::~pcsAxis(){

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

static void PcsAxisRegister(void)
{
    iocshRegister(&configPcsAxis, configPcsAxisCallFunc);
}


extern "C" { epicsExportRegistrar(PcsAxisRegister); }