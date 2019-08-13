//
// Created by jjc62351 on 06/08/19.
//

#include "pcsAxis.h"
#include "pcsController.h"
#include <stdlib.h>
#include <epicsThread.h>
#include <sstream>
#include <iostream>

#include <epicsExport.h>
pcsAxis::pcsAxis(pcsController *ctrl, int axisNo)
        :asynMotorAxis((asynMotorController *) ctrl, axisNo),
        ctrl_(ctrl){

    static const char *functionName = "pcsAxis::pcsAxis";
    asynPrint(ctrl_->pasynUserSelf, ASYN_TRACE_FLOW, "%s\n",functionName);

    //Initialize non-static data members
    velocity_ = 0.0;
    accel_ = 0.0;

    initialise(axisNo_);

}

void pcsAxis::initialise(int axisNo) {
    static const char *functionName = "pcsAxis::initialise";
    printf("Axis %d created \n",axisNo);
}


asynStatus pcsAxis::move(double position, int relative, double minVelocity, double maxVelocity, double acceleration){
    return asynSuccess;
}
asynStatus pcsAxis::moveVelocity(double minVelocity,double maxVelocity, double acceleration){
    return asynSuccess;
}
asynStatus pcsAxis::home(double minVelocity,double maxVelocity, double acceleration, int forwards){
    return asynSuccess;
}
asynStatus pcsAxis::stop(double acceleration){
     return asynSuccess;
}
asynStatus pcsAxis::setPosition(double position){
     return asynSuccess;
}

pcsAxis::~pcsAxis(){

}