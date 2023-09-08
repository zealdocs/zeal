# AppImage Package

## Local Testing

Run Docker container:

```shell
docker run -it --rm -v $(pwd):/src --entrypoint /bin/bash ubuntu:jammy
```

Install `appimage-builder` and `appimagetool` dependencies:

```shell
apt-get update -q -y
DEBIAN_FRONTEND="noninteractive" apt-get install -q -y --no-install-recommends appstream curl desktop-file-utils fakeroot file git gnupg patchelf squashfs-tools zsync python3-pip python3-setuptools python3-wheel
```

Install appimagetool, it has to be extracted because FUSE doesn't work in containers without extra fiddling.

```shell
cd /tmp
curl -sLO https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x appimagetool-x86_64.AppImage
./appimagetool-x86_64.AppImage --appimage-extract
mv squashfs-root/ /opt/appimagetool.AppDir
ln -s /opt/appimagetool.AppDir/AppRun /usr/local/bin/appimagetool
cd -
```

Install appimage-builder.

```shell
pip3 install git+https://github.com/AppImageCrafters/appimage-builder.git@669213cb730e007d5b316ed19b39691fbdcd41c4
```

Install build dependencies:

```shell
apt-get install -q -y --no-install-recommends build-essential cmake extra-cmake-modules libappindicator-dev libarchive-dev libqt5x11extras5-dev libsqlite3-dev libxcb-keysyms1-dev ninja-build qtbase5-dev qtwebengine5-dev
```

Run `appimage-builder`:

```shell
cd /src
appimage-builder --skip-test --build-dir build.appimage --appdir build.appimage/AppDir --recipe pkg/appimage/appimage-amd64.yaml
```
