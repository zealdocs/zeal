#
# run-clazy.ps1 - Run clazy (Qt-aware static analysis) on the codebase
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#
# By default, runs clazy-standalone with the "level1" check group.
# Use -Check <name> to run a specific check (e.g., -Check qstring-allocations)
# or a different group (e.g., -Check level2).
#
# Check list: https://github.com/KDE/clazy/blob/master/README.md#list-of-checks
# Use -Fix to apply suggested fixes (requires clang-apply-replacements).
# Use -Staged to only process files staged for commit.
# Use -Verbose to print each file being processed.
#
# Requires a compile_commands.json in the build directory (build/dev).
#

[CmdletBinding()]
param(
    [string]$Check,
    [switch]$Fix,
    [switch]$Staged
)

$buildDir = Join-Path -Path $PSScriptRoot -ChildPath "..\build\dev"

if (-not (Test-Path "$buildDir/compile_commands.json")) {
    Write-Error "compile_commands.json not found in $buildDir. Run 'cmake --preset dev' first."
    exit 1
}

$buildDir = Resolve-Path $buildDir

if (-not (Get-Command clazy-standalone -ErrorAction SilentlyContinue)) {
    Write-Error "clazy-standalone not found in PATH."
    exit 1
}

$checks = if ($Check) { $Check } else { "level1" }
# -Wno-error keeps project -Werror flags from aborting a TU before clazy can
# report all findings (e.g., Qt 6.4 QT_REQUIRE_VERSION trips -Wdeprecated-declarations).
$clazyArgs = @("-p", $buildDir, "--checks=$checks", "--header-filter=.*[/\\]src[/\\].*", "--ignore-dirs=.*[/\\]src[/\\]contrib[/\\].*", "--extra-arg=-Wno-error")

$fixesDir = $null
if ($Fix) {
    if (-not (Get-Command clang-apply-replacements -ErrorAction SilentlyContinue)) {
        Write-Error "clang-apply-replacements not found in PATH (required for -Fix)."
        exit 1
    }
    $fixesDir = Join-Path ([System.IO.Path]::GetTempPath()) "zeal-clazy-fixes-$PID"
    New-Item -ItemType Directory -Path $fixesDir | Out-Null
}

$srcDir = Join-Path -Path $PSScriptRoot -ChildPath "..\src"

# Only process files present in compile_commands.json to skip platform-specific files.
$compileDb = Get-Content "$buildDir/compile_commands.json" | ConvertFrom-Json
$dbFiles = $compileDb | ForEach-Object { [System.IO.Path]::GetFullPath($_.file) }

if ($Staged) {
    $files = git diff --cached --name-only --diff-filter=ACMR -- "*.cpp" |
        ForEach-Object { Join-Path -Path $PSScriptRoot -ChildPath ".." -AdditionalChildPath $_ } |
        Where-Object { Test-Path $_ } |
        ForEach-Object { [System.IO.Path]::GetFullPath($_) } |
        Where-Object { $_ -in $dbFiles }
} else {
    $excludeDir = Join-Path -Path $srcDir -ChildPath "contrib"
    $files = Get-ChildItem -Recurse $srcDir -Include *.cpp |
        Where-Object { -not $_.FullName.StartsWith($excludeDir) -and $_.FullName -in $dbFiles } |
        Select-Object -ExpandProperty FullName
}

if (-not $files) {
    Write-Information "No files to process."
    exit 0
}

$failed = $false
try {
    $total = @($files).Count
    $i = 0
    foreach ($file in $files) {
        $i++
        Write-Verbose "[$i/$total] Processing $file"
        $perFileArgs = $clazyArgs
        if ($Fix) {
            $perFileArgs = $perFileArgs + @("--export-fixes=$fixesDir/$i.yaml")
        }
        & clazy-standalone @perFileArgs $file
        if ($LASTEXITCODE -ne 0) {
            $failed = $true
        }
    }

    if ($Fix) {
        & clang-apply-replacements $fixesDir
        if ($LASTEXITCODE -ne 0) {
            $failed = $true
        }
    }
} finally {
    if ($fixesDir -and (Test-Path $fixesDir)) {
        Remove-Item -Recurse -Force $fixesDir
    }
}

if ($failed) {
    exit 1
}
