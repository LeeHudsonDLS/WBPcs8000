//
// Created by jjc62351 on 06/08/19.
//

#include "pcsAxis.h"
#include "pcsController.h"
#include "XmlTemplates.h"

#include <epicsExport.h>

pcsAxis::pcsAxis(pcsController *ctrl, int axisNo, int slave, const char* priFeedback, const char* secFeedback, double minSensorVal)
        :asynMotorAxis((asynMotorController *) ctrl, axisNo),
        ctrl_(ctrl),
        slave_(slave),
        minSensorVal_(minSensorVal),
        absoluteMoveSequencer(){

    // Look into MSTA bits for enabling motors
    static const char *functionName = "pcsAxis::pcsAxis";
    asynPrint(ctrl_->pasynUserSelf, ASYN_TRACE_FLOW, "%s\r",functionName);

    scale_=ctrl_->scale;
    std::string file_path = __FILE__;
    //Initialize non-static data members
    velocity_ = 0.0;
    accel_ = 0.0;

    sprintf(primaryFeedbackString,"%s",priFeedback);
    sprintf(secondaryFeedbackString,"%s",secFeedback);

    initialise(axisNo_);
    absoluteMoveSequencer.setElement("//slave",slave_);
    setIntegerParam(ctrl_->motorStatusMoving_, false);
    setIntegerParam(ctrl_->motorStatusDone_,1);
    callParamCallbacks();
    ctrl_->axesInitialised++;

}

void pcsAxis::initialise(int axisNo) {
    static const char *functionName = "pcsAxis::initialise";
    printf("Axis %d created \n",axisNo);


    /* Populate the controlSet_ vector with all the XPATH locations of the XML elements they will be writing to
     * These need to be in the same order as pcsController::controlSetParams */
    controlSet_.push_back(std::pair<std::string,int>("//kp",0));
    controlSet_.push_back(std::pair<std::string,int>("//ti",0));
    controlSet_.push_back(std::pair<std::string,int>("//td",0));
    controlSet_.push_back(std::pair<std::string,int>("//t1",0));
    controlSet_.push_back(std::pair<std::string,int>("//ke",0));
    controlSet_.push_back(std::pair<std::string,int>("//ke2",0));
    controlSet_.push_back(std::pair<std::string,int>("//kff",0));
    controlSet_.push_back(std::pair<std::string,int>("//krei",0));
    controlSet_.push_back(std::pair<std::string,int>("//tau",0));
    controlSet_.push_back(std::pair<std::string,int>("//elim",0));
    controlSet_.push_back(std::pair<std::string,int>("//kdcc",0));
    controlSet_.push_back(std::pair<std::string,int>("//sym_man",0));
    controlSet_.push_back(std::pair<std::string,int>("//sym_adp",0));
    controlSet_.push_back(std::pair<std::string,int>("//gki",0));
    controlSet_.push_back(std::pair<std::string,int>("//tki",0));
    controlSet_.push_back(std::pair<std::string,int>("//pk",0));

    // Send the xml
    //sprintf(ctrl_->outString_, "%s",udpSetup.getXml().c_str());
    //ctrl_->writeController();

    /* Extract the feedback stream numbers from the string */
    sscanf(primaryFeedbackString,"phys%d",&primaryFeedback);
    sscanf(secondaryFeedbackString,"phys%d",&secondaryFeedback);


    /* Register this axis to a feedback code. The feedback code corresponds to the code
     * that comes with each udp packet to determine the which sensor the packet is referring to.
     * It's not always the feedback stream number unfortunately*/
    if(primaryFeedback>=1 && primaryFeedback <=13){
        ctrl_->registerFeedback(slave_,primaryFeedback,axisNo);
    }else{
        ctrl_->registerFeedback(slave_,primaryFeedback+67,axisNo);
    }

    ctrl_->registerAxisToSlave(slave_,axisNo);

    if(primaryFeedback == POSITION_SENSOR)
        absoluteMoveSequencer.loadXML(absoluteMoveTemplate);
    else
        absoluteMoveSequencer.loadXML(moveWaitTemplate);

}

