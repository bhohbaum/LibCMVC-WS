QT += core websockets
QT -= gui

TARGET = qtwsclient
CONFIG += console
CONFIG += app_bundle

TEMPLATE = app

SOURCES += \
	$$files(*.cpp) \

HEADERS += \
	$$files(*.h) \
	../qtws/qtws.h \

TRANSLATIONS += \
	qtws_de.ts

LIBS += -L../qtws/ -lqtws -lz

DISTFILES += ../qtws/libqtws.a

target.path = /usr/local/bin
INSTALLS += target

system("lupdate qtwsclient.pro -recursive")
system("lrelease qtwsclient.pro")
