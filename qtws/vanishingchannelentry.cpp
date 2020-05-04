#include "vanishingchannelentry.h"

#include <QTimer>

VanishingChannelEntry::VanishingChannelEntry(QObject* parent)
    : QObject(parent)
{
    QTimer timer(this);
    timer.singleShot(5000, this, SLOT(vanish()));
}

VanishingChannelEntry::~VanishingChannelEntry()
{
    m_controller->m_channels.removeAll(this);
}

void VanishingChannelEntry::setController(ChannelTimeoutCtrl* controller)
{
    m_controller = controller;
    m_controller->m_channels.append(this);
}

void VanishingChannelEntry::vanish()
{
    this->deleteLater();
}
