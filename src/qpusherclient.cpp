#include "qpusherclient.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageAuthenticationCode>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>


#define PUSHER_CLIENT_ID "QPusher"
#define PUSHER_CLIENT_VERSION 0.1
#define PUSHER_PROTOCOL 7

QPusherClient::QPusherClient(const QString &key, const QString &cluster, bool useSSL, QObject *parent)
    :QPusherClient(key, cluster, useSSL, "", QNetworkRequest(), parent) {}

QPusherClient::QPusherClient(const QString &key, const QString &cluster, bool useSSL, const QString& secret, QNetworkRequest authRequest, QObject *parent)
    : QObject{parent},
    m_key{key},
    m_cluster{cluster},
    m_useSSL{useSSL},
    m_secret{secret},
    m_authRequest{authRequest}
{
    m_websocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    // creating host url
    if (useSSL)
    {
        m_host.append("wss://");
        m_port = 443;
    }
    else
    {
        m_host.append("ws://");
        m_port = 80;
    }

    m_host.append(QString("ws-%1.pusher.com:%2/app/%3?client=%4&version=%5&protocol=%6")
                       .arg(cluster)
                       .arg(m_port)
                       .arg(key)
                       .arg(PUSHER_CLIENT_ID)
                       .arg(PUSHER_CLIENT_VERSION)
                       .arg(PUSHER_PROTOCOL));

    QObject::connect(m_websocket, &QWebSocket::disconnected, this, &QPusherClient::disconnectHandler);
    QObject::connect(m_websocket, &QWebSocket::textMessageReceived, this, &QPusherClient::dataHandler);

    // QObject::connect(m_websocket, &QWebSocket::textFrameReceived, this, [&](const QString &frame, bool isLastFrame){qDebug() << "text frame: " << frame << " last frame:" << isLastFrame;});
    QObject::connect(m_websocket, &QWebSocket::sslErrors, this, [&](const QList<QSslError> &errors){qDebug() << "ssl errors: " << errors;});
    QObject::connect(m_websocket, &QWebSocket::pong, this, [&](quint64 elapsedTime, const QByteArray &payload){qDebug() << "pong" << payload << " pong time:" << elapsedTime;});
    QObject::connect(m_websocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, [&](QAbstractSocket::SocketError error){qDebug() << "error" << error << "error string" << m_websocket->errorString();});

}

QWebSocket *QPusherClient::getSocket()
{
    return m_websocket;
}

bool QPusherClient::isConnected()
{
    return m_isConnected;
}

QByteArray QPusherClient::jsonValueToByteArray(const QJsonValue &value)
{
    switch (value.type())
    {
        case QJsonValue::Object:
        {
            return QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact);
        }
        case QJsonValue::Array:
        {
            return QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact);
        }
        case QJsonValue::String:
        {
            return value.toString().toUtf8();
        }
        case QJsonValue::Double:
        {
            return QByteArray::number(value.toDouble());
        }
        default:
        {
            qDebug() << "User Data Not Supported";
            return QByteArray();
        }
    }
}

void QPusherClient::sendEvent(const QJsonDocument &data)
{
    qDebug() << "sending :" << data;
    m_websocket->sendTextMessage(QString(data.toJson()));
}

QJsonDocument QPusherClient::createJsonEventData(const QString &event, const QJsonDocument &data, const QString &channel)
{
    QJsonObject json;

    json.insert("event", event);
    if (!channel.isEmpty())
        json.insert("channel", channel);
    if (data.isArray())
        json.insert("data", QJsonValue(data.array()));
    else
        json.insert("data", data.object());
    return QJsonDocument(json);
}

QString QPusherClient::getPrivateToken(QPusherChannel *channel)
{
    if (!m_secret.isEmpty())
    {
        // locally signed
        QByteArray key = m_secret.toUtf8();
        QByteArray msg = QString("%%1:%2").arg(m_socketID, channel->name()).toUtf8();
        QByteArray encodedMsg = QMessageAuthenticationCode::hash(msg, key, QCryptographicHash::Sha256).toHex();
        return QString("%1:%2").arg(m_key, encodedMsg);
    }
    else if (!m_authRequest.url().isEmpty())
    {
        QString authCode;
        QNetworkAccessManager manager(this);
        QObject::connect(&manager, &QNetworkAccessManager::finished, this, [&](QNetworkReply* rep){
            m_authMutex.unlock();
            int httpStatusCode = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (httpStatusCode>=200 && httpStatusCode <300)
            {
                authCode = QJsonDocument::fromJson(rep->readAll()).object()["auth"].toString();
            }

            rep->deleteLater();
        });

        QJsonObject data;
        data.insert("channel_name", channel->name());
        data.insert("socket_id", m_socketID);
        m_authMutex.lock();
        manager.post(m_authRequest, QJsonDocument(data).toJson(QJsonDocument::Compact));
        if (m_authMutex.tryLock(10000))
            m_authMutex.unlock();
        return authCode;
    }
    else
    {
        qDebug() << "this channel is private and need a secret or a auth endpoint for authentication";
    }
    return QString();
}

