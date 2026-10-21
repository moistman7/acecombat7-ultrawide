# ACE COMBAT 7 native ultrawide mod

Release v0.12.0. Locally tested with Steam build 9855922 at 3440x1440.

This x64 ASI plugin adjusts gameplay FOV and aspect constraints in memory. The original game executable is not modified. An ASI loader is required and is not included.

## Features

- Centered title, main-menu, and mission-menu layers with opaque 16:9 side bars.
- Full-width briefing map, with the blue backdrop boundary seam corrected.
- Hangar panels extend to the screen edges. Aircraft-selection UI stays at its settled scale while switching aircraft.
- HUD placement supports `HUDMode=Screen` or `HUDMode=16:9`; F6 toggles it. F7 toggles menu edge correction.
- `MenuBlackBars=0` retains experimental background-cover/edge-extension behavior.

## Install or update

Close the game and back up your existing plugin and configuration. Extract `Ace7Ultrawide.asi` beside the game executable, in the main ACE COMBAT 7 game directory where your ASI loader is installed. Preserve existing ReShade/Luma files. An ASI loader must already be installed.

For a new installation, copy the included `Ace7Ultrawide.ini` and set Width and Height for your display. For an update, merge the settings into your existing INI so your display and HUD preferences are preserved. Restart after configuration changes. Root/native Install.ps1 scripts are legacy migration tools and should not deploy this release.

Diagnostics and automatic capture default to off. Normal F12 screenshots do not start mod logging or GPU readbacks with Diagnostics=0. Set Diagnostics=1 only when gathering troubleshooting captures. Keep AutoCompositeCapture=0 for normal play.

## Performance and validation

Mask-free scenes skip render-target and shader-resource queries for menu-mask inheritance. Context epochs are cached, and a new 3D scene invalidates old menu masks across intermediate textures. No new per-frame GPU readbacks are introduced. Existing artwork detection has a bounded, cached fallback; initial UI buffer snapshots may also read back once when first encountered.

An isolated WARP CPU benchmark measured 20,000 mask-free inheritance checks at a median 1,205 microseconds without the fast path and 26 microseconds with it. These are CPU helper measurements, not an in-game FPS or frame-time benchmark.

The visual fixes were confirmed in-game by the user on the local setup. The release also passes rendered Menu, UI, and HUD tests covering side-mask composition, state restoration, map-backdrop continuity, aircraft-selection scale stability, and screenshot diagnostics gating. Other game builds and display configurations have not been verified here.

## Build and tests

Run `native/Build.ps1` with LLVM clang/lld and Windows SDK 10.0.26100.0 at the paths specified in that script. Shader headers are generated with the SDK compiler; runtime shader compilation is unnecessary.

Run `native/TestMenu.ps1`, `native/TestUI.ps1`, and `native/TestHUD.ps1` for D3D11 WARP tests. They require local game shader dumps. Private fixtures in native/build are menu-background.bc1, menu-composition.bgra (1920x1080 BGRA), and map-backdrop.bgra (480x270 BGRA). Extracted inputs are excluded from Git and the release.

## Package contents

Only the ASI plugin, default INI, and this README are packaged. No loader, game textures, shader dumps, screenshots, backups, installation manifests, or private fixtures are distributed.
