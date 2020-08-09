#ifndef OSU_TOP_REQUEST_MAPS_H
#define OSU_TOP_REQUEST_MAPS_H

#include "config_manager.h"
#include <string_view>
#include <future>
#include <vector>
#include "score.h"

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
    static std::future<std::string> top_plays(std::string_view player, Gamemode gamemode, const std::string& api_key);
    static std::future<std::string> top_plays_map(std::string beatmap, Gamemode gamemode, const std::string& api_key);

    Config_manager& config;

    Gamemode gamemode = Gamemode::osu;
    std::string player = "";

    std::future<std::string> running_request;
    bool requesting = false;

    std::string text = "";
    std::vector<Score> user_scores;
    std::vector<Score> recommendations;
    std::vector<std::string> user_scores_strings;
    std::vector<std::string> recommendations_strings;
    int recommendation_selection = 0;

    float min_pp = 0.f;
    float max_pp = 0.f;
};


#endif //OSU_TOP_REQUEST_MAPS_H
