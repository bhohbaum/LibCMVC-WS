QT += core websockets
QT -= gui

TARGET = qtwsclient
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    main.cpp \
    echoclient.cpp

HEADERS += \
    echoclient.h

target.path = /usr/local/bin
INSTALLS += target
