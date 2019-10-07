#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <stdio.h>
#include <sstream>


class XmlCommandConstructor{
public:
    void addCSV(const std::string& csvCommand);
    std::string getXml();
private:
    std::vector<std::string> command;
};