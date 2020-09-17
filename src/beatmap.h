//
// Created by space on 16.09.20.
//

#ifndef OSU_TOP_BEATMAP_H
#define OSU_TOP_BEATMAP_H

#include <string>

struct Beatmap{
    enum class State{
        loved = 4,
        qualified = 3,
        approved = 2,
        ranked = 1,
        pending = 0,
        wip = -1,
        graveyard = -2,
    };
    State state;
    std::string submit_date;
    std::string approved_date;
    std::string last_update;
    std::string artist;
    std::string map_id;
    std::string mapset_id;
    float bpm;
    std::string creator;
    std::string creator_id;
    float stars;
    float diff_aim;
    float diff_speed;
    float cs;
    float od;
    float ar;
    float hp;
    float hit_length;
    std::string source;
    int genre_id;
    int language_id;
    std::string title;
    float total_length;
    std::string diff_name;
    std::string hash;
    int mode;
    std::string tags;
    int favourites;
    float rating;
    int playcount;
    int passcount;
    int count_circle;
    int count_slider;
    int count_spinner;
    int max_combo;
    bool storyboard;
    bool video;
    bool download_unavailable;
    bool audio_unavailable;
};

inline void from_json(const nlohmann::json& j, Beatmap& bm)
{
    bm.state = static_cast<Beatmap::State>(std::stoi(j.value("approved", "0")));
    bm.submit_date = j.value("submit_date", "");
    bm.approved_date = j.value("approved_date", "");
    bm.last_update = j.value("last_update", "");
    bm.artist = j.value("artist", "");
    bm.map_id = j.value("beatmap_id", "");
    bm.mapset_id = j.value("beatmapset_id", "");
    bm.bpm = std::stof(j.value("bpm", ""));
    bm.creator = j.value("creator", "");
    bm.creator_id = j.value("creator_id", "");
    bm.stars = std::stof(j.value("difficultyrating", ""));
    bm.diff_aim = std::stof(j.value("diff_aim", ""));
    bm.diff_speed = std::stof(j.value("diff_speed", ""));
    bm.cs = std::stof(j.value("diff_size", ""));
    bm.od = std::stof(j.value("diff_overall", ""));
    bm.ar = std::stof(j.value("diff_approach", ""));
    bm.hp = std::stof(j.value("diff_drain", ""));
    bm.hit_length = std::stof(j.value("hit_length", ""));
    bm.source = j.value("source", "");
    bm.genre_id = std::stoi(j.value("genre_id", ""));
    bm.language_id = std::stoi(j.value("language_id", ""));
    bm.title = j.value("title", "");
    bm.language_id = std::stoi(j.value("language_id", ""));
    bm.total_length = std::stof(j.value("total_length", ""));
    bm.diff_name = j.value("version", "");
    bm.hash = j.value("file_md5", "");
    bm.mode = std::stoi(j.value("mode", ""));
    bm.tags = j.value("tags", "");
    bm.favourites = std::stoi(j.value("favourite_count", ""));
    bm.rating = std::stof(j.value("rating", ""));
    bm.playcount = std::stoi(j.value("playcount", ""));
    bm.passcount = std::stoi(j.value("passcount", ""));
    bm.count_circle = std::stoi(j.value("count_normal", ""));
    bm.count_slider = std::stoi(j.value("count_slider", ""));
    bm.count_spinner = std::stoi(j.value("count_spinner", ""));
    bm.max_combo = std::stoi(j.value("max_combo", ""));
    bm.storyboard = static_cast<bool>(std::stoi(j.value("storyboard", "")));
    bm.video = static_cast<bool>(std::stoi((j.value("video", ""))));
    bm.download_unavailable = static_cast<bool>(std::stoi(j.value("download_unavailable", "")));
    bm.audio_unavailable = static_cast<bool>(std::stof(j.value("audio_unavailable", "")));
}

#endif //OSU_TOP_BEATMAP_H
