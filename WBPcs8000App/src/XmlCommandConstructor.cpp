#include "XmlCommandConstructor.h"



XmlCommandConstructor::XmlCommandConstructor(const pcsController &ctrl_):ctrl_(ctrl_) {

}

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
    //Push into the vector of commands
    //commandMap.insert(std::pair<std::string,std::string>(parameter,xmlCommand));
    return xmlCommand;
}


std::string XmlCommandConstructor::extractEos(const std::string &xmlString) {
    int found = -1;
    std::string result;


    int i = strlen(xmlString.c_str());

    while((i>0&&found==-1)||(i<strlen(xmlString.c_str()))){
        if(found==1){
            result+=xmlString.c_str()[i];
        }
        if(xmlString.c_str()[i]=='/' && found ==-1){
            found=1;
            i=i-1;
            result+=xmlString.c_str()[i];
        }
        i+=found;
    }
    return result;
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
    std::vector<std::string> data;
    data.push_back(addXML(csvCommand,inputStringStream.str()));
    data.push_back(extractEos(data[0]));

    inputMap.insert(std::pair<std::string,std::vector<std::string> >(parameter,data));
}

/**
 * Adds an input parameter to the inputMap in xml form with the slave element. This version doesn't insert a value
 * as often the input comes from an XML element
 * @param parameter A key so the XML string can be retrieved from the inputMap Map container
 * @param csvCommand A comma delimited string representation of the desired XML
 */
void XmlCommandConstructor::addInputParameter(const std::string &parameter, const std::string &csvCommand) {

    std::vector<std::string> data;
    data.push_back(addXML(csvCommand,""));
    data.push_back(extractEos(data[0]));
    inputMap.insert(std::pair<std::string,std::vector<std::string> >(parameter,data));
}

/**
 * Looks up the parameter in the inputMap Map container and returns the xml string
 * @param parameter Key for the parameter
 * @return XML string for the parameter
 */
std::string XmlCommandConstructor::getInputXml(int axis, const std::string &parameter) {

    return appendSlave(axis-1,inputMap.find(parameter)->second[0]);
}

/**
 * Adds an parameter to the commandMap in xml form with the slave element added. Inserts a ! character as a place holder
 * for the value to be replaced when used
 * @param parameter A key so the XML string can be retrieved from the commandMap Map container
 * @param csvCommand A comma delimited string representation of the desired XML
 */
void XmlCommandConstructor::addParameter(const std::string &parameter, const std::string &csvCommand) {

    std::vector<std::string> data;
    data.push_back(addXML(csvCommand,"!"));
    data.push_back(extractEos(data[0]));
    commandMap.insert(std::pair<std::string,std::vector<std::string> >(parameter,data));
}

/**
 * Looks up the parameter in the commandMap Map container and returns the xml string
 * @param parameter Key for the parameter
 * @return XML string for the parameter
 */
std::string XmlCommandConstructor::getXml(int axis,const std::string &parameter) {

    std::string commandString = commandMap.find(parameter)->second[0];

    //Remove ! character for commands with no value
    commandString.erase(std::remove(commandString.begin(),commandString.end(),'!'),commandString.end());

    return appendSlave(axis-1,commandString);
}

/**
 * Looks up the parameter in the commandMap Map container and returns the xml string
 * @param parameter Key for the parameter
 * @param val Integer value to set the parameter to
 * @return XML string for the parameter
 */
std::string XmlCommandConstructor::getXml(int axis, const std::string &parameter,int val) {

    int pos;
    std::string commandString = commandMap.find(parameter)->second[0];
    std::stringstream inputStringStream;
    inputStringStream << val;

    while ((pos = commandString.find("!")) != std::string::npos)
        commandString.replace(pos, 1, inputStringStream.str());

    return appendSlave(axis-1,commandString);
}

/**
 * Looks up the parameter in the commandMap Map container and returns the xml string
 * @param parameter Key for the parameter
 * @param val String value to set the parameter to
 * @return XML string for the parameter
 */
std::string XmlCommandConstructor::getXml(int axis, const std::string &parameter,std::string val) {

    int pos;
    std::string commandString = commandMap.find(parameter)->second[0];

    while ((pos = commandString.find("!")) != std::string::npos)
        commandString.replace(pos, 1, val);

    return appendSlave(axis-1,commandString);
}


std::string XmlCommandConstructor::dumpXml() {

    std::map<std::string,std::vector<std::string> >::iterator iter1;

    for(iter1 = commandMap.begin(); iter1 != commandMap.end(); ++iter1){
        printf("%s\n",iter1->second[0].c_str());
    }


    for(iter1 = inputMap.begin(); iter1 != inputMap.end(); ++iter1){
        printf("%s\n",iter1->second[0].c_str());
    }

}

std::string XmlCommandConstructor::getEos(const std::string &parameter) {
    std::map<std::string,std::vector<std::string> >::iterator commandMapIter;
    std::map<std::string,std::vector<std::string> >::iterator inputMapIter;

    commandMapIter=commandMap.find(parameter);
    inputMapIter=inputMap.find(parameter);

    if(commandMapIter!=commandMap.end()){
        return commandMapIter->second[1];
    }

    if(inputMapIter!=inputMap.end()){
        return inputMapIter->second[1];
    }
    return "";
}