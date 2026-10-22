# Session Tracker

Geode mod for Geometry Dash 2.2081 that keeps a history of play sessions: time (paused time apart), attempts (normal, start position, practice), best percent and the percents you die at the most.

Copies of a level count together with the original: the game marks a copy with the ID of the level it came from, so copies, copies of copies and the original land in one group on their own. Levels the game can't link (rebuilt from scratch, reuploaded) can be merged and split by hand in the history popup.

## Where things are

- `src/data` - sessions, levels, groups and their JSON files in the mod's save folder (`levels/*.json`, `groups.json`)
- `src/tracker` - the session being played, fed by the hooks
- `src/hooks` - `PlayLayer` (attempts, deaths, pauses, quitting), the pause menu and level pages
- `src/ui` - the stats popup (Session, Overview, Sessions, Levels tabs), the merge popup and their components
- `resources` - UI sprites borrowed from [Blitzkrieg](https://github.com/ZhulinskiiDanil/blitzkrieg)

## Install

Grab `zhulis.session-tracker.geode` from the [latest release](https://github.com/ZhulinskiiDanil/session-tracker/releases/latest) into `geode/mods`, or install it from [Zhulis Mods Manager](https://github.com/ZhulinskiiDanil/zhulis-mods-manager).

## Build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

The build installs the `.geode` file into the GD of the current `geode profile`. For another GD folder, copy `build/zhulis.session-tracker.geode` into its `geode/mods`.
