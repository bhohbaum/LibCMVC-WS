QT = websockets

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

LIBS += -L../qtws/ -lqtws -lz

RESOURCES += securesocketclient.qrc
DISTFILES += $$RESOURCES

target.path = /usr/local/bin
INSTALLS += target
