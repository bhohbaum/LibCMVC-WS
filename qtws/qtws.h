#ifndef QTWS_H
#define QTWS_H

#include "zlib.h"
#include <QByteArray>
#include <QString>
#include <QTimer>
#include <QTranslator>
#include <QUrl>
#include <QtCore/QCoreApplication>
#include <QtNetwork/QSslError>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>
#include <iostream>
#include <limits.h>
#include <stdlib.h>

#define LOG(msg) QtWS::getInstance()->log(msg);

#define GZIP_WINDOWS_BIT 15 + 16 // gzip only decoding
#define GZIP_OR_ZLIB_WIN_BIT 15 + 32 // auto zlib or gzip decoding
#define GZIP_CHUNK_SIZE 32 * 1024

class QtWS : public QObject {
    Q_OBJECT

public:
    ~QtWS();

    static QtWS* getInstance();

    static QtWS* instance;

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
    void log(QString msg);
    void loadTranslation(QCoreApplication* app);
    QString wsInfo(QString msg, QWebSocket* pSocket);

private:
    QtWS();

    QTimer keepaliveTimer;
    QTranslator qtTranslator, qtTranslatorLib, qtTranslatorClient, qtTranslatorServer;
};

#endif // QTWS_H
