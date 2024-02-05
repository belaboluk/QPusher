#include "qpusherchannel.h"

QPusherChannel::QPusherChannel(const QString channelName, QObject *parent)
    : QObject{parent}, m_name{channelName}
{
    if (channelName.startsWith("private-encrypted-"))
    {
        m_type = channelType::PrivateEncrypted;
        if (channelName.startsWith("private-encrypted-cache-"))
            m_isCache = true;
    }
    else if (channelName.startsWith("private-"))
    {
        m_type = channelType::Private;
        if (channelName.startsWith("private-cache-"))
            m_isCache = true;
    }
    else if (channelName.startsWith("presence-"))
    {
        m_type = channelType::Presence;
        if (channelName.startsWith("presence-cache-"))
            m_isCache = true;
    }
    else
    {
        m_type = channelType::Public;
        if (channelName.startsWith("cache-"))
            m_isCache = true;
    }
}

channelType QPusherChannel::type()
{
    return m_type;
}

QString QPusherChannel::name()
{
    return m_name;
}

bool QPusherChannel::isCache()
{
    return m_isCache;
}

void QPusherChannel::dataHandler(const QString &event, const QJsonValue &data)
{
    if (event == "pusher_internal:subscription_succeeded")
    {
        emit subscribed(m_name);
    }
    else
    {
        emit newChannelEvent(event, data);
    }
}
