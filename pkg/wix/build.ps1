# TODO: This script should be replaced with wixproj and/or CPack.
# TODO: Add Debug mode to override CompressionLevel with "none".

# Sample command:
# .\build.ps1 -Arch x64 -Version 0.4.0 -PackagePath "c:\tmp\zeal-0.4.0-windows-x64" -SignMsi

param(
    [Parameter(Mandatory=$True)]
    [ValidatePattern("[0-9].[0-9].[0-9]")]
    [string]$Version,

    [Parameter(Mandatory=$True)]
    [ValidateSet('x86', 'x64')]
    [string]$Arch,

    [Parameter(Mandatory=$True)]
    [string]$PackagePath,

    [Switch]$SignMsi
)

$BaseFilename = "zeal-$Version-windows-$Arch"
$WixobjFilename = "$BaseFilename.wixobj"
$MsiFilename = "$BaseFilename.msi"
function CleanUp {
    Remove-Item -ErrorAction Ignore -Path "$WixobjFilename"

    # We are not going to build patches for now.
    # More infor about wixpdb: https://www.firegiant.com/wix/tutorial/upgrades-and-modularization/patchwork/
    Remove-Item -ErrorAction Ignore "$BaseFilename.wixpdb"
}

Write-Output "Building $MsiFilename..."

Write-Output "Running candle..."
& candle.exe -nologo -pedantic -wx -arch "$Arch" -dAppVersion="$Version" -dAppPackageDir="$PackagePath" -o "$WixobjFilename" zeal.wxs
if ($LastExitCode -ne 0) {
    CleanUp
    throw "candle failed with exit code $LastExitCode."
}


Write-Output "Running light..."
& light.exe -nologo -pedantic -wx -ext WixUIExtension -o "$MsiFilename" "$WixobjFilename"
if ($LastExitCode -ne 0) {
    CleanUp
    throw "light failed with exit code $LastExitCode."
}

if ($SignMsi) {
    $KeyName = "Open Source Developer, Oleg Shparber"
    $Description = "Zeal $Version Installer"
    $Url = "https://zealdocs.org"
    $TimestampServerUrl = "http://timestamp.comodoca.com/authenticode"

    Write-Output "Signing $MsiFilename..."
    & signtool sign /v /n "$KeyName" /d "$Description" /du "$Url" /t "$TimestampServerUrl" "$MsiFilename"
    if ($LastExitCode -ne 0) {
        CleanUp
        throw "signtool failed with exit code $LastExitCode."
    }
}

CleanUp