QString QPusherClient::getPresenceToken(QPusherChannel *channel)
{
    if (!m_secret.isEmpty())
    {
        // locally signed
        QByteArray key = m_secret.toUtf8();
        QByteArray msg = QString("%%1:%2:%3").arg(m_socketID, channel->name(), jsonValueToByteArray(m_userData)).toUtf8();
        QByteArray encodedMsg = QMessageAuthenticationCode::hash(msg, key, QCryptographicHash::Sha256).toHex();
        return QString("%1:%2").arg(m_key, encodedMsg);
    }
    else if (!m_authRequest.url().isEmpty())
    {
        QString authCode;
        QNetworkAccessManager manager(this);
        QObject::connect(&manager, &QNetworkAccessManager::finished, this, [&](QNetworkReply* rep){
            m_authMutex.unlock();
            int httpStatusCode = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (httpStatusCode>=200 && httpStatusCode <300)
            {
                authCode = QJsonDocument::fromJson(rep->readAll()).object()["auth"].toString();
            }

            rep->deleteLater();
        });

        QJsonObject data;
        data.insert("channel_name", channel->name());
        data.insert("socket_id", m_socketID);
        data.insert("user_data", m_userData);
        m_authMutex.lock();
        manager.post(m_authRequest, QJsonDocument(data).toJson(QJsonDocument::Compact));
        if (m_authMutex.tryLock(10000))
            m_authMutex.unlock();
        return authCode;
    }
    else
    {
        qDebug() << "[ERROR] this channel is private and need a secret or a auth endpoint for authentication";
    }
    return QString();
}

void QPusherClient::sendManualPing()
{
    QJsonDocument payload = createJsonEventData("pusher:ping", QJsonDocument());
    sendEvent(payload);
}

void QPusherClient::sendManualPong()
{
    QJsonDocument payload = createJsonEventData("pusher:pong", QJsonDocument());
    sendEvent(payload);
}

void QPusherClient::disconnectHandler()
{
    qDebug() << "disconnected from pusher server.";
    m_isConnected = false;
    emit disconnected();
}

void QPusherClient::dataHandler(const QString &message)
{
    QJsonObject messageJson = QJsonDocument::fromJson(((QString)message).replace("\\\"", "\"").replace("\"{", "{").replace("}\"", "}").toLatin1()).object();
    // QJsonObject messageJson = QJsonDocument::fromJson(((QString)message).replace("\\", "").replace("\"", "").toLatin1()).object();
    // qDebug() << "[SSS" << ((QString)message).replace("\\", "").replace("\"", "").toLatin1();

    if (messageJson.contains("event"))
    {
        QString event = messageJson["event"].toString();

        if (messageJson.contains("channel"))
        {
            // a apesific channel have a new massage so the data will be sended to that channel data handler
            QString channel = messageJson["channel"].toString();
            if (m_channels.contains(channel))
            {
                m_channels[channel]->dataHandler(event, messageJson["data"]);
            }
            else
                emit unsubscribeChannelMessageReady(messageJson);
        }
        else
        {
            if (event == "pusher:connection_established")
            {
                m_socketID = messageJson["data"].toObject()["socket_id"].toString();
                qDebug() << "connected to pusher.";
                m_isConnected = true;
                emit connected();
            }
            else if (event == "pusher:pong")
            {
                sendManualPing();
            }
            else if (event == "pusher:ping")
            {
                sendManualPong();
            }
            else if (event == "pusher:error")
            {
                qDebug() << " afatal error ocured in data handler messenger!!!";
            }
        }
    }
    else
    {
        qDebug() << "LOOL " << messageJson;
    }
}

void QPusherClient::connect()
{
    m_websocket->open(QUrl(m_host));
}

void QPusherClient::disconnect()
{
    m_websocket->close();
}

QPusherChannel* QPusherClient::subscribe(const QString &channelName, QJsonObject* auth)
{
    QJsonObject data;
    data.insert("channel", channelName);

    QPusherChannel* newChannel = new QPusherChannel(channelName, this);
    m_channels.insert(newChannel->name(), newChannel);
    if (auth == nullptr)
    {
        if (newChannel->type() == channelType::Presence)
        {
            data.insert("auth", getPresenceToken(newChannel));
            data.insert("channel_data", QString(jsonValueToByteArray(m_userData)));
        }
        else if (newChannel->type() == channelType::Private)
        {
            data.insert("auth", getPrivateToken(newChannel));
        }
        else if (newChannel->type() == channelType::PrivateEncrypted)
        {
            // TODO: encript the data and send it as private!!!
        }
    }
    else
        data.insert("auth", *auth);

    QJsonDocument msg = createJsonEventData("pusher:subscribe", QJsonDocument(data));
    m_websocket->sendTextMessage(QString(msg.toJson()));
    return newChannel;
}

void QPusherClient::unsubscribe(const QString &channelName)
{
    if (m_channels.contains(channelName))
    {
        QJsonObject data;
        data.insert("channel", channelName);
        QJsonDocument msg = createJsonEventData("pusher:unsubscribe", QJsonDocument(data));
        m_websocket->sendTextMessage(QString(msg.toJson()));
        m_channels[channelName]->deleteLater();
        m_channels.remove(channelName);
        qDebug() << "unsubescribed from channel " << channelName;
    }
    else
    {
        qDebug() << "channel with name " << channelName << " doesn\'t exists in subscribed channels";
    }
}

QPusherChannel *QPusherClient::channel(const QString &channelName)
{
    if (m_channels.contains(channelName))
    {
        return m_channels[channelName];
    }
    return nullptr;
}

void QPusherClient::setUserData(const QString &userData)
{
    m_userData = QJsonValue(userData);
}

void QPusherClient::setUserData(const QJsonDocument &userData)
{
    if (userData.isObject())
        m_userData = QJsonValue(userData.object());
    else if (userData.isArray())
        m_userData = QJsonValue(userData.array());
}

void QPusherClient::setUserData(const QJsonValue &userData)
{
    m_userData = userData;
}

QJsonValue QPusherClient::userData()
{
    return m_userData;
}
