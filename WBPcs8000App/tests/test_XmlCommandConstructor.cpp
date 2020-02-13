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



//#include <libxml/xmlreader.h>
#define private public
#include "../src/pcsController.h"

BOOST_AUTO_TEST_SUITE(XmlCommandConstructorTest)
BOOST_AUTO_TEST_CASE(test_XmlCommandConstructor) {



    XmlCommandConstructor testObject;
    testObject.addParameter(START_UDP_CMD,"udpxmit,start");
    testObject.addParameter(CLEAR_UDP_CMD,"udpxmit,clear");

    BOOST_CHECK(!strcmp(testObject.getXml(0,START_UDP_CMD).c_str(),"<udpxmit><slave>0</slave><start></start></udpxmit>"));

}

BOOST_AUTO_TEST_SUITE_END()

