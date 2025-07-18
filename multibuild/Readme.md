# What inside this folder

This folder saves different qpm manifest that can be used for multiple game version.

The `target.cmake` is included in `CMakeLists.txt` and for code compat.

The main idea is release multiple `.qmod` with the same source code, to make sure the old version of game can receive the latest update for the mod.

# what we do at version switch

All `.json` describe a replace rule for each json, and other files will be replaced directly.

`qpm.shared.json` will not in the git, which means we always use the latest version of dependencies.

avaliable cpp macros:
- some macro will be defined, e.g. `GAME_VER_1_35_0`
- a macro string called `GAME_VERSION`, example value:`"1_35_0"`

# auto update mod versions

this folder contains some script that can auto update the mod dependency when the game has small changes that doesn't affect this mod.