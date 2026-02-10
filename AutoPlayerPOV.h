#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include <memory>
#include <string>
#include <filesystem>

extern std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

class AutoPlayerPOV : public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase
{
public:
    virtual void onLoad();
    virtual void onUnload();
    void RenderSettings() override;

private:
    void CheckAndFocus();

    std::string savedMyPlayerId = "";
    std::string savedMyName = "";
    std::string configFilePath = "";

    // Toggle state
    bool isEnabled = true;

    // Prevents camera override spam within the same match
    std::string lastMatchGuid = "";

    void SaveData(std::string id, std::string name, bool enabled);
    void LoadData();
};