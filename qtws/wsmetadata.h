#ifndef WSMETADATA_H
#define WSMETADATA_H

#include <QObject>
#include <QtWebSockets/QWebSocket>

QT_FORWARD_DECLARE_CLASS(ExtWebSocket)

class WsMetaData : public QObject {
    Q_OBJECT

    friend ExtWebSocket;

public:
    explicit WsMetaData(QObject* parent = nullptr);
    void addChannels(QList<QString> channels);
    void addChannel(QString channel);
    QList<QString> getChannels();
    ExtWebSocket* getWebSocket();

signals:

private:
    void setWebSocket(ExtWebSocket* pSocket);

    QList<QString> m_channels;
};

#endif // WSMETADATA_H
