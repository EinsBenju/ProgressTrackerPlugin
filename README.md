# ProgressTracker

A BakkesMod plugin to track and visualize your progress in workshop maps.

Let's say you start training a rings map. Maybe you want to see if you make progress over time and complete the map in a shorter time and with fewer deaths. 

Well, now you can! 

This plugin allows you to insert your needed time and amount of deaths and save it!

The plugin even lets you track different maps, for example, you can submit values for your rings map AND submit values for your dibble map.

But the best part is, not only does the plugin save your times, you can plot it in a beautiful graph and see your progress visualized! 

IMPORTANT:

This plugin is work in progress, so I can't confirm 100% that it won't crash your game and doesn't randomly loose all your progress data, so maybe keep backups. (Data files are stored in "C:/Users/%userprofile%/AppData/Roaming/bakkesmod/bakkesmod/data/ProgressTrackerData/")

Also, I never learned or coded with C++, so a lot of code is generated with the help of ChatGPT, so be aware of some inconsistencies in the implementation.

If you have any recommendations or bugs you can contact me via Discord @einsbenju.

HOW TO DOWNLOAD:

The folder Plugins contains the actual plugin files. Download both files and go to "C:/Users/%userprofile%/AppData/Roaming/bakkesmod/bakkesmod/plugins/" on your computer and put the ProgressTracker.dll file there. Then go to the settings folder and put the progresstracker.set file in it. 

Now you can (re)start RocketLeague and BakkesMod and you have the plugin installed.

HOW TO USE:

Step 1:
Check if the plugin is enabled. Press f2 and go to the tab plugins. Open the plugin manager and search for ProgressTracker. Make sure it is enabled.
![image](https://github.com/user-attachments/assets/873c867b-24e3-4f94-a7fa-e7c780751c53)


Step 2: 
Go to the plugins tab and click on ProgressTracker Plugin.
There you can select a file you want to save values to. You can also create more files by writing a filename and clicking the Add File button.
You can also delete the currently selected file by pressing the Delete File button.

Below that you have two input fields, one for your time and one for your deaths. Time has to be in the format {minutes}:{seconds} for example 12:34.
By clicking Submit the values get saved to the selected file.

Print values to console prints all values of the current selected file to console (Press f6 to open console).

Toggle Progress Graph shows your progress over time of the currently selected file.

![image](https://github.com/user-attachments/assets/a75604af-5e5e-405e-a738-e81084b82b42)

Currently there is no way to delete single values, but you can go to the data files and edit them yourself. The data files are stored in "C:/Users/%userprofile%/AppData/Roaming/bakkesmod/bakkesmod/data/ProgressTrackerData/".

