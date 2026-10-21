QT += core testlib
QT -= gui
CONFIG += console testcase
CONFIG -= app_bundle
TEMPLATE = app
TARGET = archive_test
QMAKE_CXXFLAGS += -std=gnu++98
SOURCES += archive_test.cpp ../archive.cpp
HEADERS += ../archive.h
