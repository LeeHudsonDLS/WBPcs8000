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
#include "XmlCommandConstructor.h"

#define PCS_C_FirstParamString           "PCS_C_FIRSTPARAM"
#define NUM_OF_PCS_PARAMS   1

#define START_UDP_CMD "start_udp_cmd"
#define START_SEQ_CMD "start_seq_cmd"
#define REGISTER_STREAM_CMD "register_stream_cmd"
#define CLEAR_UDP "clear_udp"




class pcsController
        : public asynMotorController {
public:
    pcsController(const char *portName, const char *lowLevelPortName, int lowLevelPortAddress,
                  int numAxes, double movingPollPeriod,
                  double idlePollPeriod);
    ~pcsController();

    asynStatus poll();
    enum OperatingMode {MotorRecord=0, Jog=1};
    enum JogOperation {Stop=0, Forward=1, Reverse=2};
    void createAsynParams(void);
    pcsAxis *getAxis(asynUser *pasynUser);
    pcsAxis *getAxis(int axisNo);

    XmlCommandConstructor commandConstructor;
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