#include "XmlCommandConstructor.h"

/*
 * Method to derive an XML string form a comma delimited string. Last element is the value.
 * @param csvCommand A string representing the desired XML
 */
void XmlCommandConstructor::addCSV(const std::string& csvCommand) {
    int direction = 1; // Flag used for bidirectional iteration of vector
    std::string xmlElement, xmlCommand;
    std::stringstream input(csvCommand);
    std::vector<std::string> singleCommand;
    std::vector<std::string>::iterator iterator,value;

    // Populate vector with xml elements using comma as delimiter
    while(std::getline(input,xmlElement,',')){
        singleCommand.push_back(xmlElement);
    }

    // Last element is the value
    value = singleCommand.end()-1;

    for(iterator = singleCommand.begin(); iterator <= value && iterator >= singleCommand.begin(); iterator+=direction) {
        if(iterator != value) {
            if(direction == 1)
                xmlCommand += "<" + *iterator + ">";
            else
                xmlCommand += "</" + *iterator + ">";
        }
        // If element is last then it must be the value
        if(iterator == value) {
            xmlCommand += *iterator;
            direction = -1;
        }
    }
    //Push into the vector of commands
    command.push_back(xmlCommand);
}
/*
 * A method to get the xml that has been stored in the object as a
 */
std::string XmlCommandConstructor::getXml() {
    std::string completeCommand;
    std::vector<std::string>::iterator iter;

    for(iter = command.begin(); iter < command.end(); ++iter)
        completeCommand+= *iter;

    return completeCommand;
}
