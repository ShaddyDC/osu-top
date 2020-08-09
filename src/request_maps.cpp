#include "request_maps.h"
#include <imgui.h>
#include "web_request.h"
#include <misc/cpp/imgui_stdlib.h>
#include <nlohmann/json.hpp>

Request_maps::Request_maps(Config_manager &config) : config{ config }, player{config.config.player }
{

}

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
            const auto max_index = std::min(static_cast<std::size_t>(29), user_scores.size() - 1);
            min_pp = user_scores[max_index].pp;
            max_pp = min_pp * 1.25f;
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
    top_plays(player, gamemode);
}

void Request_maps::top_plays(std::string_view player, Gamemode gamemode)
{
    running_request = get_url_async_ratelimited("https://osu.ppy.sh/api/get_user_best?k=" + config.config.api_key + "&u=" + std::string{player} + "&m=" + std::to_string(static_cast<int>(gamemode)) + "&limit=100");
    requesting = true;
}
