TEMPLATE = app
TARGET = gl-lnl-test
CONFIG += c++14
CONFIG += testcase
QT -= gui
QT += testlib

include(../src/src.pri)
INCLUDEPATH += ../src

HEADERS += Ply/PlyModelTest.h

SOURCES += main.cpp
SOURCES += Ply/PlyModelTest.cpp
