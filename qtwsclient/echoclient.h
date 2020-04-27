#ifndef ECHOCLIENT_H
#define ECHOCLIENT_H

#include "../qtws/qtws.h"
#include <QtCore/QObject>
#include <QtWebSockets/QWebSocket>

class EchoClient : public QObject {
    Q_OBJECT
public:
    explicit EchoClient(const QUrl& url, bool debug = false, bool compression = false, QObject* parent = nullptr);

Q_SIGNALS:
    void closed();

private Q_SLOTS:
    void onConnected();
    void onTextMessageReceived(QString message);
    void onBinaryMessageReceived(QByteArray message);
    void onSslErrors(const QList<QSslError>& errors);

private:
    QWebSocket m_webSocket;
    QUrl m_url;
    bool m_debug;
    bool m_compression;
};

#endif // ECHOCLIENT_H
