TEMPLATE = app
TARGET = gl-lnl-test
CONFIG += c++14
CONFIG += testcase
QT -= gui
QT += testlib

include(../src/src.pri)
INCLUDEPATH += ../src

HEADERS += SampleTest.h

SOURCES += main.cpp
SOURCES += SampleTest.cpp
