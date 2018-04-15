#include "job_server.h"
#include "ipc_defines.h"

#include <QWebSocketServer>
#include <QWebSocket>

#include <assert.h>
#ifdef Q_OS_WIN
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <signal.h>
#endif

JobServer::JobServer(QObject * a_pParent) : QObject(a_pParent)
        , m_pWebSocketServer(nullptr)
{
    cmds_indes = 0;
    m_pWebSocketServer = new QWebSocketServer(JOB_SERVER_NAME, QWebSocketServer::NonSecureMode, this);
    connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &JobServer::slotNewConnection);
    connect(this, SIGNAL(signalLogMessage(const QString &)), this, SLOT(slotLogMessage(const QString &)));
    connect(&m_process, SIGNAL(started()),this, SLOT(slotProcessStarted()));
    connect(&m_process, SIGNAL(finished(int, QProcess::ExitStatus)),this, SLOT(slotProcessFinished(int, QProcess::ExitStatus)));
    connect(&m_process, SIGNAL(error(QProcess::ProcessError)),this, SLOT(slotProcessError(QProcess::ProcessError)));
    connect(&m_process, SIGNAL(readyReadStandardError()), this, SLOT(slotProcessReadyReadStandardError()));
}

JobServer::~JobServer()
{
    for(QWebSocket * pClient : m_clients)
    {
        disconnect(pClient, &QWebSocket::disconnected, this, &JobServer::slotSocketDisconnected);
        delete pClient;
    }
    m_clients.clear();
    m_pWebSocketServer->close();
}

bool JobServer::start()
{
    assert(m_pWebSocketServer);
    return m_pWebSocketServer->listen(QHostAddress::Any, JOB_SERVER_PORT);
}

void JobServer::slotLogMessage(const QString & a_message)
{
    broadcastMessage(jsonMessage(SMSG_LOG_MESSAGE, a_message));
}

void JobServer::slotNewConnection()
{
    QWebSocket * pSocket = m_pWebSocketServer->nextPendingConnection();
    m_clients.push_back(pSocket);

    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &JobServer::slotBinaryMessageReceived);
    connect(pSocket, &QWebSocket::textMessageReceived, this, &JobServer::slotTextMessageReceived);
    connect(pSocket, &QWebSocket::disconnected, this, &JobServer::slotSocketDisconnected);

    QByteArray message = jsonMessage(SMSG_TRUSTED_CLIENTS_INFO, QJsonArray::fromStringList(QStringList()));
    pSocket->sendBinaryMessage(message);

}

void JobServer::slotBinaryMessageReceived(const QByteArray & a_message)
{
    QWebSocket * pClient = qobject_cast<QWebSocket *>(sender());
    if(!pClient)
        return;
    QString messageString = trUtf8(a_message);
    processMessage(pClient, messageString);
}

void JobServer::slotTextMessageReceived(const QString & a_message)
{
    QWebSocket * pClient = qobject_cast<QWebSocket *>(sender());
    if(!pClient)
        return;
    processMessage(pClient, a_message);
}

QByteArray JobServer::jsonMessage(const QString & a_command, const QJsonObject & a_jsonObject)
{
    return jsonMessage(a_command, QJsonDocument(a_jsonObject));
}

QByteArray JobServer::jsonMessage(const QString & a_command, const QJsonArray & a_jsonArray)
{
    return jsonMessage(a_command, QJsonDocument(a_jsonArray));
}

QByteArray JobServer::jsonMessage(const QString & a_command, const QJsonDocument & a_jsonDocument)
{
    return a_command.toUtf8() + ' ' + a_jsonDocument.toJson();
}

QByteArray JobServer::jsonMessage(const QString & a_command, const QString & a_message)
{
    return a_command.toUtf8() + ' ' + a_message.toUtf8();
}

void JobServer::slotSocketDisconnected()
{
    QWebSocket * pClient = qobject_cast<QWebSocket *>(sender());
    if(!pClient)
        return;
    m_clients.remove(pClient);
    pClient->deleteLater();
}

void JobServer::processMessage(QWebSocket * a_pClient, const QString & a_message)
{
    QString command = a_message;
    QString arguments;

    int spaceIndex = a_message.indexOf(' ');
    if(spaceIndex >= 0)
    {
        command = a_message.left(spaceIndex);
        arguments = a_message.mid(spaceIndex + 1);
    }

    qDebug()<<command;
    qDebug()<<arguments;

    if(command == QString(MSG_CLOSE_SERVER))
    {
#ifdef Q_OS_WIN
        DebugActiveProcessStop((DWORD)m_process.processId());
#endif
        broadcastMessage(SMSG_CLOSING_SERVER, true);
        emit finish();
        return;
    }

    if(command == QString(MSG_START_ALL_WAITING_JOBS))
    {
        cmds = arguments.split("\n");
        cmds_indes = 1;
        for(QString cmd : cmds){
            startJob(cmd);
            break;
        }
        return;
    }

    if(command == QString(MSG_PAUSE_ACTIVE_JOBS))
    {
        pauseJob();
        return;
    }

    if(command == QString(MSG_RESUME_PAUSED_JOBS))
    {
        resumeJob();
        return;
    }

    if(command == QString(MSG_ABORT_ACTIVE_JOBS))
    {
        abortJob();
        return;
    }

    a_pClient->sendBinaryMessage(QString("Received an unknown command: %1").arg(a_message).toUtf8());
}

