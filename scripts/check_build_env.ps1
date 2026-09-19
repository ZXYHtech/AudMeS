[CmdletBinding()]
param(
    [string]$Msys2Root,
    [switch]$PassThru
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-Msys2CandidateRoots {
    $candidates = New-Object System.Collections.Generic.List[string]

    if ($Msys2Root) {
        $candidates.Add($Msys2Root)
    }
    if ($env:MSYS2_ROOT) {
        $candidates.Add($env:MSYS2_ROOT)
    }
    if ($env:MSYS_ROOT) {
        $candidates.Add($env:MSYS_ROOT)
    }

    $launcher = Get-Command 'msys2_shell.cmd' -ErrorAction SilentlyContinue
    if ($launcher) {
        $candidates.Add((Split-Path -Parent $launcher.Source))
    }

    $candidates.Add('C:\msys64')
    $candidates.Add('C:\tools\msys64')
    $candidates.Add('D:\msys64')

    if ($env:LOCALAPPDATA) {
        $candidates.Add((Join-Path $env:LOCALAPPDATA 'Programs\msys64'))
    }
    if ($env:USERPROFILE) {
        $candidates.Add((Join-Path $env:USERPROFILE 'scoop\apps\msys2\current'))
    }

    return $candidates | Select-Object -Unique
}

function Get-FirstLine {
    param([object[]]$Text)

    return (($Text | Out-String).Trim() -split "`r?`n")[0]
}

Write-Host '正在检查 AudMeS Windows 构建环境...' -ForegroundColor Cyan

$resolvedRoot = $null
foreach ($candidate in Get-Msys2CandidateRoots) {
    if (-not $candidate) {
        continue
    }

    $expanded = [Environment]::ExpandEnvironmentVariables($candidate)
    if (Test-Path -LiteralPath (Join-Path $expanded 'mingw64\bin')) {
        $resolvedRoot = (Resolve-Path -LiteralPath $expanded).Path
        break
    }
}

if (-not $resolvedRoot) {
    throw @'
未找到 64 位 MSYS2。请先安装 MSYS2，并安装以下软件包：
  pacman -S --needed base-devel git mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-wxwidgets3.2
如果安装在非默认目录，请使用 -Msys2Root 指定路径，例如：-Msys2Root D:\msys64
'@
}

$mingwBin = Join-Path $resolvedRoot 'mingw64\bin'
$usrBin = Join-Path $resolvedRoot 'usr\bin'
$requiredTools = [ordered]@{
    Bash = Join-Path $usrBin 'bash.exe'
    GCC = Join-Path $mingwBin 'gcc.exe'
    GXX = Join-Path $mingwBin 'g++.exe'
    CMake = Join-Path $mingwBin 'cmake.exe'
    CPack = Join-Path $mingwBin 'cpack.exe'
    Make = Join-Path $mingwBin 'mingw32-make.exe'
    Objdump = Join-Path $mingwBin 'objdump.exe'
    WxConfig = Join-Path $mingwBin 'wx-config'
}

$missingTools = @()
foreach ($entry in $requiredTools.GetEnumerator()) {
    if (-not (Test-Path -LiteralPath $entry.Value -PathType Leaf)) {
        $missingTools += "$($entry.Key): $($entry.Value)"
    }
}

$gitCommand = Get-Command 'git.exe' -ErrorAction SilentlyContinue
if (-not $gitCommand) {
    $msysGit = Join-Path $usrBin 'git.exe'
    if (Test-Path -LiteralPath $msysGit -PathType Leaf) {
        $gitPath = $msysGit
    }
    else {
        $missingTools += "Git: $msysGit"
    }
}
else {
    $gitPath = $gitCommand.Source
}

if ($missingTools.Count -gt 0) {
    $details = $missingTools -join "`n  - "
    throw @"
MSYS2 已找到，但构建工具不完整：
  - $details
请在 MSYS2 MINGW64 终端执行：
  pacman -S --needed base-devel git mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-wxwidgets3.2
"@
}

$oldPath = $env:Path
$oldMsystem = $env:MSYSTEM
try {
    $env:Path = "$mingwBin;$usrBin;$oldPath"
    $env:MSYSTEM = 'MINGW64'

    $cmakeVersion = Get-FirstLine (& $requiredTools.CMake --version 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw 'CMake 无法运行。'
    }

    $gccVersion = Get-FirstLine (& $requiredTools.GXX --version 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw 'G++ 无法运行。'
    }

    $wxVersion = Get-FirstLine (& $requiredTools.Bash -lc 'wx-config --version' 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw 'wx-config 无法运行，请确认安装了 MinGW64 版本的 wxWidgets 3.2。'
    }

    if ($wxVersion -notmatch '^3\.2(?:\.|$)') {
        throw "wxWidgets 版本不符合要求：$wxVersion（需要 3.2.x）。"
    }
}
finally {
    $env:Path = $oldPath
    $env:MSYSTEM = $oldMsystem
}

$environmentInfo = [pscustomobject]@{
    Msys2Root = $resolvedRoot
    MingwBin = $mingwBin
    UsrBin = $usrBin
    Bash = $requiredTools.Bash
    GCC = $requiredTools.GCC
    GXX = $requiredTools.GXX
    CMake = $requiredTools.CMake
    CPack = $requiredTools.CPack
    Make = $requiredTools.Make
    Objdump = $requiredTools.Objdump
    WxConfig = $requiredTools.WxConfig
    Git = $gitPath
    CMakeVersion = $cmakeVersion
    GCCVersion = $gccVersion
    WxWidgetsVersion = $wxVersion
}

Write-Host "MSYS2:    $resolvedRoot" -ForegroundColor Green
Write-Host "CMake:    $cmakeVersion" -ForegroundColor Green
Write-Host "编译器:   $gccVersion" -ForegroundColor Green
Write-Host "wxWidgets: $wxVersion" -ForegroundColor Green
Write-Host '构建环境检查通过。' -ForegroundColor Green

if ($PassThru) {
    $environmentInfo
}