void pcsAxis::setLoop(bool set) {

    size_t nwrite,nread;
    int eomReason;
    double currentPositionScaled;
    char seqBuffer[4096];
    char rxBuffer[1024];
    static const char *functionName = "setLoop";
    asynStatus status;

    currentPositionScaled = status_.position/scale_;

    if(primaryFeedback==POSITION_SENSOR)
        return;

    if(set){

        /* Set PID parameters */
        setSeqControlParams();

        absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[1]/acc",0);
        absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[1]/rate",0);
        absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[1]/end_rate",0);
        absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[1]/end_ampl",currentPositionScaled);

        absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[2]/acc",0);
        absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[2]/rate",0);
        absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[2]/end_rate",0);
        absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[2]/end_ampl",currentPositionScaled);
        setupSensorExitConditions();

        status = sendSequencer(functionName);

        ctrl_->wakeupPoller();

        setIntegerParam(ctrl_->PCS_A_enableLoop,1);
        callParamCallbacks();

    }else{
        stopSequencer(true);
    }
}

asynStatus pcsAxis::stopSequencer(bool clearEnableLoop) {
    size_t nwrite,nread;
    int eomReason;
    char rxBuffer[1024];

    rxBuffer[0]='\0';

    /* If  clearEnableLoop == true, clear all enable flags including the one tied to this axis.*/
    if(clearEnableLoop)
        ctrl_->clearEnableLoops(slave_,-1);
    else
        ctrl_->clearEnableLoops(slave_,axisNo_);

    return pasynOctetSyncIO->writeRead(ctrl_->pasynUserController_,ctrl_->commandConstructor.getXml(slave_,SEQ_CONTROL_PARAM,"Setup").c_str(),strlen(ctrl_->commandConstructor.getXml(slave_,SEQ_CONTROL_PARAM,"Setup").c_str()),rxBuffer,1024,0.1,&nwrite,&nread,&eomReason);

}

asynStatus pcsAxis::move(double position, int relative, double minVelocity, double maxVelocity, double acceleration){

    asynStatus status = asynSuccess;
    static const char *functionName = "move";
    double decelTime,decelDist,midPosition;
    printf("pcsAxis::move(%f) called\n",position);
    ctrl_->inString_[0]='\0';
    double accRate,accTime,distance,actVel,deltaDist,theta,moveVel,actualAccTime,maxVelocityScaled;

    // Stop the sequencer before moving
    status = stopSequencer(false);

    absoluteMoveSequencer.setElement("//stream",primaryFeedbackString);
    absoluteMoveSequencer.setElement("//stream2",secondaryFeedbackString);

    /* Set PID parameters */
    setSeqControlParams();


    /* Max velocity in EGUs*/
    maxVelocityScaled=maxVelocity/scale_;

    /* Acceleration rate in EGUs */
    accRate=acceleration/scale_;

    /* Total distance to move in EGUs*/
    distance = fabs(position - status_.position)/scale_;

    /* Acceleration time in seconds */
    accTime = maxVelocityScaled/accRate;

    /* The distance covered during either the acceleration or deceleration */
    deltaDist= (accTime * maxVelocityScaled);

    /* The angle of the acceleration profile */
    theta = atan2(maxVelocityScaled,accTime)*(180/M_PI);


    /* If the distance to be travelled will be covered during the accel/decel
     * phase of the profile then a triangular motion profile should be applied*/
    if(deltaDist > distance){

        /* Triangle profile */
        actualAccTime = sqrt(distance/(maxVelocityScaled/accTime));
        actVel=(maxVelocityScaled/accTime)*actualAccTime;
        moveVel = actVel;

        if(position > status_.position)
            midPosition=(position/scale_)-(distance/2);
        else
            midPosition=(position/scale_)+(distance/2);

    }else{
        moveVel=maxVelocityScaled;

        /*
         * Determine the amount of time the axis will take to decelerate and the amount the axis will travel
         * during this deceleration.
         * maxVelocity  -   VELO * MRES
         * decelTime    -   .ACCL
         */
        decelTime = maxVelocity/acceleration;
        decelDist = (maxVelocityScaled * decelTime) / 2;

        /*
         * Determine "midPosition". Here "position" is the demand from the motor record and status_.position is the
         * actual position. The "midPosition" is the end position for the first synthesizer, the first synthesizer
         * takes care of the initial acceleration and the entire move up to the point where deceleration starts. This
         * "midPosition" must be the desired end position - how far the axis will travel during deceleration given
         * the velocity and deceleration rate.
         */
        if(position > status_.position)
            midPosition=(position/scale_)-decelDist;
        else
            midPosition=(position/scale_)+decelDist;

    }


    /*
     * Set up the first half of the motion profile. The "rate" is 0 as this assumes starting from a stand still
     *    ________________
     *   /
     *  /
     * /
     *
     */
    absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[1]/acc",acceleration/scale_);
    absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[1]/rate",0);
    absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[1]/end_rate",moveVel);
    absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[1]/end_ampl",midPosition);


    /*
     * Set up the second half of the motion profile. This is purely the deceleration stage. "end_rate" cannot be
     * zero.
     *
     *      \
     *       \
     *        \
     */
    absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[2]/acc",acceleration/scale_);
    absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[2]/rate",moveVel);
    absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[2]/end_rate",0.001);
    absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[2]/end_ampl",position/scale_);


    /* If the primary feedback device is anything other than position we will need to add
     * a minimum allowable value to prevent run away*/
    if(primaryFeedback!=POSITION_SENSOR){
        setupSensorExitConditions();
    }

    status = sendSequencer(functionName);

    setIntegerParam(ctrl_->PCS_A_enableLoop,1);
    ctrl_->wakeupPoller();
    return status;
}

