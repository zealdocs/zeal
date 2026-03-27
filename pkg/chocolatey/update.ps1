# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT

param(
    [string]$Version,
    [switch]$Pack
)

$ErrorActionPreference = 'Stop'
$InformationPreference = 'Continue'
$root = $PSScriptRoot

if (-not $Version -and -not $Pack) {
    throw "Provide -Version to update packages, -Pack to build .nupkg files, or both."
}

if ($Version) {
    $baseUrl = "https://github.com/zealdocs/zeal/releases/download/v$Version"
    $msiUrl = "$baseUrl/zeal-$Version-windows-x64.msi"
    $portableUrl = "$baseUrl/zeal-$Version-portable-windows-x64.7z"

    $tempDir = Join-Path ([System.IO.Path]::GetTempPath()) "zeal-update-$Version"
    New-Item -ItemType Directory -Path $tempDir -Force | Out-Null

    try {
        Write-Information "Downloading MSI..."
        $msiPath = Join-Path $tempDir "zeal.msi"
        Invoke-WebRequest -Uri $msiUrl -OutFile $msiPath

        Write-Information "Downloading portable archive..."
        $portablePath = Join-Path $tempDir "zeal.7z"
        Invoke-WebRequest -Uri $portableUrl -OutFile $portablePath

        $msiChecksum = (Get-FileHash -Path $msiPath -Algorithm SHA256).Hash.ToLower()
        $portableChecksum = (Get-FileHash -Path $portablePath -Algorithm SHA256).Hash.ToLower()

        Write-Information "MSI checksum:      $msiChecksum"
        Write-Information "Portable checksum: $portableChecksum"

        Write-Information "Verifying checksums against release digests..."
        $msiDigest = ((Invoke-WebRequest -Uri "$msiUrl.sha256").ToString().Trim() -split '\s+')[0]
        $portableDigest = ((Invoke-WebRequest -Uri "$portableUrl.sha256").ToString().Trim() -split '\s+')[0]

        if ($msiChecksum -ne $msiDigest) {
            throw "MSI checksum mismatch! Expected: $msiDigest, Got: $msiChecksum"
        }
        if ($portableChecksum -ne $portableDigest) {
            throw "Portable checksum mismatch! Expected: $portableDigest, Got: $portableChecksum"
        }
        Write-Information "Checksums verified."
    }
    finally {
        Remove-Item -Path $tempDir -Recurse -Force -ErrorAction SilentlyContinue
    }

    $year = (Get-Date).Year
    $copyrightPattern = '2015-\d{4}'
    $copyrightReplace = "2015-$year"

    $files = @(
        'zeal\zeal.nuspec'
        'zeal.install\zeal.install.nuspec'
        'zeal.portable\zeal.portable.nuspec'
        'zeal.install\tools\chocolateyinstall.ps1'
        'zeal.portable\tools\chocolateyinstall.ps1'
    )

    foreach ($file in $files) {
        $path = Join-Path $root $file
        $content = Get-Content $path -Raw
        $content = $content -replace '\d+\.\d+\.\d+', $Version
        $content = $content -replace $copyrightPattern, $copyrightReplace
        Set-Content $path $content -NoNewline
        Write-Information "Updated $file"
    }

    $path = Join-Path $root 'zeal.install\tools\chocolateyinstall.ps1'
    $content = Get-Content $path -Raw
    if ($content -notmatch '(?<=Checksum64\s+=\s+'')[a-f0-9]{64}') { throw "Checksum pattern not found in $path" }
    $content = $content -replace '(?<=Checksum64\s+=\s+'')[a-f0-9]{64}', $msiChecksum
    Set-Content $path $content -NoNewline
    Write-Information "Updated MSI checksum"

    $path = Join-Path $root 'zeal.portable\tools\chocolateyinstall.ps1'
    $content = Get-Content $path -Raw
    if ($content -notmatch '(?<=Checksum64\s+=\s+'')[a-f0-9]{64}') { throw "Checksum pattern not found in $path" }
    $content = $content -replace '(?<=Checksum64\s+=\s+'')[a-f0-9]{64}', $portableChecksum
    Set-Content $path $content -NoNewline
    Write-Information "Updated portable checksum"

    Write-Information "`nDone. Updated to v$Version."
}

if ($Pack) {
    Write-Information "`nPacking..."
    Push-Location $root
    try {
        choco pack zeal.install/zeal.install.nuspec
        if ($LASTEXITCODE -ne 0) { throw "choco pack failed for zeal.install" }
        choco pack zeal.portable/zeal.portable.nuspec
        if ($LASTEXITCODE -ne 0) { throw "choco pack failed for zeal.portable" }
        choco pack zeal/zeal.nuspec
        if ($LASTEXITCODE -ne 0) { throw "choco pack failed for zeal" }
    }
    finally {
        Pop-Location
    }
}
