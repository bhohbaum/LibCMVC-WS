#ifndef EXTWEBSOCKET_H
#define EXTWEBSOCKET_H

#include <QObject>
#include <QtWebSockets/QWebSocket>

#include "wsmetadata.h"

class ExtWebSocket : public QWebSocket {
    Q_OBJECT

    friend WsMetaData;

public:
    ExtWebSocket();
    WsMetaData* getMetaData();

private:
    WsMetaData metaData;
};

#endif // EXTWEBSOCKET_H
