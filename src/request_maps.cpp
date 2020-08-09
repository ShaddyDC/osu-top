#include "request_maps.h"
#include <imgui.h>
#include "web_request.h"
#include <misc/cpp/imgui_stdlib.h>
#include <nlohmann/json.hpp>
#include <execution>

Request_maps::Request_maps(Config_manager &config) : config{ config }, player{config.config.player }
{

}

auto acc(const Score& score)
{
    return static_cast<float>(300 * score.count_300 + 100 * score.count_100 + 50 * score.count_50)
        / static_cast<float>(300 * (score.count_300 + score.count_100 + score.count_50 + score.count_miss));
}

#include <Corrade/Utility/Debug.h>

void Request_maps::update()
{
    if(requesting && running_request.wait_for(std::chrono::microseconds::zero()) == std::future_status::ready){
        requesting = false;
        text = running_request.get();
        const auto json = nlohmann::json::parse(text);
        user_scores.clear();
        for(const auto score : json){
            user_scores.push_back(score.get<Score>());
        }
        user_scores_strings.clear();
        for(const auto& score : user_scores){
            user_scores_strings.push_back(score.beatmap_id + " " + std::to_string(score.pp));
        }
        if(!user_scores.empty()){
            const auto max_index = std::min(static_cast<std::size_t>(30), user_scores.size() - 1);
            const auto pp_values = std::invoke([](const auto& scores, const auto max_index)
                                               {
                                                    std::vector<float> values;
                                                    std::transform(scores.cbegin(), scores.cbegin() + max_index, std::back_inserter(values),
                                                                   [](const auto& score){ return score.pp; });
                                                    return values;
                                               }, user_scores, max_index);
            const auto pp_total = std::reduce(pp_values.cbegin(), pp_values.cend());
            min_pp = pp_total / pp_values.size();
            max_pp = min_pp * 1.25f;

            std::vector<std::future<std::string>> maps_loading(max_index);
            std::transform(std::execution::par_unseq, user_scores.cbegin(), user_scores.cbegin() + max_index, maps_loading.begin(),
                          [gamemode = this->gamemode, api_key = config.config.api_key](const auto& score)
            {
                return top_plays_map(score.beatmap_id, gamemode, api_key);
            });

            std::vector<std::string> map_scores;
            std::transform(maps_loading.begin(), maps_loading.end(), std::back_inserter(map_scores),
                           [](auto& map)
                           {
                                return map.get();
                           });

            std::vector<std::vector<std::string>> player_lists(map_scores.size());
            // std::transform(std::execution::par_unseq, map_scores.cbegin(), map_scores.cend(), player_lists.begin(),
           std::transform(std::execution::par_unseq, map_scores.cbegin(), map_scores.cend(), player_lists.begin(),
                           [pp_target = min_pp](const auto& map_scores)
                           {
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
                                const auto max_number = std::min(static_cast<std::size_t>(20), scores.size() - 1);
                                std::vector<std::string> players;
                                std::transform(scores.cbegin(), scores.cbegin() + max_number, std::back_inserter(players),
                                               [](const auto& score)
                                               {
                                                    return score.user_id;
                                               });
                                return players;
                           });
            std::vector<std::string> players;
            for(const auto& player_list : player_lists){
                players.insert(players.end(), player_list.cbegin(), player_list.cend());
            }
            std::sort(players.begin(), players.end());
            players.erase(std::unique(players.begin(), players.end()), players.end());

            std::vector<std::vector<Score>> score_lists(players.size());
            std::transform(std::execution::par_unseq, players.cbegin(), players.cend(), score_lists.begin(),
                          [gamemode = this->gamemode, api_key = config.config.api_key](const auto& player)
                          {
                                const auto scores_string = top_plays(player, gamemode, api_key).get();
                                const auto json = nlohmann::json::parse(scores_string);
                                std::vector<Score> scores;
                                for(const auto score : json){
                                    scores.emplace_back();
                                    score.get_to(scores.back());
                                }
                                return scores;
                          });
            recommendations.clear();
            for(const auto& score_list : score_lists){
                recommendations.insert(recommendations.end(), score_list.cbegin(), score_list.cend());
            }

            recommendations.erase(std::remove_if(recommendations.begin(), recommendations.end(),
                                                 [max_pp = max_pp, min_pp = min_pp](const auto& score)
                                                 {
                                                    score.pp > max_pp || score.pp < min_pp;
                                                 }), recommendations.end());
            std::sort(recommendations.begin(), recommendations.end(), [](const auto& a, const auto& b)
            {
                return a.beatmap_id < b.beatmap_id;
            });

            std::transform(recommendations.cbegin(), recommendations.cend(), std::back_inserter(recommendations_strings),
                          [](const auto& score)
                          {
                                return score.beatmap_id + " - " + score.username + " " + std::to_string(score.pp) + " (" + std::to_string(score.maxcombo) + "): " + std::to_string(acc(score));
                          });
        }
    }
}

void Request_maps::request_window()
{
    if(ImGui::Begin("Request Maps")){
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


        std::vector<const char*> rec_items;
        std::transform(recommendations_strings.cbegin(), recommendations_strings.cend(), std::back_inserter(rec_items),
                       [](const auto& item)
                       {
                           return item.c_str();
                       });
        ImGui::ListBox("User Scores", &recommendation_selection, rec_items.data(), rec_items.size(), 8);
        if(recommendation_selection < recommendations.size()){
            ImGui::SameLine();
            ImGui::LabelText("Map", recommendations[recommendation_selection].beatmap_id.c_str());
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

        ImGui::InputTextMultiline("text", text.data(), text.size());
    }
    ImGui::End();
}

void Request_maps::request()
{
    running_request = top_plays(player, gamemode, config.config.api_key);
    requesting = true;
}

std::future<std::string> Request_maps::top_plays(std::string_view player, Gamemode gamemode, const std::string& api_key)
{
    return get_url_async_ratelimited("https://osu.ppy.sh/api/get_user_best?k=" + api_key + "&u=" + std::string{player} + "&m=" + std::to_string(static_cast<int>(gamemode)) + "&limit=100");
}

std::future<std::string> Request_maps::top_plays_map(std::string beatmap, Gamemode gamemode, const std::string& api_key)
{
    return get_url_async_ratelimited("https://osu.ppy.sh/api/get_scores?k=" + api_key + "&m=" + std::to_string(static_cast<int>(gamemode)) + "&b=" + beatmap);
}
