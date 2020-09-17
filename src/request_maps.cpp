#include "request_maps.h"
#include <imgui.h>
#include "web_request.h"
#include <misc/cpp/imgui_stdlib.h>
#include <nlohmann/json.hpp>
#include <cstdlib>

#include <Corrade/Utility/DebugStl.h>

Request_maps::Request_maps(Config_manager& config) : config{ config }, player{ config.config.player }
{

}

static auto acc(const Score& score)
{
    return static_cast<float>(300 * score.count_300 + 100 * score.count_100 + 50 * score.count_50)
           / static_cast<float>(300 * (score.count_300 + score.count_100 + score.count_50 + score.count_miss));
}

static std::string mods_string(int mods)
{
    if(mods == 0) return "NM";
    std::string mod_string = "";

    if((mods & 1) > 0) mod_string += "NF";
    if((mods & 2) > 0) mod_string += "EZ";
    if((mods & 4) > 0) mod_string += "TD";
    if((mods & 8) > 0) mod_string += "HD";
    if((mods & 16) > 0) mod_string += "HR";
    if((mods & 32) > 0) mod_string += "SD";
    if((mods & 64) > 0) mod_string += "DT";
    if((mods & 128) > 0) mod_string += "RX";
    if((mods & 256) > 0) mod_string += "HT";
    if((mods & 512) > 0) mod_string += "NC";
    if((mods & 1024) > 0) mod_string += "FL";
    if((mods & 2048) > 0) mod_string += "Auto";
    if((mods & 4096) > 0) mod_string += "SO";
    if((mods & 8192) > 0) mod_string += "AP";
    if((mods & 16384) > 0) mod_string += "PF";

    return mod_string;
}

