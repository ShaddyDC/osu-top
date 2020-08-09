#ifndef OSU_TOP_SCORE_H
#define OSU_TOP_SCORE_H

#include <nlohmann/json.hpp>

struct Score{
    std::string beatmap_id;
    std::string score_id;
    uint64_t score;
    int maxcombo;
    int count_50;
    int count_100;
    int count_300;
    int count_miss;
    int count_katu;
    int count_geki;
    bool perfect;
    int enabled_mods;
    std::string user_id;
    std::string date;
    std::string rank;
    float pp;
    bool replay_available;
    std::string username;
};

inline void from_json(const nlohmann::json& j, Score& score)
{
    std::string buffer;
    score.beatmap_id = j.value("beatmap_id", "");
    j.at("score_id").get_to(score.score_id);
    j.at("score").get_to(buffer);
    score.score = std::stoll(buffer.c_str());
    j.at("maxcombo").get_to(buffer);
    score.maxcombo = std::stoi(buffer.c_str());
    j.at("count50").get_to(buffer);
    score.count_50 = std::stoi(buffer.c_str());
    j.at("count100").get_to(buffer);
    score.count_100 = std::stoi(buffer.c_str());
    j.at("count300").get_to(buffer);
    score.count_300 = std::stoi(buffer.c_str());
    j.at("countmiss").get_to(buffer);
    score.count_miss = std::stoi(buffer.c_str());
    j.at("countkatu").get_to(buffer);
    score.count_katu = std::stoi(buffer.c_str());
    j.at("countgeki").get_to(buffer);
    score.count_geki = std::stoi(buffer.c_str());
    j.at("perfect").get_to(buffer);
    score.perfect = static_cast<bool>(std::stoi(buffer.c_str()));
    j.at("enabled_mods").get_to(buffer);
    score.enabled_mods = std::stoi(buffer.c_str());
    j.at("user_id").get_to(score.user_id);
    j.at("date").get_to(score.date);
    j.at("rank").get_to(score.rank);
    j.at("pp").get_to(buffer);
    score.pp = std::stof(buffer.c_str());
    j.at("replay_available").get_to(buffer);
    score.replay_available = static_cast<bool>(std::stoi(buffer.c_str()));
    score.username = j.value("username", "");
}

#endif //OSU_TOP_SCORE_H
