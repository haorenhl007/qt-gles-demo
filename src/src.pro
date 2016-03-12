TEMPLATE = app
TARGET = gl-lnl
CONFIG += c++14
QT += widgets

include(src.pri)

FORMS += MainWindow.ui

HEADERS += GlWidget.h
HEADERS += MainWindow.h

SOURCES += GlWidget.cpp
SOURCES += main.cpp
SOURCES += MainWindow.cpp
