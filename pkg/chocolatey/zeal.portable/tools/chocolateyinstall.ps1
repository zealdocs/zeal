# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT

$ErrorActionPreference = 'Stop'

$packageArgs = @{
    PackageName    = 'zeal.portable'
    UnzipLocation  = "$(Split-Path -Parent $MyInvocation.MyCommand.Definition)"

    Url64bit       = 'https://github.com/zealdocs/zeal/releases/download/v0.8.0/zeal-0.8.0-portable-windows-x64.7z'
    Checksum64     = '2d5a2049159984401b8bdf12f982b68de1a2740a955e86d01c4ab3c8936d5b67'
    ChecksumType64 = 'sha256'
}

Install-ChocolateyZipPackage @packageArgs
