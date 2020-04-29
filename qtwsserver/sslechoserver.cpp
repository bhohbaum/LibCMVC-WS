#include "sslechoserver.h"
#include "QtWebSockets/QWebSocket"
#include "QtWebSockets/QWebSocketServer"
#include <QThread>
#include <QTimer>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>

QT_USE_NAMESPACE

SslEchoServer::SslEchoServer(quint16 port, quint16 sslPort, QObject* parent, bool encrypted, QString cert, QString key, QString bbUrl)
    : QObject(parent)
{
    m_qtws.m_pWebSocketServer = new QWebSocketServer(QStringLiteral("WS PubSub Server"),
        QWebSocketServer::NonSecureMode,
        this);
    m_qtws.m_pWebSocketServerSSL = new QWebSocketServer(QStringLiteral("WS PubSub SSL Server"),
        QWebSocketServer::SecureMode,
        this);
    if (encrypted) {
        QSslConfiguration sslConfiguration;
        QFile certFile(cert);
        QFile keyFile(key);
        if (!certFile.open(QIODevice::ReadOnly)) {
            qDebug() << "Error opening SSL certificate.";
            return;
        }
        if (!keyFile.open(QIODevice::ReadOnly)) {
            qDebug() << "Error opening SSL key.";
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
        m_qtws.m_pWebSocketServerSSL->setSslConfiguration(sslConfiguration);
        if (sslPort != 0) {
            if (m_qtws.m_pWebSocketServerSSL->listen(QHostAddress::Any, sslPort)) {
                qDebug() << "WS PubSub SSL Server listening on port" << sslPort;
                connect(m_qtws.m_pWebSocketServerSSL,
                    &QWebSocketServer::newConnection,
                    this,
                    &SslEchoServer::onNewSSLConnection);
                connect(m_qtws.m_pWebSocketServerSSL,
                    &QWebSocketServer::sslErrors,
                    &m_qtws,
                    &QtWS::onSslErrors);
            } else {
                qDebug() << "Error binding SSL socket!!";
                return;
            }
        }
    }
    if (port != 0) {
        if (m_qtws.m_pWebSocketServer->listen(QHostAddress::Any, port)) {
            qDebug() << "WS PubSub Server listening on port" << port;
            connect(m_qtws.m_pWebSocketServer,
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
        //m_qtws.m_pWebSocketBackbone = new QWebSocket();
        connect(m_qtws.m_pWebSocketBackbone,
            &QWebSocket::connected,
            this,
            &SslEchoServer::onBackboneConnected);
        connect(m_qtws.m_pWebSocketBackbone,
            &QWebSocket::disconnected,
            this,
            &SslEchoServer::onBackboneDisconnected);
        connect(m_qtws.m_pWebSocketBackbone,
            QOverload<const QList<QSslError>&>::of(&QWebSocket::sslErrors),
            &m_qtws,
            &QtWS::onSslErrors);
        //m_pWebSocketBackbone->open(m_sBackbone);
        m_qtws.m_pWebSocketBackbone->open(m_qtws.secureBackboneUrl(m_sBackbone));
        connect(m_qtws.m_pWebSocketBackbone,
            &QWebSocket::textMessageReceived,
            this,
            &SslEchoServer::processTextMessageBB);
        connect(m_qtws.m_pWebSocketBackbone,
            &QWebSocket::binaryMessageReceived,
            this,
            &SslEchoServer::processBinaryMessageBB);
        connect(m_qtws.m_pWebSocketBackbone,
            &QWebSocket::pong,
            this,
            &SslEchoServer::resetBackboneResetTimer);
        m_backbones.removeAll(m_qtws.m_pWebSocketBackbone);
        m_backbones << m_qtws.m_pWebSocketBackbone;
        bbResetTimer.setInterval(3500);
        connect(&bbResetTimer, SIGNAL(timeout()), this, SLOT(resetBackboneConnection()));
        bbResetTimer.start();
    }
    startupComplete = true;
}

SslEchoServer::~SslEchoServer()
{
    m_qtws.m_pWebSocketServer->close();
    m_qtws.m_pWebSocketServerSSL->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
    qDeleteAll(m_backbones.begin(), m_backbones.end());
}

void SslEchoServer::onNewConnection()
{
    qDebug() << "New PLAIN connection...";
    QWebSocket* pSocket;
    pSocket = m_qtws.m_pWebSocketServer->nextPendingConnection();
    while (pSocket != nullptr) {
        qDebug() << "PLAIN Client connected:" << pSocket->peerName() << pSocket->origin()
                 << pSocket->peerAddress().toString() << pSocket->peerPort()
                 << pSocket->requestUrl().toString();
        connect(pSocket, &QWebSocket::textMessageReceived, this, &SslEchoServer::processTextMessage);
        connect(pSocket, &QWebSocket::binaryMessageReceived, this, &SslEchoServer::processBinaryMessage);
        connect(pSocket, &QWebSocket::disconnected, this, &SslEchoServer::socketDisconnected);
        m_clients.removeAll(pSocket);
        m_clients << pSocket;
        //m_channels[pSocket->request().url().path()].append(pSocket);
        pSocket = m_qtws.m_pWebSocketServer->nextPendingConnection();
    }
}

void SslEchoServer::onNewSSLConnection()
{
    qDebug() << "New SSL connection...";
    QWebSocket* pSocket;
    pSocket = m_qtws.m_pWebSocketServerSSL->nextPendingConnection();
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
            &m_qtws,
            &QtWS::onSslErrors);
        m_clients.removeAll(pSocket);
        m_clients << pSocket;
        //m_channels[pSocket->request().url().path()].append(pSocket);
        pSocket = m_qtws.m_pWebSocketServerSSL->nextPendingConnection();
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
            m_backbones.removeAll(pClient);
            m_backbones << pClient;
            pClient->request().url().setPath("/");
            pClient->requestUrl().setPath("/");
            qDebug() << "Client identified as another server, moving connection to backbone pool:"
                     << pClient->peerName() << pClient->origin()
                     << pClient->peerAddress().toString() << pClient->peerPort()
                     << pClient->requestUrl().toString();
        }
    } else {
        int ctr = 0, ctr2 = 0;
        if (pClient) {
            /*
            QList<QWebSocket*>::iterator iter = m_channels.find(pClient->request().url().path())->begin();
            const QList<QWebSocket*>::iterator end = m_channels.find(pClient->request().url().path())->end();
            for (; iter != end; ++iter) {
                (*iter)->sendTextMessage(message);
            }

            for (int i = 0; i < m_channels.find(pClient->request().url().path())->count(); i++) {
                QWebSocket* pSocket = m_channels.find(pClient->request().url().path())->at(i);
                pSocket->sendTextMessage(message);
            }
*/
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
    __processBinaryMessage(message, "");
    /*
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
*/
}

void SslEchoServer::__processBinaryMessage(QByteArray message, QString channel)
{
    QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
    channel = (channel == "") ? pClient->request().url().path() : channel;
    QByteArray ba;
    QString msg(message);
    if (m_qtws.gzipDecompress(message, ba)) {
        msg = ba;
        message = ba;
    }
    if (channel == "/") {
        for (int i = 0; i < m_backbones.count(); i++) {
            if (pClient == m_backbones[i]) {
                QStringList arr = msg.split("\n");
                channel = arr[0];
                arr.pop_front();
                msg = arr.join("\n");
                message = msg.toUtf8();
            }
        }
    }
    m_qtws.gzipCompress(message, ba, 9);
    QByteArray cMessage(ba);
    QString bbMessage(channel);
    bbMessage.append("\n");
    bbMessage.append(message);
    m_qtws.gzipCompress(bbMessage.toUtf8(), ba, 9);
    QByteArray cbbMessage(ba);
    if (message == BACKBONE_REGISTRATION_MSG) {
        if (pClient) {
            m_clients.removeAll(pClient);
            m_backbones.removeAll(pClient);
            m_backbones << pClient;
            qDebug() << "Client identified as another server, moving connection to backbone pool:"
                     << pClient->peerName() << pClient->origin()
                     << pClient->peerAddress().toString() << pClient->peerPort()
                     << pClient->requestUrl().toString();
        }
    } else {
        int ctr = 0, ctr2 = 0;
        if (pClient) {
            /*
            QList<QWebSocket*>::iterator iter = m_channels.find(pClient->request().url().path())->begin();
            const QList<QWebSocket*>::iterator end = m_channels.find(pClient->request().url().path())->end();
            for (; iter != end; ++iter) {
                (*iter)->sendTextMessage(message);
            }

            for (int i = 0; i < m_channels.find(pClient->request().url().path())->count(); i++) {
                QWebSocket* pSocket = m_channels.find(pClient->request().url().path())->at(i);
                pSocket->sendTextMessage(message);
            }
            */
            for (int i = 0; i < m_clients.count(); i++) {
                if (m_clients[i]->request().url().path() == channel) { // && pClient != m_clients[i]
                    m_clients[i]->sendBinaryMessage(cMessage);
                    ctr++;
                }
            }
            for (int i = 0; i < m_backbones.count(); i++) {
                if (pClient != m_backbones[i]) {
                    m_backbones[i]->sendBinaryMessage(cbbMessage);
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

void SslEchoServer::socketDisconnected()
{
    QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
    qDebug() << "Client disconnected:" << pClient->peerName() << pClient->origin()
             << pClient->peerAddress().toString() << pClient->peerPort()
             << pClient->requestUrl().toString();
    if (pClient) {
        m_clients.removeAll(pClient);
        //m_channels.find(pClient->request().url().path())->removeAll(pClient);
        m_backbones.removeAll(pClient);
        pClient->deleteLater();
    }
}

void SslEchoServer::onBackboneConnected()
{
    m_qtws.m_pWebSocketBackbone->sendTextMessage(BACKBONE_REGISTRATION_MSG);
}

void SslEchoServer::onBackboneDisconnected()
{
    qDebug() << "Backbone disconnected. Restoring connection....";
    bbResetTimer.stop();
    m_backbones.removeAll(m_qtws.m_pWebSocketBackbone);
    QTimer* timer = new QTimer(this);
    timer->singleShot(1000, this, SLOT(restoreBackboneConnection()));
}

void SslEchoServer::restoreBackboneConnection()
{
    qDebug() << "Opening new backbone connection....";
    //m_pWebSocketBackbone->open(m_sBackbone);
    m_qtws.m_pWebSocketBackbone->open(m_qtws.secureBackboneUrl(m_sBackbone));
    m_backbones.removeAll(m_qtws.m_pWebSocketBackbone);
    m_backbones << m_qtws.m_pWebSocketBackbone;
    bbResetTimer.start();
}

void SslEchoServer::resetBackboneConnection()
{
    qDebug() << "Doing a full restart....";
    m_qtws.m_pWebSocketBackbone->disconnect();
    m_qtws.m_pWebSocketBackbone->close();
    m_backbones.removeAll(m_qtws.m_pWebSocketBackbone);
    m_clients.removeAll(m_qtws.m_pWebSocketBackbone);
    delete m_qtws.m_pWebSocketBackbone;

    for (int i = 0; i < m_backbones.count(); i++) {
        m_backbones.at(i)->close();
    }
    for (int i = 0; i < m_clients.count(); i++) {
        m_clients.at(i)->close();
    }

    m_qtws.m_pWebSocketServer->close();
    m_qtws.m_pWebSocketServerSSL->close();

    QCoreApplication::quit();
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
    QByteArray ba;
    QString msg(message);
    if (m_qtws.gzipDecompress(message, ba)) {
        msg = ba;
        message = ba;
    }
    QStringList arr = msg.split("\n");
    QString channel = arr[0];
    arr.pop_front();
    msg = arr.join("\n");
    __processBinaryMessage(msg.toUtf8(), channel);
}

void SslEchoServer::resetBackboneResetTimer()
{
    qDebug() << "PONG";
    bbResetTimer.stop();
    bbResetTimer.start();
}
