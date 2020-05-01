QT = websockets
QT -= gui

TARGET = qtwsserver
CONFIG += console
CONFIG += app_bundle

TEMPLATE = app

SOURCES += \
	main.cpp \
	sslechoserver.cpp \


HEADERS += \
	sslechoserver.h \
	../qtws/qtws.h \

TRANSLATIONS += \
	qtws_de.ts

LIBS += -L../qtws/ -lqtws -lz

RESOURCES +=
DISTFILES += $$RESOURCES

target.path = /usr/local/bin
INSTALLS += target

system("lupdate qtwsserver.pro -recursive")
system("lrelease qtwsserver.pro")

