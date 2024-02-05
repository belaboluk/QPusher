#include <QCoreApplication>
#include <QWebSocket>
#include <QDebug>
#include <QObject>
#include <QTimer>

#include "src/qpusherclient.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QTimer timer;
    timer.setInterval(10000);
    timer.setSingleShot(true);

    QString key = "YOUR CHANNEL KEY";
    QPusherClient client(key, "mt1", false, &a);

    QObject::connect(&timer, &QTimer::timeout, &client, [&](){client.unsubscribe("karim_news");});


    QObject::connect(&client, &QPusherClient::connected, &client, [&](){
        QPusherChannel* channel = client.subscribe("karim_news");
        QObject::connect(channel, &QPusherChannel::subscribed, [=](const QString& name){
            qDebug() << "[SUB] subscribed to channel " << name;
        });
        QObject::connect(channel, &QPusherChannel::newChannelEvent,
                         [=](const QString& eventName, const QJsonValue& data){
            qDebug() << "[EVENT] " << eventName << " >> " << data;
        });

        timer.start();
    });
    client.connect();

    return a.exec();
}
