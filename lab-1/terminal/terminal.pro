QT += core gui widgets serialport

CONFIG += c++17

TARGET = terminal
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    datahistory.cpp \
    historyview.cpp \
    formatregistry.cpp \
    asciiformat.cpp \
    numericformat.cpp

HEADERS += \
    mainwindow.h \
    datahistory.h \
    historyview.h \
    dataformat.h \
    formatregistry.h \
    asciiformat.h \
    numericformat.h
