#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <stdio.h>
#include <sstream>
#include <algorithm>

class XmlCommandConstructor{
public:;

    void addInputParameter(const std::string& parameter, const std::string& csvCommand,int input);
    void addInputParameter(const std::string& parameter, const std::string& csvCommand);
    std::string getInputXml(int axis, const std::string& parameter);

    void addParameter(const std::string& parameter, const std::string& csvCommand);
    std::string getXml(int axis, const std::string& parameter);
    std::string getXml(int axis, const std::string& parameter, int val);
    std::string getXml(int axis, const std::string& parameter, std::string val);
    std::string getEos(const std::string& xmlString);

    std::string dumpXml();

private:
    std::map<std::string,std::string> commandMap;
    std::map<std::string,std::string> inputMap;
    std::string addXML(const std::string& csvCommand, const std::string& val);
    std::string appendSlave(int slave, std::string xml);
};
