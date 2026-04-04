#
# run-include-cleaner.ps1 - Find unused #include directives in the codebase
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#
# By default, only reports unused includes (removals).
# Use -Insert to also report missing includes.
# Use -Fix to apply suggested changes.
# Use -Staged to only process files staged for commit.
# Use -Verbose to print each file being processed.
#
# Requires a compile_commands.json in the build directory (build/dev).
#

[CmdletBinding()]
param(
    [switch]$Insert,
    [switch]$Fix,
    [switch]$Staged
)

$buildDir = Join-Path -Path $PSScriptRoot -ChildPath "..\build\dev"

if (-not (Test-Path "$buildDir/compile_commands.json")) {
    Write-Error "compile_commands.json not found in $buildDir. Run 'cmake --preset dev' first."
    exit 1
}

$buildDir = Resolve-Path $buildDir

$toolArgs = @("-p", $buildDir)
if (-not $Insert) {
    $toolArgs += "--disable-insert"
}
if ($Fix) {
    $toolArgs += "--edit"
} else {
    $toolArgs += "--print=changes"
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

foreach ($file in $files) {
    Write-Verbose "Processing $file"
    $output = & clang-include-cleaner @toolArgs $file 2>&1
    if ($output) {
        Write-Output "${file}:"
        Write-Output $output
        Write-Output ""
    }
}
