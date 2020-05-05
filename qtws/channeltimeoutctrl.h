#ifndef CHANNELTIMOUTCTRL_H
#define CHANNELTIMOUTCTRL_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(VanishingChannelEntry)

class ChannelTimeoutCtrl : public QObject {
    Q_OBJECT

    friend VanishingChannelEntry;

public:
    explicit ChannelTimeoutCtrl(QObject* parent = nullptr);
    ~ChannelTimeoutCtrl();

public slots:
    void addChannel(QString channel);
    QStringList getBufferedChannels();

signals:

private:
    QList<VanishingChannelEntry*> m_channels;
};

#endif // CHANNELTIMOUTCTRL_H
