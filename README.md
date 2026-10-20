# ACE COMBAT 7 native ultrawide mod

Development candidate v0.12.0-rc.1 for the Steam build 9855922 tested locally.

The x64 ASI plugin adjusts gameplay FOV and aspect constraints in memory. The original game executable is not modified. It requires an ASI loader; no loader or game assets are distributed here.

## Menu and scene behavior

- `MenuBlackBars=1` centers supported title, main-menu and mission menu/card layers at 16:9, with opaque black side masks.
- The briefing map remains full width. Scene draws reset menu-mask state.
- Hangar panels extend to the screen edges without scaling their text. The captured hangar depth is recognized across its scale transition.
- HUD placement uses `HUDMode=Screen` or `HUDMode=16:9`; F6 toggles it. F7 toggles menu edge correction.
- `MenuBlackBars=0` retains the experimental background-cover/edge-extension mode. Pure-art tags are invalidated when other elements are composed over artwork; separate background lineage supports masks without stretching text.

## Install

Close the game. Back up any previous `Ace7Ultrawide.asi` and INI, then place the ASI in the game directory beside the executable. An ASI loader must already be installed. Preserve existing ReShade/Luma files. Copy the supplied INI for a new installation, or merge the settings into an existing configuration. The root and native Install.ps1 scripts are legacy migration scripts and should not deploy this candidate.

Defaults target 3440x1440; set Width and Height for your display. Restart after configuration changes. Diagnostics and automatic capture are disabled in the release INI.

## Build and validation

Run `native/Build.ps1` in PowerShell with LLVM clang/lld and Windows SDK 10.0.26100.0 in the paths specified by the script. Shader headers are generated with the SDK compiler; runtime shader compilation is unnecessary.

`native/TestMenu.ps1`, `native/TestUI.ps1`, and `native/TestHUD.ps1` exercise D3D11 WARP rendering and state restoration. Local tests currently require shader dumps from the installed game, and TestUI requires a private artwork fixture under native/build. These extracted inputs are intentionally excluded from this repository and release.

The candidate passes local rendered tests, including opaque side-mask pixels, central glyph proportions, both windings, scene-mask reset, background lineage after overlays, and hangar transition scales. In-game confirmation of this candidate is pending. Prior screenshots showed remaining streaks and nonblack side regions; passing tests alone does not establish that those are fixed in-game.

## Package contents

Only the plugin, default configuration, and this README are packaged. Game textures, shader dumps, screenshots, backups, installation manifests, and private fixtures are excluded.
