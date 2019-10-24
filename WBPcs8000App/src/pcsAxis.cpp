//
// Created by jjc62351 on 06/08/19.
//

#include "pcsAxis.h"
#include "pcsController.h"
#include "XmlTemplates.h"

#include <epicsExport.h>

pcsAxis::pcsAxis(pcsController *ctrl, int axisNo)
        :asynMotorAxis((asynMotorController *) ctrl, axisNo),
        ctrl_(ctrl),
        relativeMoveSequencer(relativeMoveTemplate),
        absoluteMoveSequencer(absoluteMoveTemplate){

    // Look into MSTA bits for enabling motors
    static const char *functionName = "pcsAxis::pcsAxis";
    asynPrint(ctrl_->pasynUserSelf, ASYN_TRACE_FLOW, "%s\r",functionName);

    std::string file_path = __FILE__;
    //Initialize non-static data members
    velocity_ = 0.0;
    accel_ = 0.0;
    initialise(axisNo_);

    relativeMoveSequencer.setElement("//slave",axisNo-1);
    absoluteMoveSequencer.setElement("//slave",axisNo-1);

    sprintf(ctrl_->outString_,ctrl_->commandConstructor.getXml(axisNo,SYS_STATE_PARAM,"Ready").c_str());
    ctrl_->writeController();


    sprintf(ctrl_->outString_,ctrl_->commandConstructor.getXml(axisNo,REGISTER_STREAM_PARAM,"phys14").c_str());
    status = pasynOctetSyncIO->setInputEos(ctrl_->pasynUserController_,ctrl_->commandConstructor.getEos(REGISTER_STREAM_PARAM).c_str(),2);
    status=ctrl_->writeReadController();


    sprintf(ctrl_->outString_,ctrl_->commandConstructor.getXml(axisNo,START_UDP_CMD).c_str());
    pasynOctetSyncIO->setInputEos(ctrl_->pasynUserController_,ctrl_->commandConstructor.getEos(START_UDP_CMD).c_str(),2);
    status=ctrl_->writeReadController();



}

void pcsAxis::initialise(int axisNo) {
    static const char *functionName = "pcsAxis::initialise";
    printf("Axis %d created \n",axisNo);



    // Send the xml
    //sprintf(ctrl_->outString_, "%s",udpSetup.getXml().c_str());
    //ctrl_->writeController();

}


asynStatus pcsAxis::move(double position, int relative, double minVelocity, double maxVelocity, double acceleration){

    double distance = 0;
    size_t nwrite;
    asynStatus status = asynError;
    static const char *functionName = "move";
    char seqBuffer[4096];
    printf("pcsAxis::move() called\n");

    // Set the velocity
    absoluteMoveSequencer.setElement("//rate",maxVelocity);
    absoluteMoveSequencer.setElement("//end_ampl",position);

    sprintf(seqBuffer,absoluteMoveSequencer.getXml().c_str());
    printf(seqBuffer);
    pasynOctetSyncIO->write(ctrl_->pasynUserController_,seqBuffer,strlen(seqBuffer),DEFAULT_CONTROLLER_TIMEOUT,&nwrite);


    sprintf(ctrl_->outString_,ctrl_->commandConstructor.getXml(axisNo_,SEQ_CONTROL_PARAM,"Program").c_str());
    printf("Command Size = %d\n",strlen(ctrl_->outString_));
    ctrl_->writeController();

    asynPrint(ctrl_->pasynUserSelf, ASYN_TRACE_FLOW, "%s\n", functionName);


    setIntegerParam(ctrl_->motorStatusMoving_, false);
    // ctrl_->wakeupPoller();
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

/*

    sprintf(ctrl_->outString_,"#%dp\r",axisNo_);
    ctrl_->writeReadController();
 */

    setIntegerParam(ctrl_->motorStatusDone_,1);
    setDoubleParam(ctrl_->motorPosition_,atoi(ctrl_->inString_));
    *moving = false;
    callParamCallbacks();
    return asynSuccess;

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