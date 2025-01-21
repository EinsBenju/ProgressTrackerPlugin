#include "pch.h"
#include "ProgressTracker.h"
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <filesystem>

BAKKESMOD_PLUGIN(ProgressTracker, "ProgressTracker", "1.0", PERMISSION_ALL)

void ProgressTracker::onLoad() {
    // Set file paths
    SetBakkesmodFilePaths();

    // Create CVar for Dropdown menu
    cvarManager->registerCvar("progresstracker_file_name", "none", "File");

    // Set default selected value
    cvarManager->getCvar("progresstracker_file_name").setValue("none");

    // Create CVar for new filename input field
    cvarManager->registerCvar("progresstracker_file_name_input", "", "Filename Input", true, false, 0.0f, false, 0.0f, false);

    // Create CVar for user input
    cvarManager->registerCvar("progresstracker_time_input", "", "Time Input", true, false, 0.0f, false, 0.0f, false);
    cvarManager->registerCvar("progresstracker_deaths_input", "", "Deaths Input", true, false, 0.0f, false, 0.0f, false);

    // Button: Submit
    cvarManager->registerNotifier("progresstracker_submit", [this](std::vector<std::string> args) {

        // Check if selected file is valid
        if (!ValidFilename()) {
            this->Log("Filename is empty or none, select different file or add new file!");
            return;
        }

        // Extract values from input fields
        std::string time = cvarManager->getCvar("progresstracker_time_input").getStringValue();
        std::string deaths = cvarManager->getCvar("progresstracker_deaths_input").getStringValue();

        // Check if time and death are valid
        if (IsValidTime(time) && IsValidDeaths(deaths)) {
            SaveInputToFile(time + " " + deaths);  // Save inputs to file
            this->Log("Input saved: " + time + " " + deaths);

            // Clear input fields if submit was successful
            cvarManager->getCvar("progresstracker_time_input").setValue("");
            cvarManager->getCvar("progresstracker_deaths_input").setValue("");
        }
        else {
            this->Log("Invalid input!");

            // Display errors for user
            cvarManager->getCvar("progresstracker_time_input").setValue("Invalid Input, must be {int}:{int}:2");
            cvarManager->getCvar("progresstracker_deaths_input").setValue("Invalid Input, must be {int}");
        }
        }, "Submit input for ProgressTracker", PERMISSION_ALL);

    // Button: Print values to console
    cvarManager->registerNotifier("progresstracker_print_all", [this](std::vector<std::string> args) {

        // Check if selected file is valid
        if (!ValidFilename()) {
            this->Log("Filename is empty or none, select different file or add new file!");
            return;
        }

        // Load values from file
        auto values = LoadInputsFromFile();

        // Print values to console
        this->Log("Saved Values:");
        for (const std::string& value : values) {
            this->Log(value);
        }
        }, "Print all saved values to console", PERMISSION_ALL);

    // Button: Toggle Progress Graph
    cvarManager->registerNotifier("progresstracker_toggle_progress_graph", [this](std::vector<std::string> args) {

        // If graph is already visible, disable graph and clear loaded data
        if (isGraphVisible) {
            gameWrapper->UnregisterDrawables();
            timeData.clear();
            deathData.clear();
            this->Log("Progress Graph hidden.");
        }
        else {

            // Check if selected file is valid
            if (!ValidFilename()) {
                this->Log("Filename is empty or none, select different file or add new file!");
                return;
            }

            // Load data from file if graph isn't visible
            LoadDataFromFile(progressFilePath + cvarManager->getCvar("progresstracker_file_name").getStringValue() + ".txt");
            if (timeData.empty() || deathData.empty()) {
                this->Log("No data available to plot.");
                return;
            }

            // Show graph
            gameWrapper->RegisterDrawable(std::bind(&ProgressTracker::Render, this, std::placeholders::_1));
            this->Log("Progress Graph shown.");
        }

        // Switch state
        isGraphVisible = !isGraphVisible;
        this->Log("Toggled Progress Graph.");

        }, "Toggle Progress Grpah", PERMISSION_ALL);

    // Button: Add File
    cvarManager->registerNotifier("progresstracker_add_file", [this](std::vector<std::string> args) {

        // Extract new filename
        std::string newFilename = cvarManager->getCvar("progresstracker_file_name_input").getStringValue();

        // Check if empty
        if (newFilename.empty()) {
            this->Log("Input is empty. Cannot add to dropdown.");
            cvarManager->getCvar("progresstracker_file_name_input").setValue("Filename cannot be empty!");
            return;
        }

        // Check if already exists
        if (IsDropdownItemExists(newFilename)) {
            this->Log("Item already exists in dropdown.");
            cvarManager->getCvar("progresstracker_file_name_input").setValue("Item already exists!");
            return;
        }

        // Add item to Dropdown
        if (AddItemToDropdown(newFilename)) {

            // Refresh settings file
            cvarManager->executeCommand("cl_settings_refreshplugins");
            this->Log("Dropdown menu refreshed.");

            // Set new file as selected
            cvarManager->getCvar("progresstracker_file_name").setValue(newFilename);
            this->Log("Item added to dropdown and selected: " + newFilename);

            // Clear input field if success
            cvarManager->getCvar("progresstracker_file_name_input").setValue("");
        }
        else {
            this->Log("Failed to add item to dropdown.");
        }
        }, "Add item to dropdown menu", PERMISSION_ALL);

    // Button: Delete File
    cvarManager->registerNotifier("progresstracker_delete_file", [this](std::vector<std::string> args) {

        // Extract current Item
        std::string currentItem = cvarManager->getCvar("progresstracker_file_name").getStringValue();

        // Check if selected file is valid
        if (!ValidFilename()) {
            this->Log("Cannot delete empty or none file!");
            return;
        }

        // Remove File
        RemoveItemFromDropdown(currentItem);
        this->Log("Item removed from dropdown: " + currentItem);

        // Refresh settings file
        cvarManager->executeCommand("cl_settings_refreshplugins");
        this->Log("Dropdown menu refreshed.");

        // Set none as selected
        cvarManager->getCvar("progresstracker_file_name").setValue("none");
        
        }, "Remove current item from dropdown menu", PERMISSION_ALL);

}

