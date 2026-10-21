QT += core gui testlib
CONFIG += console testcase
CONFIG -= app_bundle
TEMPLATE = app
TARGET = usage_gui_test
QMAKE_CXXFLAGS += -std=gnu++98
SOURCES += usage_gui_test.cpp ../archive.cpp ../usagewidget.cpp
HEADERS += ../mainwindow.h ../archive.h ../usagewidget.h
FORMS += ../mainwindow.ui
RESOURCES += ../resources.qrc
