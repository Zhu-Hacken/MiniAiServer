#include "main_window.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include "server_process_manager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_processManager(new ServerProcessManager(this))
{
    setWindowTitle("MiniAiServer Control Center");
    resize(800, 600);
    QWidget *central_widget = new QWidget(this);
    setCentralWidget(central_widget);

    QVBoxLayout *main_layout = new QVBoxLayout(central_widget);

    m_statusLabel = new QLabel("Server Status: Stopped", central_widget);
    m_startButton = new QPushButton("Start Server", central_widget);
    m_stopButton = new QPushButton("Stop Server", central_widget);
    m_stopButton->setEnabled(false); // Initially disabled
    m_logTextEdit = new QTextEdit(central_widget);
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setPlaceholderText("Server logs will appear here...");

    connect(m_processManager, &ServerProcessManager::serverStarted, this, &MainWindow::onServerStarted);
    connect(m_processManager, &ServerProcessManager::serverStopped, this, &MainWindow::onServerStopped);
    connect(m_processManager, &ServerProcessManager::serverError, this, &MainWindow::onServerError);
    connect(m_processManager, &ServerProcessManager::logReceived, this, &MainWindow::onLogReceived);

    main_layout->addWidget(m_statusLabel);
    main_layout->addWidget(m_startButton);
    main_layout->addWidget(m_stopButton);
    main_layout->addWidget(m_logTextEdit);

    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(m_stopButton, &QPushButton::clicked, this, [this]() {
        m_statusLabel->setText("Server Status: Stopping...");
        m_stopButton->setEnabled(false);
        m_processManager->stopServer();
    });
    main_layout->addStretch(); // Add stretch to push the widgets to the top



}

void MainWindow::onStartButtonClicked()
{
    m_statusLabel->setText("Server Status: Starting...");
    m_startButton->setEnabled(false);
    m_processManager->startServer();
}

void MainWindow::onServerStarted()
{
    m_statusLabel->setText("Server Status: Running");
    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
}

void MainWindow::onServerStopped()
{
    m_statusLabel->setText("Server Status: Stopped");
    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
}

void MainWindow::onServerError(const QString &message)
{
    m_statusLabel->setText("Server Status: Failed");
    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
}

void MainWindow::onLogReceived(const QString &message)
{
    m_logTextEdit->append(message);
}