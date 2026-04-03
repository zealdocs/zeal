#
# run-clang-format.ps1 - Check or fix code formatting using clang-format
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#
# By default, runs in check mode (dry-run).
# Use -Fix to apply changes.
# Use -Staged to only process files staged for commit.
# Use -Verbose to print each file being processed.
#

[CmdletBinding()]
param(
    [switch]$Fix,
    [switch]$Staged
)

$srcDir = Join-Path -Path $PSScriptRoot -ChildPath "..\src"

if ($Staged) {
    $files = git diff --cached --name-only --diff-filter=ACMR -- "*.cpp" "*.h" |
        ForEach-Object { Join-Path -Path $PSScriptRoot -ChildPath ".." -AdditionalChildPath $_ } |
        Where-Object { Test-Path $_ }
} else {
    $files = Get-ChildItem -Recurse $srcDir -Include *.cpp, *.h |
        Select-Object -ExpandProperty FullName
}

if (-not $files) {
    Write-Information "No files to format."
    exit 0
}

$formatArgs = @("--dry-run", "--Werror")
if ($Fix) {
    $formatArgs = @("-i")
}

$failed = $false
foreach ($file in $files) {
    Write-Verbose "Processing $file"
    & clang-format @formatArgs $file
    if ($LASTEXITCODE -ne 0) {
        $failed = $true
    }
}

if ($failed) {
    exit 1
}
