$ErrorActionPreference='Stop'
$sdk='C:\Program Files (x86)\Windows Kits\10'
$version='10.0.26100.0'
$llvm='C:\Program Files\LLVM\bin'
& "$sdk\bin\$version\x64\fxc.exe" /nologo /O3 /T vs_5_0 /E main "/Fh$PSScriptRoot\TestHUDVS.h" /Vn TestHUDVS "$PSScriptRoot\TestHUDVS.hlsl"
if($LASTEXITCODE){throw 'Fixture shader failed'}
& "$llvm\clang.exe" --target=x86_64-pc-windows-msvc -O2 -fno-stack-protector -ffreestanding "-I$sdk\Include\$version\um" "-I$sdk\Include\$version\shared" "-I$sdk\Include\$version\ucrt" -c "$PSScriptRoot\TestHUD.c" -o "$PSScriptRoot\build\TestHUD.obj" -Wall -Wextra -Werror
if($LASTEXITCODE){throw 'HUD test compilation failed'}
& "$llvm\lld-link.exe" /entry:TestMain /subsystem:console /nodefaultlib "/out:$PSScriptRoot\build\TestHUD.exe" "$PSScriptRoot\build\TestHUD.obj" "$sdk\Lib\$version\um\x64\kernel32.lib" "$sdk\Lib\$version\um\x64\user32.lib" "$sdk\Lib\$version\um\x64\dxguid.lib"
if($LASTEXITCODE){throw 'HUD test link failed'}
& "$PSScriptRoot\build\TestHUD.exe"
if($LASTEXITCODE){throw "Rendered HUD test failed: $LASTEXITCODE"}
Write-Host 'Rendered HUD tests passed: glyph placement and state restoration; both final-blend variants preserve radar contrast, and the portrait variant preserves portrait contrast in both modes.'
