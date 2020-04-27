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
    connect(&m_webSocket, &QWebSocket::connected, this, &EchoClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &EchoClient::closed);
    connect(&m_webSocket, QOverload<const QList<QSslError>&>::of(&QWebSocket::sslErrors), this, &EchoClient::onSslErrors);

    m_webSocket.open(QUrl(url));
}
//! [constructor]

//! [onConnected]
void EchoClient::onConnected()
{
    if (m_debug)
        qDebug() << "WebSocket connected";
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &EchoClient::onTextMessageReceived);
    connect(&m_webSocket, &QWebSocket::binaryMessageReceived, this, &EchoClient::onBinaryMessageReceived);
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
        QtWS::gzipCompress(text.toUtf8(), ba, 9);
        m_webSocket.sendBinaryMessage(ba);
    } else {
        m_webSocket.sendTextMessage(text);
    }
}
//! [onConnected]

//! [onTextMessageReceived]
void EchoClient::onTextMessageReceived(QString message)
{
    if (m_debug)
        qDebug() << "Message received:" << message;
    m_webSocket.close();
}
//! [onTextMessageReceived]

void EchoClient::onBinaryMessageReceived(QByteArray message)
{
    QByteArray ba;
    if (QtWS::gzipDecompress(message, ba)) {
        message = ba;
    }
    if (m_debug)
        qDebug() << "Message received:" << message;
    m_webSocket.close();
}

void EchoClient::onSslErrors(const QList<QSslError>& errors)
{
    //Q_UNUSED(errors);

    foreach (QSslError e, errors) {
        qDebug() << "SSL error:" << e.errorString();
    }
    // WARNING: Never ignore SSL errors in production code.
    // The proper way to handle self-signed certificates is to add a custom root
    // to the CA store.

    qDebug() << "Trying to ignore the error(s) and go on....";

    m_webSocket.ignoreSslErrors();
}
