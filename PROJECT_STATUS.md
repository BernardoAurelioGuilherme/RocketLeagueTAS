# Project status

RocketLeagueTAS is a BakkesMod plugin for building and replaying car-input TAS shots in Rocket Plugin LAN matches. The intended scope is local car input playback plus restoring the local car and ball to the shot's initial physics state. It is for offline/LAN use, not competitive online play.

## Current implementation

- Dear ImGui settings panel with New Shot, Play / Record, Stop, Update, and Stop and Update.
- Captures and replays local car controller inputs through the BakkesMod vehicle input hook.
- New Shot captures the local car and ball physics states; subsequent Play / Record attempts restore both before replay.
- Update commits the candidate input segment and saves a `.rltas` file.
- Saved TAS dropdown, Load TAS, and Refresh list controls.
- TAS files are stored in BakkesMod's data folder at `RocketLeagueTAS/shots`.
- Optional slow motion during a TAS attempt, with a 0.10x to 1.00x speed control. The previous game time dilation is restored on Stop.
- HUD flip availability readout using `CarWrapper::HasFlip()`.
- Insert Flip Reset queues a car-state action at the next input frame. That action is stored in the shot and replays; using it during playback starts a new recorded branch.
- `.rltas` format version 2 stores flip reset markers and continues to load version 1 files.
- Other future functions are not implemented yet.

## Latest build

- Latest source commit: `9960c25` (`Add TAS speed control and recorded flip reset actions`).
- Windows GitHub Actions run `41913908677` succeeded.
- The built DLL was installed to the user's BakkesMod plugin folder. Reload the plugin or restart Rocket League to use it.
- The speed and flip features compile successfully; both still need in-game verification in a Rocket Plugin LAN match. Flip Reset insertion currently changes the local car jump flags (`bDoubleJumped` and `bCanJump`) at the chosen frame.

## Next test steps

1. Reload RocketLeagueTAS in BakkesMod.
2. In a LAN match, put the car and ball at the intended start and click New Shot.
3. Record a segment, stop it, then use Stop and Update to commit and save.
4. Move the car and ball, select the shot from Saved TAS, click Load TAS, then Play / Record.
5. Check whether the car and ball return to their recorded states and whether playback behaves as expected in the LAN match.
6. Enable Slow motion while TAS runs and try a short attempt at a reduced game speed; check that Stop restores normal speed.
7. During a recording, press Insert Flip Reset and test whether the next airborne jump input grants the intended flip. Save and replay the shot to verify the marker is reproduced.
8. Report any issues before adding other features.

## Repository

Private GitHub repository: `BernardoAurelioGuilherme/RocketLeagueTAS`.
