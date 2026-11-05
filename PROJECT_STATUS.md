# Project status

RocketLeagueTAS is a two-plugin BakkesMod bundle for building and replaying car-input TAS shots, plus hosting or joining basic LAN matches. `RocketLeagueTASLAN` is designed to provide the basic LAN controls directly, without requiring Rocket Plugin. The intended scope is offline/LAN use, not competitive online play.

## Current implementation

- Dear ImGui settings panel with New Shot, Play / Record, Stop, Update, and Stop and Update.
- Captures and replays local car controller inputs through the BakkesMod vehicle input hook.
- New Shot captures the local car and ball physics states; subsequent Play / Record attempts restore both before replay.
- Update commits the candidate input segment and saves a `.rltas` file.
- Saved TAS dropdown, Load TAS, and Refresh list controls.
- TAS files are stored in BakkesMod's data folder at `RocketLeagueTAS/shots`.
- Global game speed control (0.10x to 2.00x), applied outside TAS attempts and periodically enforced through `ServerWrapper::SetGameSpeed`.
- Changing the Game speed slider during recording or playback queues a speed point that is attached atomically to the next input frame. Playback speed points save immediately; recording speed points save with Stop and Update. The HUD reports the speed-point count.
- Each shot stores its starting game speed and restores it at the beginning of every retry, so speed points are replayed from a known baseline.
- Ball hit speed multiplier slider from 1.0x to 5.0x in 0.1 increments. It scales the local car's velocity change to the ball on contact and raises the ball max linear speed to match.
- `.rltas` format version 5 stores the shot starting speed and speed markers. Versions 1–4 remain loadable; legacy flip-reset markers are ignored.
- The project now builds two separate BakkesMod entries: `RocketLeagueTAS` for shot TAS controls and `RocketLeagueTASLAN` for basic match hosting/joining.
- The initial LAN panel supports standard arenas, Soccar/Hoops/Snow Day/Rumble/Dropshot, bot difficulty tags, player slots, and direct IP/port joining. VPN setup is external; friends should enter the host's VPN address.
- Other future functions are not implemented yet.

## Latest build

- Latest source commit: `17e3176` (`Add companion LAN host plugin`).
- Windows GitHub Actions run `41955844244` succeeded and produced both DLLs.
- Both DLLs are installed in the user's BakkesMod plugin folder; reload them or restart Rocket League to use them.
- The LAN host/join commands are a first implementation and still need in-game validation, including arena/mode selection and bot spawning.
- Ball hit velocity scaling is applied 0.01 seconds after the hit callback so the normal collision physics can settle first. The ball multiplier is built and installed; retest to confirm the ball no longer feels heavy.

## Next test steps

1. Reload both `RocketLeagueTAS` and `RocketLeagueTASLAN` in BakkesMod, or restart Rocket League.
2. Open the `RocketLeagueTASLAN` panel and check the host arena, mode, bot difficulty, player slots, and join address/port controls.
3. Host a match and verify the selected arena/mode and bot count. From another machine, try joining with the host's VPN IP and port 7777.
4. In a LAN match, put the car and ball at the intended start and click New Shot.
5. Record a segment, stop it, then use Stop and Update to commit and save.
6. Move the car and ball, select the shot from Saved TAS, click Load TAS, then Play / Record.
7. Check whether the car and ball return to their recorded states and whether playback behaves as expected in the LAN match.
8. Change Game speed and verify it applies even while no TAS attempt is active.
9. Set Ball hit speed multiplier to 1.2x, hit a still ball with a repeatable touch, and compare its outgoing speed with 1.0x. Then try 5.0x and confirm the ball remains controllable.
10. During recording or playback, change Game speed; verify the speed-point counter increments and the change repeats on playback.
11. Report any issues before adding other features.

## Repository

GitHub repository: `BernardoAurelioGuilherme/RocketLeagueTAS`.
