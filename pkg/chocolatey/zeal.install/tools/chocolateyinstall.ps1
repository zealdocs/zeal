# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT

$ErrorActionPreference = 'Stop'

$packageArgs = @{
    PackageName    = 'zeal.install'
    FileType       = 'msi'
    SilentArgs     = '/quiet /norestart'
    ValidExitCodes = @(0, 3010, 1641)

    Url64bit       = 'https://github.com/zealdocs/zeal/releases/download/v0.8.0/zeal-0.8.0-windows-x64.msi'
    Checksum64     = 'ef52de58863c7a9817cbea826c3d656f4b88082268465068cd3c2af55446a74e'
    ChecksumType64 = 'sha256'
}

Install-ChocolateyPackage @packageArgs
