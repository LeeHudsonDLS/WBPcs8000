//
// Created by jjc62351 on 06/08/19.
//

#ifndef PCSAXIS_H
#define PCSAXIS_H
#include "asynMotorController.h"
#include "asynMotorAxis.h"
#include <string>

class pcsController;

class pcsAxis : public asynMotorAxis{
    public:
    pcsAxis(pcsController *ctrl, int axisNo);
    virtual ~pcsAxis();
    void initialise(int axisNo);

    // Overridden from asynMotorAxis
    virtual asynStatus move(double position, int relative,
            double minVelocity, double maxVelocity, double acceleration);
    virtual asynStatus moveVelocity(double minVelocity,
            double maxVelocity, double acceleration);
    virtual asynStatus home(double minVelocity,
            double maxVelocity, double acceleration, int forwards);
    virtual asynStatus stop(double acceleration);
    virtual asynStatus setPosition(double position);
    virtual asynStatus poll(bool *moving);
private:
    /* Data */
    pcsController *ctrl_;
    signed char pmacHeader[8];
    double velocity_ ;
    double accel_;
    friend class pcsController;
};

#endif //PCSAXIS_H
