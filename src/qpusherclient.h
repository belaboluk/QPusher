#ifndef QPUSHERCLIENT_H
#define QPUSHERCLIENT_H

#include <QObject>
#include <QUrl>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonValue>
#include <QNetworkRequest>
#include <QMutex>
#include <QMap>

#include "qpusherchannel.h"


class QPusherClient : public QObject
{
    Q_OBJECT

public:
    explicit QPusherClient(const QString& key,
                           const QString& cluster,
                           bool useSSL,
                           QObject *parent = nullptr);
    explicit QPusherClient(const QString& key,
                           const QString& cluster,
                           bool useSSL,
                           const QString& secret,
                           QNetworkRequest authRequest,
                           QObject *parent = nullptr);

    void connect();
    void disconnect();
    QPusherChannel* subscribe(const QString& channelName, QJsonObject* auth=nullptr);
    void unsubscribe(const QString& channelName);
    QPusherChannel* channel(const QString& channelName);

    void setUserData(const QString& userData);
    void setUserData(const QJsonDocument& userData);
    void setUserData(const QJsonValue& userData);
    QJsonValue userData();


    QWebSocket* getSocket();
    bool isConnected();


private:
    bool m_isConnected = false;
    QWebSocket* m_websocket;
    QString m_socketID;

    int m_port;
    QString m_key;
    QString m_host;
    QString m_cluster;
    bool m_useSSL;
    QString m_secret;
    QNetworkRequest m_authRequest;
    QJsonValue m_userData;

    QMap<QString, QPusherChannel*> m_channels;

    QMutex m_authMutex;
    QByteArray jsonValueToByteArray(const QJsonValue& value);


    void sendEvent(const QJsonDocument& data);
    QJsonDocument createJsonEventData(const QString& event, const QJsonDocument& data, const QString& channel="");
    QString getPrivateToken(QPusherChannel* channel);
    QString getPresenceToken(QPusherChannel* channel);

    void sendManualPing();
    void sendManualPong();


private slots:
    void disconnectHandler();
    void dataHandler(const QString &message);


signals:
    void connected();
    void disconnected();
    void unsubscribeChannelMessageReady(const QJsonObject& msg);   // if a message come with a channel name but the channel name is not in m_channels map this signal will be trigered
};

#endif // QPUSHERCLIENT_H
