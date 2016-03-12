TEMPLATE = app
TARGET = gl-lnl
CONFIG += c++14
QT += widgets

include(src.pri)

FORMS += $$PWD/MainWindow.ui

HEADERS += $$PWD/GlWidget.h
HEADERS += $$PWD/MainWindow.h

SOURCES += $$PWD/GlWidget.cpp
SOURCES += $$PWD/main.cpp
SOURCES += $$PWD/MainWindow.cpp
