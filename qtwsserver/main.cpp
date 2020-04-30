#include "sslechoserver.h"
#include <QFile>
#include <QTranslator>
#include <QtCore/QCoreApplication>
#include <limits.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    Q_INIT_RESOURCE(qtws);
    Q_INIT_RESOURCE(securesocketclient);

    QCoreApplication a(argc, argv);
    unsigned short p = 0;
    unsigned short ps = 0;
    bool encrypted = false;
    QString certificate(":/localhost.cert");
    QString key(":/localhost.key");
    QString bbUrl("");

    QString trde;
    trde.append(":/qtws_").append(QLocale::system().name().split("_").at(0)).append("_001.qm");
    qDebug() << trde;

    QTranslator qtTranslator;
    qtTranslator.load(QLocale::system(), QStringLiteral("qtbase_"));
    qtTranslator.load(trde);
    a.installTranslator(&qtTranslator);

    if (argc == 2) {
        QString port(argv[1]);
        p = static_cast<unsigned short>(port.toShort());
    }

    for (int i = 0; i < argc; i++) {
        if (QString::compare(QString(argv[i]), QString("-p")) == 0) {
            QString port(argv[i + 1]);
            p = static_cast<unsigned short>(port.toShort());
        }
        if (QString::compare(QString(argv[i]), QString("-c")) == 0) {
            certificate = QString(argv[i + 1]);
        }
        if (QString::compare(QString(argv[i]), QString("-k")) == 0) {
            key = QString(argv[i + 1]);
        }
        if (QString::compare(QString(argv[i]), QString("-s")) == 0 || QString::compare(QString(argv[i]), QString("-e")) == 0) {
            encrypted = true;
            QString port(argv[i + 1]);
            ps = static_cast<unsigned short>(port.toShort());
        }
        if (QString::compare(QString(argv[i]), QString("-b")) == 0) {
            bbUrl = QString(argv[i + 1]);
        }
    }

    SslEchoServer server(p, ps, nullptr, encrypted, certificate, key, bbUrl);
    if (!server.startupComplete) {
        return 1;
    }
    //Q_UNUSED(server)

    int res = 0;

    res = a.exec();

    QString cmd;

    for (int i = 0; i < argc; i++) {
        cmd.append(argv[i]).append(" ");
    }
    system(cmd.toUtf8());
    //execve(argv[0], argv, NULL);

    return res;
}
