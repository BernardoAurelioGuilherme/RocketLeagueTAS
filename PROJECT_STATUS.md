# Project status

RocketLeagueTAS is a BakkesMod plugin for building and replaying car-input TAS shots in Rocket Plugin LAN matches. The intended scope is local car input playback plus restoring the local car and ball to the shot's initial physics state. It is for offline/LAN use, not competitive online play.

## Current implementation

- Dear ImGui settings panel with New Shot, Play / Record, Stop, Update, and Stop and Update.
- Captures and replays local car controller inputs through the BakkesMod vehicle input hook.
- New Shot captures the local car and ball physics states; subsequent Play / Record attempts restore both before replay.
- Update commits the candidate input segment and saves a `.rltas` file.
- Saved TAS dropdown, Load TAS, and Refresh list controls.
- TAS files are stored in BakkesMod's data folder at `RocketLeagueTAS/shots`.
- Slow motion and other future functions are not implemented yet.

## Latest build

- Latest source commit: `4723696` (`Fix loading physics state flags`), preceded by `7f5527f` (car start restore and saved TAS selector).
- Windows GitHub Actions run `41911369585` succeeded.
- The built DLL was installed to the user's BakkesMod plugins folder in the Heroic Rocket League prefix. Reload the plugin or restart Rocket League to use it.
- The plugin compiled successfully; the latest car restore and saved-file workflow still need in-game verification in a Rocket Plugin LAN match.

## Next test steps

1. Reload RocketLeagueTAS in BakkesMod.
2. In a LAN match, put the car and ball at the intended start and click New Shot.
3. Record a segment, stop it, then use Stop and Update to commit and save.
4. Move the car and ball, select the shot from Saved TAS, click Load TAS, then Play / Record.
5. Check whether the car and ball return to their recorded states and whether playback behaves as expected in the LAN match.
6. Report any issues before adding slow motion or other features.

## Repository

Private GitHub repository: `BernardoAurelioGuilherme/RocketLeagueTAS`.
