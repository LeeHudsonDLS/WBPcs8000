//
// Created by jjc62351 on 06/08/19.
//

#include "pcsAxis.h"
#include "pcsController.h"
#include "XmlTemplates.h"

#include <epicsExport.h>

pcsAxis::pcsAxis(pcsController *ctrl, int axisNo, int slave, const char* priFeedback, const char* secFeedback, double minSensorVal)
        : asynMotorAxis((asynMotorController *) ctrl, axisNo),
          ctrl_(ctrl),
          slave_(slave),
          minSensorVal_(minSensorVal),
          moveSequencer(){

    // Look into MSTA bits for enabling motors
    static const char *functionName = "pcsAxis::pcsAxis";
    asynPrint(ctrl_->pasynUserSelf, ASYN_TRACE_FLOW, "%s\r",functionName);

    scale_=ctrl_->scale;
    std::string file_path = __FILE__;

    sprintf(primaryFeedbackString,"%s",priFeedback);
    sprintf(secondaryFeedbackString,"%s",secFeedback);

    initialise(axisNo_);
    moveSequencer.setElement("//slave", slave_);
    setIntegerParam(ctrl_->motorStatusMoving_, false);
    setIntegerParam(ctrl_->motorStatusDone_,1);
    callParamCallbacks();
    ctrl_->axesInitialised++;
}

/**
 * Basic initialisation of the given axis, remove param and use axisNo_?
 * @param axisNo
 */
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

    /* Load XML template depending on feedback device */
    if(primaryFeedback == POSITION_SENSOR)
        moveSequencer.loadXML(moveTemplate);
    else
        moveSequencer.loadXML(moveWaitTemplate);
}

/**
 * Move the motor to an absolute location or by a relative amount.
 * @param position  The absolute position to move to (if relative=0) or the relative distance to move
 * by (if relative=1). Units=steps.
 * @param relative  Flag indicating relative move (1) or absolute move (0).
 * @param minVelocity The initial velocity, often called the base velocity. Units=steps/sec.
 * @param maxVelocity The maximum velocity, often called the slew velocity. Units=steps/sec.
 * @param acceleration The acceleration value. Units=steps/sec/sec.
 * @return asynStatus as a result of the asyn operation
 * */
