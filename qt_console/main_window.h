#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QString>

class QLabel;
class QPushButton;
class ServerProcessManager;
class QTextEdit;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QLabel *m_statusLabel;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
    QTextEdit *m_logTextEdit;

    ServerProcessManager *m_processManager;

    void onStartButtonClicked();
    void onServerStarted();
    void onServerStopped();
    void onServerError(const QString &message);
    void onLogReceived(const QString &message);
};

#endif