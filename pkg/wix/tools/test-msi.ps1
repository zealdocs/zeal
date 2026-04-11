#
# test-msi.ps1 - Validate a Zeal MSI package
#
# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT
#
# Inspects the MSI database to verify product metadata, custom actions,
# and registry entries without needing to install the package.
#
# Usage:
#   ./test-msi.ps1                          # finds latest zeal-*.msi under build/
#   ./test-msi.ps1 -MsiPath <path>          # validate a specific MSI
#   ./test-msi.ps1 -Install                 # also run an install/uninstall smoke test
#

[CmdletBinding()]
param(
    [string]$MsiPath,
    [switch]$Install
)

$ErrorActionPreference = "Stop"

if ($Install) {
    $isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)
    if (-not $isAdmin) {
        Write-Output "Install test requires admin privileges. Relaunching with UAC prompt..."
        $transcript = Join-Path $env:TEMP "zeal-msi-test-transcript.log"
        if (Test-Path $transcript) { Remove-Item $transcript }
        $inner = "Start-Transcript -Path '$transcript' | Out-Null; & '$PSCommandPath' -Install"
        if ($MsiPath) { $inner += " -MsiPath '$((Resolve-Path $MsiPath).Path)'" }
        $inner += "; Stop-Transcript | Out-Null; exit `$LASTEXITCODE"
        $proc = Start-Process pwsh.exe -ArgumentList @("-NoProfile", "-Command", $inner) -Verb RunAs -Wait -PassThru
        if (Test-Path $transcript) {
            Get-Content $transcript | Where-Object { $_ -notmatch '^\*{10,}|^Transcript|^PSVersion|^PSEdition|^WSMan|^BuildVersion|^CLRVersion|^PSCompat|^PSRemot|^SerializationVersion|^Username|^RunAs|^Configuration|^Machine|^Host Application|^Process ID|^Start time|^End time' }
        }
        exit $proc.ExitCode
    }
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")

if (-not $MsiPath) {
    $candidate = Get-ChildItem -Path (Join-Path $repoRoot "build") -Recurse -Filter "zeal-*.msi" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if (-not $candidate) {
        Write-Error "No zeal-*.msi found under build/. Build the package target first."
    }
    $MsiPath = $candidate.FullName
}

if (-not (Test-Path $MsiPath)) {
    Write-Error "MSI not found: $MsiPath"
}

$MsiPath = (Resolve-Path $MsiPath).Path

Write-Output "Validating: $MsiPath"

$installer = New-Object -ComObject WindowsInstaller.Installer
$database = $installer.GetType().InvokeMember(
    "OpenDatabase", "InvokeMethod", $null, $installer, @($MsiPath, 0))

function Invoke-MsiQuery {
    param([string]$Sql)
    $view = $database.GetType().InvokeMember(
        "OpenView", "InvokeMethod", $null, $database, @($Sql))
    [void]$view.GetType().InvokeMember("Execute", "InvokeMethod", $null, $view, $null)
    $rows = New-Object System.Collections.Generic.List[object]
    while ($true) {
        $record = $view.GetType().InvokeMember("Fetch", "InvokeMethod", $null, $view, $null)
        if (-not $record) { break }
        $fieldCount = $record.GetType().InvokeMember("FieldCount", "GetProperty", $null, $record, $null)
        $row = New-Object System.Collections.Generic.List[string]
        for ($i = 1; $i -le $fieldCount; $i++) {
            $row.Add($record.GetType().InvokeMember("StringData", "GetProperty", $null, $record, @($i)))
        }
        $rows.Add($row)
    }
    [void]$view.GetType().InvokeMember("Close", "InvokeMethod", $null, $view, $null)
    Write-Output $rows -NoEnumerate
}

function Get-MsiProperty {
    param([string]$Name)
    $rows = Invoke-MsiQuery "SELECT Value FROM Property WHERE Property = '$Name'"
    if ($rows.Count -eq 0) { return $null }
    return $rows[0][0]
}

$errors = @()

function Assert-Equal {
    param($Actual, $Expected, [string]$Label)
    if ($Actual -eq $Expected) {
        Write-Output "  OK   $Label = $Actual"
    } else {
        Write-Output "  FAIL ${Label}: expected '$Expected', got '$Actual'"
        $script:errors += $Label
    }
}

