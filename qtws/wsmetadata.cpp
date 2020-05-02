#include "wsmetadata.h"
#include "qtws.h"

WsMetaData::WsMetaData(QObject* parent)
    : QObject(parent)
{
    LOG(tr("Adding new MetaData element to list."));
    QtWS::getInstance()->m_wsMetaDataList.append(this);
}

WsMetaData::~WsMetaData()
{
    LOG(tr("Removing old MetaData element from list."));
    QtWS::getInstance()->m_wsMetaDataList.removeAll(this);
}

void WsMetaData::clearChannels()
{
    m_channels.clear();
}

void WsMetaData::addChannels(QStringList channels)
{
    for (int i = 0; i < channels.length(); i++) {
        if (!m_channels.contains(channels.at(i)))
            m_channels.append(channels.at(i));
    }
}

void WsMetaData::addChannel(QString channel)
{
    if (!m_channels.contains(channel))
        m_channels.append(channel);
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
}

bool WsMetaData::isBackboneSocket()
{
    return QtWS::getInstance()->m_backbones.contains(getWebSocket());
}

bool WsMetaData::isClientSocket()
{
    return QtWS::getInstance()->m_clients.contains(getWebSocket());
}
