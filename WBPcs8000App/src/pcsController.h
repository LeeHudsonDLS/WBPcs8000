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

#undef MAX_CONTROLLER_STRING_SIZE
#define MAX_CONTROLLER_STRING_SIZE 2048
#define MAX_SLAVES 8
#define MAX_AXES_PER_SLAVE 8
#define PCS_C_FirstParamString      "PCS_C_FIRSTPARAM"
#define PCS_A_kpParamString         "AXIS_KP"
#define PCS_A_tiParamString         "AXIS_TI"
#define PCS_A_tdParamString         "AXIS_TD"
#define PCS_A_t1ParamString         "AXIS_T1"
#define PCS_A_keParamString         "AXIS_KE"
#define PCS_A_ke2ParamString        "AXIS_KE2"
#define PCS_A_kffParamString        "AXIS_KFF"
#define PCS_A_kreiParamString       "AXIS_KREI"
#define PCS_A_tauParamString        "AXIS_TAU"
#define PCS_A_elimParamString       "AXIS_ELIM"
#define PCS_A_kdccParamString       "AXIS_KDCC"
#define PCS_A_symManParamString     "AXIS_SYM_MAN"
#define PCS_A_symAdpParamString     "AXIS_SYM_ADP"
#define PCS_A_gkiParamString        "AXIS_GKI"
#define PCS_A_tkiParamString        "AXIS_TKI"
#define PCS_A_pkParamString         "AXIS_PK"
#define PCS_A_enableLoopString      "AXIS_ENABLE_LOOP"

#define NUM_OF_PCS_PARAMS   17+(MAX_SLAVES*4)

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

// UDP Stream related definitions
#define UDP_PACKET_SIZE 3236    //36 bytes of meta data and 800 4byte data objects. 36+(800*4)
#define PHYS1_UDP_STREAM_CODE   1
#define PHYS2_UDP_STREAM_CODE   2
#define PHYS3_UDP_STREAM_CODE   3
#define POSITION_UDP_STREAM_CODE 81
#define VELOCITY_UDP_STREAM_CODE 82


// TCP Event related definitions
#define EVENT_CLIENT_HANDSHAKE_SIZE 11
#define EVENT_PACKET_SIZE 28

#define SEQ_STATE_CHANGE_EVT    20
#define NLIM_STATE_CHANGE_EVT    203
#define PLIM_STATE_CHANGE_EVT    202
#define SYNTH_COMPLETE_EVT      303



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
    pcsController(const char *portName, int lowLevelPortAddress, int numSlaves,
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
    }streamUDPPacket;

    typedef struct{
        unsigned int ev_code;
        unsigned int slave;
        unsigned long long int ts;
        unsigned int ev_value;
        unsigned int num_data;
        unsigned int data;
    }eventTCPPacket;

    typedef struct myData {
        epicsMutexId mutexId;
        char         *portName;
        double       readTimeout;
        asynOctet    *pasynOctet;
        void         *octetPvt;
        void         *registrarPvt;
        asynUser     *pasynUser;
        void         *pController;
    }portData;


    asynStatus poll();

    /* Method to override */
    asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    void udpReadTask();
    static void tcpClientConnectedCallback(void *drvPvt, asynUser *pasynUser, char *portName,
                                           size_t len, int eomReason);
    void eventListener(portData *pPvt);
    void registerAxisToSlave(int slave, int axis);
    void registerFeedback(int slave, int feedback, int axisNo);

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
    int axesInitialised;



protected:
    /* Asyn parameters */
    int PCS_C_FirstParam;
    int PCS_C_SeqState[MAX_SLAVES];
    int PCS_C_XmlSequencer[MAX_SLAVES];
    int PCS_C_UserXmlLoaded[MAX_SLAVES];
    int PCS_C_StartSequencer[MAX_SLAVES];
    int PCS_A_enableLoop;

    /* asyn parameters for the tuning */
    std::vector<std::pair<std::string,int> > controlSetParams;


    /* Array containing the number of axes per slave. The index is the slave number*/
    int axesPerSlave[MAX_SLAVES];

    /* Array containing which axis belongs to which slave */
    int axisNumbers[MAX_SLAVES][MAX_AXES_PER_SLAVE];

    char outString_[MAX_CONTROLLER_STRING_SIZE];
    char inString_[MAX_CONTROLLER_STRING_SIZE];
    int clearEnableLoops(const int& slaveNo, const int& axis);

    /* Overriding this method as we don't know what the EOS is going to be*/
    asynStatus writeReadController();
    template <typename T>
    asynStatus sendXmlCommand(int axisNo,const std::string& parameter,T value);
    asynStatus sendXmlCommand(int axisNo,const std::string& parameter);


#define FIRST_PCS_PARAM PCS_C_FirstParam

private:
    int initialised_;
    int connected_;
    int numSlaves_;
    epicsEventId startEventId;
    epicsEventId stopEventId;
    friend class pcsAxis;
    int clientsConnected;

    std::vector<std::map<int, int> > feedbackMap;

    asynStatus configureServer(const char* portname, portData *&pPvt, interruptCallbackOctet callBackRoutine);
    asynOctet *pasynOctet;
    portData *pStreamPvt;

    // For TCP event server
    portData *pEventPvt;

};

#endif //PCSCONTROLLER_H