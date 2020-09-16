#ifndef OSU_TOP_REQUEST_MAPS_H
#define OSU_TOP_REQUEST_MAPS_H

#include "config_manager.h"
#include <string_view>
#include <future>
#include <vector>
#include "score.h"
#include "web_request.h"

enum class Gamemode : int{
    osu = 0,
    taiko = 1,
    ctb = 2,
    mania = 3
};

enum class Request_stage{
    idle,
    fetching_user,
    finding_players,
    loading_scores,
};

inline auto stage_to_string(const Request_stage stage)
{
    switch (stage) {
        case Request_stage::idle:
            return "idle";
        case Request_stage::fetching_user:
            return "fetching_user";
        case Request_stage::finding_players:
            return "finding_players";
        case Request_stage::loading_scores:
            return "loading_scores";
        default:
            return "invalid value";
    }
}

class Request_maps {
public:
    Request_maps(Config_manager& config);

    void update();
    void request_window();
    void request();
    static Future top_plays(std::string_view player, Gamemode gamemode, const std::string& api_key);
    static Future top_plays_map(std::string beatmap, Gamemode gamemode, const std::string& api_key);

    Config_manager& config;

    Gamemode gamemode = Gamemode::osu;
    std::string player = "";

    Future running_request;
    Request_stage request_stage = Request_stage::idle;

    std::vector<Score> user_scores;
    std::vector<Score> recommendations;
    std::vector<std::string> user_scores_strings;
    std::vector<std::string> recommendations_strings;
    int recommendation_selection = 0;

    float min_pp = 0.f;
    float max_pp = 0.f;

    std::vector<Future> maps_loading;
    std::vector<Future> scores_loading;
};


#endif //OSU_TOP_REQUEST_MAPS_H
