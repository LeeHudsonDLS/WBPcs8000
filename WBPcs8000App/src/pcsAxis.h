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
    asynStatus stop(double acceleration);
    asynStatus poll(bool *moving);
    void enableLoop(bool set);

private:
    /* Data */
    pcsController *ctrl_;
    Sequencer moveSequencer;
    double minSensorVal_;
    int scale_,slave_;
    int primaryFeedback, secondaryFeedback;
    asynStatus status;

    /* Some C strings used to set up sequencer */
    char primaryFeedbackString[10],secondaryFeedbackString[10],minTrigger[48],allTriggers[96];

    void setupSensorExitConditions();
    void setSeqControlParams();
    asynStatus sendSequencer(const char* functionName);
    asynStatus stopSequencer(bool clearEnableLoop);


    /* Vector storing the XPATH locations all the control_set parameters (Tuning) and their values taken from asynParams
     * These values are written to in pcsController::writeFloat64() */
    std::vector<std::pair<std::string,double> > controlSet_;

    friend class pcsController;
};

#endif //PCSAXIS_H
