//
// Created by jjc62351 on 07/10/2019.
//
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <stdio.h>
#include <sstream>
#include <libxml/parser.h>
#include <libxml/xpath.h>

class Sequencer {
public:
    Sequencer(std::string xmlTemplateFile);
    int setElement(std::string xPath, std::string value);
    std::string getXml();
private:
    xmlDocPtr pDoc;
    xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr searchResult;
};


