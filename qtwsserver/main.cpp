#include <QtCore/QCoreApplication>
#include "sslechoserver.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    unsigned short p = 8888;
    bool encrypted = false;
    QString certificate("qrc:/localhost.cert");
    QString key("qrc:/localhost.key");

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
        }
    }

    SslEchoServer server(p, nullptr, encrypted, certificate, key);
    Q_UNUSED(server)

    return a.exec();
}