asynStatus pcsAxis::move(double position, int relative, double minVelocity, double maxVelocity, double acceleration){

    asynStatus status = asynSuccess;
    static const char *functionName = "move";
    double decelTime,decelDist,midPosition;
    printf("pcsAxis::move(%f) called\n",position);
    ctrl_->inString_[0]='\0';
    double accRate,accTime,distance,actVel,deltaDist,theta,moveVel,actualAccTime,maxVelocityScaled;

    // Stop the sequencer before moving, don't clear the loop enabled flag for this axis
    status = stopSequencer(false);

    moveSequencer.setElement("//stream", primaryFeedbackString);
    moveSequencer.setElement("//stream2", secondaryFeedbackString);

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
    moveSequencer.setElement("/sequencer_prog/synth_set[1]/acc", acceleration / scale_);
    moveSequencer.setElement("/sequencer_prog/synth_set[1]/rate", 0);
    moveSequencer.setElement("/sequencer_prog/synth_set[1]/end_rate", moveVel);
    moveSequencer.setElement("/sequencer_prog/synth_set[1]/end_ampl", midPosition);


    /*
     * Set up the second half of the motion profile. This is purely the deceleration stage. "end_rate" cannot be
     * zero.
     *
     *      \
     *       \
     *        \
     */
    moveSequencer.setElement("/sequencer_prog/synth_set[2]/acc", acceleration / scale_);
    moveSequencer.setElement("/sequencer_prog/synth_set[2]/rate", moveVel);
    moveSequencer.setElement("/sequencer_prog/synth_set[2]/end_rate", 0.001);
    moveSequencer.setElement("/sequencer_prog/synth_set[2]/end_ampl", position / scale_);


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

/**
 * Stop the motor
 * @param acceleration
 * @return
 */
asynStatus pcsAxis::stop(double acceleration){
    asynStatus status = asynSuccess;

    // Put sequencer into "Setup" state to stop it and any associated movement.
    /* Stop the sequencer and clear the loop enabled flag */
    status = stopSequencer(true);

    return asynSuccess;
}

/**
 * Poll
 * @param moving
 * @return
 */
asynStatus pcsAxis::poll(bool *moving) {

    callParamCallbacks();
    return asynSuccess;
}

/**
 * Puts the axis in question into closed loop and sets the relevant
 * asyn parameters. This is only applicable to axes where primaryFeedback != POSITION_SENSOR
 * @param value Determines if the loop will be enabled or disabled.
 */
void pcsAxis::enableLoop(bool value) {

    double currentPositionScaled;
    static const char *functionName = "enableLoop";
    asynStatus status;

    currentPositionScaled = status_.position/scale_;

    if(primaryFeedback==POSITION_SENSOR)
        return;

    if(value){

        /* Set PID parameters */
        setSeqControlParams();

        moveSequencer.setElement("/sequencer_prog/synth_set[1]/acc", 0);
        moveSequencer.setElement("/sequencer_prog/synth_set[1]/rate", 0);
        moveSequencer.setElement("/sequencer_prog/synth_set[1]/end_rate", 0);
        moveSequencer.setElement("/sequencer_prog/synth_set[1]/end_ampl", currentPositionScaled);

        moveSequencer.setElement("/sequencer_prog/synth_set[2]/acc", 0);
        moveSequencer.setElement("/sequencer_prog/synth_set[2]/rate", 0);
        moveSequencer.setElement("/sequencer_prog/synth_set[2]/end_rate", 0);
        moveSequencer.setElement("/sequencer_prog/synth_set[2]/end_ampl", currentPositionScaled);
        setupSensorExitConditions();

        status = sendSequencer(functionName);

        ctrl_->wakeupPoller();

        setIntegerParam(ctrl_->PCS_A_enableLoop,1);
        callParamCallbacks();

    }else{
        stopSequencer(true);
    }
}

/**
 * Stops whatever sequencer is currently running on this slave
 * @param[in] clearEnableLoop Bool parameter that determines if the
 * enableLoop flag is cleared for this all axes or just the other
 * axes. True = clear all, False = only clear other axes
 * @return asynStatus
 * */
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

/**
 * Configures the parameters relating to the control loop of an axis by setting the relevant XML
 * elements of the moveSequencer object
 */
void pcsAxis::setSeqControlParams() {

    std::vector<std::pair<std::string,double> >::iterator pidIterator = controlSet_.begin();
    while(pidIterator!=controlSet_.end()){
        moveSequencer.setElement(pidIterator->first.c_str(), pidIterator->second);
        pidIterator++;
    }
}

/**
 * Sends the sequencer that is currently stored in the moveSequencer object
 * This method also stops whatever sequencer is currently running and clears
 * the loop enabled flag for all other axes but not this one as it's about to
 * enable the loop anyway.
 * @param functionName The name of the original function for trace purposes
 * @return asynStatus as a result of the asyn operation
 */
asynStatus pcsAxis::sendSequencer(const char* functionName) {

    /* Stop whatever sequencer is currently running on the given slave */
    stopSequencer(false);

    /* Send the sequencer to the controller */
    sprintf(ctrl_->outString_, moveSequencer.getXml().c_str());
    status = ctrl_->writeReadController();

    /* Update asyn param to allow what sequencer is loaded to be visible*/
    ctrl_->lock();
    ctrl_->setStringParam(ctrl_->PCS_C_XmlSequencer[slave_],ctrl_->outString_);
    ctrl_->setIntegerParam(ctrl_->PCS_C_UserXmlLoaded[slave_], 0);
    ctrl_->unlock();
    ctrl_->callParamCallbacks();

    /* Send the command to start the sequencer */
    sprintf(ctrl_->outString_,ctrl_->commandConstructor.getXml(slave_,SEQ_CONTROL_PARAM,"Program").c_str());
    status = ctrl_->writeReadController();

    asynPrint(ctrl_->pasynUserSelf, ASYN_TRACE_FLOW, "%s\n", functionName);
    setIntegerParam(ctrl_->motorStatusDone_,1);

    ctrl_->wakeupPoller();
    return status;
}

/**
 * Sets the moveSequencer object up in such a way that the sequencer to be sent will
 * remain in a running state until the minSensorVal is passed. This is only ever called for axes
 * that don't use the position sensor as feedback as position loops are enabled by default.
 */
void pcsAxis::setupSensorExitConditions() {

    moveSequencer.setElement("/sequencer_prog/clear[1]/ndrag", primaryFeedbackString);
    moveSequencer.setElement("/sequencer_prog/clear[3]/ndrag", primaryFeedbackString);
    moveSequencer.setElement("/sequencer_prog/clear[5]/ndrag", primaryFeedbackString);

    sprintf(minTrigger,"TRIG_MIN_PHY(%d)",primaryFeedback);
    moveSequencer.setElement("/sequencer_prog/trigger_set[1]/name", minTrigger);
    moveSequencer.setElement("/sequencer_prog/trigger_set[1]/value", minSensorVal_);
    moveSequencer.setElement("/sequencer_prog/trigger_set[2]/name", minTrigger);
    moveSequencer.setElement("/sequencer_prog/trigger_set[2]/value", minSensorVal_);
    moveSequencer.setElement("/sequencer_prog/trigger_set[3]/name", minTrigger);
    moveSequencer.setElement("/sequencer_prog/trigger_set[3]/value", minSensorVal_);

    moveSequencer.setElement("/sequencer_prog/clear[4]/triggers", minTrigger);
    moveSequencer.setElement("/sequencer_prog/wait[2]/triggers", minTrigger);
    moveSequencer.setElement("/sequencer_prog/clear[6]/triggers", minTrigger);
    moveSequencer.setElement("/sequencer_prog/wait[3]/triggers", minTrigger);

    sprintf(allTriggers,"TRIG_SYNTH_R;TRIG_MIN_PHY(%d)",primaryFeedback);
    moveSequencer.setElement("/sequencer_prog/clear[2]/triggers", allTriggers);
    moveSequencer.setElement("/sequencer_prog/wait[1]/triggers", allTriggers);
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