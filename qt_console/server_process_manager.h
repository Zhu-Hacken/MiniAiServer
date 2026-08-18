#ifndef SERVER_PROCESS_MANAGER_H
#define SERVER_PROCESS_MANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>

class QProcess;

class ServerProcessManager : public QObject
{
    Q_OBJECT
public:
    explicit ServerProcessManager(QObject *parent = nullptr);
    void startServer(const QStringList &arguments);
    void stopServer();
    void restartServer();  
    qint64 processId() const;
signals:
    void serverStarted();
    void serverStopped();
    void serverError(const QString &message);
    void logReceived(const QString &message);
private:
    QProcess *m_process;
    bool m_restartPending;  // 是否正在等待“退出后重启”
    QStringList m_lastArguments;    // 上一次启动 MiniAiServer 使用的参数
};

#endif