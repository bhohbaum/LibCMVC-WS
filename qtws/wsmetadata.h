#ifndef WSMETADATA_H
#define WSMETADATA_H

#include <QObject>
#include <QtWebSockets/QWebSocket>

class WsMetaData : public QObject {
    Q_OBJECT

    friend QWebSocket;

public:
    explicit WsMetaData(QObject* parent = nullptr);
    ~WsMetaData();

public slots:
    void clearChannels();
    void addChannels(QStringList channels);
    void addChannel(QString channel);
    QStringList getChannels();
    QWebSocket* getWebSocket();
    void setWebSocket(QWebSocket* pSocket);
    bool isBackboneSocket();
    bool isClientSocket();
    QStringList calcChannelsForConnection();
    void updateChannelAnnouncement();
    void forceChannelAnnouncement();
    bool isValidChannelName(QString name);

signals:

private:
    QStringList m_channels;
    QWebSocket* m_webSocket;
    QString oldChannelListString;
};

#endif // WSMETADATA_H
