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


class pcsController;

class pcsAxis : public asynMotorAxis{
    public:
    pcsAxis(pcsController *ctrl, int axisNo);
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
    Sequencer relativeMoveSequencer;
    Sequencer absoluteMoveSequencer;
    double velocity_ ;
    double accel_;
    int scale_;
    friend class pcsController;
    asynStatus status;
};

#endif //PCSAXIS_H