void Request_maps::update()
{
    if(request_stage == Request_stage::fetching_user &&
       running_request.ready()) {
        const auto text = running_request.get();
        const auto json = nlohmann::json::parse(text);
        user_scores.clear();
        for(const auto score : json) {
            user_scores.push_back(score.get<Score>());
        }
        user_scores_strings.clear();
        for(const auto& score : user_scores) {
            user_scores_strings.push_back(score.beatmap_id + " " + std::to_string(score.pp));
        }
        if(!user_scores.empty()) {
            const auto pp_values = std::invoke([](const auto& scores)
                                               {
                                                   std::vector<float> values;
                                                   std::transform(scores.cbegin(), scores.cend(),
                                                                  std::back_inserter(values),
                                                                  [](const auto& score) { return score.pp; });
                                                   return values;
                                               }, user_scores);
            const auto pp_total = std::reduce(pp_values.cbegin(), pp_values.cend());
            min_pp = pp_total / pp_values.size();
            max_pp = pp_values.front() * 1.2f;

            maps_loading.clear();
            const auto max_number = std::min(static_cast<std::size_t>(30), user_scores.size());
            std::transform(user_scores.cbegin(), user_scores.cbegin() + max_number,
                           std::back_inserter(maps_loading),
                           [gamemode = this->gamemode, api_key = config.config.api_key](const auto& score)
                           {
                               return top_plays_map(score.beatmap_id, gamemode, api_key);
                           });
            request_stage = Request_stage::finding_players;
        } else {
            request_stage = Request_stage::idle;
        }
    } else if(request_stage == Request_stage::finding_players &&
              std::all_of(maps_loading.cbegin(), maps_loading.cend(),
                          [](const auto& map)
                          {
                              return map.ready();
                          })) {
        std::vector<std::string> map_scores;
        std::transform(maps_loading.begin(), maps_loading.end(), std::back_inserter(map_scores),
                       [](auto& map)
                       {
                           return map.get();
                       });
        maps_loading.clear();

        std::vector<std::vector<std::string>> player_lists;
        std::transform(map_scores.cbegin(), map_scores.cend(), std::back_inserter(player_lists),
                       [pp_target = min_pp](const auto& map_scores)
                       {
                           try {
                               const auto scores_json = nlohmann::json::parse(map_scores);
                               std::vector<Score> scores;
                               std::transform(scores_json.cbegin(), scores_json.cend(), std::back_inserter(scores),
                                              [](const auto& score)
                                              {
                                                  Score s{};
                                                  score.get_to(s);
                                                  return s;
                                              });
                               std::sort(scores.begin(), scores.end(),
                                         [pp_target](const auto& a, const auto& b)
                                         {
                                             return std::abs(a.pp - pp_target) < std::abs(b.pp - pp_target);
                                         });
                               const auto max_number = std::min(static_cast<std::size_t>(20), scores.size());
                               std::vector<std::string> players;
                               std::transform(scores.cbegin(), scores.cbegin() + max_number,
                                              std::back_inserter(players),
                                              [](const auto& score)
                                              {
                                                  return score.user_id;
                                              });
                               return players;
                           }
                           catch(const nlohmann::detail::parse_error&) {
                               return std::vector<std::string>{};
                           }
                       });
        std::vector<std::string> players;
        for(const auto& player_list : player_lists) {
            players.insert(players.end(), player_list.cbegin(), player_list.cend());
        }

        if(players.empty()) request_stage = Request_stage::idle;
        else {
            std::sort(players.begin(), players.end());
            players.erase(std::unique(players.begin(), players.end()), players.end());

            scores_loading.clear();
            std::transform(players.cbegin(), players.cend(), std::back_inserter(scores_loading),
                           [gamemode = this->gamemode, api_key = config.config.api_key](const auto& player)
                           {
                               return top_plays(player, gamemode, api_key);
                           });

            request_stage = Request_stage::loading_scores;
        }

    } else if(request_stage == Request_stage::loading_scores) {
        std::vector<Future> scores_loaded;
        for(auto& scores : scores_loading) {
            if(scores.ready()) {
                scores_loaded.push_back(std::move(scores));
            }
        }

        const auto removable = std::remove_if(scores_loading.begin(), scores_loading.end(),
                                              [](const auto& scores)
                                              {
                                                  return !scores.valid();
                                              });
        scores_loading.erase(removable, scores_loading.end());
        if(scores_loading.empty()) request_stage = Request_stage::idle;

        std::vector<std::vector<Score>> score_lists;
        std::transform(scores_loaded.begin(), scores_loaded.end(), std::back_inserter(score_lists),
                       [](auto& scores_string)
                       {
                           try {
                               const auto json = nlohmann::json::parse(scores_string.get());
                               std::vector<Score> scores;
                               for(const auto score : json) {
                                   scores.emplace_back();
                                   score.get_to(scores.back());
                               }
                               return scores;
                           }
                           catch(const nlohmann::detail::parse_error&) {
                               return std::vector<Score>{};
                           }
                       });

        std::vector<Score> new_scores;
        
        for(const auto& score_list : score_lists) {
            std::copy_if(score_list.cbegin(), score_list.cend(), std::back_inserter(new_scores),
                         [max_pp = max_pp, min_pp = min_pp, &user_scores = user_scores](const auto& score)
                         {
                             return score.pp <= max_pp && score.pp >= min_pp
                                    && std::none_of(user_scores.cbegin(), user_scores.cend(),
                                                    [id = score.beatmap_id](const auto& score)
                                                    {
                                                        return score.beatmap_id == id;
                                                    });
                         });
        }

        for(const auto& score : new_scores){
            if(const auto it = map_info.find(score.beatmap_id);
            it == map_info.end()){
                map_info_loading.push_back(get_map(score.beatmap_id));
                map_info.emplace(score.beatmap_id, std::nullopt);
            }
        }

        for(const auto& score : new_scores) {
            const auto pos = std::upper_bound(recommendations.begin(), recommendations.end(), score,
                                              [](const auto& a, const auto& b)
            {
                return a.beatmap_id < b.beatmap_id;
            });
            recommendations.insert(pos, score);
        }

        recommendations_strings.clear();
        std::transform(recommendations.cbegin(), recommendations.cend(), std::back_inserter(recommendations_strings),
                       [](const auto& score)
                       {
                           return score.beatmap_id + " - " + score.username + " " + std::to_string(score.pp) + " (" +
                                  std::to_string(score.maxcombo) + "): " + std::to_string(acc(score)) + " " +
                                  mods_string(score.enabled_mods);
                       });

    }

    std::vector<Future> map_info_loaded;
    for(auto& map : map_info_loading) {
        if(map.ready()) {
            map_info_loaded.push_back(std::move(map));
        }
    }
    const auto removable = std::remove_if(map_info_loading.begin(), map_info_loading.end(),
                                          [](const auto& scores)
                                          {
                                              return !scores.valid();
                                          });
    map_info_loading.erase(removable, map_info_loading.end());

    for(auto& map : map_info_loaded){
        const auto s = map.get();
        try {
            const auto json = nlohmann::json::parse(s);
            if(json.size() != 1){
                Magnum::Debug() << "Map_info doesn't contain 1 element" << s;
            }
            else{
                for(auto map : json){
                    auto beatmap = map.get<Beatmap>();
                    map_info[beatmap.map_id] = beatmap;
                }
            }
        }
        catch(const nlohmann::detail::parse_error&) {
            Magnum::Debug() << "Couldn't parse map_info" << s;
        }
    }
}

