#ifndef SERVER_PROCESS_MANAGER_H
#define SERVER_PROCESS_MANAGER_H

#include <QObject>
#include <QString>

class QProcess;

class ServerProcessManager : public QObject
{
    Q_OBJECT
public:
    explicit ServerProcessManager(QObject *parent = nullptr);
    void startServer();
signals:
    void serverStarted();
    void serverStopped();
    void serverError(const QString &message);
private:
    QProcess *m_process;
};

#endif