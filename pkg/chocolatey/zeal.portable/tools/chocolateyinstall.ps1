# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT

$ErrorActionPreference = 'Stop'

$packageArgs = @{
    PackageName    = 'zeal.portable'
    UnzipLocation  = "$(Split-Path -Parent $MyInvocation.MyCommand.Definition)"

    Url64bit       = 'https://github.com/zealdocs/zeal/releases/download/v0.0.0/zeal-0.0.0-portable-windows-x64.7z'
    Checksum64     = '0000000000000000000000000000000000000000000000000000000000000000'
    ChecksumType64 = 'sha256'
}

Install-ChocolateyZipPackage @packageArgs
