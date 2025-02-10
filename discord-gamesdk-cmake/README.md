<picture>
  <img width="500px" alt="Discord GameSDK for CMake-based projects" src="docs/header.png">
</picture>

Bringing [Discord GameSDK](https://discord.com/developers/docs/developer-tools/game-sdk) to [CMake](https://cmake.org) build system.

## Table of Contents
- [Overview](#overview)
- [Usage](#usage)
- [License](#license)

## Overview
This project aims to integrate **Discord GameSDK** into **CMake**, enabling features like rich presence, voice chat, achievements, and networking. The integration is streamlined for cross-platform compatibility, making it easier to build.

## Usage
1. Add the repository as a sub-module:
	```sh
	git submodule add https://github.com/borfei/discord-gamesdk-cmake.git
	```
	- Alternatively, you can download the source code [here](https://github.com/borfei/discord-gamesdk-cmake/archive/refs/heads/main.zip).
3. Manually download the additional SDK files:
	- There are two versions provided in the official SDK documentation:
		- [v3.2.1](https://dl-game-sdk.discordapp.net/3.2.1/discord_game_sdk.zip) - Latest version, includes support for **AArch64**
		- [v2.5.6](https://dl-game-sdk.discordapp.net/2.5.6/discord_game_sdk.zip) - Stable version, very stable
	- Once downloaded, open the archive and extract the `lib/` folder to this project (not your project)
4. Modify your project's `CMakeLists.txt`:
	```cmake
 	add_subdirectory(<path/to/discord-gamesdk-cmake>)
 	...
 	target_link_libraries(<TARGET>
 		...
 		discord_gamesdk)
 	```
5. Profit

## License
This project is licensed under the Unlicense license, see [LICENSE](LICENSE) for more information.
