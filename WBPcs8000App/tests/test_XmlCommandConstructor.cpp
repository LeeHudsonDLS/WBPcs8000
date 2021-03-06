//
// Created by jjc62351 on 13/02/2020.
//
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <boost/test/unit_test.hpp>
#include <unistd.h>
#include <signal.h>
#include <thread>

#define private public


//#include <libxml/xmlreader.h>
#define private public
#include "../src/pcsController.h"

BOOST_AUTO_TEST_SUITE(XmlCommandConstructorTest)
BOOST_AUTO_TEST_CASE(test_XmlCommandConstructor) {


}

/*
 * Tests:
 * std::string addXML(const std::string& csvCommand, const std::string& val);
 *
 */
BOOST_AUTO_TEST_CASE(test_XmlCommandConstructor_addXml){
    XmlCommandConstructor testObject;
    int index;
    printf("Testing XmlCommandConstructor::addXml():\n");
    BOOST_CHECK(!strcmp(testObject.addXML("level1,level2,level3","Value1").c_str(),"<level1><slave></slave><level2><level3>Value1</level3></level2></level1>"));

}

/*
 * Tests:
 * void addParameter(const std::string& parameter, const std::string& csvCommand);
 * std::string getXml(int slave, const std::string& parameter);
 * std::string getXml(int slave, const std::string& parameter, int val);
 * std::string getXml(int slave, const std::string& parameter, std::string val);
 *
 */
#define SET_INT_TEST "set_int_test"
BOOST_AUTO_TEST_CASE(test_XmlCommandConstructor_addParameter) {

    XmlCommandConstructor testObject;
    int index;

    printf("Testing XmlCommandConstructor::addParameter():\n");

    /* Set up some parameters */
    testObject.addParameter(START_UDP_CMD,"udpxmit,start");
    testObject.addParameter(CLEAR_UDP_CMD,"udpxmit,clear");
    testObject.addParameter(SEQ_CONTROL_PARAM,"sequencer,set,seq_state");
    testObject.addParameter(REGISTER_STREAM_PARAM,"udpxmit,register,stream");
    testObject.addParameter(SYS_STATE_PARAM,"maincontrol,set,sys_state");
    testObject.addParameter(SET_INT_TEST,"maincontrol,set,sys_state");

    /* Vectors containing the expected strings for these commands for each slave
     * number (0 - MAX_SLAVES) */
    std::vector<std::string> START_UDP_CMD_slaves;
    std::vector<std::string> CLEAR_UDP_CMD_slaves;
    std::vector<std::string> SEQ_CONTROL_PARAM_slaves;
    std::vector<std::string> REGISTER_STREAM_PARAM_slaves;
    std::vector<std::string> SYS_STATE_PARAM_slaves;
    std::vector<std::string> SET_INT_TEST_slaves;

    /* Setup the vectors for each slave */
    for(index = 0; index < MAX_SLAVES; index++) {
        char START_UDP_CMD_temp[128];
        char CLEAR_UDP_CMD_temp[128];
        char SEQ_CONTROL_PARAM_temp[128];
        char REGISTER_STREAM_PARAM_temp[128];
        char SYS_STATE_PARAM_temp[128];
        char SET_INT_TEST_temp[128];

        sprintf(START_UDP_CMD_temp,"<udpxmit><slave>%d</slave><start></start></udpxmit>",index);
        sprintf(CLEAR_UDP_CMD_temp,"<udpxmit><slave>%d</slave><clear></clear></udpxmit>",index);
        sprintf(SEQ_CONTROL_PARAM_temp,"<sequencer><slave>%d</slave><set><seq_state></seq_state></set></sequencer>",index);
        sprintf(REGISTER_STREAM_PARAM_temp,"<udpxmit><slave>%d</slave><register><stream>phys14</stream></register></udpxmit>",index);
        sprintf(SYS_STATE_PARAM_temp,"<maincontrol><slave>%d</slave><set><sys_state></sys_state></set></maincontrol>",index);
        sprintf(SET_INT_TEST_temp,"<maincontrol><slave>%d</slave><set><sys_state>%d</sys_state></set></maincontrol>",index,index+1);

        START_UDP_CMD_slaves.push_back(START_UDP_CMD_temp);
        CLEAR_UDP_CMD_slaves.push_back(CLEAR_UDP_CMD_temp);
        SEQ_CONTROL_PARAM_slaves.push_back(SEQ_CONTROL_PARAM_temp);
        REGISTER_STREAM_PARAM_slaves.push_back(REGISTER_STREAM_PARAM_temp);
        SYS_STATE_PARAM_slaves.push_back(SYS_STATE_PARAM_temp);
        SET_INT_TEST_slaves.push_back(SET_INT_TEST_temp);
    }


    /* Check those parameters are as expected for each slave*/
    for(index = 0; index < MAX_SLAVES; index++) {

        /* Test  getXml(int,string) */
        BOOST_CHECK(!strcmp(testObject.getXml(index, START_UDP_CMD).c_str(),START_UDP_CMD_slaves[index].c_str()));
        BOOST_CHECK(!strcmp(testObject.getXml(index, CLEAR_UDP_CMD).c_str(),CLEAR_UDP_CMD_slaves[index].c_str()));
        BOOST_CHECK(!strcmp(testObject.getXml(index, SEQ_CONTROL_PARAM).c_str(),SEQ_CONTROL_PARAM_slaves[index].c_str()));
        BOOST_CHECK(!strcmp(testObject.getXml(index, SYS_STATE_PARAM).c_str(),SYS_STATE_PARAM_slaves[index].c_str()));

        /* Test  getXml(int,string,string) */
        BOOST_CHECK(!strcmp(testObject.getXml(index, REGISTER_STREAM_PARAM,"phys14").c_str(),REGISTER_STREAM_PARAM_slaves[index].c_str()));

        /* Test  getXml(int,string,int) */
        BOOST_CHECK(!strcmp(testObject.getXml(index, SET_INT_TEST,index+1).c_str(),SET_INT_TEST_slaves[index].c_str()));
    }
}


/*
 * Tests:
 * void addInputParameter(const std::string& parameter, const std::string& csvCommand,int input);
 * std::string getInputXml(int slave, const std::string& parameter);
 */
BOOST_AUTO_TEST_CASE(test_XmlCommandConstructor_addInputParameter) {

        #define NO_OF_INPUTS 10

        XmlCommandConstructor testObject;
        std::vector<std::string> node1;
        int index,slaveIndex;

        printf("Testing XmlCommandConstructor::addInputParameter():\n");

        /* Setup the vectors for each input with the parameter name node1_%d*/
        for(index = 0; index < NO_OF_INPUTS; index++) {
            char node1_temp[128];
            sprintf(node1_temp,"node1_%d",index);
            node1.push_back(node1_temp);
            testObject.addInputParameter(node1_temp,GET_INPUT,index);
        }

        /* Check each combination of input and slave */
        for(index = 0; index < NO_OF_INPUTS; index++) {
            for(slaveIndex = 0; slaveIndex < MAX_SLAVES; slaveIndex++) {
                char buffer[128];
                sprintf(buffer, "<digital_io><slave>%d</slave><get><input>%d</input></get></digital_io>",slaveIndex, index);
                BOOST_CHECK(!strcmp(testObject.getInputXml(slaveIndex, node1[index]).c_str(), buffer));
            }
        }

    }

BOOST_AUTO_TEST_SUITE_END()

