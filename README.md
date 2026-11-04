# RocketLeagueTAS

A BakkesMod plugin project for building Rocket League shots from recorded car
inputs in Rocket Plugin LAN games.

## Current status

This is an early project foundation, not a usable TAS yet. It currently contains:

- A shot editing state model for replay, candidate recording, Stop, and Update.
- Console commands: `rltas_new <name>`, `rltas_start`, `rltas_stop`,
  `rltas_update`, and `rltas_stopandupdate`.
- A CMake target configured for a Windows x64 MSVC build.

Game input interception and playback are deliberately not wired yet. The next
implementation step is to confirm the SDK hook's input parameter behavior in a
local Rocket Plugin match, then connect one input frame to `ShotSession` per
game frame. The commands currently exercise only the project state model and
write status to the BakkesMod console.

## Shot editing rules

1. Create a shot while the game is in the desired starting setup.
2. Start an attempt. Existing frames replay from the beginning.
3. A configured input interruption or the end of the saved sequence switches
   playback to live recording.
4. Stop stages the candidate segment; Update commits it. Stopping without
   updating leaves the committed shot unchanged.
5. Starting another attempt clears the previous uncommitted candidate and
   begins again from frame zero. Updating after a mid-playback interruption
   replaces the old suffix with the new candidate segment.

Only the local player's controller input belongs in the shot sequence. The
plugin does not rewind the ball or other cars. Slow motion is a separate
integration concern: the server-side time-scale command in the previous stub
was removed because it can affect the whole LAN simulation. We need to verify
how the host and connected clients behave before selecting a slow-motion method.

## Build (Windows)

The BakkesMod SDK import library uses Microsoft's C++ ABI, so this plugin must
be built with MSVC or an MSVC-compatible toolchain. MinGW can compile the source
but cannot link against `pluginsdk.lib`. From a Visual Studio 2022 x64 developer
prompt, configure and build from this directory:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The output should be `build/Release/RocketLeagueTAS.dll`. Copy it to the BakkesMod
plugins directory and load it from the in-game console with `plugin load
RocketLeagueTAS`.

Linux users can build in a Windows VM or with a configured MSVC-compatible
environment. The current workspace has MinGW but no MSVC-compatible compiler,
so it cannot produce a valid BakkesMod DLL here.

## GitHub Actions build

The workflow at `.github/workflows/build-plugin.yml` builds the DLL on a
Windows x64 runner whenever code is pushed, and can also be started manually
from the repository's Actions page. Download the `RocketLeagueTAS-windows-x64`
artifact from that workflow run. GitHub keeps the artifact for 14 days.

The workflow is ready to run after the project is pushed to its GitHub
repository. A private repository is a good default while development continues.

## Research notes

- The existing TAS Plugin documents the iterative pattern: create a shot from
  the current setup, start playback, interrupt to record, Stop, Update, and
  repeat. It also notes that map and car settings can affect reproducibility:
  [TAS Plugin](https://bakkesplugins.com/plugin/351).
- The BakkesMod SDK exposes game state queries and input wrappers such as
  `GameWrapper::IsInGame`, `IsInOnlineGame`, `GetPlayerController`, and
  `ControllerInput`. The exact hook callback and input override behavior still
  needs runtime validation:
  [GameWrapper SDK header](https://github.com/bakkesmodorg/BakkesModSDK/blob/master/include/bakkesmod/wrappers/GameWrapper.h).
- Rocket Plugin is intended to host and join local games:
  [Rocket Plugin](https://bakkesplugins.com/plugin/26).
- Epic's current Code of Conduct says third-party mods may be used with EAC
  disabled in training, offline, and LAN matches, provided they comply with the
  applicable terms:
  [Rocket League Code of Conduct](https://www.rocketleague.com/code-of-conduct?lang=en).
