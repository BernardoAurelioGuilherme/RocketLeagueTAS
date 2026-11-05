# RocketLeagueTAS

**Copyright (c) 2026 BernardoAurelioGuilherme.**

RocketLeagueTAS is a BakkesMod plugin bundle for recording Rocket League shots
and hosting or joining basic LAN matches. The bundle contains two plugin DLLs,
which appear as separate settings entries in BakkesMod:

- `RocketLeagueTAS.dll` records and replays the local car's controller input,
  restores the recorded starting car and ball state, and saves named shots.
- `RocketLeagueTASLAN.dll` hosts or joins a LAN match with basic arena, mode,
  bot difficulty, player-slot, and direct IP/port settings.

## Features

- Record, play, stop, update, and retry shot attempts from frame zero.
- Save and select named shots in the plugin menu; shot files use the `.rltas`
  format (version 5; versions 1–4 remain loadable).
- Restore the starting car and ball state for repeatable attempts.
- Change game speed from 0.10x to 2.00x, including while a shot is playing or
  recording.
- Insert game-speed changes into a shot while recording or playing. A queued
  change is saved at the next input frame and reproduced during playback. Each
  shot remembers its starting speed, which is restored on every retry.
- Scale local car velocity changes on ball contact from 1.0x to 5.0x, in 0.1x
  steps.
- Host or join a basic LAN match from the separate RocketLeagueTASLAN settings
  entry. Friends outside your local network can join using the host's VPN IP.

The plugin records car inputs only. It does not record or replay other players'
inputs. These tools are intended for offline and LAN sessions with BakkesMod
available, not public online matchmaking.

## Install

1. Download the Windows x64 plugin ZIP from the latest release.
2. Copy both `RocketLeagueTAS.dll` and `RocketLeagueTASLAN.dll` to the BakkesMod plugins directory, usually
   `%APPDATA%\\bakkesmod\\bakkesmod\\plugins`.
3. In Rocket League, open the BakkesMod console and run `plugin load
   RocketLeagueTAS` and `plugin load RocketLeagueTASLAN`, or enable both from
   BakkesMod's plugin manager.
4. Open the BakkesMod F2 menu and select either plugin's settings entry.

## Build

The BakkesMod SDK import library uses Microsoft's C++ ABI, so build with Visual
Studio 2022 and CMake on Windows:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The two DLLs are written to `build/Release/`. The solution file
`RocketLeagueTAS.sln` builds both plugins in Visual Studio. GitHub Actions also
builds both Windows x64 DLLs for each workflow run; those temporary workflow
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