void Request_maps::request_window()
{
    if(ImGui::Begin("Request Maps")) {
        ImGui::RadioButton("osu!", reinterpret_cast<int*>(&gamemode), static_cast<int>(Gamemode::osu));
        ImGui::SameLine();
        ImGui::RadioButton("taiko!", reinterpret_cast<int*>(&gamemode), static_cast<int>(Gamemode::taiko));
        ImGui::SameLine();
        ImGui::RadioButton("ctb!", reinterpret_cast<int*>(&gamemode), static_cast<int>(Gamemode::ctb));
        ImGui::SameLine();
        ImGui::RadioButton("mania!", reinterpret_cast<int*>(&gamemode), static_cast<int>(Gamemode::mania));

        ImGui::InputText("Player", &player);
        ImGui::SameLine();
        if(ImGui::Button("Request")) request();

        ImGui::LabelText("Request Stage", stage_to_string(request_stage));

        std::vector<const char*> rec_items;
        std::transform(recommendations_strings.cbegin(), recommendations_strings.cend(), std::back_inserter(rec_items),
                       [](const auto& item)
                       {
                           return item.c_str();
                       });
        ImGui::ListBox("Recommendations", &recommendation_selection, rec_items.data(), rec_items.size(), 8);
        if(static_cast<std::size_t>(recommendation_selection) < recommendations.size()) {
            ImGui::LabelText("Map", recommendations[recommendation_selection].beatmap_id.c_str());

#ifdef __unix__
            ImGui::SameLine();
            if(ImGui::Button("Copy")){
                std::system(("echo " + recommendations[recommendation_selection].beatmap_id + " | xclip -selection clipboard").c_str());
            }
#endif
        }

        std::vector<const char*> items;
        std::transform(user_scores_strings.cbegin(), user_scores_strings.cend(), std::back_inserter(items),
                       [](const auto& item)
                       {
                           return item.c_str();
                       });

        int i = 0;
        ImGui::ListBox("User Scores", &i, items.data(), items.size(), 4);
        ImGui::InputFloat("min_pp", &min_pp, ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat("max_pp", &max_pp, ImGuiInputTextFlags_ReadOnly);
    }
    ImGui::End();

    if(ImGui::Begin("Map Info")){
        if(static_cast<std::size_t>(recommendation_selection) < recommendations.size()) {
            auto& score = recommendations[recommendation_selection];
            auto& map_id = score.beatmap_id;
            ImGui::LabelText("Map", map_id.c_str());

            if(!map_info[map_id]){
                ImGui::Text("Still loading info...");
            }
            else{
                auto& info = *map_info[map_id];
                ImGui::LabelText("Title", info.title.c_str());
                ImGui::LabelText("Artist", info.artist.c_str());
                ImGui::LabelText("Difficulty", info.diff_name.c_str());
                ImGui::InputFloat("Rating", &info.rating, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_ReadOnly);
                ImGui::InputFloat("Stars", &info.stars, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_ReadOnly);
                ImGui::InputFloat("CS", &info.cs, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_ReadOnly);
                ImGui::InputFloat("AR", &info.ar, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_ReadOnly);
                ImGui::InputFloat("OD", &info.od, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_ReadOnly);
                ImGui::InputFloat("HP", &info.hp, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_ReadOnly);
                ImGui::LabelText("Mods", mods_string(score.enabled_mods).c_str());
                ImGui::InputFloat("Acc", &score.pp, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_ReadOnly);
                ImGui::InputInt("Combo", &score.maxcombo, 1, 100, ImGuiInputTextFlags_ReadOnly);
                ImGui::InputInt("Max combo", &info.max_combo, 1, 100, ImGuiInputTextFlags_ReadOnly);
                float accu = acc(score);
                ImGui::InputFloat("Acc", &accu, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_ReadOnly);
                ImGui::LabelText("Player", score.username.c_str());
                ImGui::LabelText("hash", info.hash.c_str());
            }
        }
    }
    ImGui::End();
}

void Request_maps::request()
{
    recommendations.clear();
    running_request = top_plays(player, gamemode, config.config.api_key);
    request_stage = Request_stage::fetching_user;
}

Future Request_maps::get_map(const std::string& beatmap_id) const
{
    return get_url_async_ratelimited(
            "https://osu.ppy.sh/api/get_beatmaps?k=" + config.config.api_key + "&b=" + beatmap_id + "&m=" +
            std::to_string(static_cast<int>(gamemode)) + "&limit=5");
}

Future Request_maps::top_plays(std::string_view player, Gamemode gamemode, const std::string& api_key)
{
    return get_url_async_ratelimited(
            "https://osu.ppy.sh/api/get_user_best?k=" + api_key + "&u=" + std::string{ player } + "&m=" +
            std::to_string(static_cast<int>(gamemode)) + "&limit=100");
}

Future Request_maps::top_plays_map(std::string beatmap, Gamemode gamemode, const std::string& api_key)
{
    return get_url_async_ratelimited(
            "https://osu.ppy.sh/api/get_scores?k=" + api_key + "&m=" + std::to_string(static_cast<int>(gamemode)) +
            "&b=" + beatmap);
}
