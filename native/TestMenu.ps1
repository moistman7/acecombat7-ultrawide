$ErrorActionPreference='Stop'
$sdk='C:\Program Files (x86)\Windows Kits\10'
$version='10.0.26100.0'
$llvm='C:\Program Files\LLVM\bin'
& "$sdk\bin\$version\x64\fxc.exe" /nologo /O3 /T vs_5_0 /E main "/Fh$PSScriptRoot\TestMenuVS.h" /Vn TestMenuVS "$PSScriptRoot\TestMenuVS.hlsl"
if($LASTEXITCODE){throw 'Menu fixture shader failed'}
& "$llvm\clang.exe" --target=x86_64-pc-windows-msvc -O2 -fno-stack-protector -ffreestanding "-I$sdk\Include\$version\um" "-I$sdk\Include\$version\shared" "-I$sdk\Include\$version\ucrt" -c "$PSScriptRoot\TestMenu.c" -o "$PSScriptRoot\build\TestMenu.obj" -Wall -Wextra -Werror
if($LASTEXITCODE){throw 'Menu test compilation failed'}
& "$llvm\lld-link.exe" /entry:TestMain /subsystem:console /nodefaultlib "/out:$PSScriptRoot\build\TestMenu.exe" "$PSScriptRoot\build\TestMenu.obj" "$sdk\Lib\$version\um\x64\kernel32.lib" "$sdk\Lib\$version\um\x64\user32.lib" "$sdk\Lib\$version\um\x64\dxguid.lib"
if($LASTEXITCODE){throw 'Menu test link failed'}
& "$PSScriptRoot\build\TestMenu.exe"
if($LASTEXITCODE){throw "Rendered menu test failed: $LASTEXITCODE"}
Write-Host 'Rendered menu tests passed: white side regions filled, center glyph position and width unchanged, span mode moves the left half to the edge unscaled, original geometry shader and sampler restored, existing geometry stages preserved; offscreen and sliding planes stay in place; background cover scales uniformly; centered perspective panels fill both sides without moving text.'
