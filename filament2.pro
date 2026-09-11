#-------------------------------------------------
#
# Project created by QtCreator 2023-06-24T17:08:15
#
#-------------------------------------------------

QT       += core gui

TARGET = filament2
TEMPLATE = app

# Build/output directory directly under /opt/.eko.
# Final binary: /opt/.eko/filament2-build-desktop/filament2
DESTDIR = /opt/.eko/filament2-build-desktop
OBJECTS_DIR = /opt/.eko/filament2-build-desktop/.obj
MOC_DIR = /opt/.eko/filament2-build-desktop/.moc
RCC_DIR = /opt/.eko/filament2-build-desktop/.rcc
UI_DIR = /opt/.eko/filament2-build-desktop/.ui

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc
