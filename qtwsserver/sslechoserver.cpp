#include "sslechoserver.h"
#include "QtWebSockets/QWebSocket"
#include "QtWebSockets/QWebSocketServer"
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>

QT_USE_NAMESPACE

SslEchoServer::SslEchoServer(quint16 port, quint16 sslPort, QObject* parent, bool encrypted, QString cert, QString key, QString bbUrl)
    : QObject(parent)
    , m_pWebSocketServer(nullptr)
    , m_pWebSocketServerSSL(nullptr)
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
        if (!certFile.open(QIODevice::ReadOnly)) {
            qDebug() << "Error opening SSL certificate." << sslPort;
            return;
        }
        if (!keyFile.open(QIODevice::ReadOnly)) {
            qDebug() << "Error opening SSL key." << sslPort;
            return;
        }
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
                connect(m_pWebSocketServerSSL,
                    &QWebSocketServer::newConnection,
                    this,
                    &SslEchoServer::onNewSSLConnection);
                connect(m_pWebSocketServerSSL,
                    &QWebSocketServer::sslErrors,
                    this,
                    &SslEchoServer::onSslErrors);
            } else {
                qDebug() << "Error binding SSL socket!!";
                return;
            }
        }
    }
    if (port != 0) {
        if (m_pWebSocketServer->listen(QHostAddress::Any, port)) {
            qDebug() << "WS PubSub Server listening on port" << port;
            connect(m_pWebSocketServer,
                &QWebSocketServer::newConnection,
                this,
                &SslEchoServer::onNewConnection);
        } else {
            qDebug() << "Error binding PLAIN socket!!";
            return;
        }
    }
    if (bbUrl != "") {
        m_sBackbone = bbUrl;
        m_pWebSocketBackbone = new QWebSocket();
        connect(m_pWebSocketBackbone,
            &QWebSocket::connected,
            this,
            &SslEchoServer::onBackboneConnected);
        connect(m_pWebSocketBackbone,
            &QWebSocket::disconnected,
            this,
            &SslEchoServer::onBackboneDisconnected);
        connect(m_pWebSocketBackbone,
            QOverload<const QList<QSslError>&>::of(&QWebSocket::sslErrors),
            this,
            &SslEchoServer::onSslErrors);
        QUrl u1(m_sBackbone);
        m_pWebSocketBackbone->open(QUrl(u1.scheme().append("://").append(u1.host()).append(":").append(QString::number(u1.port()))));
        m_backbones << m_pWebSocketBackbone;
    }
    startupComplete = true;
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
    QWebSocket* pSocket;
    pSocket = m_pWebSocketServer->nextPendingConnection();
    while (pSocket != nullptr) {
        qDebug() << "PLAIN Client connected:" << pSocket->peerName() << pSocket->origin()
                 << pSocket->peerAddress().toString() << pSocket->peerPort()
                 << pSocket->requestUrl().toString();
        connect(pSocket, &QWebSocket::textMessageReceived, this, &SslEchoServer::processTextMessage);
        connect(pSocket,
            &QWebSocket::binaryMessageReceived,
            this,
            &SslEchoServer::processBinaryMessage);
        connect(pSocket, &QWebSocket::disconnected, this, &SslEchoServer::socketDisconnected);
        m_clients << pSocket;
        pSocket = m_pWebSocketServer->nextPendingConnection();
    }
}

void SslEchoServer::onNewSSLConnection()
{
    qDebug() << "New SSL connection...";
    QWebSocket* pSocket;
    pSocket = m_pWebSocketServerSSL->nextPendingConnection();
    while (pSocket != nullptr) {
        qDebug() << "SSL Client connected:" << pSocket->peerName() << pSocket->origin()
                 << pSocket->peerAddress().toString() << pSocket->peerPort()
                 << pSocket->requestUrl().toString();
        connect(pSocket, &QWebSocket::textMessageReceived, this, &SslEchoServer::processTextMessage);
        connect(pSocket,
            &QWebSocket::binaryMessageReceived,
            this,
            &SslEchoServer::processBinaryMessage);
        connect(pSocket, &QWebSocket::disconnected, this, &SslEchoServer::socketDisconnected);
        connect(pSocket,
            QOverload<const QList<QSslError>&>::of(&QWebSocket::sslErrors),
            this,
            &SslEchoServer::onSslErrors);
        m_clients << pSocket;
        pSocket = m_pWebSocketServerSSL->nextPendingConnection();
    }
}

