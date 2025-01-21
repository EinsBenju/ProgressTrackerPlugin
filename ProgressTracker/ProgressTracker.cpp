#include "pch.h"
#include "ProgressTracker.h"
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <filesystem>

BAKKESMOD_PLUGIN(ProgressTracker, "ProgressTracker", "1.0", PERMISSION_ALL)

void ProgressTracker::onLoad() {
    SetBakkesmodFilePaths();

    cvarManager->registerCvar("progresstracker_file_name", "none", "File");
    cvarManager->getCvar("progresstracker_file_name").setValue("none");
    cvarManager->registerCvar("progresstracker_file_name_input", "", "Filename Input", true, false, 0.0f, false, 0.0f, false);

    // Lade gespeicherte Eingaben aus der Datei
    LoadInputsFromFile();

    // CVar für Benutzereingabe
    cvarManager->registerCvar("progresstracker_time_input", "", "Time Input", true, false, 0.0f, false, 0.0f, false);
    cvarManager->registerCvar("progresstracker_deaths_input", "", "Deaths Input", true, false, 0.0f, false, 0.0f, false);

    // Button: Eingabe speichern
    cvarManager->registerNotifier("progresstracker_submit", [this](std::vector<std::string> args) {
        if (!ValidFilename()) {
            this->Log("Filename is empty or none, select different file or add new file!");
            return;
        }
        // Eingaben aus Input Feldern bekommen
        std::string time = cvarManager->getCvar("progresstracker_time_input").getStringValue();
        std::string deaths = cvarManager->getCvar("progresstracker_deaths_input").getStringValue();

        // Überprüfen ob Eingaben valide sind danach speichern
        if (IsValidTime(time) && IsValidDeaths(deaths)) {
            SaveInputToFile(time + " " + deaths);  // Direkt in die Datei schreiben
            this->Log("Input saved: " + time + " " + deaths);

            // Input-Felder clearen, falls Submit erfolgreich
            cvarManager->getCvar("progresstracker_time_input").setValue("");
            cvarManager->getCvar("progresstracker_deaths_input").setValue("");
        }
        else {
            this->Log("Invalid input!");

            // Fehler anzeigen
            cvarManager->getCvar("progresstracker_time_input").setValue("Invalid Input, must be {int}:{int}:2");
            cvarManager->getCvar("progresstracker_deaths_input").setValue("Invalid Input, must be {int}");
        }
        }, "Submit input for ProgressTracker", PERMISSION_ALL);

    // Button: Alle gespeicherten Werte ausgeben
    cvarManager->registerNotifier("progresstracker_print_all", [this](std::vector<std::string> args) {
        if (!ValidFilename()) {
            this->Log("Filename is empty or none, select different file or add new file!");
            return;
        }
        // Lade gespeicherte Eingaben aus der Datei
        auto inputs = LoadInputsFromFile();
        this->Log("Saved Values:");
        for (const std::string& input : inputs) {
            this->Log(input);
        }
        }, "Print all saved inputs for ProgressTracker", PERMISSION_ALL);

    // Button: Progress Graph Anzeige Togglen
    cvarManager->registerNotifier("progresstracker_toggle_progress_graph", [this](std::vector<std::string> args) {

        if (isGraphVisible) {
            gameWrapper->UnregisterDrawables();
            timeData.clear();
            deathData.clear();
            this->Log("Progress Graph hidden.");
        }
        else {
            if (!ValidFilename()) {
                this->Log("Filename is empty or none, select different file or add new file!");
                return;
            }
            LoadDataFromFile(progressFilePath + cvarManager->getCvar("progresstracker_file_name").getStringValue() + ".txt");
            if (timeData.empty() || deathData.empty()) {
                this->Log("No data available to plot.");
                return;
            }
            // Zeichnen des Diagramms registrieren
            gameWrapper->RegisterDrawable(std::bind(&ProgressTracker::Render, this, std::placeholders::_1));
            this->Log("Progress Graph shown.");
        }

        // Zustand umschalten
        isGraphVisible = !isGraphVisible;
        this->Log("Toggled Progress Graph.");
        }, "Toggle Progress Grpah", PERMISSION_ALL);

    // Button: Add File
    cvarManager->registerNotifier("progresstracker_add_file", [this](std::vector<std::string> args) {
        std::string input = cvarManager->getCvar("progresstracker_file_name_input").getStringValue();

        if (input.empty()) {
            this->Log("Input is empty. Cannot add to dropdown.");
            cvarManager->getCvar("progresstracker_file_name_input").setValue("Filename cannot be empty!");
            return;
        }

        if (IsDropdownItemExists(input)) {
            this->Log("Item already exists in dropdown.");
            cvarManager->getCvar("progresstracker_file_name_input").setValue("Item already exists!");
            return;
        }

        if (AddItemToDropdown(input)) {
            // Befehl ausführen, um Dropdown zu aktualisieren
            cvarManager->executeCommand("cl_settings_refreshplugins");
            this->Log("Dropdown menu refreshed.");

            cvarManager->getCvar("progresstracker_file_name").setValue(input); // Neues Item direkt auswählen
            this->Log("Item added to dropdown and selected: " + input);

            // Eingabefeld clearen falls adden erfolgreich
            cvarManager->getCvar("progresstracker_file_name_input").setValue("");
        }
        else {
            this->Log("Failed to add item to dropdown.");
        }
        }, "Add item to dropdown menu", PERMISSION_ALL);

    // Button: Delete File
    cvarManager->registerNotifier("progresstracker_delete_file", [this](std::vector<std::string> args) {
        std::string currentItem = cvarManager->getCvar("progresstracker_file_name").getStringValue();

        if (!ValidFilename()) {
            this->Log("Cannot delete empty or none file!");
            return;
        }

        RemoveItemFromDropdown(currentItem);
        // Dropdown aktualisieren
        cvarManager->executeCommand("cl_settings_refreshplugins");
        this->Log("Item removed from dropdown: " + currentItem);
        
        }, "Remove current item from dropdown menu", PERMISSION_ALL);

}

