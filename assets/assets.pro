include($$ZEAL_COMMON_PRI)

TEMPLATE = aux

unix:!macx {
    TARGET = zeal
    appicons16.files=freedesktop/appicons/16/*
    appicons24.files=freedesktop/appicons/24/*
    appicons32.files=freedesktop/appicons/32/*
    appicons64.files=freedesktop/appicons/64/*
    appicons128.files=freedesktop/appicons/128/*

    appicons16.path=$$PREFIX/share/icons/hicolor/16x16/apps
    appicons24.path=$$PREFIX/share/icons/hicolor/24x24/apps
    appicons32.path=$$PREFIX/share/icons/hicolor/32x32/apps
    appicons64.path=$$PREFIX/share/icons/hicolor/64x64/apps
    appicons128.path=$$PREFIX/share/icons/hicolor/128x128/apps

    desktop.files=freedesktop/zeal.desktop
    desktop.path=$$PREFIX/share/applications

    INSTALLS += appicons16 appicons24 appicons32 appicons64 appicons128 desktop
}
