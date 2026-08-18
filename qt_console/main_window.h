#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QString>
#include <QDateTime>

class QLabel;
class QPushButton;
class ServerProcessManager;
class QTextEdit;
class QSpinBox;
class QComboBox;
class QTimer;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QLabel *m_statusLabel;
    QLabel *m_pidLabel;
    QLabel *m_uptimeLabel;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
    QPushButton *m_restartButton;
    QTextEdit *m_logTextEdit;
    QSpinBox *m_httpPortSpinBox;
    QSpinBox *m_workerThreadsSpinBox;
    QComboBox *m_triggerModeComboBox;
    QComboBox *m_actorModelComboBox;
    QTimer *m_uptimeTimer;
    QDateTime m_serverStartTime;

    ServerProcessManager *m_processManager;

    void onStartButtonClicked();
    void onServerStarted();
    void onServerStopped();
    void onServerError(const QString &message);
    void onLogReceived(const QString &message);
    void setLaunchConfigEnabled(bool enabled);
    void updateUptime();
};

#endif