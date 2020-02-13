#include "XmlCommandConstructor.h"



/**
 * Method to derive an XML string form a comma delimited string. Only allows access to single element. Last element is the value.
 * @param csvCommand A string representing the desired XML
 * @param val A string representation of the value, this is usually a place holder ! character to be swapped in for a value at a later use
 * @return XML representation of the csv input with the value added to the last element and the slave element added
 */
std::string XmlCommandConstructor::addXML(const std::string& csvCommand, const std::string& val) {
    int direction = 1; // Flag used for bidirectional iteration of vector
    std::string xmlElement, xmlCommand;
    std::stringstream input(csvCommand);
    std::vector<std::string> singleCommand;
    std::vector<std::string>::iterator iterator,last;

    // Populate vector with xml elements using comma as delimiter
    while(std::getline(input,xmlElement,',')){
        singleCommand.push_back(xmlElement);
    }
    // Last element is the value
    last = singleCommand.end()-1;

    for(iterator = singleCommand.begin(); iterator <= last && iterator >= singleCommand.begin(); iterator+=direction) {
        if(iterator == last) {
            direction = -1;
            xmlCommand += "<" + *iterator + ">";
            xmlCommand += val;
            xmlCommand += "</" + *iterator + ">";
        }else{
            if(direction == 1) {
                xmlCommand += "<" + *iterator + ">";
                if(iterator == singleCommand.begin())
                    xmlCommand += "<slave></slave>";
            }else
                xmlCommand += "</" + *iterator + ">";
        }
    }

    return xmlCommand;
}


std::string XmlCommandConstructor::appendSlave(int slave, std::string xml) {

    int pos;
    std::stringstream slaveString;
    slaveString<<"<slave>"<< slave<<"</slave>";
    while ((pos = xml.find("<slave></slave>")) != std::string::npos)
        xml.replace(pos, 15, slaveString.str());

    return xml;
}


/**
 * Adds an input parameter to the inputMap in xml form with the slave element and the input value added.
 * @param parameter A key so the XML string can be retrieved from the inputMap Map container
 * @param csvCommand A comma delimited string representation of the desired XML
 * @param input The input number
 */
void XmlCommandConstructor::addInputParameter(const std::string &parameter, const std::string &csvCommand,int input) {
    std::stringstream inputStringStream;
    inputStringStream << input;

    inputMap.insert(std::pair<std::string,std::string >(parameter,addXML(csvCommand,inputStringStream.str())));
}

/**
 * Adds an input parameter to the inputMap in xml form with the slave element. This version doesn't insert a value
 * as often the input comes from an XML element
 * @param parameter A key so the XML string can be retrieved from the inputMap Map container
 * @param csvCommand A comma delimited string representation of the desired XML
 */
void XmlCommandConstructor::addInputParameter(const std::string &parameter, const std::string &csvCommand) {

    inputMap.insert(std::pair<std::string,std::string >(parameter,addXML(csvCommand,"")));
}

/**
 * Looks up the parameter in the inputMap Map container and returns the xml string
 * @param slave Slave number to append to xml
 * @param parameter Key for the parameter
 * @return XML string for the parameter
 */
std::string XmlCommandConstructor::getInputXml(int slave, const std::string &parameter) {

    return appendSlave(slave,inputMap.find(parameter)->second);
}

/**
 * Adds an parameter to the commandMap in xml form with the slave element added. Inserts a ! character as a place holder
 * for the value to be replaced when used
 * @param parameter A key so the XML string can be retrieved from the commandMap Map container
 * @param csvCommand A comma delimited string representation of the desired XML
 */
void XmlCommandConstructor::addParameter(const std::string &parameter, const std::string &csvCommand) {

    commandMap.insert(std::pair<std::string,std::string >(parameter,addXML(csvCommand,"!")));
}

/**
 * Looks up the parameter in the commandMap Map container and returns the xml string
 * @param slave Slave number to append to xml
 * @param parameter Key for the parameter
 * @return XML string for the parameter
 */
std::string XmlCommandConstructor::getXml(int slave,const std::string &parameter) {

    std::string commandString = commandMap.find(parameter)->second;

    //Remove ! character for commands with no value
    commandString.erase(std::remove(commandString.begin(),commandString.end(),'!'),commandString.end());

    return appendSlave(slave,commandString);
}

/**
 * Looks up the parameter in the commandMap Map container and returns the xml string
 * @param slave Slave number to append to xml
 * @param parameter Key for the parameter
 * @param val Integer value to set the parameter to
 * @return XML string for the parameter
 */
std::string XmlCommandConstructor::getXml(int slave, const std::string &parameter,int val) {

    int pos;
    std::string commandString = commandMap.find(parameter)->second;
    std::stringstream inputStringStream;
    inputStringStream << val;

    while ((pos = commandString.find("!")) != std::string::npos)
        commandString.replace(pos, 1, inputStringStream.str());

    return appendSlave(slave,commandString);
}

/**
 * Looks up the parameter in the commandMap Map container and returns the xml string
 * @param slave Slave number to append to xml
 * @param parameter Key for the parameter
 * @param val String value to set the parameter to
 * @return XML string for the parameter
 */
std::string XmlCommandConstructor::getXml(int slave, const std::string &parameter,std::string val) {

    int pos;
    std::string commandString = commandMap.find(parameter)->second;

    while ((pos = commandString.find("!")) != std::string::npos)
        commandString.replace(pos, 1, val);

    return appendSlave(slave,commandString);
}


std::string XmlCommandConstructor::dumpXml() {

    std::map<std::string,std::string>::iterator iter1;

    for(iter1 = commandMap.begin(); iter1 != commandMap.end(); ++iter1){
        printf("%s\n",iter1->second.c_str());
    }


    for(iter1 = inputMap.begin(); iter1 != inputMap.end(); ++iter1){
        printf("%s\n",iter1->second.c_str());
    }

}
