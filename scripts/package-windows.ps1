# Creates the production package dist\ErgoPro2-<version>-windows-x64.zip from a
# Release build (documentation/BUILD.md 2.5). Run in a "x64 Native Tools"
# (MSVC) prompt so dumpbin and VCToolsRedistDir are available.
#
#   powershell -File scripts\package-windows.ps1 [-BuildDir build-release] [-OutDir dist] [-Installer]
#
# -Installer also builds the setup program dist\ErgoPro2-<version>-windows-x64-setup.exe
# from the same folder with Inno Setup 6 (packaging\windows\ErgoPro2.iss; ISCC.exe
# from PATH, $env:ISCC or the default Inno Setup folders).
#
# The zip contains one folder "ErgoPro2" with ONLY the files ErgoPro2.exe needs
# on Windows 11:
#   - ErgoPro2.exe
#   - the Qt DLLs the program actually loads (found by following the import
#     tables of the exe and the plugins with dumpbin)
#   - the Qt plugins ErgoPro2 uses: platforms\qwindows (window system),
#     imageformats\qsvg and iconengines\qsvgicon (SVG icons)
#   - the Microsoft C++ runtime DLLs (app-local, so the Visual C++
#     Redistributable does not have to be installed)
# Everything else windeployqt copies (translations, software OpenGL, D3D
# compiler, styles, TLS, network information, other image formats) is not used
# by ErgoPro2 (Fusion style, local sockets only) and is left out.
param(
    [string]$BuildDir = 'build-release',
    [string]$OutDir = 'dist',
    [switch]$Installer
)
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$match = Select-String -Path (Join-Path $root 'CMakeLists.txt') -Pattern '^\s+VERSION (\d+\.\d+\.\d+)' | Select-Object -First 1
if (-not $match) { throw 'Cannot read the version from CMakeLists.txt' }
$version = $match.Matches[0].Groups[1].Value

$exe = Join-Path $BuildDir 'bin\ErgoPro2.exe'
if (-not (Test-Path $exe)) { throw "$exe not found - build first" }
if (-not (Get-Command dumpbin -ErrorAction SilentlyContinue)) { throw 'dumpbin not found - run in an MSVC developer prompt' }

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$stage = Join-Path (Resolve-Path $OutDir) 'ErgoPro2'
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Path $stage | Out-Null
Copy-Item $exe $stage

Write-Host '== windeployqt'
windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw --no-compiler-runtime (Join-Path $stage 'ErgoPro2.exe')
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed ($LASTEXITCODE)" }

# --- plugins: keep only the ones ErgoPro2 uses --------------------------------
$keepPlugins = @('platforms\qwindows.dll', 'imageformats\qsvg.dll', 'iconengines\qsvgicon.dll')
foreach ($p in $keepPlugins) {
    if (-not (Test-Path (Join-Path $stage $p))) { throw "Required Qt plugin missing after windeployqt: $p" }
}
Get-ChildItem $stage -Recurse -File | Where-Object { $_.DirectoryName -ne $stage } | ForEach-Object {
    $relative = $_.FullName.Substring($stage.Length + 1)
    if ($keepPlugins -notcontains $relative) { Remove-Item $_.FullName -Force }
}
Get-ChildItem $stage -Recurse -Directory | Sort-Object { $_.FullName.Length } -Descending |
    Where-Object { -not (Get-ChildItem $_.FullName -Force) } | Remove-Item -Force

# --- Microsoft C++ runtime (app-local) ----------------------------------------
$redist = $env:VCToolsRedistDir
$crtDir = $null
if ($redist) {
    $crtDir = Get-ChildItem (Join-Path $redist 'x64') -Directory -Filter 'Microsoft.VC*.CRT' -ErrorAction SilentlyContinue |
        Select-Object -First 1
}
if (-not $crtDir) { throw 'Microsoft.VC*.CRT folder not found (VCToolsRedistDir) - run in an MSVC developer prompt' }
Copy-Item (Join-Path $crtDir.FullName '*.dll') $stage

