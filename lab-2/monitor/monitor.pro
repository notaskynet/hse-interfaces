QT += core gui widgets serialport printsupport

CONFIG += c++17

TARGET = monitor
TEMPLATE = app

INCLUDEPATH += qcustomplot

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    qcustomplot/qcustomplot.cpp

HEADERS += \
    mainwindow.h \
    qcustomplot/qcustomplot.h
