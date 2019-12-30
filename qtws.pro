TEMPLATE = subdirs
SUBDIRS = qtwsclient \
          qtwsserver

# build must be last:
CONFIG += ordered
CONFIG += static
#SUBDIRS += build


