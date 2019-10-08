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

int Sequencer::setElement(std::string xPath, std::string value) {

	xmlChar *keyword;
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

        xmlFree(keyword);
		xmlXPathFreeObject (searchResult);
	}

}

std::string Sequencer::getXml() {
    xmlChar* xmlbuff;
    int buffersize;

    xmlDocDumpFormatMemory(pDoc, &xmlbuff, &buffersize, 1);
    return (char*)xmlbuff;
}