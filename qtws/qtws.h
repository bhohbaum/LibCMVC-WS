#ifndef QTWS_H
#define QTWS_H

#include "zlib.h"
#include <QByteArray>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QtNetwork/QSslError>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>
#include <limits.h>
#include <stdlib.h>

#define GZIP_WINDOWS_BIT 15 + 16 // gzip only decoding
#define GZIP_OR_ZLIB_WIN_BIT 15 + 32 // auto zlib or gzip decoding
#define GZIP_CHUNK_SIZE 32 * 1024

class QtWS : public QObject {
    Q_OBJECT

public:
    QtWS();

    QWebSocketServer *m_pWebSocketServer, *m_pWebSocketServerSSL;
    QWebSocket* m_pWebSocketBackbone;

public slots:
    bool gzipCompress(QByteArray input, QByteArray& output, int level = -1);
    bool gzipDecompress(QByteArray input, QByteArray& output);
    QString secureBackboneUrl(QString url);
    void onSslErrors(const QList<QSslError>& errors);
    void sendKeepAlivePing();
    void startKeepAliveTimer();
    void stopKeepAliveTimer();
    void startBackboneWatchdog();

private:
    QTimer keepaliveTimer;
};

#endif // QTWS_H