void JobServer::broadcastMessage(const QString & a_message, bool a_includeNonSubscribers, bool a_trustedOnly)
{
    broadcastMessage(a_message.toUtf8(), a_includeNonSubscribers,
        a_trustedOnly);
}

void JobServer::broadcastMessage(const char * a_message, bool a_includeNonSubscribers, bool a_trustedOnly)
{
    broadcastMessage(QByteArray(a_message), a_includeNonSubscribers, a_trustedOnly);
}

void JobServer::broadcastMessage(const QByteArray & a_message, bool a_includeNonSubscribers, bool a_trustedOnly)
{
    std::list<QWebSocket *> & clients =  m_clients;
    for(QWebSocket * pClient : clients)
    {
        pClient->sendBinaryMessage(a_message);
        qDebug()<<QString(a_message);
    }
}

void JobServer::startJob(QString &commandLine)
{
    emit signalLogMessage(trUtf8("Command line:"));
    emit signalLogMessage(commandLine);
    broadcastMessage(SMSG_JOBS_RUNNING);
    qDebug()<<commandLine;
    m_process.start(commandLine);
}

void JobServer::abortJob()
{
#ifdef Q_OS_WIN
    DebugActiveProcessStop((DWORD)m_process.processId());
#endif
    m_process.kill();
    m_process.waitForFinished(-1);
    qDebug()<<"Abort Process.";
    broadcastMessage(SMSG_JOBS_ABORTED);
}

void JobServer::pauseJob()
{
#ifdef Q_OS_WIN
    BOOL result = DebugActiveProcess((DWORD)m_process.processId());
    if(result){
        qDebug()<<"Pause process.";
        broadcastMessage(SMSG_JOBS_PAUSED);
    }
    else{
        emit signalLogMessage(trUtf8("Failed to pause process. Error %1.").arg(GetLastError()));
        qDebug()<<"Failed to pause process.";
        broadcastMessage(SMSG_JOBS_FAILED);
    }
#else
    int error = kill((pid_t)m_process.processId(), SIGSTOP);
    if(!error){
    }
    else{
        emit signalLogMessage(trUtf8("Failed to pause process. Error %1.").arg(error));
    }
#endif
}

void JobServer::resumeJob()
{
#ifdef Q_OS_WIN
    BOOL result = DebugActiveProcessStop((DWORD)m_process.processId());
    if(result){
        qDebug()<<"Resume process.";
        emit signalLogMessage(trUtf8("Resume process."));
    }
    else{
        qDebug()<<"Resume process error.";
        emit signalLogMessage(trUtf8("Failed to resume process. Error %1.").arg(GetLastError()));
        broadcastMessage(SMSG_JOBS_FAILED);
    }
#else
    int error = kill((pid_t)m_process.processId(), SIGCONT);
    if(!error){
    }
    else{
        emit signalLogMessage(trUtf8("Failed to resume process. Error %1.").arg(error));
    }
#endif
}

void JobServer::slotProcessStarted()
{
    broadcastMessage(SMSG_JOBS_RUNNING);
    emit signalLogMessage(trUtf8("Process started."));

    if(!m_process.isWritable())
    {
        emit signalLogMessage(trUtf8("Can not write to encoder. Aborting."));
        qDebug()<<"Can not write to encoder. Aborting.";
        abortJob();
        return;
    }
}

void JobServer::slotProcessReadyReadStandardError()
{
    QByteArray standardError = m_process.readAllStandardError();
    QString standardErrorText = QString::fromUtf8(standardError);
    standardErrorText = standardErrorText.trimmed();
    if(!standardErrorText.isEmpty()){
        emit signalLogMessage(standardErrorText);
    }
}

void JobServer::slotProcessError(QProcess::ProcessError a_error)
{
    switch(a_error)
    {
    case QProcess::FailedToStart:
        emit signalLogMessage(trUtf8("Process has failed to start."));
        qDebug()<<"Process has failed to start.";
        break;

    case QProcess::Crashed:
//        emit signalLogMessage(trUtf8("Process has crashed."));
        qDebug()<<"Process has crashed.";
        break;

    default:
        break;
    }
}

void JobServer::slotProcessFinished(int a_exitCode, QProcess::ExitStatus a_exitStatus)
{
    QString message = trUtf8("Process has completed.");

    if(a_exitStatus == QProcess::CrashExit)
    {
        message = trUtf8("Process has crashed.");
    }

    qDebug()<<message;
    emit signalLogMessage(trUtf8("%1 Exit code: %2").arg(message).arg(a_exitCode));
    broadcastMessage(SMSG_JOBS_COMPLETED);

    if(!isAllJobCompleted()){
        cmds_indes++;
        QString cmd = cmds.at(cmds_indes - 1);
        startJob(cmd);
    }
}

bool JobServer::isAllJobCompleted()
{
    bool at_completed = false;
    int at_len = cmds.length();
    if(cmds_indes >= at_len){
        at_completed = true;
    }
    return at_completed;
}
