param([string]$GamePath = 'D:\SteamLibrary\steamapps\common\ACE COMBAT 7')
$ErrorActionPreference = 'Stop'
if (Get-Process Ace7Game -ErrorAction SilentlyContinue) { throw 'Close the game first.' }
$record = Join-Path (Split-Path $PSScriptRoot) 'installation.json'
$manifest = Get-Content -Raw -LiteralPath $record | ConvertFrom-Json
$exeEntry = $manifest.Entries | Where-Object { $_.Destination -eq (Join-Path $GamePath 'Ace7Game.exe') }
if (!$exeEntry) { throw 'Original executable backup is not recorded.' }
if ((Get-FileHash -LiteralPath $exeEntry.Destination).Hash -ne $exeEntry.InstalledHash) { throw 'The executable changed since installation.' }
if (!(Test-Path -LiteralPath $exeEntry.Backup)) { throw 'Missing original executable backup.' }
$newEntries = @()
foreach ($name in @('Ace7Ultrawide.asi','Ace7Ultrawide.ini','dinput8.dll')) {
    $destination = Join-Path $GamePath $name
    if (Test-Path -LiteralPath $destination) { throw "Already exists: $destination" }
    $source = if ($name -eq 'dinput8.dll') { Join-Path $GamePath 'version.dll' } else { Join-Path $PSScriptRoot "build\$name" }
    if (!(Test-Path -LiteralPath $source)) { throw "Missing build: $source" }
    $newEntries += [pscustomobject]@{ Destination=$destination; Backup=''; Existed=$false; InstalledHash=(Get-FileHash -LiteralPath $source).Hash }
}
Copy-Item -LiteralPath $record -Destination ($record + '.before-native') -Force
try {
    foreach ($entry in $newEntries) {
        $name = Split-Path $entry.Destination -Leaf
        $source = if ($name -eq 'dinput8.dll') { Join-Path $GamePath 'version.dll' } else { Join-Path $PSScriptRoot "build\$name" }
        Copy-Item -LiteralPath $source -Destination $entry.Destination
    }
    Copy-Item -LiteralPath $exeEntry.Backup -Destination $exeEntry.Destination -Force
    $exeEntry.InstalledHash = (Get-FileHash -LiteralPath $exeEntry.Backup).Hash
    if ((Get-FileHash -LiteralPath $exeEntry.Destination).Hash -ne $exeEntry.InstalledHash) { throw 'Original EXE restore verification failed.' }
    $manifest.Entries = @($manifest.Entries) + $newEntries
    $manifest.BinaryReport = 'Native ASI runtime patch; original packed executable restored byte-for-byte.'
    $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $record -Encoding UTF8
} catch {
    foreach ($entry in $newEntries) { if (Test-Path -LiteralPath $entry.Destination) { Remove-Item -LiteralPath $entry.Destination } }
    Copy-Item -LiteralPath (Join-Path (Split-Path $PSScriptRoot) 'stage\Ace7Game.exe') -Destination $exeEntry.Destination -Force
    Copy-Item -LiteralPath ($record + '.before-native') -Destination $record -Force
    throw
}
Write-Host 'Native ASI installed. Original EXE restored and verified; existing loader and shader corrections retained.'
