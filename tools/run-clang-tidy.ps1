#
# run-clang-tidy.ps1 - Run clang-tidy static analysis on the codebase
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#
# By default, uses the project's .clang-tidy configuration.
# Use -Check <name> to run a specific check (e.g., -Check readability-qualified-auto).
# Also supports clang-diagnostic-* checks (e.g., -Check clang-diagnostic-shadow).
#
# Check lists:
#   clang-tidy checks: https://clang.llvm.org/extra/clang-tidy/checks/list.html
#   clang diagnostics: https://clang.llvm.org/docs/DiagnosticsReference.html
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
    if ($Check -match '^clang-diagnostic-(.+)$') {
        # clang-diagnostic-* checks can't be enabled via Checks; use a no-op check
        # as a placeholder and enable the compiler warning via -extra-arg.
        $tidyArgs += "--config={Checks: '-*,android-cloexec-accept', HeaderFilterRegex: 'src/.*'}"
        $tidyArgs += "-extra-arg=-W$($Matches[1])"
    } else {
        $tidyArgs += "--config={Checks: '-*,$Check', HeaderFilterRegex: 'src/.*'}"
    }
}
if ($Fix) {
    $tidyArgs += "--fix"
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
    & clang-tidy @tidyArgs $file
    if ($LASTEXITCODE -ne 0) {
        exit 1
    }
}
