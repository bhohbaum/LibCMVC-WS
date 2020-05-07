#include "echoclient.h"
#include <QtCore/QDebug>
#include <iostream>

QT_USE_NAMESPACE

/**
 * @brief EchoClient::EchoClient
 * @param url
 * @param debug
 * @param compression
 * @param parent
 */
EchoClient::EchoClient(const QUrl& url, bool debug, bool compression, QObject* parent)
    : QObject(parent)
    , m_url(url)
    , m_debug(debug)
    , m_compression(compression)
{
    if (m_debug) {
        QString msg(tr("WebSocket server: "));
        msg.append(url.toString());
        LOG(msg);
    }
    if (QtWS::getInstance()->m_pWebSocketBackbone.count() < 1) {
        QtWS::getInstance()->m_pWebSocketBackbone.append(new QWebSocket());
    }
    connect(QtWS::getInstance()->m_pWebSocketBackbone[0], &QWebSocket::connected, this, &EchoClient::onConnected);
    connect(QtWS::getInstance()->m_pWebSocketBackbone[0], &QWebSocket::disconnected, this, &EchoClient::closed);
    connect(QtWS::getInstance()->m_pWebSocketBackbone[0], QOverload<const QList<QSslError>&>::of(&QWebSocket::sslErrors), QtWS::getInstance(), &QtWS::onSslErrors);

    QtWS::getInstance()->m_pWebSocketBackbone[0]->open(QUrl(url));
}

/**
 * @brief EchoClient::onConnected
 */
void EchoClient::onConnected()
{
    if (m_debug) {
        LOG(tr("WebSocket connected"));
    }
    connect(QtWS::getInstance()->m_pWebSocketBackbone[0], &QWebSocket::textMessageReceived, this, &EchoClient::onTextMessageReceived);
    connect(QtWS::getInstance()->m_pWebSocketBackbone[0], &QWebSocket::binaryMessageReceived, this, &EchoClient::onBinaryMessageReceived);
    QByteArray content;
#ifdef Q_OS_WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif
    while (!std::cin.eof()) {
        char arr[1024];
        std::cin.read(arr, static_cast<int>(sizeof(arr)));
        int s = std::cin.gcount();
        content.append(arr, s);
    }
    QString text;
    text = text.fromUtf8(content);
    QString msgComp;
    if (m_compression)
        msgComp = tr(" (compressed, binary)");
    if (m_debug) {
        QString msg(tr("Sending message"));
        msg.append(msgComp).append(": ").append(text);
        LOG(msg);
    }
    if (m_compression) {
        QByteArray ba;
        QtWS::getInstance()->gzipCompress(text.toUtf8(), ba, 9);
        QtWS::getInstance()->m_pWebSocketBackbone[0]->sendBinaryMessage(ba);
    } else {
        QtWS::getInstance()->m_pWebSocketBackbone[0]->sendTextMessage(text);
    }
    if (text.startsWith(BACKBONE_REGISTRATION_MSG) || text.startsWith(CHANNEL_LIST_NOTIFICATION) || m_debug == false) {
        QtWS::getInstance()->m_pWebSocketBackbone[0]->flush();
        emit closed();
    }
}

/**
 * @brief EchoClient::onTextMessageReceived
 * @param message
 */
void EchoClient::onTextMessageReceived(QString message)
{
    if (m_debug) {
        QString msg(tr("Message received: "));
        msg.append(message);
        LOG(msg);
    }
    QtWS::getInstance()->m_pWebSocketBackbone[0]->close();
}

/**
 * @brief EchoClient::onBinaryMessageReceived
 * @param message
 */
void EchoClient::onBinaryMessageReceived(QByteArray message)
{
    QByteArray ba;
    if (QtWS::getInstance()->gzipDecompress(message, ba)) {
        message = ba;
    }
    if (m_debug) {
        QString msg(tr("Message received: "));
        msg.append(message);
        LOG(msg);
    }
    QtWS::getInstance()->m_pWebSocketBackbone[0]->close();
}
