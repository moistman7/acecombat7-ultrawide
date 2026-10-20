$ErrorActionPreference='Stop'
$sdk='C:\Program Files (x86)\Windows Kits\10'
$llvm='C:\Program Files\LLVM\bin'
$version='10.0.26100.0'
& "$llvm\clang.exe" --target=x86_64-pc-windows-msvc -O2 -fno-stack-protector -ffreestanding "-I$sdk\Include\$version\um" "-I$sdk\Include\$version\shared" "-I$sdk\Include\$version\ucrt" -c "$PSScriptRoot\TestUI.c" -o "$PSScriptRoot\build\TestUI.obj" -Wall -Wextra -Werror
if($LASTEXITCODE){throw 'Test compilation failed'}
& "$llvm\lld-link.exe" /entry:TestMain /subsystem:console /nodefaultlib "/out:$PSScriptRoot\build\TestUI.exe" "$PSScriptRoot\build\TestUI.obj" "$sdk\Lib\$version\um\x64\kernel32.lib" "$sdk\Lib\$version\um\x64\user32.lib" "$sdk\Lib\$version\um\x64\dxguid.lib"
if($LASTEXITCODE){throw 'Test link failed'}
& "$PSScriptRoot\build\TestUI.exe"
if($LASTEXITCODE){throw "Native WARP UI test failed: $LASTEXITCODE"}
Write-Host 'Native WARP UI tests passed: background widening, state restoration, small-panel exclusion, cutscene text centering, already-centered text exclusion, native map/update hooks.'
