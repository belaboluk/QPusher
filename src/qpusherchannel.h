#ifndef QPUSHERCHANNEL_H
#define QPUSHERCHANNEL_H

#include <QObject>
#include <QJsonObject>

enum channelType{
    Public,
    Private,
    PrivateEncrypted,
    Presence,
};

class QPusherChannel : public QObject
{
    Q_OBJECT
public:
    explicit QPusherChannel(const QString channelName, QObject *parent = nullptr);
    channelType type();
    QString name();
    bool isCache();
    void dataHandler(const QString& event, const QJsonValue& data);

private:
    channelType m_type;
    const QString m_name;
    bool m_isCache = false;

signals:
    void subscribed(const QString& channelName);
    void newChannelEvent(const QString& eventName, const QJsonValue& data);
};

#endif // QPUSHERCHANNEL_H
