#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/wrappers/CanvasWrapper.h"
#include <vector>
#include <string>
#include <fstream>
#include <cctype>
#include <regex>

#pragma comment(lib, "pluginsdk.lib")

class ProgressTracker : public BakkesMod::Plugin::BakkesModPlugin
{
public:
    virtual void onLoad();
    virtual void onUnload();

private:

    std::vector<int> timeData;
    std::vector<int> deathData;

    bool isGraphVisible = false;

    std::string progressFilePath;
    std::string settingsFilePath;


    void SaveInputToFile(const std::string& input);
    std::vector<std::string> LoadInputsFromFile();
    bool IsValidTime(const std::string& str);
    bool IsValidDeaths(const std::string& str);
    void LoadDataFromFile(const std::string& filePath);
    void Render(CanvasWrapper canvas);
    bool IsDropdownItemExists(const std::string& item);
    bool AddItemToDropdown(const std::string& item);
    bool ValidFilename();
    void RemoveItemFromDropdown(const std::string& item);
    void SetBakkesmodFilePaths();
    void Log(std::string msg);
};
