//
// Created by jjc62351 on 06/08/19.
//

#include "pcsAxis.h"
#include "pcsController.h"
#include "XmlTemplates.h"

#include <epicsExport.h>

pcsAxis::pcsAxis(pcsController *ctrl, int axisNo, int slave, const char* priFeedback, const char* secFeedback)
        :asynMotorAxis((asynMotorController *) ctrl, axisNo),
        ctrl_(ctrl),
        slave_(slave),
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

    if(primaryFeedback == 14)
        absoluteMoveSequencer.loadXML(absoluteMoveTemplate);
    else
        absoluteMoveSequencer.loadXML(moveWaitTemplate);

}

asynStatus pcsAxis::move(double position, int relative, double minVelocity, double maxVelocity, double acceleration){

    size_t nwrite,nread,nact;
    int eomReason;
    asynStatus status = asynSuccess;
    static const char *functionName = "move";
    char seqBuffer[4096];
    char rxBuffer[1024];
    double decelTime,decelDist,midPosition;
    printf("pcsAxis::move(%f) called\n",position);
    rxBuffer[0]='\0';
    ctrl_->inString_[0]='\0';


    absoluteMoveSequencer.setElement("//stream",primaryFeedbackString);

    absoluteMoveSequencer.setElement("//stream2",secondaryFeedbackString);

    /*
     * Set PID parameters from asyn parameters
     */

    std::vector<std::pair<std::string,double> >::iterator pidIterator = controlSet_.begin();
    while(pidIterator!=controlSet_.end()){
        absoluteMoveSequencer.setElement(pidIterator->first.c_str(),pidIterator->second);
        pidIterator++;
    }


    /*
     * Determine the amount of time the axis will take to decelerate and the amount the axis will travel
     * during this deceleration.
     */
    decelTime = maxVelocity/acceleration;
    decelDist = ((maxVelocity/scale_)*decelTime)/2;

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
    absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[1]/end_rate",maxVelocity/scale_);
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
    absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[2]/rate",maxVelocity/scale_);
    absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[2]/end_rate",0.001);
    absoluteMoveSequencer.setElement("/sequencer_prog/synth_set[2]/end_ampl",position/scale_);


    // Get the completed sequncer in XML form
    sprintf(seqBuffer,absoluteMoveSequencer.getXml().c_str());
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
    return asynSuccess;
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
    size_t nwrite,nread;
    int eomReason;
    asynStatus status = asynSuccess;
    char rxBuffer[1024];

    rxBuffer[0]='\0';
    printf ("Stop called \n");
    // Put sequencer into "Setup" state to stop it and any associated movement.
    status = pasynOctetSyncIO->writeRead(ctrl_->pasynUserController_,ctrl_->commandConstructor.getXml(slave_,SEQ_CONTROL_PARAM,"Setup").c_str(),strlen(ctrl_->commandConstructor.getXml(slave_,SEQ_CONTROL_PARAM,"Setup").c_str()),rxBuffer,1024,0.1,&nwrite,&nread,&eomReason);

    return asynSuccess;
}
asynStatus pcsAxis::setPosition(double position){
    printf("pcsAxis::setPosition() called\n");
    return asynSuccess;
}

asynStatus pcsAxis::poll(bool *moving) {

    //*moving = false;
    callParamCallbacks();
    /*
    int motorMovingStore;
    ctrl_->getIntegerParam(ctrl_->motorStatusMoving_,&motorMovingStore);
    if(motorMovingStore)
        *moving = true;
    else
        *moving = false;
    */
/*

    sprintf(ctrl_->outString_,"#%dp\r",axisNo_);
    ctrl_->writeReadController();
 */

    return asynSuccess;

}

pcsAxis::~pcsAxis(){

}



/** Axis configuration command, called directly or from iocsh.
  * \param[in] portName The name of the controller asyn port.
  * \param[in] axisNum The number of the axis, zero based.
  * \param[in] homeMode The homing mode of the axis
  */
extern "C" int pcsAxisConfig(const char *controllerName, int axisNum, int slave, const char *priFeedback, const char *secFeedback)
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
        new pcsAxis(controller, axisNum,slave,priFeedback,secFeedback);
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

static const iocshArg *const pcsAxisConfigArgs[] = {&pcsAxisConfigArg0,
                                                    &pcsAxisConfigArg1,
                                                    &pcsAxisConfigArg2,
                                                    &pcsAxisConfigArg3,
                                                    &pcsAxisConfigArg4};

static const iocshFuncDef configPcsAxis =
        {"pcsAxisConfig", 5, pcsAxisConfigArgs};

static void configPcsAxisCallFunc(const iocshArgBuf *args)
{
    pcsAxisConfig(args[0].sval, args[1].ival, args[2].ival,args[3].sval,args[4].sval);
}

static void PcsAxisRegister(void)
{
    iocshRegister(&configPcsAxis, configPcsAxisCallFunc);
}


extern "C" { epicsExportRegistrar(PcsAxisRegister); }