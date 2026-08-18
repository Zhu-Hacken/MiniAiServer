#include "server_process_manager.h"

#include <QProcess>
#include <QCoreApplication>
#include <QDir>

ServerProcessManager::ServerProcessManager(QObject *parent)
    : QObject(parent), m_process(new QProcess(this)), m_restartPending(false)
{
    connect(m_process, &QProcess::started, this, [this]() {
        emit serverStarted();
    });
    connect(m_process, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {   // int exit_code,QProcess::ExitStatus exit_status
        emit serverStopped();

        if (m_restartPending) {
            m_restartPending = false;
            startServer(m_lastArguments);
        }
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

void ServerProcessManager::startServer(const QStringList &arguments) {
    if (m_process->state() != QProcess::NotRunning) {
        return; // Server is already running
    }
    m_lastArguments = arguments; // Store the last used arguments for potential restart
    QDir project_dir(QCoreApplication::applicationDirPath());

    project_dir.cdUp(); // Move up to the project root directory
    project_dir.cdUp(); // Move up to the parent directory of the project root: /home/zhu/apps/MiniAiServer

    QString server_path = project_dir.filePath("build/MiniAiServer");

    m_process->setWorkingDirectory(project_dir.absolutePath());
    m_process->start(server_path, arguments);
}

void ServerProcessManager::stopServer() {
    if (m_process->state() == QProcess::NotRunning) {
        return; // Server is not running
    }

    m_process->terminate();
}

void ServerProcessManager::restartServer() {
    if (m_process->state() == QProcess::NotRunning) {
        startServer(m_lastArguments); 
        return;
    }

    m_restartPending = true;
    stopServer();
}

qint64 ServerProcessManager::processId() const {
    return m_process->processId();
}

bool ServerProcessManager::isRunning() const {
    return m_process->state() != QProcess::NotRunning;
}

bool ServerProcessManager::waitForFinished(int timeout_ms) {
    return m_process->waitForFinished(timeout_ms);
}

void ServerProcessManager::killServer() {
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
    }
}