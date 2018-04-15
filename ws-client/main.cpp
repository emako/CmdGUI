#include "ws_client.h"
#include "application_instance_file_guard.h"
#include <QApplication>
#include <QProcess>
#include <QDir>

QString resolvePathFromApplication(const QString & a_relativePath)
{
    // Remember the working directory and change it to application directory.
    QString cwd = QDir::currentPath();
    QString applicationDirPath = QCoreApplication::applicationDirPath();
    QDir::setCurrent(applicationDirPath);

    QFileInfo fileInfo(a_relativePath);
    // If no parent directory is specified - leave the path as it is.
    if(fileInfo.path() == ".")
        return(a_relativePath);
    QString absolutePath = fileInfo.absoluteFilePath();

    // Restore the working directory.
    QDir::setCurrent(cwd);

    return absolutePath;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //a.setQuitOnLastWindowClosed(false);

    QString serverPath = resolvePathFromApplication(QString("./%1").arg(JOB_SERVER_NAME));

    bool started = QProcess::startDetached(serverPath);
    if(!started)
    {
        qDebug()<<"Could not start server.";
        return -1;
    }

    ApplicationInstanceFileGuard guard("job_server_watcher_running");
    if(!guard.isLocked())
    {
        return -1;
    }

    ws_client w;
    w.show();
    return a.exec();
}
