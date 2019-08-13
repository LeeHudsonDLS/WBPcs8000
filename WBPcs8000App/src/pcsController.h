//
// Created by jjc62351 on 06/08/19.
//

#ifndef PCSCONTROLLER_H
#define PCSCONTROLLER_H

#include <epicsTypes.h>
#include <epicsThread.h>
#include "asynMotorController.h"
#include "asynMotorAxis.h"
#include "pcsAxis.h"
#include <vector>

class pcsController
        : public asynMotorController {
public:
    pcsController(const char *portName, const char *lowLevelPortName, int lowLevelPortAddress,
                  int numAxes, double movingPollPeriod,
                  double idlePollPeriod);
    virtual  ~pcsController();

// These are the methods that we override from asynMotorController
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value,
            size_t maxChars, size_t *nActual);
    virtual asynStatus poll();
    enum OperatingMode {MotorRecord=0, Jog=1};
    enum JogOperation {Stop=0, Forward=1, Reverse=2};
private:

    friend class pcsAxis;

};

#endif //PCSCONTROLLER_H