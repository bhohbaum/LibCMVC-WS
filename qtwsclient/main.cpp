#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>
#include "echoclient.h"

void quit_app() {
    QCoreApplication::quit();
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("LibCMVC Websocket Client");
    parser.addHelpOption();

    QCommandLineOption dbgOption(QStringList() << "d" << "debug",
            QCoreApplication::translate("main", "Debug output [default: off]."));
    parser.addOption(dbgOption);
    parser.process(a);
    bool debug = parser.isSet(dbgOption);

    QString serverLocation = "";
    for (int i = 0; i < argc; i++) {
        QString arg(argv[i]);
        if (arg.toLower().startsWith("ws://") || arg.toLower().startsWith("wss://")) {
            serverLocation = arg;
        }
    }

    serverLocation.replace("\n", "").replace("\r", "");
    EchoClient client(QUrl(serverLocation), debug);
    QObject::connect(&client, &EchoClient::closed, &a, &quit_app);

    return a.exec();
}