# --- keep only the DLLs that are really loaded (import tables) ----------------
function Get-Imports([string]$file) {
    $lines = & dumpbin /nologo /dependents $file
    if ($LASTEXITCODE -ne 0) { throw "dumpbin failed for $file" }
    # Dependency lines are bare file names ("Qt6Core.dll"); skip "Dump of file C:\...\x.dll".
    $lines | ForEach-Object { $_.Trim() } | Where-Object { $_ -match '^[^\s\\/:]+\.dll$' }
}
$needed = @{}
$queue = New-Object System.Collections.Queue
$queue.Enqueue((Join-Path $stage 'ErgoPro2.exe'))
foreach ($p in $keepPlugins) { $queue.Enqueue((Join-Path $stage $p)) }
while ($queue.Count -gt 0) {
    foreach ($dll in Get-Imports $queue.Dequeue()) {
        $key = $dll.ToLowerInvariant()
        $local = Join-Path $stage $dll
        if (-not $needed.ContainsKey($key) -and (Test-Path $local)) {
            $needed[$key] = $true
            $queue.Enqueue($local)
        }
    }
}
Get-ChildItem $stage -File -Filter '*.dll' | Where-Object { -not $needed.ContainsKey($_.Name.ToLowerInvariant()) } |
    ForEach-Object { Write-Host "  not needed: $($_.Name)"; Remove-Item $_.FullName -Force }

# --- verify: every import is in the folder or part of Windows ------------------
$system32 = Join-Path $env:WINDIR 'System32'
$problems = @()
foreach ($file in Get-ChildItem $stage -Recurse -File -Include '*.exe', '*.dll') {
    foreach ($dll in Get-Imports $file.FullName) {
        if (Test-Path (Join-Path $stage $dll)) { continue }
        if ($dll -match '^(api|ext)-ms-') { continue } # Windows API sets
        $isRuntime = $dll -match '^(msvcp140|vcruntime140|concrt140)'
        if (-not $isRuntime -and (Test-Path (Join-Path $system32 $dll))) { continue }
        $problems += "$($file.Name) needs $dll"
    }
}
if ($problems.Count -gt 0) { throw "Missing DLLs:`n$($problems -join "`n")" }

Write-Host '== Package contents'
Get-ChildItem $stage -Recurse -File | ForEach-Object {
    '{0,-40} {1,8:N0} KB' -f $_.FullName.Substring($stage.Length + 1), ($_.Length / 1KB)
}

$zip = Join-Path $OutDir "ErgoPro2-$version-windows-x64.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path $stage -DestinationPath $zip -CompressionLevel Optimal
Write-Host "Created $zip ($([math]::Round((Get-Item $zip).Length / 1MB, 1)) MB)"

# --- setup program (Inno Setup) -------------------------------------------------
if ($Installer) {
    $iscc = $env:ISCC
    if (-not $iscc) { $iscc = (Get-Command ISCC.exe -ErrorAction SilentlyContinue).Source }
    foreach ($candidate in @("${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe", "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
                             "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe")) {
        if (-not $iscc -and (Test-Path $candidate)) { $iscc = $candidate }
    }
    if (-not $iscc) { throw 'ISCC.exe (Inno Setup 6) not found - install it (choco install innosetup) or set $env:ISCC' }
    $iss = Join-Path $root 'packaging\windows\ErgoPro2.iss'
    $out = (Resolve-Path $OutDir).Path
    & $iscc /Qp "/DAppVersion=$version" "/DSourceDir=$stage" "/DOutputDir=$out" $iss
    if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed ($LASTEXITCODE)" }
    $setup = Join-Path $out "ErgoPro2-$version-windows-x64-setup.exe"
    if (-not (Test-Path $setup)) { throw "$setup was not created" }
    Write-Host "Created $setup ($([math]::Round((Get-Item $setup).Length / 1MB, 1)) MB)"
}
