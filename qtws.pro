TEMPLATE = subdirs
SUBDIRS = \
	qtws \
	qtwsclient \
	qtwsserver \


CONFIG += ordered

# do a static build if requested. otherwise this flag is ignored.
CONFIG += static


