# RocketLeagueTAS

**Copyright (c) 2026 BernardoAurelioGuilherme.**

RocketLeagueTAS is a BakkesMod plugin for recording and iterating on Rocket
League shots in offline and LAN matches hosted with Rocket Plugin. It records
the local car's controller input and restores the recorded starting car and ball
state when a shot is replayed.

## Features

- Record, play, stop, update, and retry shot attempts from frame zero.
- Save and select named shots in the plugin menu; shot files use the `.rltas`
  format.
- Restore the starting car and ball state for repeatable attempts.
- Change game speed from 0.10x to 2.00x, including while a shot is playing or
  recording.
- Insert game-speed changes into a shot while recording or playing. A queued
  change is saved at the next input frame and reproduced during playback.
- Scale local car velocity changes on ball contact from 1.0x to 5.0x, in 0.1x
  steps.
- Optionally grant a flip after a dodge ends.

The plugin records car inputs only. It does not record or replay other players'
inputs. These tools are intended for offline and LAN sessions with BakkesMod
available, not public online matchmaking.

## Install

1. Download `RocketLeagueTAS.dll` from the latest release for Windows x64.
2. Copy it to the BakkesMod plugins directory, usually
   `%APPDATA%\\bakkesmod\\bakkesmod\\plugins`.
3. In Rocket League, open the BakkesMod console and run
   `plugin load RocketLeagueTAS`, or enable the plugin from BakkesMod's plugin
   manager.
4. Open the BakkesMod F2 menu and find the RocketLeagueTAS settings panel.

## Build

The BakkesMod SDK import library uses Microsoft's C++ ABI, so build with Visual
Studio 2022 and CMake on Windows:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The DLL is written to `build/Release/RocketLeagueTAS.dll`. GitHub Actions also
builds a Windows x64 DLL for each workflow run; those temporary workflow
artifacts expire after 14 days.

## Shot editing

Create a shot in the desired starting setup, then record and refine it with
Play, Stop, and Update. Each retry begins at the first frame. Stopping stages a
candidate attempt; Update commits it. A speed change made during a shot is
queued for the next input frame and stored with that frame.

## Project status

This is an early release. The speed-point change was just corrected to attach
the selected speed to the next input frame; please verify it in your LAN setup.
Physics and timing can vary with map, game version, host, and match settings.

## Copyright and reuse

Copyright (c) 2026 BernardoAurelioGuilherme. All rights reserved. The project
is publicly viewable only if the repository owner changes its GitHub
visibility. No open-source license is included, so public visibility alone
does not grant permission to reuse, modify, or redistribute the code.
