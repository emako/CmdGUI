#ifndef WS_CLIENT_H
#define WS_CLIENT_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QWebSocket>
#include <QJsonObject>
#include <QHostAddress>

#include "ipc_defines.h"


namespace Ui {
class ws_client;
}

enum class JobState
{
    Unknowned,
    Waiting,
    Running,
    Pausing,
    Paused,
    Aborting,
    Aborted,
    Failed,
    Completed,
};

class ws_client : public QMainWindow
{
    Q_OBJECT

public:
    explicit ws_client(QWidget *parent = 0);
    ~ws_client();
    void connectToServer(const QHostAddress & a_address);
    void slotShutdownServer();
    void ntfModeChange(JobState rv_state);
    QString convertStateToStr(JobState rv_state);

private slots:

    void slotServerConnected();
    void slotServerDisconnected();
    void slotBinaryMessageReceived(const QByteArray & a_message);
    void slotTextMessageReceived(const QString & a_message);

    void on_startBt_clicked();
    void on_pauseBt_clicked();
    void on_abortBt_clicked();

private:
    Ui::ws_client *ui;
    QWebSocket * m_pServerSocket;
    QHostAddress m_nextServerAddress;
    JobState job_state;

protected:
     void closeEvent(QCloseEvent *event);
};

#endif // WS_CLIENT_H
