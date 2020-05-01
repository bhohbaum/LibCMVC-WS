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
            QtWS::getInstance()->log(tr("Error opening SSL certificate."));
            return;
        }
        if (!keyFile.open(QIODevice::ReadOnly)) {
            QtWS::getInstance()->log(tr("Error opening SSL key."));
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
        QtWS::getInstance()->m_pWebSocketServerSSL->setSslConfiguration(sslConfiguration);
        if (sslPort != 0) {
            if (QtWS::getInstance()->m_pWebSocketServerSSL->listen(QHostAddress::Any, sslPort)) {
                QtWS::getInstance()->log(tr("WS PubSub SSL Server listening on port ").append(QString::number(sslPort)));
                connect(QtWS::getInstance()->m_pWebSocketServerSSL,
                    &QWebSocketServer::newConnection,
                    this,
                    &SslEchoServer::onNewSSLConnection);
                connect(QtWS::getInstance()->m_pWebSocketServerSSL,
                    &QWebSocketServer::sslErrors,
                    QtWS::getInstance(),
                    &QtWS::onSslErrors);
            } else {
                QtWS::getInstance()->log(tr("Error binding SSL socket!!"));
                return;
            }
        }
    }
    if (port != 0) {
        if (QtWS::getInstance()->m_pWebSocketServer->listen(QHostAddress::Any, port)) {
            QtWS::getInstance()->log(tr("WS PubSub Server listening on port ").append(QString::number(port)));
            connect(QtWS::getInstance()->m_pWebSocketServer,
                &QWebSocketServer::newConnection,
                this,
                &SslEchoServer::onNewConnection);
        } else {
            QtWS::getInstance()->log(tr("Error binding PLAIN socket!!"));
            return;
        }
    }
    if (bbUrl != "") {
        m_sBackbone = bbUrl;
        //QtWS::getInstance()->m_pWebSocketBackbone = new QWebSocket();
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
        //m_pWebSocketBackbone->open(m_sBackbone);
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

SslEchoServer::~SslEchoServer()
{
    QtWS::getInstance()->m_pWebSocketServer->close();
    QtWS::getInstance()->m_pWebSocketServerSSL->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
    qDeleteAll(m_backbones.begin(), m_backbones.end());
}

void SslEchoServer::onNewConnection()
{
    QtWS::getInstance()->log(tr("New PLAIN connection..."));
    QWebSocket* pSocket;
    pSocket = QtWS::getInstance()->m_pWebSocketServer->nextPendingConnection();
    while (pSocket != nullptr) {
        QtWS::getInstance()->log(QtWS::getInstance()->wsInfo(tr("PLAIN Client connected: "), pSocket));
        connect(pSocket, &QWebSocket::textMessageReceived, this, &SslEchoServer::processTextMessage);
        connect(pSocket, &QWebSocket::binaryMessageReceived, this, &SslEchoServer::processBinaryMessage);
        connect(pSocket, &QWebSocket::disconnected, this, &SslEchoServer::socketDisconnected);
        m_clients.removeAll(pSocket);
        m_clients << pSocket;
        //m_channels[pSocket->request().url().path()].append(pSocket);
        pSocket = QtWS::getInstance()->m_pWebSocketServer->nextPendingConnection();
    }
}

void SslEchoServer::onNewSSLConnection()
{
    qDebug() << "New SSL connection...";
    QWebSocket* pSocket;
    pSocket = QtWS::getInstance()->m_pWebSocketServerSSL->nextPendingConnection();
    while (pSocket != nullptr) {
        QtWS::getInstance()->log(QtWS::getInstance()->wsInfo(tr("SSL Client connected: "), pSocket));
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
        //m_channels[pSocket->request().url().path()].append(pSocket);
        pSocket = QtWS::getInstance()->m_pWebSocketServerSSL->nextPendingConnection();
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
            QtWS::getInstance()->log(QtWS::getInstance()->wsInfo(
                tr("Client identified as another server, moving connection to backbone pool: "),
                pClient));
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
        QString msg;
        msg.append(tr("Distributing event to "))
            .append(QString::number(ctr))
            .append(tr(" clients and "))
            .append(QString::number(ctr2))
            .append(tr(" other servers. Channel: "))
            .append(channel)
            .append(tr(" Message: "))
            .append(message);
        QtWS::getInstance()->log(msg);
    }
    QString msg2;
    msg2.append(tr("Total number of clients: "))
        .append(QString::number(m_clients.count()))
        .append(tr(" Connections to other servers: "))
        .append(QString::number(m_backbones.count()));
    QtWS::getInstance()->log(msg2);
}

void SslEchoServer::processBinaryMessage(QByteArray message)
{
    __processBinaryMessage(message, "");
}

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
    if (message == BACKBONE_REGISTRATION_MSG) {
        if (pClient) {
            m_clients.removeAll(pClient);
            m_backbones.removeAll(pClient);
            m_backbones << pClient;
            QtWS::getInstance()->log(QtWS::getInstance()->wsInfo(
                tr("Client identified as another server, moving connection to backbone pool: "),
                pClient));
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
        QString msg;
        msg.append(tr("Distributing event to "))
            .append(QString::number(ctr))
            .append(tr(" clients and "))
            .append(QString::number(ctr2))
            .append(tr(" other servers. Channel: "))
            .append(channel)
            .append(tr(" Message: "))
            .append(message);
        QtWS::getInstance()->log(msg);
    }
    QString msg2;
    msg2.append(tr("Total number of clients: "))
        .append(QString::number(m_clients.count()))
        .append(tr(" Connections to other servers: "))
        .append(QString::number(m_backbones.count()));
    QtWS::getInstance()->log(msg2);
}

void SslEchoServer::socketDisconnected()
{
    QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
    QtWS::getInstance()->log(QtWS::getInstance()->wsInfo(tr("Client disconnected: "), pClient));
    if (pClient) {
        m_clients.removeAll(pClient);
        //m_channels.find(pClient->request().url().path())->removeAll(pClient);
        m_backbones.removeAll(pClient);
        pClient->deleteLater();
    }
}

void SslEchoServer::onBackboneConnected()
{
    QtWS::getInstance()->m_pWebSocketBackbone->sendTextMessage(BACKBONE_REGISTRATION_MSG);
}

void SslEchoServer::onBackboneDisconnected()
{
    QtWS::getInstance()->log(tr("Backbone disconnected. Restoring connection...."));
    bbResetTimer.stop();
    m_backbones.removeAll(QtWS::getInstance()->m_pWebSocketBackbone);
    QTimer* timer = new QTimer(this);
    timer->singleShot(1000, this, SLOT(restoreBackboneConnection()));
}

void SslEchoServer::restoreBackboneConnection()
{
    QtWS::getInstance()->log(tr("Opening new backbone connection...."));
    //m_pWebSocketBackbone->open(m_sBackbone);
    QtWS::getInstance()->m_pWebSocketBackbone->open(QtWS::getInstance()->secureBackboneUrl(m_sBackbone));
    m_backbones.removeAll(QtWS::getInstance()->m_pWebSocketBackbone);
    m_backbones << QtWS::getInstance()->m_pWebSocketBackbone;
    bbResetTimer.start();
}

void SslEchoServer::resetBackboneConnection()
{
    QtWS::getInstance()->log(tr("Doing a full restart...."));
    QtWS::getInstance()->m_pWebSocketBackbone->disconnect();
    QtWS::getInstance()->m_pWebSocketBackbone->close();
    m_backbones.removeAll(QtWS::getInstance()->m_pWebSocketBackbone);
    m_clients.removeAll(QtWS::getInstance()->m_pWebSocketBackbone);
    delete QtWS::getInstance()->m_pWebSocketBackbone;

    for (int i = 0; i < m_backbones.count(); i++) {
        m_backbones.at(i)->close();
    }
    for (int i = 0; i < m_clients.count(); i++) {
        m_clients.at(i)->close();
    }

    QtWS::getInstance()->m_pWebSocketServer->close();
    QtWS::getInstance()->m_pWebSocketServerSSL->close();

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

void SslEchoServer::resetBackboneResetTimer()
{
    QtWS::getInstance()->log(tr("PONG"));
    bbResetTimer.stop();
    bbResetTimer.start();
}
