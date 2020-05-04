#include "channeltimeoutctrl.h"
#include "vanishingchannelentry.h"

ChannelTimeoutCtrl::ChannelTimeoutCtrl(QObject* parent)
    : QObject(parent)
{
}

ChannelTimeoutCtrl::~ChannelTimeoutCtrl() {}

void ChannelTimeoutCtrl::addChannel(QString channel)
{
    for (int i = 0; i < m_channels.length(); i++) {
        if (m_channels.at(i)->channelName == channel) {
            m_channels.at(i)->deleteLater();
        }
    }
    VanishingChannelEntry* vet = new VanishingChannelEntry();
    vet->setController(this);
    vet->channelName = channel;
}

QStringList ChannelTimeoutCtrl::getBufferedChannels()
{
    QStringList ret;
    for (int i = 0; i < m_channels.length(); i++) {
        ret.append(m_channels.at(i)->channelName);
    }
    return ret;
}
