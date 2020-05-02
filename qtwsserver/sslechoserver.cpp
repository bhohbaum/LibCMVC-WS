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

/**
 * @brief SslEchoServer::SslEchoServer
 * @param port
 * @param sslPort
 * @param parent
 * @param encrypted
 * @param cert
 * @param key
 * @param bbUrl
 */
SslEchoServer::SslEchoServer(quint16 port, quint16 sslPort, QObject* parent, bool encrypted, QString cert, QString key, QString bbUrl)
    : QObject(parent)
{
    QtWS::getInstance()->m_pWebSocketServer = new QWebSocketServer(QStringLiteral("WS PubSub Server"),
        QWebSocketServer::NonSecureMode,
        this);
    QtWS::getInstance()->m_pWebSocketServerSSL = new QWebSocketServer(QStringLiteral("WS PubSub SSL Server"),
        QWebSocketServer::SecureMode,
        this);
    if (encrypted) {
        QSslConfiguration sslConfiguration;
        QFile certFile(cert);
        QFile keyFile(key);
        if (!certFile.open(QIODevice::ReadOnly)) {
            LOG(tr("Error opening SSL certificate."));
            return;
        }
        if (!keyFile.open(QIODevice::ReadOnly)) {
            LOG(tr("Error opening SSL key."));
            return;
        }
        QSslCertificate certificate(&certFile, QSsl::Pem);
        QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
        certFile.close();
        keyFile.close();
        sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
        sslConfiguration.setLocalCertificate(certificate);
        sslConfiguration.setPrivateKey(sslKey);
        sslConfiguration.setProtocol(QSsl::SecureProtocols);
        QtWS::getInstance()->m_pWebSocketServerSSL->setSslConfiguration(sslConfiguration);
        if (sslPort != 0) {
            if (QtWS::getInstance()->m_pWebSocketServerSSL->listen(QHostAddress::Any, sslPort)) {
                LOG(tr("WS PubSub SSL Server listening on port ").append(QString::number(sslPort)));
                connect(QtWS::getInstance()->m_pWebSocketServerSSL,
                    &QWebSocketServer::newConnection,
                    this,
                    &SslEchoServer::onNewSSLConnection);
                connect(QtWS::getInstance()->m_pWebSocketServerSSL,
                    &QWebSocketServer::sslErrors,
                    QtWS::getInstance(),
                    &QtWS::onSslErrors);
            } else {
                LOG(tr("Error binding SSL socket!!"));
                return;
            }
        }
    }
    if (port != 0) {
        if (QtWS::getInstance()->m_pWebSocketServer->listen(QHostAddress::Any, port)) {
            LOG(tr("WS PubSub Server listening on port ").append(QString::number(port)));
            connect(QtWS::getInstance()->m_pWebSocketServer,
                &QWebSocketServer::newConnection,
                this,
                &SslEchoServer::onNewConnection);
        } else {
            LOG(tr("Error binding PLAIN socket!!"));
            return;
        }
    }
    if (bbUrl != "") {
        m_sBackbone = bbUrl;
        connect(QtWS::getInstance()->m_pWebSocketBackbone,
            &QWebSocket::connected,
            this,
            &SslEchoServer::onBackboneConnected);
        connect(QtWS::getInstance()->m_pWebSocketBackbone,
            &QWebSocket::disconnected,
            this,
            &SslEchoServer::onBackboneDisconnected);
        connect(QtWS::getInstance()->m_pWebSocketBackbone,
            QOverload<const QList<QSslError>&>::of(&QWebSocket::sslErrors),
            QtWS::getInstance(),
            &QtWS::onSslErrors);
        QtWS::getInstance()->m_pWebSocketBackbone->open(QtWS::getInstance()->secureBackboneUrl(m_sBackbone));
        connect(QtWS::getInstance()->m_pWebSocketBackbone,
            &QWebSocket::textMessageReceived,
            this,
            &SslEchoServer::processTextMessageBB);
        connect(QtWS::getInstance()->m_pWebSocketBackbone,
            &QWebSocket::binaryMessageReceived,
            this,
            &SslEchoServer::processBinaryMessageBB);
        connect(QtWS::getInstance()->m_pWebSocketBackbone,
            &QWebSocket::pong,
            this,
            &SslEchoServer::resetBackboneResetTimer);
        m_backbones.removeAll(QtWS::getInstance()->m_pWebSocketBackbone);
        m_backbones << QtWS::getInstance()->m_pWebSocketBackbone;
        bbResetTimer.setInterval(3500);
        connect(&bbResetTimer, SIGNAL(timeout()), this, SLOT(resetBackboneConnection()));
        bbResetTimer.start();
    }
    startupComplete = true;
}

/**
 * @brief SslEchoServer::~SslEchoServer
 */
SslEchoServer::~SslEchoServer()
{
    QtWS::getInstance()->m_pWebSocketServer->close();
    QtWS::getInstance()->m_pWebSocketServerSSL->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
    qDeleteAll(m_backbones.begin(), m_backbones.end());
}

