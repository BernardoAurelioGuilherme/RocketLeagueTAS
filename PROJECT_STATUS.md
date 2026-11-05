# Project status

RocketLeagueTAS is a BakkesMod plugin for building and replaying car-input TAS shots in Rocket Plugin LAN matches. The intended scope is local car input playback plus restoring the local car and ball to the shot's initial physics state. It is for offline/LAN use, not competitive online play.

## Current implementation

- Dear ImGui settings panel with New Shot, Play / Record, Stop, Update, and Stop and Update.
- Captures and replays local car controller inputs through the BakkesMod vehicle input hook.
- New Shot captures the local car and ball physics states; subsequent Play / Record attempts restore both before replay.
- Update commits the candidate input segment and saves a `.rltas` file.
- Saved TAS dropdown, Load TAS, and Refresh list controls.
- TAS files are stored in BakkesMod's data folder at `RocketLeagueTAS/shots`.
- Global game speed control (0.10x to 2.00x), applied outside TAS attempts and periodically enforced through `ServerWrapper::SetGameSpeed`.
- Changing the Game speed slider during recording or playback queues a speed point that is attached atomically to the next input frame. Playback speed points save immediately; recording speed points save with Stop and Update. The HUD reports the speed-point count.
- Ball hit speed multiplier slider from 1.0x to 5.0x in 0.1 increments. It scales the local car's velocity change to the ball on contact and raises the ball max linear speed to match.
- `.rltas` format version 4 stores speed markers and continues to load versions 1–3; legacy flip-reset markers are ignored.
- Other future functions are not implemented yet.

## Latest build

- Latest source commit: `d51cf93` (`Remove automatic flip reset feature`).
- Windows GitHub Actions run `41944315189` succeeded.
- The latest DLL is installed in the user's BakkesMod plugin folder; reload the plugin or restart Rocket League to use it.
- Ball hit velocity scaling is applied 0.01 seconds after the hit callback so the normal collision physics can settle first. The ball multiplier is built and installed; retest to confirm the ball no longer feels heavy.

## Next test steps

1. Reload RocketLeagueTAS in BakkesMod.
2. In a LAN match, put the car and ball at the intended start and click New Shot.
3. Record a segment, stop it, then use Stop and Update to commit and save.
4. Move the car and ball, select the shot from Saved TAS, click Load TAS, then Play / Record.
5. Check whether the car and ball return to their recorded states and whether playback behaves as expected in the LAN match.
6. Change Game speed and verify it applies even while no TAS attempt is active.
7. Set Ball hit speed multiplier to 1.2x, hit a still ball with a repeatable touch, and compare its outgoing speed with 1.0x. Then try 5.0x and confirm the ball remains controllable.
8. During recording or playback, change Game speed; verify the speed-point counter increments and the change repeats on playback.
9. Report any issues before adding other features.

## Repository

Private GitHub repository: `BernardoAurelioGuilherme/RocketLeagueTAS`.