void pcsAxis::setSeqControlParams() {

    std::vector<std::pair<std::string,double> >::iterator pidIterator = controlSet_.begin();
    while(pidIterator!=controlSet_.end()){
        absoluteMoveSequencer.setElement(pidIterator->first.c_str(),pidIterator->second);
        pidIterator++;
    }
}

asynStatus pcsAxis::sendSequencer(const char* functionName) {

    size_t nwrite,nread,nact;
    int eomReason;
    char seqBuffer[4096];
    char rxBuffer[1024];

    // Get the completed sequncer in XML form
    sprintf(seqBuffer,absoluteMoveSequencer.getXml().c_str());
    printf("%s\n",seqBuffer);
    status = pasynOctetSyncIO->writeRead(ctrl_->pasynUserController_,ctrl_->commandConstructor.getXml(slave_,SEQ_CONTROL_PARAM,"Setup").c_str(),strlen(ctrl_->commandConstructor.getXml(slave_,SEQ_CONTROL_PARAM,"Setup").c_str()),rxBuffer,1024,0.1,&nwrite,&nread,&eomReason);

    // Send the sequencer to the controller
    sprintf(ctrl_->outString_,seqBuffer);
    status = ctrl_->writeReadController();

    /* Update asyn param to allow what's sequencer is loaded to be visible */
    ctrl_->lock();
    ctrl_->setStringParam(ctrl_->PCS_C_XmlSequencer[slave_],seqBuffer);
    ctrl_->setIntegerParam(ctrl_->PCS_C_UserXmlLoaded[slave_], 0);
    ctrl_->unlock();
    ctrl_->callParamCallbacks();

    // Send the command to start the sequencer
    sprintf(ctrl_->outString_,ctrl_->commandConstructor.getXml(slave_,SEQ_CONTROL_PARAM,"Program").c_str());
    status = ctrl_->writeReadController();


    asynPrint(ctrl_->pasynUserSelf, ASYN_TRACE_FLOW, "%s\n", functionName);
    setIntegerParam(ctrl_->motorStatusDone_,1);

    ctrl_->wakeupPoller();
    return status;

}


