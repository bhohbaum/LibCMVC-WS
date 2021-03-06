#include "sslechoserver.h"
#include <QFile>
#include <QTranslator>
#include <QtCore/QCoreApplication>
#include <limits.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);
    unsigned short p = 0;
    unsigned short ps = 0;
    bool encrypted = false;
    QString certificate(":/cert/localhost.cert");
    QString key(":/cert/localhost.key");
    QStringList bbUrl("");

    QtWS::getInstance()->loadTranslation(&a);

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
            bbUrl.append(QString(argv[i + 1]));
        }
    }

    SslEchoServer server(p, ps, nullptr, encrypted, certificate, key, bbUrl);
    if (!server.startupComplete) {
        return 1;
    }

    int res = 0;

    res = a.exec();

    QString cmd;

    for (int i = 0; i < argc; i++) {
        cmd.append(argv[i]).append(" ");
    }
    system(cmd.toUtf8());

    return res;
}
