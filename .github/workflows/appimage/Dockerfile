FROM ubuntu:noble

# Force older pipx to use global location.
ENV PIPX_HOME=/opt/pipx PIPX_BIN_DIR=/usr/local/bin PIPX_MAN_DIR=/usr/local/share/man

RUN apt-get update -q -y \
    # Install appimage-builder and appimagetool dependencies.
    && DEBIAN_FRONTEND="noninteractive" apt-get install -q -y --no-install-recommends \
       appstream curl desktop-file-utils fakeroot file git gnupg patchelf pipx squashfs-tools zsync \
    # Install appimagetool, it has to be extracted because FUSE doesn't work in containers without extra fiddling.
    && cd /tmp \
    && curl -sLO https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage \
    && chmod +x appimagetool-x86_64.AppImage \
    && ./appimagetool-x86_64.AppImage --appimage-extract \
    && mv squashfs-root/ /opt/appimagetool.AppDir \
    && ln -s /opt/appimagetool.AppDir/AppRun /usr/local/bin/appimagetool \
    && rm appimagetool-x86_64.AppImage \
    && cd - \
    # Install appimage-builder.
    && pipx install git+https://github.com/AppImageCrafters/appimage-builder.git@d96cf01e131e01b9f9e713db8efa7d20c57f6a09 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Set entrypoint.
COPY entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
