#ifndef SSLECHOSERVER_H
#define SSLECHOSERVER_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QByteArray>
#include <QtNetwork/QSslError>

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)

class SslEchoServer : public QObject
{
    Q_OBJECT
public:
    explicit SslEchoServer(quint16 port, quint16 sslPort, QObject *parent = nullptr, bool encrypted = false, QString cert = "", QString key = "");
    ~SslEchoServer() override;

private Q_SLOTS:
    void onNewConnection();
    void onNewSSLConnection();
    void processTextMessage(QString message);
    void processBinaryMessage(QByteArray message);
    void socketDisconnected();
    void onSslErrors(const QList<QSslError> &errors);

private:
    QWebSocketServer *m_pWebSocketServer, *m_pWebSocketServerSSL;
    QList<QWebSocket *> m_clients;

};

#endif //SSLECHOSERVER_H
