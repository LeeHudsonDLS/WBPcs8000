#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <stdio.h>
#include <sstream>
#include <algorithm>

class XmlCommandConstructor{
public:
    XmlCommandConstructor(int slaveNo);

    void addInputParameter(const std::string& parameter, const std::string& csvCommand,int input);
    void addInputParameter(const std::string& parameter, const std::string& csvCommand);
    std::string getInputXml(const std::string& parameter);

    void addParameter(const std::string& parameter, const std::string& csvCommand);
    std::string getXml(const std::string& parameter);
    std::string getXml(const std::string& parameter, int val);
    std::string getXml(const std::string& parameter, std::string val);

private:
    int slaveNo;
    std::string slaveXml;
    std::vector<std::string> command;
    std::map<std::string,std::string> commandMap;
    std::map<std::string,std::string> inputMap;
    std::string addXML(const std::string& csvCommand, const std::string& val);
};
