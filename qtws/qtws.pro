QT -= gui

TEMPLATE = lib
CONFIG += staticlib

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
	extwebsocket.cpp \
	qtws.cpp \
	wsmetadata.cpp

HEADERS += \
	extwebsocket.h \
	qtws.h \
	wsmetadata.h

TRANSLATIONS += \
	qtws_de.ts \
	../qtwsclient/qtws_de.ts \
	../qtwsserver/qtws_de.ts \

RESOURCES += \
	qtws.qrc

target.path = /usr/local/lib
INSTALLS += target

#system("lupdate qtws.pro -recursive -tr-function-alias tr=trans")
system("lupdate qtws.pro -recursive")
system("lrelease qtws.pro")
