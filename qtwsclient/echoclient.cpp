#include "echoclient.h"
#include <QtCore/QDebug>
#include <iostream>

QT_USE_NAMESPACE

//! [constructor]
EchoClient::EchoClient(const QUrl& url, bool debug, bool compression, QObject* parent)
    : QObject(parent)
    , m_url(url)
    , m_debug(debug)
    , m_compression(compression)
{
    if (m_debug)
        qDebug() << "WebSocket server:" << url;
    //m_qtws.m_pWebSocketBackbone = new QWebSocket();
    connect(m_qtws.m_pWebSocketBackbone, &QWebSocket::connected, this, &EchoClient::onConnected);
    connect(m_qtws.m_pWebSocketBackbone, &QWebSocket::disconnected, this, &EchoClient::closed);
    connect(m_qtws.m_pWebSocketBackbone, QOverload<const QList<QSslError>&>::of(&QWebSocket::sslErrors), &m_qtws, &QtWS::onSslErrors);

    m_qtws.m_pWebSocketBackbone->open(QUrl(url));
}
//! [constructor]

//! [onConnected]
void EchoClient::onConnected()
{
    if (m_debug)
        qDebug() << "WebSocket connected";
    connect(m_qtws.m_pWebSocketBackbone, &QWebSocket::textMessageReceived, this, &EchoClient::onTextMessageReceived);
    connect(m_qtws.m_pWebSocketBackbone, &QWebSocket::binaryMessageReceived, this, &EchoClient::onBinaryMessageReceived);
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
        msgComp = "(compressed, binary)";
    if (m_debug)
        qDebug() << "Sending message:" << msgComp << text;
    if (m_compression) {
        QByteArray ba;
        m_qtws.gzipCompress(text.toUtf8(), ba, 9);
        m_qtws.m_pWebSocketBackbone->sendBinaryMessage(ba);
    } else {
        m_qtws.m_pWebSocketBackbone->sendTextMessage(text);
    }
}
//! [onConnected]

//! [onTextMessageReceived]
void EchoClient::onTextMessageReceived(QString message)
{
    if (m_debug)
        qDebug() << "Message received:" << message;
    m_qtws.m_pWebSocketBackbone->close();
}
//! [onTextMessageReceived]

void EchoClient::onBinaryMessageReceived(QByteArray message)
{
    QByteArray ba;
    if (m_qtws.gzipDecompress(message, ba)) {
        message = ba;
    }
    if (m_debug)
        qDebug() << "Message received:" << message;
    m_qtws.m_pWebSocketBackbone->close();
}
