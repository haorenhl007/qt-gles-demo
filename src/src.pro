TEMPLATE = app
TARGET = gl-lnl
CONFIG += c++14
QT += widgets

include(src.pri)

RESOURCES += resources/resources.qrc

FORMS += MainWindow.ui

HEADERS += GlWidget.h
HEADERS += MainWindow.h
HEADERS += ModelTools.h

SOURCES += GlWidget.cpp
SOURCES += main.cpp
SOURCES += MainWindow.cpp
SOURCES += ModelTools.cpp
