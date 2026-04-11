#
# run-pwsh-analyzer.ps1 - Check or fix PowerShell script issues using PSScriptAnalyzer
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#
# Prerequisites:
#   Install-Module -Name PSScriptAnalyzer -Scope CurrentUser
#
# By default, runs in check mode.
# Use -Fix to apply automatic formatting fixes.
# Use -Staged to only process files staged for commit.
#

[CmdletBinding()]
param(
    [switch]$Fix,
    [switch]$Staged
)

if (-not (Get-Module -ListAvailable -Name PSScriptAnalyzer)) {
    Write-Error "PSScriptAnalyzer is not installed. Run: Install-Module -Name PSScriptAnalyzer -Scope CurrentUser"
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

if ($Staged) {
    $files = git diff --cached --name-only --diff-filter=ACMR -- "*.ps1" |
        ForEach-Object { Join-Path $repoRoot $_ } |
        Where-Object { Test-Path $_ }
} else {
    $files = Get-ChildItem -Recurse $repoRoot -Include *.ps1 -File |
        Where-Object { $_.FullName -notmatch '\\(build|\.git)\\' } |
        Select-Object -ExpandProperty FullName
}

if (-not $files) {
    Write-Information "No files to analyze."
    exit 0
}

$failed = $false
foreach ($file in $files) {
    Write-Verbose "Processing $file"

    if ($Fix) {
        $original = Get-Content -Raw $file
        $formatted = Invoke-Formatter -ScriptDefinition $original -Settings CodeFormattingOTBS
        if ($original -ne $formatted) {
            Set-Content -Path $file -Value $formatted -NoNewline
            Write-Output "Formatted: $file"
        }
    }

    $results = Invoke-ScriptAnalyzer -Path $file
    if ($results) {
        $failed = $true
        foreach ($r in $results) {
            Write-Output "$($r.ScriptName):$($r.Line):$($r.Column) [$($r.Severity)] $($r.RuleName): $($r.Message)"
        }
    }
}

if ($failed) {
    exit 1
}
