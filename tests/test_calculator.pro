QT       += core
QT       -= gui

CONFIG   += console c++11
CONFIG   -= app_bundle

TEMPLATE = app
TARGET   = test_calculator

SOURCES += \
    test_calculator.cpp \
    ../calculator.cpp

HEADERS += \
    ../calculator.h
