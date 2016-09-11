TEMPLATE = subdirs

SUBDIRS += \
    app \
    libs

app.subdir = app
libs.subdir = libs

app.depends = libs