function Assert-NotNull {
    param($Value, [string]$Label)
    if ($null -ne $Value -and $Value -ne "") {
        Write-Output "  OK   $Label = $Value"
    } else {
        Write-Output "  FAIL $Label is missing"
        $script:errors += $Label
    }
}

Write-Output "`n[Product metadata]"
$productName = Get-MsiProperty "ProductName"
$productVersion = Get-MsiProperty "ProductVersion"
$manufacturer = Get-MsiProperty "Manufacturer"
$upgradeCode = Get-MsiProperty "UpgradeCode"
$productCode = Get-MsiProperty "ProductCode"

Assert-Equal -Actual $productName -Expected "Zeal" -Label "ProductName"
Assert-NotNull -Value $productVersion -Label "ProductVersion"
Assert-NotNull -Value $manufacturer -Label "Manufacturer"
Assert-Equal -Actual $upgradeCode -Expected "{5C4B6030-A1B4-4EFE-A5AF-28F6FA2E7978}" -Label "UpgradeCode"
Assert-NotNull -Value $productCode -Label "ProductCode"

Write-Output "`n[File table]"
try {
    $fileRows = Invoke-MsiQuery "SELECT File FROM File"
    $expectedMinFiles = 20
    if ($fileRows.Count -ge $expectedMinFiles) {
        Write-Output "  OK   $($fileRows.Count) file(s) in package"
    } else {
        Write-Output "  FAIL Only $($fileRows.Count) file(s) in package (expected at least $expectedMinFiles)"
        $errors += "file count"
    }
} catch {
    $msg = if ($_.Exception.InnerException) { $_.Exception.InnerException.Message } else { $_.Exception.Message }
    Write-Output "  FAIL File table query failed ($msg)"
    $errors += "File table"
}

Write-Output "`n[Install sequence]"
$allActions = Invoke-MsiQuery "SELECT Action, Sequence FROM InstallExecuteSequence"
$closeActionRows = @($allActions | Where-Object { $_[0] -like 'Wix4CloseApplications*' })
$removeFilesRows = Invoke-MsiQuery "SELECT Sequence FROM InstallExecuteSequence WHERE Action = 'RemoveFiles'"

if ($closeActionRows.Count -eq 0) {
    Write-Output "  FAIL Wix4CloseApplications action not scheduled"
    $errors += "CloseApplications scheduling"
} elseif ($removeFilesRows.Count -eq 0) {
    Write-Output "  FAIL RemoveFiles action not found"
    $errors += "RemoveFiles scheduling"
} else {
    $closeAction = $closeActionRows[0][0]
    $closeSeq = [int]$closeActionRows[0][1]
    $removeSeq = [int]$removeFilesRows[0][0]
    if ($closeSeq -lt $removeSeq) {
        Write-Output "  OK   $closeAction ($closeSeq) scheduled before RemoveFiles ($removeSeq)"
    } else {
        Write-Output "  FAIL $closeAction ($closeSeq) must run before RemoveFiles ($removeSeq)"
        $errors += "CloseApplications order"
    }
}

Write-Output "`n[Protocol handlers]"
foreach ($scheme in @("dash", "dash-plugin")) {
    try {
        $rows = Invoke-MsiQuery "SELECT ``Key`` FROM ``Registry`` WHERE ``Key`` = 'Software\Classes\$scheme'"
        if ($rows.Count -gt 0) {
            Write-Output "  OK   $scheme protocol handler registered"
        } else {
            Write-Output "  FAIL $scheme protocol handler missing"
            $errors += "$scheme handler"
        }
    } catch {
        $msg = if ($_.Exception.InnerException) { $_.Exception.InnerException.Message } else { $_.Exception.Message }
        Write-Output "  FAIL $scheme protocol handler: Registry table query failed ($msg)"
        $errors += "$scheme handler"
    }
}

Write-Output "`n[Launch action]"
$launchRows = Invoke-MsiQuery "SELECT Action FROM CustomAction WHERE Action = 'LaunchApplication'"
if ($launchRows.Count -gt 0) {
    Write-Output "  OK   LaunchApplication custom action present"
} else {
    Write-Output "  FAIL LaunchApplication custom action missing"
    $errors += "LaunchApplication"
}

