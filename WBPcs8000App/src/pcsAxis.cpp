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

    static const char *functionName = "pmacAxis::pmacAxis";
    asynPrint(ctrl_->pasynUserSelf, ASYN_TRACE_FLOW, "%s\n",functionName);
    
    //Initialize non-static data members
    velocity_ = 0.0;
    accel_ = 0.0;

    initialise(axisNo_);

}


pcsAxis::~pcsAxis(){

}