void ProgressTracker::onUnload() {
    this->Log("ProgressTracker plugin unloaded!");
}

// Speichere Eingaben in die Datei
void ProgressTracker::SaveInputToFile(const std::string& input) {
    if (!ValidFilename()) {
        this->Log("Filename is empty or none, select different file or add new file!");
        return;
    }
    std::ofstream outFile(progressFilePath + cvarManager->getCvar("progresstracker_file_name").getStringValue() + ".txt", std::ios::app);
    if (!outFile) {
        this->Log("Failed to open file for writing!");
        return;
    }
    outFile << input << "\n";  // Neue Eingabe in einer neuen Zeile anhängen
    outFile.close();
    this->Log("Input saved to file: " + input);
}

// Lade Eingaben aus der Datei
std::vector<std::string> ProgressTracker::LoadInputsFromFile() {
    std::vector<std::string> inputs;
    if (!ValidFilename()) {
        this->Log("Filename is empty or none, select different file or add new file!");
        return inputs;
    }

    std::ifstream inFile(progressFilePath + cvarManager->getCvar("progresstracker_file_name").getStringValue() + ".txt");
    if (!inFile) {
        this->Log("File is empty or does not exist, select different file or submit more values!");
        return inputs;
    }

    std::string line;
    while (std::getline(inFile, line)) {
        if (!line.empty()) {
            inputs.push_back(line);
        }
    }
    inFile.close();
    return inputs;
}

// Überprüfe ob die Eingabe gültig ist
bool ProgressTracker::IsValidTime(const std::string& str) {

    if (str.empty()) return false;

    std::regex formatRegex(R"(^\d+:\d{2}$)");

    return std::regex_match(str, formatRegex);
}

bool ProgressTracker::IsValidDeaths(const std::string& str) {

    if (str.empty()) return false;

    for (char c : str) {
        if (!std::isdigit(c)) return false;
    }

    return true; 
}

void ProgressTracker::LoadDataFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        this->Log("File is empty or does not exist, select different file or submit more values!");
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        int minutes, seconds, deaths;
        char colon; // Trennzeichen ':'

        if (ss >> minutes >> colon >> seconds >> deaths && colon == ':') {
            int totalSeconds = minutes * 60 + seconds;
            timeData.push_back(totalSeconds);
            deathData.push_back(deaths);
        }
        else {
            this->Log("Invalid line ignored: " + line);
        }
    }

    file.close();
}