/**
 * @brief SslEchoServer::onNewConnection
 */
void SslEchoServer::onNewConnection()
{
    LOG(tr("New PLAIN connection..."));
    QWebSocket* pSocket;
    pSocket = QtWS::getInstance()->m_pWebSocketServer->nextPendingConnection();
    while (pSocket != nullptr) {
        LOG(QtWS::getInstance()->wsInfo(tr("PLAIN Client connected: "), pSocket));
        connect(pSocket, &QWebSocket::textMessageReceived, this, &SslEchoServer::processTextMessage);
        connect(pSocket, &QWebSocket::binaryMessageReceived, this, &SslEchoServer::processBinaryMessage);
        connect(pSocket, &QWebSocket::disconnected, this, &SslEchoServer::socketDisconnected);
        m_clients.removeAll(pSocket);
        m_clients << pSocket;
        pSocket = QtWS::getInstance()->m_pWebSocketServer->nextPendingConnection();
    }
}

/**
 * @brief SslEchoServer::onNewSSLConnection
 */
void SslEchoServer::onNewSSLConnection()
{
    LOG(tr("New SSL connection..."));
    QWebSocket* pSocket;
    pSocket = QtWS::getInstance()->m_pWebSocketServerSSL->nextPendingConnection();
    while (pSocket != nullptr) {
        LOG(QtWS::getInstance()->wsInfo(tr("SSL Client connected: "), pSocket));
        connect(pSocket, &QWebSocket::textMessageReceived, this, &SslEchoServer::processTextMessage);
        connect(pSocket,
            &QWebSocket::binaryMessageReceived,
            this,
            &SslEchoServer::processBinaryMessage);
        connect(pSocket, &QWebSocket::disconnected, this, &SslEchoServer::socketDisconnected);
        connect(pSocket,
            QOverload<const QList<QSslError>&>::of(&QWebSocket::sslErrors),
            QtWS::getInstance(),
            &QtWS::onSslErrors);
        m_clients.removeAll(pSocket);
        m_clients << pSocket;
        pSocket = QtWS::getInstance()->m_pWebSocketServerSSL->nextPendingConnection();
    }
}

/**
 * @brief SslEchoServer::processTextMessage
 * @param message
 */
void SslEchoServer::processTextMessage(QString message)
{
    __processTextMessage(message, "");
}

/**
 * @brief SslEchoServer::__processTextMessage
 * @param message
 * @param channel
 */
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
    if (pClient) {
        if (message == BACKBONE_REGISTRATION_MSG) {
            m_clients.removeAll(pClient);
            m_backbones.removeAll(pClient);
            m_backbones << pClient;
            LOG(QtWS::getInstance()->wsInfo(
                tr("Client identified as another server, moving connection to backbone pool: "),
                pClient));
        } else if (message == CHANNEL_LIST_QUERY_MSG) {
            m_clients.removeAll(pClient);
            m_backbones.removeAll(pClient);
            m_backbones << pClient;
            LOG(QtWS::getInstance()->wsInfo(
                tr("Client identified as another server, moving connection to backbone pool: "),
                pClient));
        } else {
            int ctr = 0, ctr2 = 0;
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
            QString msg;
            msg.append(tr("Distributing event to "))
                .append(QString::number(ctr))
                .append(tr(" clients and "))
                .append(QString::number(ctr2))
                .append(tr(" other servers. Channel: "))
                .append(channel)
                .append(tr(" Message: "))
                .append(message);
            LOG(msg);
        }
    }
    QString msg2;
    msg2.append(tr("Total number of clients: "))
        .append(QString::number(m_clients.count()))
        .append(tr(" Connections to other servers: "))
        .append(QString::number(m_backbones.count()));
    LOG(msg2);
}

/**
 * @brief SslEchoServer::processBinaryMessage
 * @param message
 */
void SslEchoServer::processBinaryMessage(QByteArray message)
{
    __processBinaryMessage(message, "");
}

/**
 * @brief SslEchoServer::__processBinaryMessage
 * @param message
 * @param channel
 */
