#ifndef JOB_SERVER_H
#define JOB_SERVER_H

#ifndef WEB_SOCKET_JOB_SERVER_H_INCLUDED
#define WEB_SOCKET_JOB_SERVER_H_INCLUDED

#include <QDebug>
#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <list>

#include <QUuid>
#include <QDateTime>
#include <QProcess>
#include <vector>

class SettingsManagerCore;
class JobsManager;
class QWebSocketServer;
class QWebSocket;
class QHostAddress;

class JobServer : public QObject
{
    Q_OBJECT

public:

    JobServer(QObject * a_pParent = nullptr);
    virtual ~JobServer();

    bool start();

signals:
    void signalLogMessage(const QString & a_message);
    void finish();

private slots:

    void slotNewConnection();
    void slotBinaryMessageReceived(const QByteArray & a_message);
    void slotTextMessageReceived(const QString & a_message);
    void slotSocketDisconnected();
    void slotLogMessage(const QString & a_message);

private:
    void processMessage(QWebSocket * a_pClient, const QString & a_message);
    bool isAllJobCompleted();

    void broadcastMessage(const QString & a_message, bool a_includeNonSubscribers = false, bool a_trustedOnly = false);
    void broadcastMessage(const char * a_message, bool a_includeNonSubscribers = false, bool a_trustedOnly = false);
    void broadcastMessage(const QByteArray & a_message, bool a_includeNonSubscribers = false, bool a_trustedOnly = false);

    QByteArray jsonMessage(const QString & a_command, const QJsonObject & a_jsonObject);
    QByteArray jsonMessage(const QString & a_command, const QJsonArray & a_jsonArray);
    QByteArray jsonMessage(const QString & a_command, const QJsonDocument & a_jsonDocument);
    QByteArray jsonMessage(const QString & a_command, const QString & a_message);

    QWebSocketServer * m_pWebSocketServer;

    std::list<QWebSocket *> m_clients;

    QStringList m_trustedClientsAddresses;

protected:

    void startJob(QString &commandLine);
    void abortJob();
    void pauseJob();
    void resumeJob();
    QProcess m_process;
    QStringList cmds;
    int cmds_indes;

protected slots:

    virtual void slotProcessStarted();
    virtual void slotProcessFinished(int a_exitCode, QProcess::ExitStatus a_exitStatus);
    virtual void slotProcessError(QProcess::ProcessError a_error);
    virtual void slotProcessReadyReadStandardError();
};

#endif // WEB_SOCKET_JOB_SERVER_H_INCLUDED


#endif // JOB_SERVER_H
