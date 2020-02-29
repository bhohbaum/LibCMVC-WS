#include "sslechoserver.h"
#include "QtWebSockets/QWebSocketServer"
#include "QtWebSockets/QWebSocket"
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>

QT_USE_NAMESPACE

SslEchoServer::SslEchoServer(quint16 port, quint16 sslPort, QObject *parent, bool encrypted, QString cert, QString key) :
    QObject(parent),
    m_pWebSocketServer(nullptr)
{
    m_pWebSocketServer = new QWebSocketServer(QStringLiteral("WS PubSub Server"),
                                              QWebSocketServer::NonSecureMode,
                                              this);
    m_pWebSocketServerSSL = new QWebSocketServer(QStringLiteral("WS PubSub SSL Server"),
                                              QWebSocketServer::SecureMode,
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
        m_pWebSocketServerSSL->setSslConfiguration(sslConfiguration);
        if (sslPort != 0) {
            if (m_pWebSocketServerSSL->listen(QHostAddress::Any, sslPort)) {
                qDebug() << "WS PubSub SSL Server listening on port" << sslPort;
                connect(m_pWebSocketServerSSL, &QWebSocketServer::newConnection, this, &SslEchoServer::onNewSSLConnection);
                connect(m_pWebSocketServerSSL, &QWebSocketServer::sslErrors, this, &SslEchoServer::onSslErrors);
            } else {
                qDebug() << "Error binding SSL socket!!";
            }
        }
    }
    if (port != 0) {
        if (m_pWebSocketServer->listen(QHostAddress::Any, port)) {
            qDebug() << "WS PubSub Server listening on port" << port;
            connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &SslEchoServer::onNewConnection);
        } else {
            qDebug() << "Error binding PLAIN socket!!";
        }
    }
}

SslEchoServer::~SslEchoServer()
{
    m_pWebSocketServer->close();
    m_pWebSocketServerSSL->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

void SslEchoServer::onNewConnection()
{
    qDebug() << "New PLAIN connection...";
    QWebSocket *pSocket;
    pSocket = m_pWebSocketServer->nextPendingConnection();
    while (pSocket != nullptr) {
        qDebug() << "PLAIN Client connected:" << pSocket->peerName() << pSocket->origin() << pSocket->peerAddress().toString() << pSocket->peerPort() << pSocket->requestUrl().toString();
        connect(pSocket, &QWebSocket::textMessageReceived, this, &SslEchoServer::processTextMessage);
        connect(pSocket, &QWebSocket::binaryMessageReceived, this, &SslEchoServer::processBinaryMessage);
        connect(pSocket, &QWebSocket::disconnected, this, &SslEchoServer::socketDisconnected);
        m_clients << pSocket;
        pSocket = m_pWebSocketServer->nextPendingConnection();
    }
}

void SslEchoServer::onNewSSLConnection()
{
    qDebug() << "New SSL connection...";
    QWebSocket *pSocket;
    pSocket = m_pWebSocketServerSSL->nextPendingConnection();
    while (pSocket != nullptr) {
        qDebug() << "SSL Client connected:" << pSocket->peerName() << pSocket->origin() << pSocket->peerAddress().toString() << pSocket->peerPort() << pSocket->requestUrl().toString();
        connect(pSocket, &QWebSocket::textMessageReceived, this, &SslEchoServer::processTextMessage);
        connect(pSocket, &QWebSocket::binaryMessageReceived, this, &SslEchoServer::processBinaryMessage);
        connect(pSocket, &QWebSocket::disconnected, this, &SslEchoServer::socketDisconnected);
        m_clients << pSocket;
        pSocket = m_pWebSocketServerSSL->nextPendingConnection();
    }
}

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
    qDebug() << "Total number of clients:" << m_clients.count();
}


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
