#
# run-clang-tidy.ps1 - Run clang-tidy static analysis on the codebase
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#
# By default, uses the project's .clang-tidy configuration.
# Use -Check <name> to run a specific check (e.g., -Check readability-qualified-auto).
# Use -Fix to apply suggested fixes.
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

$tidyArgs = @("-p", $buildDir)
if ($Check) {
    $tidyArgs += "--config={Checks: '-*,$Check', HeaderFilterRegex: 'src/.*'}"
}
if ($Fix) {
    $tidyArgs += "--fix"
}

$srcDir = Join-Path -Path $PSScriptRoot -ChildPath "..\src"

if ($Staged) {
    $files = git diff --cached --name-only --diff-filter=ACMR -- "*.cpp" |
        ForEach-Object { Join-Path -Path $PSScriptRoot -ChildPath ".." -AdditionalChildPath $_ } |
        Where-Object { Test-Path $_ }
} else {
    $excludeDir = Join-Path -Path $srcDir -ChildPath "contrib"
    $files = Get-ChildItem -Recurse $srcDir -Include *.cpp |
        Where-Object { -not $_.FullName.StartsWith($excludeDir) } |
        Select-Object -ExpandProperty FullName
}

if (-not $files) {
    Write-Information "No files to process."
    exit 0
}

$failed = $false
foreach ($file in $files) {
    Write-Verbose "Processing $file"
    & clang-tidy @tidyArgs $file
    if ($LASTEXITCODE -ne 0) {
        $failed = $true
    }
}

if ($failed) {
    exit 1
}
