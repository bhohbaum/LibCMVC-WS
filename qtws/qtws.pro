
QT -= gui

TEMPLATE = lib
CONFIG += staticlib

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
	channeltimeoutctrl.cpp \
	qtws.cpp \
	vanishingchannelentry.cpp \
	wsmetadata.cpp

HEADERS += \
	channeltimeoutctrl.h \
	qtws.h \
	vanishingchannelentry.h \
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