void ProgressTracker::onUnload() {
    this->Log("ProgressTracker plugin unloaded!");
}

// Save an input to selected file
void ProgressTracker::SaveInputToFile(const std::string& input) {

    // Check if selected file is valid
    if (!ValidFilename()) {
        this->Log("Filename is empty or none, select different file or add new file!");
        return;
    }

    // Write new value to new line
    std::ofstream outFile(progressFilePath + cvarManager->getCvar("progresstracker_file_name").getStringValue() + ".txt", std::ios::app);
    if (!outFile) {
        this->Log("Failed to open file for writing!");
        return;
    }
    outFile << input << "\n";
    outFile.close();
    this->Log("Input saved to file: " + input);
}

// Load and return values from file
std::vector<std::string> ProgressTracker::LoadInputsFromFile() {

    std::vector<std::string> inputs;

    // Check if selected file is valid
    if (!ValidFilename()) {
        this->Log("Filename is empty or none, select different file or add new file!");
        return inputs;
    }

    // Open file
    std::ifstream inFile(progressFilePath + cvarManager->getCvar("progresstracker_file_name").getStringValue() + ".txt");
    if (!inFile) {
        this->Log("File is empty or does not exist, select different file or submit more values!");
        return inputs;
    }

    // Read line by line
    std::string line;
    while (std::getline(inFile, line)) {
        if (!line.empty()) {
            inputs.push_back(line);
        }
    }
    inFile.close();

    return inputs;
}

// Check if time input is valid
bool ProgressTracker::IsValidTime(const std::string& str) {

    if (str.empty()) return false;

    std::regex formatRegex(R"(^\d+:\d{2}$)");

    return std::regex_match(str, formatRegex);
}

// Check if deaths input is valid
bool ProgressTracker::IsValidDeaths(const std::string& str) {

    if (str.empty()) return false;

    for (char c : str) {
        if (!std::isdigit(c)) return false;
    }

    return true; 
}

