#ifndef SLIDER_DRAW_CONFIG_MANAGER_H
#define SLIDER_DRAW_CONFIG_MANAGER_H

#include <string>

struct Config{
    std::string api_key;
    std::string player;
};

class Config_manager{
public:
    Config_manager();
    void load();
    void save();
    void config_window();

    Config config;
    const char* save_status = nullptr;

private:
    bool show_apikey = false;
};

#endif //SLIDER_DRAW_CONFIG_MANAGER_H
