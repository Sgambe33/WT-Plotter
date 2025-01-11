# WT-Plotter

[![Windows Release](https://github.com/Sgambe33/WT-Plotter/actions/workflows/windows_release.yaml/badge.svg)](https://github.com/Sgambe33/WT-Plotter/actions/workflows/windows_release.yaml)

A simple application to display the contents of War Thunder's replay files, even older ones.
Huge thanks to [FlareFlo](https://github.com/FlareFlo) for his work on the [wt_ext_cli](https://github.com/Warthunder-Open-Source-Foundation/wt_ext_cli)
> **Note:** This project is still in development and many things need to be finished/polished, expect bugs.

## Features

- Display replay metadata such as the map, game mode, date and if the player won or lost.
- Display result leaderboard for both allies and axis teams with all possible information.
- Ability to record players'positions in an ongoing match to later display them in a map or contribute to my other project [WT-Heatmaps](http://warthunder-heatmaps.crabdance.com/).

![img.png](.github/readme_assets/image.png)

![img_1.png](.github/readme_assets/image2.png)


**Planned features:**

- Display each player used/unused vehicles.
- Image export of the player's path in the map.
- Replay data export/persistence using a local database.
- Open arbitrary replays.
- MacOS support if requested.

## How to use

1. Download the latest release from the releases page.
2. Unzip the archive in an empty folder.
3. Make sure to have `Autosave replays` setting active in War Thunder.
4. Open the executable.
5. From the `File` menu select `Preferences` and set the path to your War Thunder replays directory. Click OK.
6. Restart the app.
7. The left side of the application will show a list of all the replays in the directory that can be read.
8. Select a replay from the list and the right side of the application will show the contents of the replay.

# DISCLAIMER
This project is not affiliated with Gaijin Entertainment in any way. War Thunder is a registered trademark of Gaijin Entertainment. All rights reserved.

By using this software you agree to share the data (such as players' positions, match map, match start time, author's user id) gathered while *both the game and this software are in execution* with the [WT-Heatmaps](http://warthunder-heatmaps.crabdance.com/) project.