void ProgressTracker::Render(CanvasWrapper canvas) {
    
    // Farben definieren
    canvas.SetColor(10, 10, 10, 220); // Grau für Hintergrund

    // Koordinaten und Dimensionen
    Vector2 origin = { 100, 500 };  // Ursprung (unten links)
    int width = 800;                // Diagrammbreite
    int height = 400;               // Diagrammhöhe

    // Hintergrund zeichnen
    canvas.SetPosition(Vector2({ origin.X-50, origin.Y - height -50})); // Ursprung setzen
    canvas.FillBox(Vector2({ width+100, height+100 })); // Rechteck zeichnen

    // Achsen zeichnen
    canvas.SetColor(255, 255, 255, 255); // Weiß für Achsen
    canvas.DrawLine(Vector2({ origin.X, origin.Y }), Vector2({ origin.X + width, origin.Y })); // X-Achse
    canvas.DrawLine(Vector2({ origin.X, origin.Y }), Vector2({ origin.X, origin.Y - height })); // Y-Achse

    // Achsenbeschriftung
    canvas.SetColor(75, 192, 192, 255); // Blaugrün
    canvas.SetPosition(Vector2({ origin.X-75 + width / 2, origin.Y + 20 })); // Unterhalb der X-Achse
    canvas.DrawString("Time", 1.0f, 1.0f, true, false);

    canvas.SetColor(192, 75, 75, 255); // Rot
    canvas.SetPosition(Vector2({ origin.X+75 + width / 2, origin.Y + 20 })); // Unterhalb der X-Achse
    canvas.DrawString("Deaths", 1.0f, 1.0f, true, false);

    // Titel
    canvas.SetColor(255, 255, 255, 255);
    std::string title = "Progress for: " + cvarManager->getCvar("progresstracker_file_name").getStringValue();
    canvas.SetPosition(Vector2({ origin.X-100 + width / 2, origin.Y - height - 30 })); // Oberhalb des Diagramms
    canvas.DrawString(title, 1.2f, 1.2f, true, false);

    // Maximalwerte ermitteln
    int maxTime = *std::max_element(timeData.begin(), timeData.end());
    int maxDeaths = *std::max_element(deathData.begin(), deathData.end());



    // Skalierung berechnen
    float timeScale = maxTime > 0 ? static_cast<float>(height) / maxTime : 1.0f;
    float deathScale = maxDeaths > 0 ? static_cast<float>(height) / maxDeaths : 1.0f;

    // Zeit-Daten zeichnen
    canvas.SetColor(75, 192, 192, 255); // Blaugrün
    for (size_t i = 1; i < timeData.size(); ++i) {
        Vector2F prevPoint = {
            origin.X + static_cast<float>(width * (i - 1)) / (timeData.size() - 1),
            origin.Y - timeData[i - 1] * timeScale
        };
        Vector2F currPoint = {
            origin.X + static_cast<float>(width * i) / (timeData.size() - 1),
            origin.Y - timeData[i] * timeScale
        };
        canvas.DrawLine(prevPoint, currPoint);
    }

    // Tode-Daten zeichnen
    canvas.SetColor(192, 75, 75, 255); // Rot
    for (size_t i = 1; i < deathData.size(); ++i) {
        Vector2F prevPoint = {
            origin.X + static_cast<float>(width * (i - 1)) / (deathData.size() - 1),
            origin.Y - deathData[i - 1] * deathScale
        };
        Vector2F currPoint = {
            origin.X + static_cast<float>(width * i) / (deathData.size() - 1),
            origin.Y - deathData[i] * deathScale
        };
        canvas.DrawLine(prevPoint, currPoint);
    }
}

bool ProgressTracker::IsDropdownItemExists(const std::string& item) {
    std::ifstream file(settingsFilePath);
    if (!file.is_open()) {
        this->Log("Could not open settings file: " + settingsFilePath);
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("progresstracker_file_name|") != std::string::npos) {
            size_t pos = line.find('|', line.find('|', line.find('|') + 1) + 1);
            if (pos != std::string::npos) {
                std::string items = line.substr(pos + 1);
                std::istringstream stream(items);
                std::string currentItem;
                while (std::getline(stream, currentItem, '&')) {
                    size_t atPos = currentItem.find('@');
                    if (atPos != std::string::npos && currentItem.substr(0, atPos) == item) {
                        file.close();
                        return true;
                    }
                }
            }
        }
    }

    file.close();
    return false;
}

