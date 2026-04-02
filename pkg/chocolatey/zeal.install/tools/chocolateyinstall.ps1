# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT

$ErrorActionPreference = 'Stop'

$packageArgs = @{
    PackageName    = 'zeal.install'
    FileType       = 'msi'
    SilentArgs     = '/quiet /norestart'
    ValidExitCodes = @(0, 3010, 1641)

    Url64bit       = 'https://github.com/zealdocs/zeal/releases/download/v0.0.0/zeal-0.0.0-windows-x64.msi'
    Checksum64     = '0000000000000000000000000000000000000000000000000000000000000000'
    ChecksumType64 = 'sha256'
}

Install-ChocolateyPackage @packageArgs
