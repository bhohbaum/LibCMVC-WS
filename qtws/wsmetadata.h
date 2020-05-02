#ifndef WSMETADATA_H
#define WSMETADATA_H

#include <QObject>
#include <QtWebSockets/QWebSocket>

QT_FORWARD_DECLARE_CLASS(QWebSocket)

class WsMetaData : public QObject {
    Q_OBJECT

    friend QWebSocket;

public:
    explicit WsMetaData(QObject* parent = nullptr);
    ~WsMetaData();

    void clearChannels();
    void addChannels(QStringList channels);
    void addChannel(QString channel);
    QStringList getChannels();
    QWebSocket* getWebSocket();
    void setWebSocket(QWebSocket* pSocket);
    bool isBackboneSocket();
    bool isClientSocket();

signals:

private:
    QStringList m_channels;
    QWebSocket* m_webSocket;
};

#endif // WSMETADATA_H
