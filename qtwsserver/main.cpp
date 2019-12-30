#include <QtCore/QCoreApplication>
#include "sslechoserver.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    unsigned short p = 8888;

    if (argc == 2) {
        QString port(argv[1]);
        p = static_cast<unsigned short>(port.toShort());
    }

    SslEchoServer server(p);
    Q_UNUSED(server)

    return a.exec();
}
