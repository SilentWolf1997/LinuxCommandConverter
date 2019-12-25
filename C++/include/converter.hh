#ifndef __CONVERTER_HH__
#define __CONVERTER_HH__

#include <map>
#include <vector>
#include <tuple>
#include <string>
#include <functional>

class converter {
private:
    std::map<std::string, std::function<std::string (std::string, std::string)>> command_map;

    std::tuple<std::string, std::string> get_command(std::string command);
public:
    converter();

    void initialize();
    std::string convert(std::string command);
};

#endif
