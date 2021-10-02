FROM ubuntu:focal

RUN apt-get update -q -y \
   # Install appimage-builder and appimagetool dependencies.
    && DEBIAN_FRONTEND="noninteractive" apt-get install -q -y --no-install-recommends \
       appstream curl desktop-file-utils fakeroot file git gnupg patchelf zsync  \
       python3-pip python3-setuptools python3-wheel \
    && \
    # Install appimagetool, it has to be extracted because FUSE doesn't work in containers without extra fiddling.
    cd /tmp && \
    curl -sLO https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage && \
    chmod +x appimagetool-x86_64.AppImage && \
    ./appimagetool-x86_64.AppImage --appimage-extract && \
    mv squashfs-root/ /opt/appimagetool.AppDir && \
    ln -s /opt/appimagetool.AppDir/AppRun /usr/local/bin/appimagetool && \
    rm appimagetool-x86_64.AppImage && \
    cd - && \
    # Install appimage-builder.
    pip3 install git+https://github.com/AppImageCrafters/appimage-builder.git@v0.8.3 && \
    apt-get clean && \
        rm -rf /var/lib/apt/lists/*

# Set entrypoint.
COPY entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
