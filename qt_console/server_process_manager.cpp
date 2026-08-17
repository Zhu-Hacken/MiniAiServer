#include "server_process_manager.h"

#include <QProcess>
#include <QCoreApplication>
#include <QDir>

ServerProcessManager::ServerProcessManager(QObject *parent)
    : QObject(parent), m_process(new QProcess(this))
{
    connect(m_process, &QProcess::started, this, [this]() {
        emit serverStarted();
    });
    connect(m_process, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {   // int exit_code,QProcess::ExitStatus exit_status
        emit serverStopped();
    });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        emit serverError(m_process->errorString());
    });
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        QByteArray data = m_process->readAllStandardOutput();
        emit logReceived(QString::fromLocal8Bit(data));
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        QByteArray data = m_process->readAllStandardError();
        emit logReceived(QString::fromLocal8Bit(data));
    });
}

void ServerProcessManager::startServer() {
    if (m_process->state() != QProcess::NotRunning) {
        return; // Server is already running
    }

    QDir project_dir(QCoreApplication::applicationDirPath());

    project_dir.cdUp(); // Move up to the project root directory
    project_dir.cdUp(); // Move up to the parent directory of the project root: /home/zhu/apps/MiniAiServer

    QString server_path = project_dir.filePath("build/MiniAiServer");

    m_process->setWorkingDirectory(project_dir.absolutePath());
    m_process->start(server_path);
}

void ServerProcessManager::stopServer() {
    if (m_process->state() == QProcess::NotRunning) {
        return; // Server is not running
    }

    m_process->terminate();
}