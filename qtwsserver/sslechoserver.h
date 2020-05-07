#ifndef SSLECHOSERVER_H
#define SSLECHOSERVER_H

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtNetwork/QSslError>

#include "../qtws/qtws.h"
#include "../qtws/wsmetadata.h"

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)

class SslEchoServer : public QObject {
    Q_OBJECT
public:
    explicit SslEchoServer(quint16 port, quint16 sslPort, QObject* parent = nullptr, bool encrypted = false, QString cert = "", QString key = "", QStringList bbUrl = QStringList());
    ~SslEchoServer() override;

    bool startupComplete = false;

private slots:
    void onNewConnection();
    void onNewSSLConnection();
    void processTextMessage(QString message);
    void __processTextMessage(QString message, QString channel = "");
    void processBinaryMessage(QByteArray message);
    void __processBinaryMessage(QByteArray message, QString channel = "");
    void processTextMessageBB(QString message);
    void processBinaryMessageBB(QByteArray message);
    void socketDisconnected();
    void onBackboneConnected();
    void onBackboneDisconnected();
    void restoreBackboneConnection();
    void resetBackboneConnection();
    void resetBackboneResetTimer();

private:
    QMap<QString, QList<QWebSocket*>> m_channels;
    QTimer bbResetTimer;
    QStringList m_sBackbone;
    QStringList m_messageHashes;
    QList<QWebSocket*> m_bbRestoreClientQueue;
};

#endif //SSLECHOSERVER_H
