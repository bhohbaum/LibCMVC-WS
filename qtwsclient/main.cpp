#include "echoclient.h"
#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>

void quit_app()
{
    QCoreApplication::quit();
}

int main(int argc, char* argv[])
{
    Q_INIT_RESOURCE(qtws);

    QCoreApplication a(argc, argv);

    QtWS::getInstance()->loadTranslation(&a);

    QCommandLineParser parser;
    parser.setApplicationDescription("LibCMVC Websocket Client");
    parser.addHelpOption();

    QCommandLineOption dbgOption(QStringList() << "d"
                                               << "debug",
        QCoreApplication::translate("main", "Debug output [default: off]."));
    parser.addOption(dbgOption);
    QCommandLineOption compressOption(QStringList() << "c"
                                                    << "compress",
        QCoreApplication::translate("main", "Compress data for transmission [default: raw data]."));
    parser.addOption(compressOption);
    parser.process(a);
    bool debug = parser.isSet(dbgOption);
    bool comp = parser.isSet(compressOption);

    QString serverLocation = "";
    for (int i = 0; i < argc; i++) {
        QString arg(argv[i]);
        if (arg.toLower().startsWith("ws://") || arg.toLower().startsWith("wss://")) {
            serverLocation = arg;
        }
    }

    serverLocation.replace("\n", "").replace("\r", "");
    EchoClient client(QUrl(serverLocation), debug, comp);
    QObject::connect(&client, &EchoClient::closed, &a, &quit_app);

    return a.exec();
}
