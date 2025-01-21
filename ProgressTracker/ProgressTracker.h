#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include <vector>
#include <string>
#include <fstream>
#include <cctype>
#include <regex>
#include "bakkesmod/wrappers/CanvasWrapper.h"

#pragma comment(lib, "pluginsdk.lib")

class ProgressTracker : public BakkesMod::Plugin::BakkesModPlugin
{
public:
    virtual void onLoad();
    virtual void onUnload();

private:
    void Log(std::string msg);

    // Zeit- und Tod-Daten
    std::vector<int> timeData;  // Zeit in Sekunden
    std::vector<int> deathData; // Anzahl der Tode

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
};