void pcsAxis::setupSensorExitConditions() {


    absoluteMoveSequencer.setElement("/sequencer_prog/clear[1]/ndrag",primaryFeedbackString);
    absoluteMoveSequencer.setElement("/sequencer_prog/clear[3]/ndrag",primaryFeedbackString);
    absoluteMoveSequencer.setElement("/sequencer_prog/clear[5]/ndrag",primaryFeedbackString);

    sprintf(minTrigger,"TRIG_MIN_PHY(%d)",primaryFeedback);
    absoluteMoveSequencer.setElement("/sequencer_prog/trigger_set[1]/name",minTrigger);
    absoluteMoveSequencer.setElement("/sequencer_prog/trigger_set[1]/value",minSensorVal_);
    absoluteMoveSequencer.setElement("/sequencer_prog/trigger_set[2]/name",minTrigger);
    absoluteMoveSequencer.setElement("/sequencer_prog/trigger_set[2]/value",minSensorVal_);
    absoluteMoveSequencer.setElement("/sequencer_prog/trigger_set[3]/name",minTrigger);
    absoluteMoveSequencer.setElement("/sequencer_prog/trigger_set[3]/value",minSensorVal_);


    absoluteMoveSequencer.setElement("/sequencer_prog/clear[4]/triggers",minTrigger);
    absoluteMoveSequencer.setElement("/sequencer_prog/wait[2]/triggers",minTrigger);
    absoluteMoveSequencer.setElement("/sequencer_prog/clear[6]/triggers",minTrigger);
    absoluteMoveSequencer.setElement("/sequencer_prog/wait[3]/triggers",minTrigger);

    sprintf(allTriggers,"TRIG_SYNTH_R;TRIG_MIN_PHY(%d)",primaryFeedback);
    absoluteMoveSequencer.setElement("/sequencer_prog/clear[2]/triggers",allTriggers);
    absoluteMoveSequencer.setElement("/sequencer_prog/wait[1]/triggers",allTriggers);

}

asynStatus pcsAxis::moveVelocity(double minVelocity,double maxVelocity, double acceleration){
    printf("pcsAxis::moveVelocity() called\n");
    return asynSuccess;
}
asynStatus pcsAxis::home(double minVelocity,double maxVelocity, double acceleration, int forwards){
    printf("pcsAxis::home() called\n");
    return asynSuccess;
}
asynStatus pcsAxis::stop(double acceleration){
    asynStatus status = asynSuccess;

    // Put sequencer into "Setup" state to stop it and any associated movement.
    status = stopSequencer(true);

    return asynSuccess;
}
asynStatus pcsAxis::setPosition(double position){
    printf("pcsAxis::setPosition() called\n");
    return asynSuccess;
}

asynStatus pcsAxis::poll(bool *moving) {

    callParamCallbacks();

    return asynSuccess;

}

pcsAxis::~pcsAxis(){

}

/** Axis configuration command, called directly or from iocsh.
  * \param[in] portName The name of the controller asyn port.
  * \param[in] axisNum The number of the axis, zero based.
  * \param[in] homeMode The homing mode of the axis
  */
extern "C" int pcsAxisConfig(const char *controllerName, int axisNum, int slave, const char *priFeedback, const char *secFeedback, double minSensorVal)
{
    int result = asynSuccess;
    pcsController* controller = (pcsController*)findAsynPortDriver(controllerName);
    if(controller == NULL)
    {
        printf("Could not find a GCS controller with this port name\n");
        result = asynError;
    }
    else
    {
        new pcsAxis(controller, axisNum,slave,priFeedback,secFeedback,minSensorVal);
    }
    return result;
}

/* Code for iocsh registration for pcsAxis*/
/*
 * Following parameters required for asynMotorController constructor:
 * name of associated asynMotorController char*
 * axis number int
 */

static const iocshArg pcsAxisConfigArg0 = {"Controller port name", iocshArgString};
static const iocshArg pcsAxisConfigArg1 = {"Axis number", iocshArgInt};
static const iocshArg pcsAxisConfigArg2 = {"Slave number", iocshArgInt};
static const iocshArg pcsAxisConfigArg3 = {"Primary Feedback", iocshArgString};
static const iocshArg pcsAxisConfigArg4 = {"Secondary Feedback", iocshArgString};
static const iocshArg pcsAxisConfigArg5 = {"Minimum sensor value", iocshArgDouble};

static const iocshArg *const pcsAxisConfigArgs[] = {&pcsAxisConfigArg0,
                                                    &pcsAxisConfigArg1,
                                                    &pcsAxisConfigArg2,
                                                    &pcsAxisConfigArg3,
                                                    &pcsAxisConfigArg4,
                                                    &pcsAxisConfigArg5};

static const iocshFuncDef configPcsAxis =
        {"pcsAxisConfig", 6, pcsAxisConfigArgs};

static void configPcsAxisCallFunc(const iocshArgBuf *args)
{
    pcsAxisConfig(args[0].sval, args[1].ival, args[2].ival,args[3].sval,args[4].sval,args[5].dval);
}

static void PcsAxisRegister(void)
{
    iocshRegister(&configPcsAxis, configPcsAxisCallFunc);
}


extern "C" { epicsExportRegistrar(PcsAxisRegister); }