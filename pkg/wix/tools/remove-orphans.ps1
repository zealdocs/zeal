#
# remove-orphans.ps1 - Remove orphaned Zeal installer state
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#
# Removes orphaned Windows Installer entries, component references, ARP entries,
# installed files, and protocol handler registry keys left behind by broken
# install/uninstall cycles. Does not invoke msiexec.
#
# Dry-run by default. Use -Force to actually delete.
#
# Usage:
#   ./remove-orphans.ps1           # dry-run
#   ./remove-orphans.ps1 -Force    # actually clean up
#

[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = "Stop"

if ($Force) {
    $isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)
    if (-not $isAdmin) {
        Write-Output "Cleanup requires admin privileges. Relaunching with UAC prompt..."
        $transcript = Join-Path $env:TEMP "zeal-remove-orphans-transcript.log"
        if (Test-Path $transcript) { Remove-Item $transcript }
        $inner = "Start-Transcript -Path '$transcript' | Out-Null; & '$PSCommandPath' -Force; Stop-Transcript | Out-Null; exit `$LASTEXITCODE"
        $proc = Start-Process pwsh.exe -ArgumentList @("-NoProfile", "-Command", $inner) -Verb RunAs -Wait -PassThru
        if (Test-Path $transcript) {
            Get-Content $transcript | Where-Object { $_ -notmatch '^\*{10,}|^Transcript|^PSVersion|^PSEdition|^WSMan|^BuildVersion|^CLRVersion|^PSCompat|^PSRemot|^SerializationVersion|^Username|^RunAs|^Configuration|^Machine|^Host Application|^Process ID|^Start time|^End time' }
        }
        exit $proc.ExitCode
    }
}

function ConvertTo-SquishedGuid {
    param([string]$Guid)
    $g = $Guid -replace '[{}-]', ''
    $part1 = -join ($g[7..0])
    $part2 = -join ($g[11..8])
    $part3 = -join ($g[15..12])
    $part4 = -join ($g[17..16]) + -join ($g[19..18])
    $part5 = -join ($g[21..20]) + -join ($g[23..22]) + -join ($g[25..24]) + -join ($g[27..26]) + -join ($g[29..28]) + -join ($g[31..30])
    return "$part1$part2$part3$part4$part5"
}

$mode = if ($Force) { "DELETE" } else { "DRY-RUN" }
Write-Output "Mode: $mode"
Write-Output ""

function Invoke-SafeRemoval {
    param([string]$Path, [string]$Label)
    if (Test-Path $Path) {
        if ($Force) {
            Remove-Item $Path -Recurse -Force
            Write-Output "  REMOVED $Label"
        } else {
            Write-Output "  WOULD REMOVE $Label"
        }
    }
}

# 1. Find all registered Zeal products
Write-Output "[1] Windows Installer product entries"
$products = Get-ChildItem "HKLM:\SOFTWARE\Classes\Installer\Products" -ErrorAction SilentlyContinue | Where-Object {
    (Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue).ProductName -eq "Zeal"
}
$productSquished = @($products | ForEach-Object { $_.PSChildName })
Write-Output "  Found $($products.Count) product(s): $($productSquished -join ', ')"
foreach ($p in $products) {
    Invoke-SafeRemoval $p.PSPath "product $($p.PSChildName)"
}

# 2. UserData\S-1-5-18\Products entries
Write-Output "`n[2] UserData product entries"
foreach ($squished in $productSquished) {
    $path = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Installer\UserData\S-1-5-18\Products\$squished"
    Invoke-SafeRemoval $path "UserData product $squished"
}

# 3. Component client references
Write-Output "`n[3] Component client references"
$componentsRoot = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Installer\UserData\S-1-5-18\Components"
$affectedComponents = 0
Get-ChildItem $componentsRoot -ErrorAction SilentlyContinue | ForEach-Object {
    $item = Get-Item $_.PSPath -ErrorAction SilentlyContinue
    $values = $item.Property
    $zealRefs = $values | Where-Object { $productSquished -contains $_ }
    if ($zealRefs) {
        $script:affectedComponents++
        foreach ($ref in $zealRefs) {
            if ($Force) {
                Remove-ItemProperty -Path $_.PSPath -Name $ref -ErrorAction SilentlyContinue
            }
        }
        # If no clients left after removal, delete the component key entirely
        $remaining = @($values | Where-Object { $productSquished -notcontains $_ })
        if ($remaining.Count -eq 0) {
            Invoke-SafeRemoval $_.PSPath "empty component $($_.PSChildName)"
        }
    }
}
if ($Force) {
    Write-Output "  REMOVED references from $affectedComponents component(s)"
} else {
    Write-Output "  WOULD REMOVE references from $affectedComponents component(s)"
}

# 4. ARP (Add/Remove Programs) entries
Write-Output "`n[4] ARP (uninstall) entries"
$arpEntries = Get-ChildItem "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall" -ErrorAction SilentlyContinue | Where-Object {
    (Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue).DisplayName -eq "Zeal"
}
Write-Output "  Found $($arpEntries.Count) entry/entries"
foreach ($e in $arpEntries) {
    Invoke-SafeRemoval $e.PSPath "ARP entry $($e.PSChildName)"
}

# 5. Upgrade code entry
Write-Output "`n[5] Upgrade code entry"
$upgradeCodeSquished = ConvertTo-SquishedGuid "{5C4B6030-A1B4-4EFE-A5AF-28F6FA2E7978}"
$upgradePath = "HKLM:\SOFTWARE\Classes\Installer\UpgradeCodes\$upgradeCodeSquished"
Invoke-SafeRemoval $upgradePath "upgrade code mapping ($upgradeCodeSquished)"

# 6. Installed files
Write-Output "`n[6] Installed files"
$installDir = Join-Path ${env:ProgramFiles} "Zeal"
Invoke-SafeRemoval $installDir "install directory"

# 7. Protocol handler registry keys
Write-Output "`n[7] Protocol handlers"
foreach ($scheme in @("dash", "dash-plugin")) {
    Invoke-SafeRemoval "HKLM:\SOFTWARE\Classes\$scheme" "HKLM protocol handler: $scheme"
    Invoke-SafeRemoval "HKCU:\SOFTWARE\Classes\$scheme" "HKCU protocol handler: $scheme"
}

Write-Output ""
if ($Force) {
    Write-Output "Cleanup complete."
} else {
    Write-Output "Dry-run complete. Re-run with -Force to actually delete."
}
