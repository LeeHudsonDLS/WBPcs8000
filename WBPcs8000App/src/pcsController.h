//
// Created by jjc62351 on 06/08/19.
//

#ifndef PCSCONTROLLER_H
#define PCSCONTROLLER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include <epicsTypes.h>
#include <epicsThread.h>
#include "asynMotorController.h"
#include "asynMotorAxis.h"
#include "pcsAxis.h"
#include <vector>
#include "XmlCommandConstructor.h"

#define PCS_C_FirstParamString           "PCS_C_FIRSTPARAM"
#define NUM_OF_PCS_PARAMS   1

// Items used for initial handshaking
#define NAME "DLS"
#define VERSION 1.00
#define CODE 1234

#define MAIN_PORT_SUFFIX "_CTRL"
#define STREAMS_PORT_SUFFIX "_UDP"
#define EVENT_PORT_SUFFIX   "_TCP"


#define START_UDP_CMD "start_udp_cmd"
#define CLEAR_UDP_CMD "clear_udp_cmd"

#define SEQ_CONTROL_PARAM "seq_control_param"
#define REGISTER_STREAM_PARAM "register_stream_param"
#define SYS_STATE_PARAM "sys_state_param"

#define POS_LIMIT_INPUT "pos_limit_input"
#define NEG_LIMIT_INPUT "neg_limit_input"
#define DRV_READY_INPUT "drive_ready_input"

/*
 * Commonly used xml locations in CSV representation to be used directly by the XmlCommandConstructor
 */
#define GET_INPUT "digital_io,get,input"


#define BUFFER_SIZE 1024


class pcsController
        : public asynMotorController {
public:
    pcsController(const char *portName, int lowLevelPortAddress,
                  int numAxes, double movingPollPeriod,
                  double idlePollPeriod);
    ~pcsController();

    asynStatus poll();
    char* lowLevelPortName;
    char* streamPortName;
    char* eventPortName;
    enum OperatingMode {MotorRecord=0, Jog=1};
    enum JogOperation {Stop=0, Forward=1, Reverse=2};
    void createAsynParams(void);
    pcsAxis *getAxis(asynUser *pasynUser);
    pcsAxis *getAxis(int axisNo);
    asynOctet *pasynOctet;
    XmlCommandConstructor commandConstructor;
    asynInterface* pasynInterface;
    void* octetPvt;

    typedef struct{
        unsigned int code;
        unsigned int slave;
        unsigned int nData;
        unsigned long long int pkgIndex;
        unsigned long long int ts;
        float minDrag;
        float maxDrag;
        float data;
    }udpPacket;

protected:
    //pcsAxis **pAxes_;    /**< Array of pointers to axis objects */

    int PCS_C_FirstParam;
    asynStatus sendXmlCommand(int axisNo,const std::string& parameter);
    asynStatus sendXmlCommand(int axisNo,const std::string& parameter, int val);
    asynStatus sendXmlCommand(int axisNo,const std::string& parameter, std::string val);
    void* drvPtr;
    asynUser *pasynUserUDPStream;

#define FIRST_PCS_PARAM PCS_C_FirstParam

private:
    int initialised_;
    int connected_;

    friend class pcsAxis;

};

#endif //PCSCONTROLLER_H