#include "main_window.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QDebug>
#include <QTimer>

#include "server_process_manager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_processManager(new ServerProcessManager(this)), m_uptimeTimer(new QTimer(this))
{
    setWindowTitle("MiniAiServer Control Center");
    resize(800, 600);
    QWidget *central_widget = new QWidget(this);
    setCentralWidget(central_widget);

    QVBoxLayout *main_layout = new QVBoxLayout(central_widget);

    m_statusLabel = new QLabel("Server Status: Stopped", central_widget);
    m_pidLabel = new QLabel("PID: -", central_widget);

    m_uptimeLabel = new QLabel("Uptime: 00:00:00", central_widget);
    m_uptimeTimer->setInterval(1000); // Update every second

    m_startButton = new QPushButton("Start Server", central_widget);
    
    m_stopButton = new QPushButton("Stop Server", central_widget);
    m_stopButton->setEnabled(false); // Initially disabled
    
    m_restartButton = new QPushButton("Restart Server", central_widget);
    m_restartButton->setEnabled(false); // Initially disabled

    m_logTextEdit = new QTextEdit(central_widget);
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setPlaceholderText("Server logs will appear here...");
    
    m_httpPortSpinBox = new QSpinBox(central_widget);
    m_httpPortSpinBox->setRange(1, 65535);
    m_httpPortSpinBox->setValue(9006);
    
    m_workerThreadsSpinBox = new QSpinBox(central_widget);
    m_workerThreadsSpinBox->setRange(1, 64);
    m_workerThreadsSpinBox->setValue(8);

    m_triggerModeComboBox = new QComboBox(central_widget);
    m_triggerModeComboBox->addItem("LT");
    m_triggerModeComboBox->addItem("ET");

    m_actorModelComboBox = new QComboBox(central_widget);
    m_actorModelComboBox->addItem("Proactor");
    m_actorModelComboBox->addItem("Reactor");
    m_actorModelComboBox->setCurrentIndex(1);

    connect(m_processManager, &ServerProcessManager::serverStarted, this, &MainWindow::onServerStarted);
    connect(m_processManager, &ServerProcessManager::serverStopped, this, &MainWindow::onServerStopped);
    connect(m_processManager, &ServerProcessManager::serverError, this, &MainWindow::onServerError);
    connect(m_processManager, &ServerProcessManager::logReceived, this, &MainWindow::onLogReceived);

    main_layout->addWidget(m_statusLabel);
    main_layout->addWidget(m_pidLabel);
    main_layout->addWidget(m_uptimeLabel);
    main_layout->addWidget(m_startButton);
    main_layout->addWidget(m_stopButton);
    main_layout->addWidget(m_restartButton);
    main_layout->addWidget(new QLabel("HTTP Port", central_widget));
    main_layout->addWidget(m_httpPortSpinBox);

    main_layout->addWidget(new QLabel("Worker Threads", central_widget));
    main_layout->addWidget(m_workerThreadsSpinBox);

    main_layout->addWidget(new QLabel("Trigger Mode", central_widget));
    main_layout->addWidget(m_triggerModeComboBox);

    main_layout->addWidget(new QLabel("Actor Model", central_widget));
    main_layout->addWidget(m_actorModelComboBox);

    main_layout->addWidget(m_logTextEdit);



    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(m_stopButton, &QPushButton::clicked, this, [this]() {
        m_statusLabel->setText("Server Status: Stopping...");
        m_stopButton->setEnabled(false);
        m_processManager->stopServer();
    });
    connect(m_restartButton, &QPushButton::clicked, this, [this]() {
        m_statusLabel->setText("Server Status: Restarting...");
        m_stopButton->setEnabled(false);
        m_restartButton->setEnabled(false);
        m_processManager->restartServer();
    });
    connect(m_uptimeTimer, &QTimer::timeout, this, &MainWindow::updateUptime);
    main_layout->addStretch(); // Add stretch to push the widgets to the top



}

void MainWindow::onStartButtonClicked()
{
    m_statusLabel->setText("Server Status: Starting...");
    m_startButton->setEnabled(false);

    QStringList arguments;

    arguments << "-p" << QString::number(m_httpPortSpinBox->value())
              << "-t" << QString::number(m_workerThreadsSpinBox->value())
              << "-m" << QString::number(m_triggerModeComboBox->currentIndex())
              << "-a" << QString::number(m_actorModelComboBox->currentIndex());

    qDebug() << "Arguments:" << arguments;

    m_processManager->startServer(arguments);
}

void MainWindow::onServerStarted()
{
    m_statusLabel->setText("Server Status: Running");
    m_pidLabel->setText(QString("PID: %1").arg(m_processManager->processId()));
    m_startButton->setEnabled(false);
    setLaunchConfigEnabled(false);
    m_stopButton->setEnabled(true);
    m_restartButton->setEnabled(true);
    m_serverStartTime = QDateTime::currentDateTime();
    m_uptimeTimer->start();
}

void MainWindow::onServerStopped()
{
    m_statusLabel->setText("Server Status: Stopped");
    m_pidLabel->setText("PID: -");
    m_startButton->setEnabled(true);
    setLaunchConfigEnabled(true);
    m_stopButton->setEnabled(false);
    m_restartButton->setEnabled(false);
    m_uptimeTimer->stop();
    m_uptimeLabel->setText("Uptime: 00:00:00");
}

void MainWindow::onServerError(const QString &message)
{
    m_statusLabel->setText("Server Status: Failed");
    m_pidLabel->setText("PID: -");
    m_startButton->setEnabled(true);
    setLaunchConfigEnabled(true);
    m_stopButton->setEnabled(false);
    m_restartButton->setEnabled(false);
    m_uptimeTimer->stop();
    m_uptimeLabel->setText("Uptime: 00:00:00");
}

void MainWindow::onLogReceived(const QString &message)
{
    m_logTextEdit->append(message);
}

void MainWindow::setLaunchConfigEnabled(bool enabled) {
    m_httpPortSpinBox->setEnabled(enabled);
    m_workerThreadsSpinBox->setEnabled(enabled);
    m_triggerModeComboBox->setEnabled(enabled);
    m_actorModelComboBox->setEnabled(enabled);
}

void MainWindow::updateUptime() {
    qint64 elapsed_seconds = m_serverStartTime.secsTo(QDateTime::currentDateTime());

    int hours = elapsed_seconds / 3600;
    int minutes = (elapsed_seconds % 3600) / 60;
    int seconds = elapsed_seconds % 60;

    m_uptimeLabel->setText(QString("Uptime: %1:%2:%3")
                            .arg(hours, 2, 10, QChar('0'))
                            .arg(minutes, 2, 10, QChar('0'))
                            .arg(seconds, 2, 10, QChar('0')));

}