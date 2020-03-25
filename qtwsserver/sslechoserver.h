#ifndef SSLECHOSERVER_H
#define SSLECHOSERVER_H

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtNetwork/QSslError>

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)

constexpr char BACKBONE_REGISTRATION_MSG[32] = "___BACKBONE_REGISTRATION_MSG___";

class SslEchoServer : public QObject {
    Q_OBJECT
public:
    explicit SslEchoServer(quint16 port, quint16 sslPort, QObject* parent = nullptr, bool encrypted = false, QString cert = "", QString key = "", QString bbUrl = "");
    ~SslEchoServer() override;

    bool startupComplete = false;

private Q_SLOTS:
    void onNewConnection();
    void onNewSSLConnection();
    void processTextMessage(QString message);
    void __processTextMessage(QString message, QString channel = "");
    void processBinaryMessage(QByteArray message);
    void processTextMessageBB(QString message);
    void processBinaryMessageBB(QByteArray message);
    void socketDisconnected();
    void onSslErrors(const QList<QSslError>& errors);
    void onBackboneConnected();
    void onBackboneDisconnected();
    void restoreBackboneConnection();

private:
    QWebSocketServer *m_pWebSocketServer, *m_pWebSocketServerSSL;
    QWebSocket* m_pWebSocketBackbone;

    QList<QWebSocket*> m_clients;
    QMap<QString, QList<QWebSocket*>> m_channels;
    QList<QWebSocket*> m_backbones;

    QString m_sBackbone = "";
};

#endif //SSLECHOSERVER_H