void SslEchoServer::__processBinaryMessage(QByteArray message, QString channel)
{
    QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
    channel = (channel == "") ? pClient->request().url().path() : channel;
    QByteArray ba;
    QString msg(message);
    if (QtWS::getInstance()->gzipDecompress(message, ba)) {
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
    QtWS::getInstance()->gzipCompress(message, ba, 9);
    QByteArray cMessage(ba);
    QString bbMessage(channel);
    bbMessage.append("\n");
    bbMessage.append(message);
    QtWS::getInstance()->gzipCompress(bbMessage.toUtf8(), ba, 9);
    QByteArray cbbMessage(ba);
    if (pClient) {
        if (message == BACKBONE_REGISTRATION_MSG) {
            m_clients.removeAll(pClient);
            m_backbones.removeAll(pClient);
            m_backbones << pClient;
            LOG(QtWS::getInstance()->wsInfo(
                tr("Client identified as another server, moving connection to backbone pool: "),
                pClient));
        } else if (message == CHANNEL_LIST_QUERY_MSG) {
            for (int i = 0; i < m_backbones.count(); i++) {
                if (pClient != m_backbones[i]) {
                    m_backbones[i]->sendBinaryMessage(CHANNEL_LIST_QUERY_MSG);
                }
            }
            LOG(QtWS::getInstance()->wsInfo(
                tr("Channel query in progress..."),
                pClient));
        } else {
            int ctr = 0, ctr2 = 0;
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
            QString msg;
            msg.append(tr("Distributing event to "))
                .append(QString::number(ctr))
                .append(tr(" clients and "))
                .append(QString::number(ctr2))
                .append(tr(" other servers. Channel: "))
                .append(channel)
                .append(tr(" Message: "))
                .append(message);
            LOG(msg);
        }
    }
    QString msg2;
    msg2.append(tr("Total number of clients: "))
        .append(QString::number(m_clients.count()))
        .append(tr(" Connections to other servers: "))
        .append(QString::number(m_backbones.count()));
    LOG(msg2);
}

/**
 * @brief SslEchoServer::socketDisconnected
 */
void SslEchoServer::socketDisconnected()
{
    QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
    LOG(QtWS::getInstance()->wsInfo(tr("Client disconnected: "), pClient));
    if (pClient) {
        m_clients.removeAll(pClient);
        m_backbones.removeAll(pClient);
        pClient->deleteLater();
    }
}

/**
 * @brief SslEchoServer::onBackboneConnected
 */
void SslEchoServer::onBackboneConnected()
{
    QtWS::getInstance()->m_pWebSocketBackbone->sendTextMessage(BACKBONE_REGISTRATION_MSG);
}

/**
 * @brief SslEchoServer::onBackboneDisconnected
 */
void SslEchoServer::onBackboneDisconnected()
{
    LOG(tr("Backbone disconnected. Restoring connection...."));
    bbResetTimer.stop();
    m_backbones.removeAll(QtWS::getInstance()->m_pWebSocketBackbone);
    QTimer timer(this);
    timer.singleShot(1000, this, SLOT(restoreBackboneConnection()));
}

/**
 * @brief SslEchoServer::restoreBackboneConnection
 */
void SslEchoServer::restoreBackboneConnection()
{
    LOG(tr("Opening new backbone connection...."));
    QtWS::getInstance()->m_pWebSocketBackbone->open(QtWS::getInstance()->secureBackboneUrl(m_sBackbone));
    m_backbones.removeAll(QtWS::getInstance()->m_pWebSocketBackbone);
    m_backbones << QtWS::getInstance()->m_pWebSocketBackbone;
    bbResetTimer.start();
}

/**
 * @brief SslEchoServer::resetBackboneConnection
 */
void SslEchoServer::resetBackboneConnection()
{
    LOG(tr("Preparing for a full restart...."));
    QtWS::getInstance()->stopKeepAliveTimer();
    QtWS::getInstance()->m_pWebSocketBackbone->disconnect();
    QtWS::getInstance()->m_pWebSocketBackbone->close();
    m_backbones.removeAll(QtWS::getInstance()->m_pWebSocketBackbone);
    m_clients.removeAll(QtWS::getInstance()->m_pWebSocketBackbone);
    delete QtWS::getInstance()->m_pWebSocketBackbone;

    for (int i = 0; i < m_backbones.count(); i++) {
        if (m_backbones.at(i) != nullptr)
            m_backbones.at(i)->close();
    }
    for (int i = 0; i < m_clients.count(); i++) {
        if (m_clients.at(i) != nullptr)
            m_clients.at(i)->close();
    }

    QtWS::getInstance()->m_pWebSocketServer->close();
    QtWS::getInstance()->m_pWebSocketServerSSL->close();

    QTimer quitTimer(this);
    quitTimer.singleShot(1000, QtWS::getInstance(), SLOT(quitApplication()));
}

/**
 * @brief SslEchoServer::processTextMessageBB
 * @param message
 */
void SslEchoServer::processTextMessageBB(QString message)
{
    QStringList arr = message.split("\n");
    QString channel = arr[0];
    arr.pop_front();
    message = arr.join("\n");
    __processTextMessage(message, channel);
}

/**
 * @brief SslEchoServer::processBinaryMessageBB
 * @param message
 */
void SslEchoServer::processBinaryMessageBB(QByteArray message)
{
    QByteArray ba;
    QString msg(message);
    if (QtWS::getInstance()->gzipDecompress(message, ba)) {
        msg = ba;
        message = ba;
    }
    QStringList arr = msg.split("\n");
    QString channel = arr[0];
    arr.pop_front();
    msg = arr.join("\n");
    __processBinaryMessage(msg.toUtf8(), channel);
}

/**
 * @brief SslEchoServer::resetBackboneResetTimer
 */
void SslEchoServer::resetBackboneResetTimer()
{
    QWebSocket* pSocket = qobject_cast<QWebSocket*>(sender());
    LOG(QtWS::getInstance()->wsInfo(tr("PONG "), pSocket));
    bbResetTimer.stop();
    bbResetTimer.start();
}