void SslEchoServer::processTextMessage(QString message)
{
    __processTextMessage(message, "");
}

void SslEchoServer::__processTextMessage(QString message, QString channel)
{
    QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
    channel = (channel == "") ? pClient->request().url().path() : channel;
    if (channel == "/") {
        for (int i = 0; i < m_backbones.count(); i++) {
            if (pClient == m_backbones[i]) {
                QStringList arr = message.split("\n");
                channel = arr[0];
                arr.pop_front();
                message = arr.join("\n");
            }
        }
    }
    if (message == BACKBONE_REGISTRATION_MSG) {
        if (pClient) {
            m_clients.removeAll(pClient);
            m_backbones << pClient;
            qDebug() << "Client identified as another server, moving connection to backbone pool:"
                     << pClient->peerName() << pClient->origin()
                     << pClient->peerAddress().toString() << pClient->peerPort()
                     << pClient->requestUrl().toString();
        }
    } else {
        int ctr = 0, ctr2 = 0;
        if (pClient) {
            for (int i = 0; i < m_clients.count(); i++) {
                if (m_clients[i]->request().url().path() == channel) { // && pClient != m_clients[i]
                    m_clients[i]->sendTextMessage(message);
                    ctr++;
                }
            }
            QString bbMessage(channel);
            bbMessage.append("\n");
            bbMessage.append(message);
            for (int i = 0; i < m_backbones.count(); i++) {
                if (pClient != m_backbones[i]) {
                    m_backbones[i]->sendTextMessage(bbMessage);
                    ctr2++;
                }
            }
        }
        qDebug() << "Distributing event to" << ctr << "clients and" << ctr2
                 << "other servers. Channel:" << channel << "Message:" << message;
    }
    qDebug() << "Total number of clients:" << m_clients.count()
             << "Connections to other servers:" << m_backbones.count();
}

void SslEchoServer::processBinaryMessage(QByteArray message)
{
    QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
    int ctr = 0;
    if (pClient) {
        for (int i = 0; i < m_clients.count(); i++) {
            if (m_clients[i]->request().url().path() == pClient->request().url().path()) { //  && pClient != m_clients[i]
                m_clients[i]->sendBinaryMessage(message);
                ctr++;
            }
        }
    }
    qDebug() << "Distributing event to" << ctr << "clients. Channel:" << pClient->request().url().path()
             << "Message:" << message;
    qDebug() << "Total number of clients:" << m_clients.count();
}

void SslEchoServer::socketDisconnected()
{
    QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
    qDebug() << "Client disconnected:" << pClient->peerName() << pClient->origin()
             << pClient->peerAddress().toString() << pClient->peerPort()
             << pClient->requestUrl().toString();
    if (pClient) {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}

void SslEchoServer::onSslErrors(const QList<QSslError>& errors)
{
    QString str("SSL error occurred: ");
    for (int i = 0; i < errors.length(); i++) {
        str.append(errors.at(i).errorString());
        str.append("\n");
    }
    qDebug() << str;
}

void SslEchoServer::onBackboneConnected()
{
    connect(m_pWebSocketBackbone,
        &QWebSocket::textMessageReceived,
        this,
        &SslEchoServer::processTextMessageBB);
    connect(m_pWebSocketBackbone,
        &QWebSocket::binaryMessageReceived,
        this,
        &SslEchoServer::processBinaryMessageBB);
    m_pWebSocketBackbone->sendTextMessage(BACKBONE_REGISTRATION_MSG);
}

void SslEchoServer::onBackboneDisconnected()
{
    m_backbones.removeAll(m_pWebSocketBackbone);
    QUrl u1(m_sBackbone);
    m_pWebSocketBackbone->open(QUrl(u1.scheme().append("://").append(u1.host()).append(":").append(QString::number(u1.port()))));
    m_backbones << m_pWebSocketBackbone;
}

void SslEchoServer::processTextMessageBB(QString message)
{
    QStringList arr = message.split("\n");
    QString channel = arr[0];
    arr.pop_front();
    message = arr.join("\n");
    __processTextMessage(message, channel);
}

void SslEchoServer::processBinaryMessageBB(QByteArray message)
{
    processBinaryMessage(message);
}
