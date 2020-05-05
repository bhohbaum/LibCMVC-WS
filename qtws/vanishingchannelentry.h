#ifndef VANISHINGCHANNELENTRY_H
#define VANISHINGCHANNELENTRY_H

#include <QObject>

#include "channeltimeoutctrl.h"

class VanishingChannelEntry : public QObject {
    Q_OBJECT

    friend ChannelTimeoutCtrl;

public:
    explicit VanishingChannelEntry(QObject* parent = nullptr);
    ~VanishingChannelEntry();

    QString m_channelName;

public slots:
    void setController(ChannelTimeoutCtrl* controller);
    void vanish();

signals:

private:
    ChannelTimeoutCtrl* m_controller;
};

#endif // VANISHINGCHANNELENTRY_H
