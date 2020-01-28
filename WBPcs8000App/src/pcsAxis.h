//
// Created by jjc62351 on 06/08/19.
//

#ifndef PCSAXIS_H
#define PCSAXIS_H
#include "asynMotorController.h"
#include "asynMotorAxis.h"
#include <math.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <algorithm>
#include <stdlib.h>
#include <asynOctetSyncIO.h>
#include "Sequencer.h"
#include <math.h>

#define NO_OF_CONTROL_SET_PARAMS 16
#define POSITION_SENSOR 14

class pcsController;

class pcsAxis : public asynMotorAxis{
    public:
    pcsAxis(pcsController *ctrl, int axisNo, int slave,const char* priFeedback,const char* secFeedback, double minSensorVal);
    ~pcsAxis();
    void initialise(int axisNo);

    // Overridden from asynMotorAxis
    asynStatus move(double position, int relative,
            double minVelocity, double maxVelocity, double acceleration);
    asynStatus moveVelocity(double minVelocity,
            double maxVelocity, double acceleration);
    asynStatus home(double minVelocity,
            double maxVelocity, double acceleration, int forwards);
    asynStatus stop(double acceleration);
    asynStatus setPosition(double position);
    asynStatus poll(bool *moving);

private:
    /* Data */
    pcsController *ctrl_;
    Sequencer absoluteMoveSequencer;
    double velocity_ ;
    double accel_;
    double minSensorVal_;
    int scale_;
    int slave_;
    int primaryFeedback, secondaryFeedback;
    char primaryFeedbackString[10],secondaryFeedbackString[10],minTrigger[48],allTriggers[96];

    /* Vector storing the XPATH locations all the control_set parameters (Tuning) and their values taken from asynParams
     * These values are written to in pcsController::writeFloat64() */
    std::vector<std::pair<std::string,double> > controlSet_;

    friend class pcsController;
    asynStatus status;
};

#endif //PCSAXIS_H
