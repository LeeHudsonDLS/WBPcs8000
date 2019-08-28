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

#define PCS_C_FirstParamString           "PCS_C_FIRSTPARAM"


class pcsController
        : public asynMotorController {
public:
    pcsController(const char *portName, const char *lowLevelPortName, int lowLevelPortAddress,
                  int numAxes, double movingPollPeriod,
                  double idlePollPeriod);
    virtual  ~pcsController();

    virtual asynStatus poll();
    enum OperatingMode {MotorRecord=0, Jog=1};
    enum JogOperation {Stop=0, Forward=1, Reverse=2};
    void createAsynParams(void);
    pcsAxis *getAxis(asynUser *pasynUser);
    pcsAxis *getAxis(int axisNo);
protected:
    //pcsAxis **pAxes_;    /**< Array of pointers to axis objects */

    int PCS_C_FirstParam;

#define FIRST_PCS_PARAM PCS_C_FirstParam

private:
    int initialised_;
    int connected_;

    friend class pcsAxis;

};

#endif //PCSCONTROLLER_H