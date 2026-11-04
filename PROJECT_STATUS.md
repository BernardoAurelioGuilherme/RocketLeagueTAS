# Project status

RocketLeagueTAS is a BakkesMod plugin for building and replaying car-input TAS shots in Rocket Plugin LAN matches. The intended scope is local car input playback plus restoring the local car and ball to the shot's initial physics state. It is for offline/LAN use, not competitive online play.

## Current implementation

- Dear ImGui settings panel with New Shot, Play / Record, Stop, Update, and Stop and Update.
- Captures and replays local car controller inputs through the BakkesMod vehicle input hook.
- New Shot captures the local car and ball physics states; subsequent Play / Record attempts restore both before replay.
- Update commits the candidate input segment and saves a `.rltas` file.
- Saved TAS dropdown, Load TAS, and Refresh list controls.
- TAS files are stored in BakkesMod's data folder at `RocketLeagueTAS/shots`.
- Global game speed control (0.10x to 2.00x), applied outside TAS attempts and periodically enforced during gameplay using BakkesMod's `gamespeed` command.
- HUD flip availability readout using `CarWrapper::HasFlip()`.
- Automatic flip restoration detects the local car's dodge ending in midair and restores the flip if one is not available. This is enabled by default and can be toggled in the UI.
- `.rltas` format version 2 stores flip reset markers and continues to load version 1 files.
- Other future functions are not implemented yet.

## Latest build

- Latest source commit: `de0d5ca` (`Fix game speed polling wrapper constness`).
- Windows GitHub Actions run `41918124333` succeeded.
- The built DLL was installed to the user's BakkesMod plugin folder. Reload the plugin or restart Rocket League to use it.
- The latest DLL is installed in the user's BakkesMod plugin folder; reload the plugin or restart Rocket League to use it.

## Next test steps

1. Reload RocketLeagueTAS in BakkesMod.
2. In a LAN match, put the car and ball at the intended start and click New Shot.
3. Record a segment, stop it, then use Stop and Update to commit and save.
4. Move the car and ball, select the shot from Saved TAS, click Load TAS, then Play / Record.
5. Check whether the car and ball return to their recorded states and whether playback behaves as expected in the LAN match.
6. Change Game speed and verify it applies even while no TAS attempt is active.
7. With automatic flip restoration enabled (default), use a flip in the air and check whether the next airborne jump input has a flip available again.
8. Report any issues before adding other features.

## Repository

Private GitHub repository: `BernardoAurelioGuilherme/RocketLeagueTAS`.