if ($Install) {
    Write-Output "`n[Install smoke test]"
    $logFile = Join-Path $env:TEMP "zeal-msi-install.log"
    $installDir = Join-Path ${env:ProgramFiles} "Zeal"
    $expectedExe = Join-Path $installDir "zeal.exe"
    $uninstallKey = "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\$productCode"

    Write-Output "  Installing silently (log: $logFile)..."
    $proc = Start-Process msiexec.exe -ArgumentList "/i", "`"$MsiPath`"", "/quiet", "/l*v", "`"$logFile`"" -Wait -PassThru
    if ($proc.ExitCode -ne 0) {
        Write-Output "  FAIL Install exited with code $($proc.ExitCode)"
        $errors += "Install"
    } else {
        Write-Output "  OK   Install succeeded"

        Write-Output "`n  [Post-install verification]"

        foreach ($file in @("zeal.exe", "archive.dll", "sqlite3.dll", "zlib1.dll")) {
            $path = Join-Path $installDir $file
            if (Test-Path $path) {
                Write-Output "    OK   $file present"
            } else {
                Write-Output "    FAIL $file missing at $path"
                $errors += "file:$file"
            }
        }

        if (Test-Path $uninstallKey) {
            Write-Output "    OK   Uninstall registry entry present"
            $displayName = (Get-ItemProperty $uninstallKey -Name DisplayName -ErrorAction SilentlyContinue).DisplayName
            if ($displayName -eq "Zeal") {
                Write-Output "    OK   Uninstall DisplayName = $displayName"
            } else {
                Write-Output "    FAIL Uninstall DisplayName: expected 'Zeal', got '$displayName'"
                $errors += "uninstall:DisplayName"
            }
        } else {
            Write-Output "    FAIL Uninstall registry entry missing"
            $errors += "uninstall:entry"
        }

        foreach ($scheme in @("dash", "dash-plugin")) {
            $cmdKey = "HKLM:\Software\Classes\$scheme\shell\open\command"
            if (Test-Path $cmdKey) {
                $cmd = (Get-ItemProperty $cmdKey).'(default)'
                $expectedCmd = "`"$expectedExe`" `"%1`""
                if ($cmd -eq $expectedCmd) {
                    Write-Output "    OK   $scheme handler points to $expectedExe"
                } else {
                    Write-Output "    FAIL $scheme handler: expected '$expectedCmd', got '$cmd'"
                    $errors += "handler:$scheme"
                }
            } else {
                Write-Output "    FAIL $scheme handler registry key missing"
                $errors += "handler:$scheme"
            }
        }

        Write-Output "`n  Uninstalling silently..."
        $proc = Start-Process msiexec.exe -ArgumentList "/x", $productCode, "/quiet", "/l*v", "`"$logFile`"" -Wait -PassThru
        if ($proc.ExitCode -ne 0) {
            Write-Output "  FAIL Uninstall exited with code $($proc.ExitCode)"
            $errors += "Uninstall"
        } else {
            Write-Output "  OK   Uninstall succeeded"

            Write-Output "`n  [Post-uninstall verification]"

            if (Test-Path $expectedExe) {
                Write-Output "    FAIL zeal.exe still present at $expectedExe"
                $errors += "cleanup:zeal.exe"
            } else {
                Write-Output "    OK   zeal.exe removed"
            }

            if (Test-Path $installDir) {
                Write-Output "    FAIL Install directory still present at $installDir"
                $errors += "cleanup:installDir"
            } else {
                Write-Output "    OK   Install directory removed"
            }

            if (Test-Path $uninstallKey) {
                Write-Output "    FAIL Uninstall registry entry still present"
                $errors += "cleanup:uninstall"
            } else {
                Write-Output "    OK   Uninstall registry entry removed"
            }

            foreach ($scheme in @("dash", "dash-plugin")) {
                if (Test-Path "HKLM:\Software\Classes\$scheme") {
                    Write-Output "    FAIL $scheme handler still present"
                    $errors += "cleanup:$scheme"
                } else {
                    Write-Output "    OK   $scheme handler removed"
                }
            }
        }
    }
}

[System.Runtime.InteropServices.Marshal]::ReleaseComObject($database) | Out-Null
[System.Runtime.InteropServices.Marshal]::ReleaseComObject($installer) | Out-Null

Write-Output ""
if ($errors.Count -gt 0) {
    Write-Output "FAILED ($($errors.Count) check(s) failed)"
    exit 1
}

Write-Output "PASSED"
