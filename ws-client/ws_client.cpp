#include "ws_client.h"
#include "ui_ws_client.h"

ws_client::ws_client(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ws_client)
  , m_pServerSocket(nullptr)
{
    ui->setupUi(this);

    job_state = JobState::Waiting;
    m_pServerSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    qDebug()<<"Connecting to local server.";
    connectToServer(QHostAddress::LocalHost);

    connect(m_pServerSocket, &QWebSocket::connected, this, &ws_client::slotServerConnected);
    connect(m_pServerSocket, &QWebSocket::disconnected, this, &ws_client::slotServerDisconnected);
    connect(m_pServerSocket, &QWebSocket::binaryMessageReceived, this, &ws_client::slotBinaryMessageReceived);
    connect(m_pServerSocket, &QWebSocket::textMessageReceived, this, &ws_client::slotTextMessageReceived);
}

ws_client::~ws_client()
{
    delete ui;
}

void ws_client::slotServerConnected()
{
    qDebug()<<"Server connected.";
//    m_pServerSocket->sendBinaryMessage(MSG_GET_JOBS_INFO);
//    m_pServerSocket->sendBinaryMessage(MSG_GET_LOG);
//    m_pServerSocket->sendBinaryMessage(MSG_SUBSCRIBE);
}

void ws_client::slotServerDisconnected()
{
    qDebug()<<"Server disconnected.";
//    QTimer::singleShot(1000, Qt::PreciseTimer, this, SLOT(connectToServer(const QHostAddress &)));
}

void ws_client::slotShutdownServer()
{
    if(!m_pServerSocket->peerAddress().isLoopback())
        return;
    m_pServerSocket->sendBinaryMessage(MSG_CLOSE_SERVER);
}

void ws_client::closeEvent(QCloseEvent *event)
{
    slotShutdownServer();
    this->close();
}

void ws_client::connectToServer(const QHostAddress & a_address)
{
    if(a_address.isNull())
        return;

    m_nextServerAddress = a_address;
    QString addressString = a_address.toString();
    QString connectionURL = QString("ws://%1:%2").arg(addressString).arg(JOB_SERVER_PORT);

    m_pServerSocket->open(connectionURL);
}

void ws_client::slotBinaryMessageReceived(const QByteArray & a_message)
{
    slotTextMessageReceived(QString::fromUtf8(a_message));
}

void ws_client::slotTextMessageReceived(const QString & a_message)
{
    QString command = a_message;
    QString arguments;
    int spaceIndex = a_message.indexOf(' ');
    if(spaceIndex >= 0)
    {
        command = a_message.left(spaceIndex);
        arguments = a_message.mid(spaceIndex + 1);
    }
    qDebug()<<QString("Received cmd is %1").arg(command);

    if(command == QString(SMSG_LOG_MESSAGE))
    {
        ui->logEdit->append(arguments);
        return;
    }

    if(command == QString(SMSG_TRUSTED_CLIENTS_INFO))
    {
        ui->logEdit->append("Server Start.");
        return;
    }

    if(command == QString(SMSG_JOBS_ABORTED))
    {
        ntfModeChange(JobState::Aborted);
        return;
    }

    if(command == QString(SMSG_JOBS_PAUSED))
    {
        ntfModeChange(JobState::Paused);
        return;
    }

    if(command == QString(SMSG_JOBS_FAILED))
    {
        ntfModeChange(JobState::Failed);
        return;
    }

    if(command == QString(SMSG_JOBS_COMPLETED))
    {
        ntfModeChange(JobState::Completed);
        return;
    }

    if(command == QString(SMSG_JOBS_RUNNING))
    {
        ntfModeChange(JobState::Running);
        return;
    }
}

void ws_client::on_startBt_clicked()
{
    QString text = QString(" ") + ui->cmdEdit->toPlainText();
    QString cmd = MSG_ABORT_ACTIVE_JOBS;

    if(job_state == JobState::Waiting)
    {
        cmd = MSG_START_ALL_WAITING_JOBS;
    }
    else if(job_state == JobState::Paused)
    {
        cmd = MSG_RESUME_PAUSED_JOBS;
    }
    else
    {
        return;
    }
    m_pServerSocket->sendBinaryMessage(cmd.toUtf8() + text.toUtf8());
    ntfModeChange(JobState::Running);
}

void ws_client::on_pauseBt_clicked()
{
    m_pServerSocket->sendBinaryMessage(MSG_PAUSE_ACTIVE_JOBS);
    ntfModeChange(JobState::Pausing);
}

void ws_client::on_abortBt_clicked()
{
    m_pServerSocket->sendBinaryMessage(MSG_ABORT_ACTIVE_JOBS);
    ntfModeChange(JobState::Aborting);
}

void ws_client::ntfModeChange(JobState rv_state)
{
    qDebug()<<QString("Mode:%1->%2").arg(convertStateToStr(job_state)).arg(convertStateToStr(rv_state));
    job_state = rv_state;
    switch(rv_state){
    case JobState::Waiting:
        break;
    case JobState::Running:
        ui->startBt->setEnabled(false);
        ui->pauseBt->setEnabled(true);
        ui->abortBt->setEnabled(true);
        ui->startBt->setText("Resume");
        break;
    case JobState::Pausing:
        ui->startBt->setEnabled(false);
        ui->pauseBt->setEnabled(false);
        ui->abortBt->setEnabled(false);
        break;
    case JobState::Paused:
        ui->startBt->setEnabled(true);
        ui->pauseBt->setEnabled(false);
        ui->abortBt->setEnabled(true);
        ui->startBt->setText("Resume");
        break;
    case JobState::Aborting:
        ui->startBt->setEnabled(false);
        ui->pauseBt->setEnabled(false);
        ui->abortBt->setEnabled(false);
        ui->startBt->setText("Start");
        break;
    case JobState::Aborted:
        job_state = JobState::Waiting;
        ui->startBt->setEnabled(true);
        ui->pauseBt->setEnabled(false);
        ui->abortBt->setEnabled(false);
        ui->startBt->setText("Start");
        break;
    case JobState::Failed:
        job_state = JobState::Waiting;
        m_pServerSocket->sendBinaryMessage(MSG_ABORT_ACTIVE_JOBS);
        break;
    case JobState::Completed:
        job_state = JobState::Waiting;
        ui->startBt->setEnabled(true);
        ui->pauseBt->setEnabled(false);
        ui->abortBt->setEnabled(false);
        ui->startBt->setText("Start");
        break;
    case JobState::Unknowned:
    default:
        job_state = JobState::Waiting;
        break;
    }
}

QString ws_client::convertStateToStr(JobState rv_state)
{
    switch(rv_state){
    case JobState::Waiting:
        return QString("Waiting");
    case JobState::Running:
        return QString("Running");
    case JobState::Pausing:
        return QString("Pausing");
    case JobState::Paused:
        return QString("Paused");
    case JobState::Aborting:
        return QString("Aborting");
    case JobState::Aborted:
        return QString("Aborted");
    case JobState::Failed:
        return QString("Failed");
    case JobState::Completed:
        return QString("Completed");
    case JobState::Unknowned:
    default:
        return QString("Unknowned");
    }
}
