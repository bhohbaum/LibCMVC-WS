#include "sslechoserver.h"
#include "QtWebSockets/QWebSocketServer"
#include "QtWebSockets/QWebSocket"
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>

QT_USE_NAMESPACE

//! [constructor]
SslEchoServer::SslEchoServer(quint16 port, QObject *parent, bool encrypted, QString cert, QString key) :
    QObject(parent),
    m_pWebSocketServer(nullptr)
{
    m_pWebSocketServer = new QWebSocketServer(QStringLiteral("WS PubSub Server"),
                                              (encrypted) ? QWebSocketServer::SecureMode : QWebSocketServer::NonSecureMode,
                                              this);
    if (encrypted) {
        QSslConfiguration sslConfiguration;
        QFile certFile(cert);
        QFile keyFile(key);
        certFile.open(QIODevice::ReadOnly);
        keyFile.open(QIODevice::ReadOnly);
        QSslCertificate certificate(&certFile, QSsl::Pem);
        QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
        certFile.close();
        keyFile.close();
        sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
        sslConfiguration.setLocalCertificate(certificate);
        sslConfiguration.setPrivateKey(sslKey);
        //sslConfiguration.setProtocol(QSsl::TlsV1SslV3);
        sslConfiguration.setProtocol(QSsl::SecureProtocols);
        m_pWebSocketServer->setSslConfiguration(sslConfiguration);
    }
    if (m_pWebSocketServer->listen(QHostAddress::Any, port)) {
        qDebug() << "WS PubSub Server listening on port" << port;
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &SslEchoServer::onNewConnection);
        if (encrypted) connect(m_pWebSocketServer, &QWebSocketServer::sslErrors, this, &SslEchoServer::onSslErrors);
    } else {
        qDebug() << "Error binding socket!!";
    }
}
//! [constructor]

SslEchoServer::~SslEchoServer()
{
    m_pWebSocketServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

//! [onNewConnection]
void SslEchoServer::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    qDebug() << "Client connected:" << pSocket->peerName() << pSocket->origin() << pSocket->peerAddress().toString() << pSocket->peerPort() << pSocket->requestUrl().toString();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &SslEchoServer::processTextMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &SslEchoServer::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &SslEchoServer::socketDisconnected);

    m_clients << pSocket;
}
//! [onNewConnection]

//! [processTextMessage]
void SslEchoServer::processTextMessage(QString message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    int ctr = 0;
    if (pClient) {
        for (int i = 0; i < m_clients.count(); i++) {
            if (m_clients[i]->request().url().path() == pClient->request().url().path()) {   // && pClient != m_clients[i]
                m_clients[i]->sendTextMessage(message);
                ctr++;
            }
        }
    }
    qDebug() << "Distributing event to" << ctr << "clients. Channel:" << pClient->request().url().path()
             << "Message:" << message;
}
//! [processTextMessage]

//! [processBinaryMessage]
void SslEchoServer::processBinaryMessage(QByteArray message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient) {
        for (int i = 0; i < m_clients.count(); i++) {
            if (m_clients[i]->request().url().path() == pClient->request().url().path()) {    //  && pClient != m_clients[i]
                m_clients[i]->sendBinaryMessage(message);
            }
        }
    }
}
//! [processBinaryMessage]

//! [socketDisconnected]
void SslEchoServer::socketDisconnected()
{
    qDebug() << "Client disconnected";
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient) {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}

void SslEchoServer::onSslErrors(const QList<QSslError> &errors)
{
    qDebug() << "SSL error occurred";
    for (int i = 0; i < errors.length(); i++) {
        qDebug() << errors.at(i).errorString();
    }
}
//! [socketDisconnected]
