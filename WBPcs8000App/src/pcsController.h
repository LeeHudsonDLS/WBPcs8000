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


// Commands
#define START_UDP_CMD "start_udp_cmd"
#define CLEAR_UDP_CMD "clear_udp_cmd"

#define SEQ_CONTROL_PARAM "seq_control_param"
#define REGISTER_STREAM_PARAM "register_stream_param"
#define SYS_STATE_PARAM "sys_state_param"

#define POS_LIMIT_INPUT "pos_limit_input"
#define NEG_LIMIT_INPUT "neg_limit_input"
#define DRV_READY_INPUT "drive_ready_input"

// UDP Stream codes
#define PHYS1_UDP_STREAM_CODE   1
#define PHYS2_UDP_STREAM_CODE   2
#define PHYS3_UDP_STREAM_CODE   3
#define POSITION_UDP_STREAM_CODE 81
#define VELOCITY_UDP_STREAM_CODE 82

/*
 * Commonly used xml locations in CSV representation to be used directly by the XmlCommandConstructor
 */
#define GET_INPUT "digital_io,get,input"


#define BUFFER_SIZE 1024
#define AXIS_SCALE_FACTOR 1000
#define MESSAGE_SIZE 80
#define WRITE_TIMEOUT 2.0

class pcsController
        : public asynMotorController {
public:
    pcsController(const char *portName, int lowLevelPortAddress,
                  int numAxes, double movingPollPeriod,
                  double idlePollPeriod);
    ~pcsController();


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

    typedef struct{
        unsigned int ev_code;
        unsigned int slave;
        unsigned long long int ts;
        unsigned int ev_value;
        unsigned int num_data;
    }eventPacket;

    typedef struct myData {
        epicsMutexId mutexId;
        char         *portName;
        double       readTimeout;
        asynOctet    *pasynOctet;
        void         *octetPvt;
        void         *registrarPvt;
    }myData;


    asynStatus poll();
    void udpReadTask();
    static void connectionCallback(void *drvPvt, asynUser *pasynUser, char *portName,
                               size_t len, int eomReason);
    static void echoListener(myData *pPvt);
    char* lowLevelPortName;
    char* streamPortName;
    char* eventPortName;
    char* driverName;
    enum OperatingMode {MotorRecord=0, Jog=1};
    enum JogOperation {Stop=0, Forward=1, Reverse=2};
    void createAsynParams(void);
    pcsAxis *getAxis(asynUser *pasynUser);
    pcsAxis *getAxis(int axisNo);
    XmlCommandConstructor commandConstructor;
    void* octetPvt;
    int scale;



protected:
    //pcsAxis **pAxes_;    /**< Array of pointers to axis objects */

    int PCS_C_FirstParam;
    template <typename T>
    asynStatus sendXmlCommand(int axisNo,const std::string& parameter,T value);
    asynStatus sendXmlCommand(int axisNo,const std::string& parameter);

    asynStatus sendXmlCommand(const std::string& eos);


#define FIRST_PCS_PARAM PCS_C_FirstParam

private:
    int initialised_;
    int connected_;
    epicsEventId startEventId;
    epicsEventId stopEventId;
    friend class pcsAxis;

    asynStatus configureServer(const char* portname, myData *&pPvt, asynUser *&pasynUser, asynInterface *&pasynInterface, interruptCallbackOctet callBackRoutine);
    asynUser *pasynUserUDPStream;
    asynOctet *pasynOctet;
    asynInterface* pasynInterface;
    myData *pStreamPvt;

    // For TCP event server
    asynUser *pasynUserEventStream;
    asynInterface* pasynInterfaceEvent;
    myData *pEventPvt;

};

#endif //PCSCONTROLLER_H