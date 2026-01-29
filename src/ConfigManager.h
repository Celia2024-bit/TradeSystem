#pragma once
#include <string>
#include <map>
#include <fstream>

class ConfigManager {
public:
    static ConfigManager& instance() {
        static ConfigManager inst;
        return inst;
    }

    void load(const std::string& path) {
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) {
            auto pos = line.find('=');
            if (pos != std::string::npos) 
                data[line.substr(0, pos)] = line.substr(pos + 1);
        }
    }

    // 统一转成 double，用的时候再强转 int，最省事
    double get(const std::string& key, double def = 0) {
        return data.count(key) ? std::stod(data[key]) : def;
    }

private:
    std::map<std::string, std::string> data;
};