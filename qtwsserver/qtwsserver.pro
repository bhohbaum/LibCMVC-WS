QT = websockets

TARGET = qtwsserver
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    main.cpp \
    sslechoserver.cpp

HEADERS += \
    sslechoserver.h

RESOURCES += securesocketclient.qrc
DISTFILES += $$RESOURCES

target.path = /usr/local/bin
INSTALLS += target
