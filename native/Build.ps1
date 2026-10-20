$ErrorActionPreference = 'Stop'
$sdk = 'C:\Program Files (x86)\Windows Kits\10'
$version = '10.0.26100.0'
$llvm = 'C:\Program Files\LLVM\bin'
$out = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Path $out -Force | Out-Null
& "$sdk\bin\$version\x64\fxc.exe" /nologo /O3 /T ps_5_0 /E main "/Fh$PSScriptRoot\ScreenHUD.h" /Vn ScreenHUD "$PSScriptRoot\ScreenHUD.hlsl"
if ($LASTEXITCODE) { throw 'HUD shader compilation failed.' }
& "$sdk\bin\$version\x64\fxc.exe" /nologo /O3 /T gs_5_0 /E main "/Fh$PSScriptRoot\MenuEdges.h" /Vn MenuEdges "$PSScriptRoot\MenuEdges.hlsl"
if ($LASTEXITCODE) { throw 'Menu geometry shader compilation failed.' }
& "$sdk\bin\$version\x64\fxc.exe" /nologo /O3 /T gs_5_0 /E main "/Fh$PSScriptRoot\MenuEdgesSlate.h" /Vn MenuEdgesSlate "$PSScriptRoot\MenuEdgesSlate.hlsl"
if ($LASTEXITCODE) { throw 'Slate menu geometry shader compilation failed.' }
& "$llvm\clang.exe" --target=x86_64-pc-windows-msvc -O2 -fno-stack-protector -ffreestanding -DUNICODE -D_UNICODE "-I$sdk\Include\$version\um" "-I$sdk\Include\$version\shared" "-I$sdk\Include\$version\ucrt" -c "$PSScriptRoot\Ultrawide.c" -o "$out\Ultrawide.obj" -Wall -Wextra -Werror
if ($LASTEXITCODE) { throw 'Compilation failed.' }
& "$llvm\lld-link.exe" /dll /entry:DllMain /nodefaultlib /machine:x64 /dynamicbase /nxcompat "/out:$out\Ace7Ultrawide.dll" "$out\Ultrawide.obj" "$sdk\Lib\$version\um\x64\kernel32.lib" "$sdk\Lib\$version\um\x64\user32.lib" "$sdk\Lib\$version\um\x64\dxguid.lib"
if ($LASTEXITCODE) { throw 'Link failed.' }
Copy-Item -LiteralPath "$out\Ace7Ultrawide.dll" -Destination "$out\Ace7Ultrawide.asi" -Force
Copy-Item -LiteralPath "$PSScriptRoot\Ace7Ultrawide.ini" -Destination "$out\Ace7Ultrawide.ini" -Force
