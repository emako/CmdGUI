#include <QCoreApplication>

#include "application_instance_file_guard.h"
#include "job_server.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    ApplicationInstanceFileGuard guard("job_server_running");
    if(!guard.isLocked())
    {
        qCritical("Couldn't start the server. "
            "Another instance is probably already running.");
        return 1;
    }

    JobServer jobServer;
    a.connect(&jobServer, &JobServer::finish, &a, &QCoreApplication::quit);

    bool started = jobServer.start();
    if(!started)
    {
        qDebug()<<"Couldn't start the server.";
        return 1;
    }

    return a.exec();
}
