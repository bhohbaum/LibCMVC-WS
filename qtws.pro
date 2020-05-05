TEMPLATE = subdirs
SUBDIRS = \
	qtws \
	qtwsclient \
	qtwsserver \

CONFIG += ordered

OTHER_FILES = \
	$$files(./scripts/*) \

# do a static build if we are parsed by a statically built qmake
contains(CONFIG, "shared"): {
	message("Qt linking: dynamic")
} else {
	message("Qt linking: static")
	CONFIG += static
}

contains(CONFIG, "debug"): {
	DEFINES += DEBUG
}
