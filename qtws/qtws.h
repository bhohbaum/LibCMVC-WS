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

#include "channeltimeoutctrl.h"
#include "wsmetadata.h"

//#define DEBUG

#ifdef DEBUG
#define LOG(msg) QtWS::getInstance()->log(QString(msg).prepend(" ").prepend(QString::number(__LINE__)).prepend(" ").prepend(__FILE__));
#else
#define LOG(msg) QtWS::getInstance()->log(msg);
#endif

#define GZIP_WINDOWS_BIT 15 + 16 // gzip only decoding
#define GZIP_OR_ZLIB_WIN_BIT 15 + 32 // auto zlib or gzip decoding
#define GZIP_CHUNK_SIZE 32 * 1024

constexpr char BACKBONE_REGISTRATION_MSG[32] = "___BACKBONE_REGISTRATION_MSG___";
constexpr char CHANNEL_LIST_NOTIFICATION[32] = "___CHANNEL_LIST_NOTIFICATION___";
constexpr char CHANNEL_LIST_REQUEST[27] = "___CHANNEL_LIST_REQUEST___";
constexpr char MESSAGE_ID_TOKEN[23] = "___MESSAGE_ID_TOKEN___";

class QtWS : public QObject {
    Q_OBJECT

public:
    ~QtWS();

    static QtWS* getInstance();

    static QtWS* instance;

    QWebSocketServer *m_pWebSocketServer, *m_pWebSocketServerSSL;
    QList<QWebSocket*> m_pWebSocketBackbone;

    QList<QWebSocket*> m_clients;
    QList<QWebSocket*> m_backbones;

    QList<WsMetaData*> m_wsMetaDataList;
    ChannelTimeoutCtrl m_channelTimeoutCtrl;

public slots:
    bool gzipCompress(QByteArray input, QByteArray& output, int level = -1);
    bool gzipDecompress(QByteArray input, QByteArray& output);
    QString secureBackboneUrl(QString url);
    void onSslErrors(const QList<QSslError>& errors);
    void log(QString msg);
    void loadTranslation(QCoreApplication* app);
    QString wsInfo(QString msg, QWebSocket* pSocket);
    void sendKeepAlivePing();
    void startKeepAliveTimer();
    void stopKeepAliveTimer();
    void startBackboneWatchdog();
    void quitApplication();
    WsMetaData* findMetaDataByWebSocket(QWebSocket* pSocket);
    QString buildChannelListString(QStringList wmd);
    QString buildChannelListString(QList<WsMetaData*>* wmd);
    QStringList buildChannelListArray(QString channels);
    void handleBackboneRegistration(QWebSocket* pClient);
    void handleChannelListNotification(QString message, QWebSocket* pClient);
    QString getChannelFromSocket(QWebSocket* pSocket);
    bool getChannelFromMessage(QString* message, QString* channel);

signals:
    void updateChannels();
    void forceUpdateChannels();

private:
    QtWS();

    QTimer m_keepaliveTimer;
    QTranslator m_qtTranslator, m_qtTranslatorLib, m_qtTranslatorClient, m_qtTranslatorServer;
};

#endif // QTWS_H
