#include "echoclient.h"
#include <QtCore/QDebug>
#include <iostream>

QT_USE_NAMESPACE

//! [constructor]
EchoClient::EchoClient(const QUrl &url, bool debug, QObject *parent) :
    QObject(parent),
    m_url(url),
    m_debug(debug)
{
    if (m_debug) qDebug() << "WebSocket server:" << url;
    connect(&m_webSocket, &QWebSocket::connected, this, &EchoClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &EchoClient::closed);
    connect(&m_webSocket, QOverload<const QList<QSslError>&>::of(&QWebSocket::sslErrors), this, &EchoClient::onSslErrors);

    m_webSocket.open(QUrl(url));
}
//! [constructor]

//! [onConnected]
void EchoClient::onConnected()
{
    if (m_debug) qDebug() << "WebSocket connected";
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &EchoClient::onTextMessageReceived);
    QByteArray content;
#ifdef Q_OS_WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif
    while(!std::cin.eof()) {
        char arr[1024];
        std::cin.read(arr, static_cast<int>(sizeof(arr)));
        int s = std::cin.gcount();
        content.append(arr, s);
    }
    QString text;
    text = text.fromUtf8(content);
    if (m_debug) qDebug() << "Sending message: " << text;
    m_webSocket.sendTextMessage(text);
}
//! [onConnected]

//! [onTextMessageReceived]
void EchoClient::onTextMessageReceived(QString message)
{
    if (m_debug) qDebug() << "Message received:" << message;
    m_webSocket.close();
}
//! [onTextMessageReceived]

void EchoClient::onSslErrors(const QList<QSslError> &errors)
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
