#ifndef OSU_TOP_REQUEST_MAPS_H
#define OSU_TOP_REQUEST_MAPS_H

#include "config_manager.h"
#include <string_view>
#include <future>

enum class Gamemode : int{
    osu = 0,
    taiko = 1,
    ctb = 2,
    mania = 3
};

class Request_maps {
public:
    Request_maps(Config_manager& config);

    void update();
    void request_window();
    void request();
    void top_plays(std::string_view player, Gamemode gamemode);

    Config_manager& config;

    Gamemode gamemode = Gamemode::osu;
    std::string player = "";

    std::future<std::string> running_request;
    bool requesting = false;

    std::string text = "";
};


#endif //OSU_TOP_REQUEST_MAPS_H
