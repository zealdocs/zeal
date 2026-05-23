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

# Strip CMake PCH flags from compile_commands.json before invoking clang-tidy.
# CMake injects MSVC-style /Yu, /Yc, /Fp, and /FI cmake_pch.h flags, but the
# resulting .pch is in MSVC's binary format which Clang cannot read, causing
# "file doesn't start with precompiled file magic" errors. Write a filtered
# copy of the database to a temp dir and point clang-tidy at it.
$tempDbDir = Join-Path ([System.IO.Path]::GetTempPath()) "zeal-clang-tidy-db-$PID"
New-Item -ItemType Directory -Path $tempDbDir -Force | Out-Null

$compileDb = Get-Content "$buildDir/compile_commands.json" | ConvertFrom-Json
foreach ($entry in $compileDb) {
    $cmd = $entry.command
    # MSVC / clang-cl style.
    $cmd = $cmd -replace '\s+/Yu\S*', ''
    $cmd = $cmd -replace '\s+/Yc\S*', ''
    $cmd = $cmd -replace '\s+/Fp\S*', ''
    $cmd = $cmd -replace '\s+/FI\S*cmake_pch\S*', ''
    # GCC / Clang style.
    $cmd = $cmd -replace '\s+-include\s+\S*cmake_pch\S*', ''
    $cmd = $cmd -replace '\s+-include-pch\s+\S*\.pch', ''
    $entry.command = $cmd
}
$compileDb | ConvertTo-Json -Depth 100 | Set-Content -Path (Join-Path $tempDbDir "compile_commands.json")

$tidyArgs = @("-p", $tempDbDir)
if ($Check) {
    if ($Check -match '^clang-diagnostic-(.+)$') {
        # clang-diagnostic-* checks can't be enabled via Checks; use a no-op check
        # as a placeholder and enable the compiler warning via -extra-arg.
        $tidyArgs += "--checks=-*,android-cloexec-accept"
        $tidyArgs += "-extra-arg=-W$($Matches[1])"
    } else {
        $tidyArgs += "--checks=-*,$Check"
    }
}
if ($Fix) {
    $tidyArgs += "--fix"
}

$srcDir = Join-Path -Path $PSScriptRoot -ChildPath "..\src"

# Only process files present in compile_commands.json to skip platform-specific files.
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
    Remove-Item -Recurse -Force $tempDbDir
    exit 0
}

# Tally warnings by check name as clang-tidy output streams through.
# Each warning/error line ends with "[check-name]" or "[check1,check2,...]".
$counts = @{}

try {
    $total = @($files).Count
    $i = 0
    foreach ($file in $files) {
        $i++
        Write-Verbose "[$i/$total] Processing $file"
        & clang-tidy @tidyArgs --quiet $file 2>&1 | ForEach-Object {
            $line = "$_"
            $line
            if ($line -match '\[([a-zA-Z0-9.,_-]+)\]\s*$') {
                foreach ($check in $Matches[1] -split ',') {
                    $check = $check.Trim()
                    if ($counts.ContainsKey($check)) {
                        $counts[$check]++
                    } else {
                        $counts[$check] = 1
                    }
                }
            }
        }
        if ($LASTEXITCODE -ne 0) {
            exit 1
        }
    }
} finally {
    Remove-Item -Recurse -Force $tempDbDir
    if ($counts.Count -gt 0) {
        ""
        "Warning summary:"
        $counts.GetEnumerator() | Sort-Object Value -Descending | ForEach-Object {
            "  {0,6}  {1}" -f $_.Value, $_.Key
        }
        $total = ($counts.Values | Measure-Object -Sum).Sum
        "  ------"
        "  {0,6}  total" -f $total
    }
}
