#include "pch.h"
#include "AutoPlayerPOV.h"
#include "bakkesmod/wrappers/SpectatorHUDWrapper.h"
#include <fstream>
#include <filesystem>

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
namespace fs = std::filesystem;

BAKKESMOD_PLUGIN(AutoPlayerPOV, "Auto-PlayerPOV (Saved Replays)", "1.0", PLUGINTYPE_REPLAY)

void AutoPlayerPOV::SaveData(std::string id, std::string name, bool enabled) {
    if (configFilePath.empty()) return;

    fs::path p(configFilePath);
    if (!fs::exists(p.parent_path())) {
        fs::create_directories(p.parent_path());
    }

    std::ofstream file(configFilePath, std::ios::trunc);
    if (file.is_open()) {
        file << id << "\n" << name << "\n" << (enabled ? "1" : "0");
        file.close();
    }
}

void AutoPlayerPOV::LoadData() {
    if (configFilePath.empty()) return;

    std::ifstream file(configFilePath);
    if (!file.is_open()) return;

    if (std::getline(file, savedMyPlayerId)) {
        std::getline(file, savedMyName);
        std::string enabledStr;
        if (std::getline(file, enabledStr)) {
            isEnabled = (enabledStr == "1");
        }
    }
    file.close();
}

void AutoPlayerPOV::onLoad()
{
    _globalCvarManager = cvarManager;
    configFilePath = gameWrapper->GetBakkesModPath().string() + "\\data\\autofocus_config.txt";

    LoadData();

    cvarManager->registerNotifier("replay_focus_me", [this](std::vector<std::string> args) {
        lastMatchGuid = "";
        CheckAndFocus();
        }, "Forces camera focus on your saved player profile", PERMISSION_ALL);

    std::vector<std::string> eventHooks = {
        "Function TAGame.GameEvent_TA.PostBeginPlay",
        "Function GameEvent_Soccar_TA.Active.StartRound"
    };

    for (const std::string& eventName : eventHooks) {
        gameWrapper->HookEvent(eventName, [this](std::string name) {
            gameWrapper->SetTimeout([this](GameWrapper* gw) {
                this->CheckAndFocus();
                }, 1.5f);
            });
    }

    gameWrapper->HookEvent("Function TAGame.PlayerController_TA.OnReceivedPlayerAndPRI", [this](std::string name) {
        gameWrapper->Execute([this](GameWrapper* gw) {
            auto pc = gw->GetPlayerController();
            if (pc.IsNull()) return;
            auto pri = pc.GetPRI();
            if (pri.IsNull()) return;

            std::string currentId = pri.GetUniqueIdWrapper().GetIdString();
            std::string currentName = pri.GetPlayerName().ToString();

            if (currentId != "Unknown|0|0" && !currentId.empty()) {
                if (savedMyPlayerId != currentId) {
                    savedMyPlayerId = currentId;
                    savedMyName = currentName;
                    SaveData(savedMyPlayerId, savedMyName, isEnabled);
                }
            }
            });
        });
}

void AutoPlayerPOV::CheckAndFocus()
{
    // Check if the plugin is toggled ON before doing anything
    if (!isEnabled || !gameWrapper->IsInReplay() || savedMyPlayerId.empty()) return;

    ServerWrapper server = gameWrapper->GetCurrentGameState();
    if (server.IsNull()) return;

    std::string currentMatchGuid = server.GetMatchGUID();
    if (!lastMatchGuid.empty() && currentMatchGuid == lastMatchGuid) {
        return;
    }

    PlayerControllerWrapper pc = gameWrapper->GetPlayerController();
    if (pc.IsNull()) return;

    SpectatorHUDWrapper spectatorHUD = pc.GetSpectatorHud();
    if (spectatorHUD.IsNull()) return;

    ReplayViewerDataWrapper replayViewer = spectatorHUD.GetViewerData();
    if (replayViewer.IsNull()) return;

    auto pris = server.GetPRIs();
    PriWrapper foundPri(0);

    for (int i = 0; i < pris.Count(); i++) {
        PriWrapper pri = pris.Get(i);
        if (pri.IsNull()) continue;

        if (pri.GetUniqueIdWrapper().GetIdString() == savedMyPlayerId) {
            foundPri = pri;
            break;
        }
    }

    if (!foundPri.IsNull()) {
        spectatorHUD.SetFocusActorString("Player_" + foundPri.GetUniqueIdWrapper().GetIdString());
        replayViewer.SetCameraMode("PlayerView");
        lastMatchGuid = currentMatchGuid;
    }
}

void AutoPlayerPOV::RenderSettings()
{
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Auto-PlayerPOV (Saved Replays)");
    ImGui::Text("Created by pMaleK");
    ImGui::Separator();
    ImGui::Spacing();

    // The Toggle
    if (ImGui::Checkbox("Enable Auto-Focus", &isEnabled)) {
        SaveData(savedMyPlayerId, savedMyName, isEnabled);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (savedMyName.empty()) {
        ImGui::Text("Focused Player: [None Detected]");
        ImGui::TextWrapped("Note: You must join at least one game (Online, Local, or Freeplay) "
            "to automatically set your Focused Player profile.");
    }
    else {
        ImGui::Text("Focused Player: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", savedMyName.c_str());
    }

    ImGui::Spacing();

    if (ImGui::Button("Manual Focus Now")) {
        lastMatchGuid = "";
        gameWrapper->Execute([this](GameWrapper* gw) { this->CheckAndFocus(); });
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset Profile")) {
        savedMyPlayerId = "";
        savedMyName = "";
        lastMatchGuid = "";
        SaveData("", "", isEnabled);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "How it works:");
    ImGui::TextWrapped("This plugin detects when you open a saved replay and automatically "
        "switches the camera to your POV (Player View) once at the start of the match.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Quick-Focus Keybind:");
    ImGui::TextWrapped("If you switch cameras mid-replay and want to return to your POV instantly:");
    ImGui::Spacing();
    ImGui::BulletText("Open 'Bindings' tab in F2.");
    ImGui::BulletText("Add a new key and enter the command:");
    ImGui::Indent();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "replay_focus_me");
    ImGui::Unindent();
}

void AutoPlayerPOV::onUnload() {}