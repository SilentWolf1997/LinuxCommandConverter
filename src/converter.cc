#include "converter.hh"
#include <iostream>

std::tuple<std::string, std::string> converter::get_command(std::string command) {
    std::string::size_type pos;
    pos = command.find(" ");
    if (pos == command.size() - 1) {
        return std::make_tuple(command.substr(0, pos), std::string());
    } else if (pos > command.size() - 1) {
        return std::make_tuple(command, std::string());
    } else {
        return std::make_tuple(command.substr(0, pos), command.substr(pos + 1));
    }
}

converter::converter() {
    initialize();
}

void converter::initialize() {
    command_map["dir"] = [](std::string command, std::string args) -> std::string {
        std::string res("ls");
        if (args.size() > 0) {
            res += " " + args;
        }
        return res;
    };
    command_map["rename"] = [](std::string command, std::string args) -> std::string {
        return "mv " + args;
    };
    command_map["move"] = [](std::string command, std::string args) -> std::string {
        return "mv " + args;
    };
    command_map["del"] = [](std::string command, std::string args) -> std::string {
        return "rm " + args;
    };
    command_map["cd"] = [](std::string command, std::string args) -> std::string {
        bool is_empty_args = true;
        if (args.size() > 0) {
            for (auto c : args) {
                if (c != ' ' && c != '\n' && c != '\0') {
                    is_empty_args = false;
                    break;
                }
            }
        }
        if (is_empty_args) {
            return std::string("pwd");
        } else {
            return "cd " + args;
        }
    };
}

std::string converter::convert(std::string command) {
    std::string cmd;
    std::string args;
    std::tie(cmd, args) = get_command(command);
    if (command_map.count(cmd) > 0) {
        return command_map[cmd](cmd, args);
    } else {
        return command;
    }
}
