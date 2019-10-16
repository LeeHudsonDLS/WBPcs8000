//
// Created by jjc62351 on 07/10/2019.
//

#include "Sequencer.h"

Sequencer::Sequencer(const std::string& xmlTemplateFile) {

    pDoc = xmlParseMemory(xmlTemplateFile.c_str(),xmlTemplateFile.size());
    if (pDoc == NULL ) {
        printf("Document not parsed successfully. \n");
        //Some error handling
    }else{
        printf("XML parsed successfully. \n");
    }

}

int Sequencer::setElement(const std::string& xPath, const std::string& value) {

    xmlChar *xpath = (xmlChar*) xPath.c_str();
    xmlXPathContextPtr context;

    context = xmlXPathNewContext(pDoc);
    if (context == NULL) {
        printf("Error in xmlXPathNewContext\n");
        return -1;
    }
    searchResult = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);
    if (searchResult == NULL) {
        printf("Error in xmlXPathEvalExpression\n");
        return -1;
    }
    if(xmlXPathNodeSetIsEmpty(searchResult->nodesetval)){
        xmlXPathFreeObject(searchResult);
        printf("No result\n");
        return -1;
    }

    if (searchResult) {
        nodeset = searchResult->nodesetval;

        //Set the value
        xmlNodeSetContent(nodeset->nodeTab[0],(const xmlChar*)value.c_str());

        xmlXPathFreeObject (searchResult);
    }

}


int Sequencer::setElement(const std::string& xPath, const int& value) {
    char buffer[255];
    sprintf(buffer,"%d\n",value);
    setElement(xPath,buffer);

}

int Sequencer::setElement(const std::string& xPath, const double& value) {
    char buffer[255];
    sprintf(buffer,"%f\n",value);
    setElement(xPath,buffer);

}

std::string Sequencer::getXml() {
    xmlChar* xmlbuff;
    int buffersize,pos;
    std::string xmlHeader = "<?xml version=\"1.0\"?>\n";
    std::string definitionsElement = "<definitions/>";
    std::string startElement = "<start/>";
    std::string commandString;

    xmlDocDumpFormatMemory(pDoc, &xmlbuff, &buffersize, 0);
    commandString = (char*)xmlbuff;


    //Remove XML header
    while ((pos = commandString.find(xmlHeader)) != std::string::npos)
        commandString.erase(pos, xmlHeader.size());

    while ((pos = commandString.find(definitionsElement)) != std::string::npos)
        commandString.replace(pos, definitionsElement.size(),"<definitions></definitions>");

    while ((pos = commandString.find(startElement)) != std::string::npos)
        commandString.replace(pos, startElement.size(),"<start></start>");

    return commandString;
}