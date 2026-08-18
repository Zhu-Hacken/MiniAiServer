#pragma once

#include <QObject>

class QNetworkAccessManager;
class QTimer;

class ServerStatusClient : public QObject
{
    Q_OBJECT
public:
    explicit ServerStatusClient(QObject *parent = nullptr);
    void requestStatus(int port);
    void startPolling(int port);
    void stopPolling();
private:
    QNetworkAccessManager *m_networkManager;
    QTimer *m_pollTimer; // 负责每隔一段时间触发一次状态请求
    int m_port; // 保存当前要访问的 HTTP 端口
signals:
    void serviceOnline();   // 本次 HTTP 请求成功，服务可访问
    void serviceOffline();  // 本次 HTTP 请求失败，服务不可访问
};