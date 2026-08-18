#include "server_status_client.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QNetworkReply>
#include <QDebug>
#include <QTimer>

ServerStatusClient::ServerStatusClient(QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)), m_pollTimer(new QTimer(this)), m_port(0)
{
    m_pollTimer->setInterval(2000); // 每隔 2 秒触发一次状态请求
    connect(m_pollTimer, &QTimer::timeout, this, [this]() {
        requestStatus(m_port);
    });
}

void ServerStatusClient::requestStatus(int port) {
    QString url_string = QString("http://192.168.132.131:%1/api/hello").arg(port);
    QNetworkRequest request{QUrl(url_string)};

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        bool network_ok = reply->error() == QNetworkReply::NoError;

        bool http_ok = (status_code >= 200 && status_code < 300);

        if (network_ok && http_ok) {
            emit serviceOnline();
        } else {
            emit serviceOffline();
        }
        
        // QByteArray data = reply->readAll();
        reply->deleteLater();
    });

}

void ServerStatusClient::startPolling(int port) {
    m_port = port;
    requestStatus(port); // 立即请求一次状态
    m_pollTimer->start();
}

void ServerStatusClient::stopPolling() {
    m_pollTimer->stop();
}