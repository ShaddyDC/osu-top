#include "request_maps.h"
#include <imgui.h>
#include "web_request.h"
#include <misc/cpp/imgui_stdlib.h>

Request_maps::Request_maps(Config_manager &config) : config{ config }, player{config.config.player }
{

}

void Request_maps::update()
{
    if(requesting && running_request.wait_for(std::chrono::microseconds::zero()) == std::future_status::ready){
        requesting = false;
        text = running_request.get();
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
