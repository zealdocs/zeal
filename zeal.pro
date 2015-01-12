lessThan(QT_VERSION, "5.2.0") {
    error("Qt 5.2.0 or above is required to build Zeal.")
}

TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    src