// Load data from file for graph (I know there is LoadInputsFromFile, but it does slightly different things)
void ProgressTracker::LoadDataFromFile(const std::string& filePath) {

    // Open file
    std::ifstream file(filePath);
    if (!file.is_open()) {
        this->Log("File is empty or does not exist, select different file or submit more values!");
        return;
    }

    // Read line by line and save time and deaths as values (not as strings like LoadInputsFromFile)
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        int minutes, seconds, deaths;
        char colon;

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

// Render graph
void ProgressTracker::Render(CanvasWrapper canvas) {

    // Origin and dimensions
    Vector2 origin = { 100, 500 };
    int width = 800;
    int height = 400;

    // Background
    canvas.SetColor(10, 10, 10, 220); // Gray
    canvas.SetPosition(Vector2({ origin.X-50, origin.Y - height -50}));
    canvas.FillBox(Vector2({ width+100, height+100 }));

    // Axes
    canvas.SetColor(255, 255, 255, 255); // White
    canvas.DrawLine(Vector2({ origin.X, origin.Y }), Vector2({ origin.X + width, origin.Y })); // X-Axis
    canvas.DrawLine(Vector2({ origin.X, origin.Y }), Vector2({ origin.X, origin.Y - height })); // Y-Axis

    // Time label
    canvas.SetColor(75, 192, 192, 255); // Light blue
    canvas.SetPosition(Vector2({ origin.X-75 + width / 2, origin.Y + 20 }));
    canvas.DrawString("Time", 1.0f, 1.0f, true, false);

    // Deaths label
    canvas.SetColor(192, 75, 75, 255); // Red
    canvas.SetPosition(Vector2({ origin.X+75 + width / 2, origin.Y + 20 }));
    canvas.DrawString("Deaths", 1.0f, 1.0f, true, false);

    // Titel
    canvas.SetColor(255, 255, 255, 255); // Black
    std::string title = "Progress for: " + cvarManager->getCvar("progresstracker_file_name").getStringValue();
    canvas.SetPosition(Vector2({ origin.X-100 + width / 2, origin.Y - height - 30 }));
    canvas.DrawString(title, 1.2f, 1.2f, true, false);

    // Max values
    int maxTime = *std::max_element(timeData.begin(), timeData.end());
    int maxDeaths = *std::max_element(deathData.begin(), deathData.end());

    // Scales
    float timeScale = maxTime > 0 ? static_cast<float>(height) / maxTime : 1.0f;
    float deathScale = maxDeaths > 0 ? static_cast<float>(height) / maxDeaths : 1.0f;

    // Draw times
    canvas.SetColor(75, 192, 192, 255); // Light blue
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

    // Draw deaths
    canvas.SetColor(192, 75, 75, 255); // Red
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

// Check if item already exists in Dropdown menu (I like how ChatGPT names things)
bool ProgressTracker::IsDropdownItemExists(const std::string& item) {

    // Open settings file
    std::ifstream file(settingsFilePath);
    if (!file.is_open()) {
        this->Log("Could not open settings file: " + settingsFilePath);
        return false;
    }

    // Check in the correct line, if item is already in there
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

// Add item to Dropdown menu
bool ProgressTracker::AddItemToDropdown(const std::string& item) {

    // Open settings file
    std::ifstream file(settingsFilePath);
    if (!file.is_open()) {
        this->Log("Could not open settings file: " + settingsFilePath);
        return false;
    }

    std::ostringstream updatedContent;
    bool updated = false;

    // Go to the correct line and insert new item at the end
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("progresstracker_file_name|") != std::string::npos) {
            size_t pos = line.find('|', line.find('|', line.find('|') + 1) + 1);
            if (pos != std::string::npos) {
                line += "&" + item + "@" + item; // Insert new item
                updated = true;
            }
        }
        updatedContent << line << "\n";
    }

    file.close();

    // Write back
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

// Check if current filename is valid
bool ProgressTracker::ValidFilename() {

    std::string filename = cvarManager->getCvar("progresstracker_file_name").getStringValue();

    return !filename.empty() && filename != "none";
}

void ProgressTracker::RemoveItemFromDropdown(const std::string& item) {

    // Open settings file
    std::ifstream file(settingsFilePath);
    if (!file.is_open()) {
        this->Log("Could not open settings file: " + settingsFilePath);
        return;
    }

    std::ostringstream updatedContent;
    bool updated = false;

    // GO to correct line and remove item
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

    // Write back
    if (updated) {
        std::ofstream outFile(settingsFilePath);
        if (!outFile.is_open()) {
            this->Log("Could not write to settings file: " + settingsFilePath);
            return;
        }
        outFile << updatedContent.str();
        outFile.close();
        this->Log("Item removed successfully: " + item);

        // Delete file with correct name
        std::string filePath = progressFilePath + item + ".txt";
        if (std::remove(filePath.c_str()) == 0) {
            this->Log("File deleted: " + filePath);
        }
        else {
            this->Log("File not found, skipping delete: " + filePath);
        }

    }
    else {
        this->Log("Item not found or could not be removed: " + item);
    }
}

// Set file paths
void ProgressTracker::SetBakkesmodFilePaths() {

    // getenv is not safe, so microsoft told me to to this
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

// Log
void ProgressTracker::Log(std::string msg) {
    cvarManager->log(msg);
}