bool ProgressTracker::AddItemToDropdown(const std::string& item) {
    std::ifstream file(settingsFilePath);
    if (!file.is_open()) {
        this->Log("Could not open settings file: " + settingsFilePath);
        return false;
    }

    std::ostringstream updatedContent;
    bool updated = false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("progresstracker_file_name|") != std::string::npos) {
            size_t pos = line.find('|', line.find('|', line.find('|') + 1) + 1);
            if (pos != std::string::npos) {
                line += "&" + item + "@" + item; // Neues Item hinzufügen
                updated = true;
            }
        }
        updatedContent << line << "\n";
    }

    file.close();

    if (updated) {
        std::ofstream outFile(settingsFilePath);
        if (!outFile.is_open()) {
            this->Log("Could not write to settings file: " + settingsFilePath);
            return false;
        }
        outFile << updatedContent.str();
        outFile.close();
    }

    return updated;
}

bool ProgressTracker::ValidFilename() {
    std::string filename = cvarManager->getCvar("progresstracker_file_name").getStringValue();
    return !filename.empty() && filename != "none";

}

void ProgressTracker::RemoveItemFromDropdown(const std::string& item) {
    std::ifstream file(settingsFilePath);
    if (!file.is_open()) {
        this->Log("Could not open settings file: " + settingsFilePath);
        return;
    }

    std::ostringstream updatedContent;
    bool updated = false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("progresstracker_file_name|") != std::string::npos) {
            size_t pos = line.find('|', line.find('|', line.find('|') + 1) + 1);
            if (pos != std::string::npos) {
                std::string items = line.substr(pos + 1);
                std::istringstream stream(items);
                std::ostringstream newItems;
                std::string currentItem;
                bool first = true;

                while (std::getline(stream, currentItem, '&')) {
                    size_t atPos = currentItem.find('@');
                    if (atPos != std::string::npos && currentItem.substr(0, atPos) != item) {
                        if (!first) {
                            newItems << "&";
                        }
                        newItems << currentItem;
                        first = false;
                    }
                }

                line = line.substr(0, pos + 1) + newItems.str();
                updated = true;
            }
        }
        updatedContent << line << "\n";
    }

    file.close();

    if (updated) {
        std::ofstream outFile(settingsFilePath);
        if (!outFile.is_open()) {
            this->Log("Could not write to settings file: " + settingsFilePath);
            return;
        }
        outFile << updatedContent.str();
        outFile.close();
        this->Log("Item removed successfully: " + item);

        // Datei mit dem Item-Namen löschen
        std::string filePath = progressFilePath + item + ".txt";
        if (std::remove(filePath.c_str()) == 0) {
            this->Log("File deleted: " + filePath);
            cvarManager->getCvar("progresstracker_file_name").setValue("none");
        }
        else {
            this->Log("File not found, skipping delete: " + filePath);
        }

    }
    else {
        this->Log("Item not found or could not be removed: " + item);
    }
}

void ProgressTracker::SetBakkesmodFilePaths() {
    // getenv is not safe so microsoft told me to to this
    char* buffer = nullptr;
    size_t size = 0;

    if (_dupenv_s(&buffer, &size, "APPDATA") != 0 || buffer == nullptr) {
        throw std::runtime_error("Failed to get APPDATA environment variable.");
    }
    std::string appData(buffer);
    free(buffer);

    // Check if Bakkesmod folder exists otherwise disable plugin
    if (!std::filesystem::exists(std::string(appData) + "/bakkesmod/bakkesmod/")) {
        this->Log("Bakkesmod folder not found! This plugin does not work!");
        cvarManager->executeCommand("plugin unload ProgressTracker");
        return;
    }

    // Check if ProgressTrackerData folder exists otherwise create folder
    std::string path = std::string(appData) + "/bakkesmod/bakkesmod/data/ProgressTrackerData/";
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }

    // Set Paths
    progressFilePath = std::string(appData) + "/bakkesmod/bakkesmod/data/ProgressTrackerData/";
    settingsFilePath = std::string(appData) + "/bakkesmod/bakkesmod/plugins/settings/progresstracker.set";
}

void ProgressTracker::Log(std::string msg) {
    cvarManager->log(msg);
}


