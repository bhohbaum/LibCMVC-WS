#include "wsmetadata.h"
#include "qtws.h"

WsMetaData::WsMetaData(QObject* parent)
    : QObject(parent)
{
    QtWS::getInstance()->m_wsMetaDataList.append(this);
    clearChannels();
    connect(QtWS::getInstance(), SIGNAL(updateChannels()), this, SLOT(updateChannelAnnouncement()));
    connect(QtWS::getInstance(), SIGNAL(forceUpdateChannels()), this, SLOT(forceChannelAnnouncement()));
}

WsMetaData::~WsMetaData()
{
    QtWS::getInstance()->m_wsMetaDataList.removeAll(this);
}

void WsMetaData::clearChannels()
{
    m_channels.clear();
}

void WsMetaData::addChannels(QStringList channels)
{
    for (int i = 0; i < channels.length(); i++) {
        if (!m_channels.contains(channels.at(i))) {
            if (isValidChannelName(channels.at(i))) {
                m_channels.append(channels.at(i));
            }
        }
    }
}

void WsMetaData::addChannel(QString channel)
{
    if (!m_channels.contains(channel)) {
        if (isValidChannelName(channel)) {
            m_channels.append(channel);
        }
    }
}

QStringList WsMetaData::getChannels()
{
    return QStringList(m_channels);
}

QWebSocket* WsMetaData::getWebSocket()
{
    return m_webSocket;
}

void WsMetaData::setWebSocket(QWebSocket* pSocket)
{
    m_webSocket = pSocket;
    connect(m_webSocket, SIGNAL(connected()), this, SLOT(updateRequestUrlFromSocket()));
}

bool WsMetaData::isBackboneSocket()
{
    return QtWS::getInstance()->m_backbones.contains(getWebSocket());
}

bool WsMetaData::isClientSocket()
{
    return QtWS::getInstance()->m_clients.contains(getWebSocket());
}

QStringList WsMetaData::calcChannelsForConnection()
{
    WsMetaData tmpWmd;
    for (int i = 0; i < QtWS::getInstance()->m_wsMetaDataList.length(); i++) {
        if (QtWS::getInstance()->m_wsMetaDataList.at(i) != this) {
            tmpWmd.addChannels(QtWS::getInstance()->m_wsMetaDataList.at(i)->getChannels());
        }
    }
    tmpWmd.addChannels(QtWS::getInstance()->m_channelTimeoutCtrl.getBufferedChannels());
    QStringList ret = tmpWmd.getChannels();
    ret.sort(Qt::CaseSensitivity::CaseSensitive);
    return ret;
}

void WsMetaData::updateChannelAnnouncement()
{
    if (isBackboneSocket()) {
        QStringList newChannelsArr = calcChannelsForConnection();
        QString newChannelsMsg = newChannelsArr.join(" ").trimmed();
        if (newChannelsMsg != oldChannelListString) {
            oldChannelListString = newChannelsMsg;
            newChannelsMsg.prepend(" ").prepend(CHANNEL_LIST_NOTIFICATION);
            QString bbMessage("/bb-event");
            bbMessage.append("\n");
            bbMessage.append(newChannelsMsg);
            getWebSocket()->sendTextMessage(bbMessage);
            LOG(QtWS::getInstance()
                    ->wsInfo(tr("Channel list changed for this connnection: "), getWebSocket())
                    .append(": ")
                    .append(oldChannelListString));
        }
    }
}

void WsMetaData::forceChannelAnnouncement()
{
    oldChannelListString = " /// ";
    updateChannelAnnouncement();
}

bool WsMetaData::isValidChannelName(QString name)
{
    return name != BACKBONE_REGISTRATION_MSG && name != CHANNEL_LIST_NOTIFICATION && name != "/";
}

QString WsMetaData::getRequestUrl()
{
    return m_requestUrl;
}

void WsMetaData::updateRequestUrlFromSocket()
{
    m_requestUrl = m_webSocket->request().url().toString();
}
