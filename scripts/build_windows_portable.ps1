[CmdletBinding()]
param(
    [string]$Msys2Root,
    [switch]$SkipClean,
    [switch]$Run,
    [switch]$CleanOnly,
    [ValidateRange(1, 64)]
    [int]$Jobs = [Math]::Max(1, [Environment]::ProcessorCount)
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot 'build\windows-mingw64-release'
$distDir = Join-Path $repoRoot 'dist'
$portableDir = Join-Path $distDir 'AudMeS-cn-preview-win64'
$dependencyDir = Join-Path $repoRoot 'libfccp'
$dependencyHeader = Join-Path $dependencyDir 'csv.h'

function Remove-GeneratedPath {
    param([string]$Path)

    if (Test-Path -LiteralPath $Path) {
        $resolved = [System.IO.Path]::GetFullPath((Resolve-Path -LiteralPath $Path).Path)
        $allowedRoots = @(
            [System.IO.Path]::GetFullPath((Join-Path $repoRoot 'build')),
            [System.IO.Path]::GetFullPath($distDir)
        )
        $isAllowed = $false
        foreach ($allowedRoot in $allowedRoots) {
            $allowedPrefix = $allowedRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
            if ($resolved.Equals($allowedRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
                $resolved.StartsWith($allowedPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
                $isAllowed = $true
                break
            }
        }
        if (-not $isAllowed) {
            throw "拒绝清理仓库生成目录之外的路径：$resolved"
        }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}

function Invoke-CheckedCommand {
    param(
        [string]$FilePath,
        [string[]]$Arguments,
        [string]$Description
    )

    Write-Host "`n[$Description]" -ForegroundColor Cyan
    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Description 失败，退出代码：$LASTEXITCODE"
    }
}

function Copy-MingwRuntimeDependencies {
    param(
        [string]$Executable,
        [string]$TargetDirectory,
        [string]$MingwDirectory,
        [string]$Objdump
    )

    $pending = New-Object System.Collections.Generic.Queue[string]
    $visitedFiles = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    $handledDlls = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    $copiedCount = 0
    $pending.Enqueue($Executable)

    while ($pending.Count -gt 0) {
        $binary = $pending.Dequeue()
        if (-not $visitedFiles.Add($binary)) {
            continue
        }

        $dump = & $Objdump -p $binary 2>&1
        if ($LASTEXITCODE -ne 0) {
            throw "无法读取运行库依赖：$binary"
        }

        foreach ($line in $dump) {
            if ($line -notmatch 'DLL Name:\s*(?<name>[^\s]+)') {
                continue
            }

            $dllName = $Matches.name.Trim()
            if (-not $handledDlls.Add($dllName)) {
                continue
            }

            $sourceDll = Join-Path $MingwDirectory $dllName
            if (-not (Test-Path -LiteralPath $sourceDll -PathType Leaf)) {
                continue
            }

            $targetDll = Join-Path $TargetDirectory $dllName
            if (-not (Test-Path -LiteralPath $targetDll -PathType Leaf)) {
                Copy-Item -LiteralPath $sourceDll -Destination $targetDll -Force
                $copiedCount++
                Write-Host "  已加入运行库：$dllName"
            }
            $pending.Enqueue($targetDll)
        }
    }

    return $copiedCount
}

if (-not $SkipClean) {
    Write-Host '正在清理旧的 Windows 构建和便携包...' -ForegroundColor Cyan
    Remove-GeneratedPath -Path $buildDir
    Remove-GeneratedPath -Path $portableDir
    if (Test-Path -LiteralPath $distDir) {
        Get-ChildItem -LiteralPath $distDir -Filter 'AudMeS-*-cn-preview-win64.zip' -File |
            ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force }
    }
}

if ($CleanOnly) {
    Write-Host '清理完成。' -ForegroundColor Green
    exit 0
}

$environmentInfo = & (Join-Path $PSScriptRoot 'check_build_env.ps1') -Msys2Root $Msys2Root -PassThru
$oldPath = $env:Path
$oldMsystem = $env:MSYSTEM
$oldCc = $env:CC
$oldCxx = $env:CXX

try {
    $env:Path = "$($environmentInfo.MingwBin);$($environmentInfo.UsrBin);$oldPath"
    $env:MSYSTEM = 'MINGW64'
    $env:CC = $environmentInfo.GCC
    $env:CXX = $environmentInfo.GXX

    if (-not (Test-Path -LiteralPath $dependencyHeader -PathType Leaf)) {
        if (Test-Path -LiteralPath $dependencyDir) {
            throw "依赖目录已存在但缺少 csv.h，请检查或移走该目录：$dependencyDir"
        }
        Invoke-CheckedCommand -FilePath $environmentInfo.Git -Arguments @(
            'clone', '--depth', '1',
            'https://github.com/ben-strasser/fast-cpp-csv-parser.git',
            $dependencyDir
        ) -Description '获取 fast-cpp-csv-parser'
    }
    else {
        Write-Host "已找到 fast-cpp-csv-parser：$dependencyHeader" -ForegroundColor Green
    }

    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
    New-Item -ItemType Directory -Path $distDir -Force | Out-Null

    Invoke-CheckedCommand -FilePath $environmentInfo.CMake -Arguments @(
        '-S', $repoRoot,
        '-B', $buildDir,
        '-G', 'MinGW Makefiles',
        '-DCMAKE_BUILD_TYPE=Release'
    ) -Description '配置 Release 构建'

    Invoke-CheckedCommand -FilePath $environmentInfo.CMake -Arguments @(
        '--build', $buildDir,
        '--target', 'package',
        '--parallel', $Jobs
    ) -Description '编译并生成 CPack ZIP'

    $cpackZip = Get-ChildItem -LiteralPath $buildDir -Filter 'AudMeS-*.zip' -File |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1
    if (-not $cpackZip) {
        throw "CPack 未生成 ZIP：$buildDir"
    }

    $extractDir = Join-Path $buildDir '_cpack_extract'
    Remove-GeneratedPath -Path $extractDir
    New-Item -ItemType Directory -Path $extractDir -Force | Out-Null
    Expand-Archive -LiteralPath $cpackZip.FullName -DestinationPath $extractDir -Force

    $packagedExe = Get-ChildItem -LiteralPath $extractDir -Filter 'AudMeS.exe' -File -Recurse |
        Select-Object -First 1
    if (-not $packagedExe) {
        throw "CPack ZIP 中缺少 AudMeS.exe：$($cpackZip.FullName)"
    }

    New-Item -ItemType Directory -Path $portableDir -Force | Out-Null
    Get-ChildItem -LiteralPath $packagedExe.DirectoryName -Force |
        Copy-Item -Destination $portableDir -Recurse -Force

    $portableGuideAssets = Join-Path $portableDir 'guide_assets'
    if (-not (Test-Path -LiteralPath $portableGuideAssets -PathType Container)) {
        Copy-Item -LiteralPath (Join-Path $repoRoot 'guide_assets') -Destination $portableGuideAssets -Recurse -Force
    }

    $portableExe = Join-Path $portableDir 'AudMeS.exe'
    $copiedDllCount = Copy-MingwRuntimeDependencies `
        -Executable $portableExe `
        -TargetDirectory $portableDir `
        -MingwDirectory $environmentInfo.MingwBin `
        -Objdump $environmentInfo.Objdump

    $zipName = 'AudMeS-{0}-cn-preview-win64.zip' -f (Get-Date -Format 'yyyy.MM.dd')
    $finalZip = Join-Path $distDir $zipName
    if (Test-Path -LiteralPath $finalZip) {
        Remove-Item -LiteralPath $finalZip -Force
    }
    Compress-Archive -LiteralPath $portableDir -DestinationPath $finalZip -CompressionLevel Optimal

    if (-not (Test-Path -LiteralPath $portableExe -PathType Leaf)) {
        throw "便携版可执行文件不存在：$portableExe"
    }
    if (-not (Test-Path -LiteralPath $finalZip -PathType Leaf)) {
        throw "便携版 ZIP 不存在：$finalZip"
    }

    Write-Host "`nWindows 中文预览版构建完成。" -ForegroundColor Green
    Write-Host "补齐 MinGW 运行库：$copiedDllCount 个"
    Write-Host "EXE：$portableExe" -ForegroundColor Green
    Write-Host "ZIP：$finalZip" -ForegroundColor Green

    if ($Run) {
        Write-Host '正在启动 AudMeS...' -ForegroundColor Cyan
        Start-Process -FilePath $portableExe -WorkingDirectory $portableDir
    }
}
finally {
    $env:Path = $oldPath
    $env:MSYSTEM = $oldMsystem
    $env:CC = $oldCc
    $env:CXX = $oldCxx
}
