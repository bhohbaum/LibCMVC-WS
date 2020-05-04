#include "qtws.h"

QtWS* QtWS::instance = nullptr;

/**
 * @brief QtWS::QtWS
 */
QtWS::QtWS()
{
    Q_INIT_RESOURCE(qtws);

    m_pWebSocketBackbone = new QWebSocket();
    keepaliveTimer.setInterval(1000);
    connect(&keepaliveTimer, SIGNAL(timeout()), this, SLOT(sendKeepAlivePing()));

    startBackboneWatchdog();
}

/**
 * @brief QtWS::~QtWS
 */
QtWS::~QtWS()
{
    Q_CLEANUP_RESOURCE(qtws);
}

/**
 * @brief QtWS::getInstance
 * @return
 */
QtWS* QtWS::getInstance()
{
    if (QtWS::instance == nullptr)
        QtWS::instance = new QtWS();
    return instance;
}

/**
 * @brief Compresses the given buffer using the standard GZIP algorithm
 * @param input The buffer to be compressed
 * @param output The result of the compression
 * @param level The compression level to be used (@c 0 = no compression, @c 9 = max, @c -1 = default)
 * @return @c true if the compression was successful, @c false otherwise
 */
bool QtWS::gzipCompress(QByteArray input, QByteArray& output, int level)
{
    output.clear();
    if (input.length()) {
        int flush = 0;
        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = Z_NULL;
        int ret = deflateInit2(&strm, qMax(-1, qMin(9, level)), Z_DEFLATED, GZIP_WINDOWS_BIT, 8, Z_DEFAULT_STRATEGY);
        if (ret != Z_OK) {
            return (false);
        }
        output.clear();
        char* input_data = input.data();
        int input_data_left = input.length();
        do {
            int chunk_size = qMin(GZIP_CHUNK_SIZE, input_data_left);
            strm.next_in = (unsigned char*)input_data;
            strm.avail_in = chunk_size;
            input_data += chunk_size;
            input_data_left -= chunk_size;
            flush = (input_data_left <= 0 ? Z_FINISH : Z_NO_FLUSH);
            do {
                char out[GZIP_CHUNK_SIZE];
                strm.next_out = (unsigned char*)out;
                strm.avail_out = GZIP_CHUNK_SIZE;
                ret = deflate(&strm, flush);
                if (ret == Z_STREAM_ERROR) {
                    deflateEnd(&strm);
                    return (false);
                }
                int have = (GZIP_CHUNK_SIZE - strm.avail_out);
                if (have > 0) {
                    output.append((char*)out, have);
                }
            } while (strm.avail_out == 0);
        } while (flush != Z_FINISH);
        (void)deflateEnd(&strm);
        return (ret == Z_STREAM_END);
    } else {
        return (true);
    }
}

/**
 * @brief Decompresses the given buffer using the standard GZIP algorithm
 * @param input The buffer to be decompressed
 * @param output The result of the decompression
 * @return @c true if the decompression was successfull, @c false otherwise
 */
bool QtWS::gzipDecompress(QByteArray input, QByteArray& output)
{
    output.clear();
    if (input.length() > 0) {
        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = Z_NULL;
        int ret = inflateInit2(&strm, GZIP_OR_ZLIB_WIN_BIT);
        if (ret != Z_OK) {
            return (false);
        }
        char* input_data = input.data();
        int input_data_left = input.length();
        do {
            int chunk_size = qMin(GZIP_CHUNK_SIZE, input_data_left);
            if (chunk_size <= 0) {
                break;
            }
            strm.next_in = (unsigned char*)input_data;
            strm.avail_in = chunk_size;
            input_data += chunk_size;
            input_data_left -= chunk_size;
            do {
                char out[GZIP_CHUNK_SIZE];
                strm.next_out = (unsigned char*)out;
                strm.avail_out = GZIP_CHUNK_SIZE;
                ret = inflate(&strm, Z_NO_FLUSH);
                switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;
                    break;
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                case Z_STREAM_ERROR:
                    inflateEnd(&strm);
                    return (false);
                }
                int have = (GZIP_CHUNK_SIZE - strm.avail_out);
                if (have > 0) {
                    output.append((char*)out, have);
                }
            } while (strm.avail_out == 0);
        } while (ret != Z_STREAM_END);
        inflateEnd(&strm);
        return (ret == Z_STREAM_END);
    } else {
        return (true);
    }
}

/**
 * @brief QtWS::secureBackboneUrl
 * @param url
 * @return
 */
QString QtWS::secureBackboneUrl(QString url)
{
    QUrl u1(url);
    QString res(QUrl(u1.scheme().append("://").append(u1.host()).append(":").append(
                         QString::number(u1.port())))
                    .toString());
    return res;
}

/**
 * @brief QtWS::onSslErrors
 * @param errors
 */
void QtWS::onSslErrors(const QList<QSslError>& errors)
{
    QString str(tr("One or more SSL errors occurred:"));
    str.append("\n");
    for (int i = 0; i < errors.length(); i++) {
        str.append("    ");
        str.append(QString::number(i + 1)).append(") ");
        str.append(errors.at(i).errorString());
        str.append("\n");
    }
    LOG(str);
    LOG(tr("Trying to ignore the error(s) and go on...."));

    m_pWebSocketBackbone->ignoreSslErrors();
}

/**
 * @brief QtWS::sendKeepAlivePing
 */
void QtWS::sendKeepAlivePing()
{
    LOG(wsInfo(tr("PING "), m_pWebSocketBackbone));
    if (m_pWebSocketBackbone->state() == QAbstractSocket::SocketState::ConnectedState) {
        m_pWebSocketBackbone->ping();
    }
}

/**
 * @brief QtWS::startKeepAliveTimer
 */
void QtWS::startKeepAliveTimer()
{
    keepaliveTimer.start();
}

/**
 * @brief QtWS::stopKeepAliveTimer
 */
void QtWS::stopKeepAliveTimer()
{
    keepaliveTimer.stop();
}

/**
 * @brief QtWS::startBackboneWatchdog
 */
void QtWS::startBackboneWatchdog()
{
    connect(m_pWebSocketBackbone, SIGNAL(connected()), this, SLOT(startKeepAliveTimer()));
    connect(m_pWebSocketBackbone, SIGNAL(disconnected()), this, SLOT(stopKeepAliveTimer()));
}

/**
 * @brief QtWS::log Print a message
 * @param msg
 */
void QtWS::log(QString msg)
{
    QString s;
    QDate date(QDate::currentDate());
    QTime time(QTime::currentTime());
    QString ts(QString::number(date.year())
                   .append(":")
                   .append(s.asprintf("%02d", date.month()))
                   .append(":")
                   .append(s.asprintf("%02d", date.day()))
                   .append(" ")
                   .append(s.asprintf("%02d", time.hour()))
                   .append(":")
                   .append(s.asprintf("%02d", time.minute()))
                   .append(":")
                   .append(s.asprintf("%02d", time.second()))
                   .append(".")
                   .append(s.asprintf("%03d", time.msec()))
                   .append(" "));
    std::cout << ts.toUtf8().constData() << msg.toUtf8().constData() << std::endl;
}

/**
 * @brief QtWS::loadTranslation Load translated strings
 * @param app
 */
void QtWS::loadTranslation(QCoreApplication* app)
{
    QString trFileLib, trFileClient, trFileServer;

    trFileLib.append(":/qtws/qtws_").append(QLocale::system().name().split("_").at(0)).append(".qm");
    trFileClient.append(":/qtwsclient/qtws_")
        .append(QLocale::system().name().split("_").at(0))
        .append(".qm");
    trFileServer.append(":/qtwsserver/qtws_")
        .append(QLocale::system().name().split("_").at(0))
        .append(".qm");

    qtTranslator.load(QLocale::system(), QStringLiteral("qtbase_"));
    qtTranslatorLib.load(trFileLib);
    qtTranslatorClient.load(trFileClient);
    qtTranslatorServer.load(trFileServer);

    app->installTranslator(&qtTranslatorLib);
    app->installTranslator(&qtTranslatorClient);
    app->installTranslator(&qtTranslatorServer);
}

/**
 * @brief QtWS::wsInfo Build a message string from a message and additional websocket info.
 * @param msg
 * @param pSocket
 * @return
 */
QString QtWS::wsInfo(QString msg, QWebSocket* pSocket)
{
    QString message;
    message.append(msg)
        .append(pSocket->peerName())
        .append(" ")
        .append(pSocket->origin())
        .append(" ")
        .append(pSocket->peerAddress().toString())
        .append(" ")
        .append(QString::number(pSocket->peerPort()))
        .append(" ")
        .append(pSocket->requestUrl().toString());
    return message;
}

/**
 * @brief QtWS::quitApplication Quit the whole application.
 */
void QtWS::quitApplication()
{
    LOG(tr("Exiting process..."));
    QCoreApplication::quit();
}

WsMetaData* QtWS::findMetaDataByWebSocket(QWebSocket* pSocket)
{
    for (int i = 0; i < m_wsMetaDataList.length(); i++) {
        if (m_wsMetaDataList.at(i)->getWebSocket() == pSocket)
            return m_wsMetaDataList.at(i);
    }
    WsMetaData* wmd;
    wmd = new WsMetaData(pSocket);
    wmd->setWebSocket(pSocket);
    return wmd;
}

QString QtWS::buildChannelListString(QStringList wmd)
{
    QString res;
    for (int k = 0; k < wmd.length(); k++) {
        res.append(wmd.at(k)).append(" ");
    }
    return res.trimmed();
}

QString QtWS::buildChannelListString(QList<WsMetaData*>* wmd)
{
    QString res;
    wmd = (wmd == nullptr) ? &m_wsMetaDataList : wmd;
    for (int i = 0; i < wmd->length(); i++) {
        for (int k = 0; k < wmd->at(i)->getChannels().length(); k++) {
            res.append(wmd->at(i)->getChannels().at(k)).append(" ");
        }
    }
    return res.trimmed();
}

QStringList QtWS::buildChannelListArray(QString channels)
{
    return channels.split(" ");
}

void QtWS::handleBackboneRegistration(QWebSocket* pClient)
{
    LOG(wsInfo(
        tr("Client identified as another server, moving connection to backbone pool: "), pClient));
    m_clients.removeAll(pClient);
    m_backbones.removeAll(pClient);
    m_backbones << pClient;
    //findMetaDataByWebSocket(pClient)->updateChannelAnnouncement();
    emit updateChannels();
}

void QtWS::handleChannelListNotification(QString message, QWebSocket* pClient)
{
    LOG(wsInfo(tr("Received channel list update notification from: "), pClient));
    QStringList arr = buildChannelListArray(message);
    arr.pop_front();
    WsMetaData* wmd = findMetaDataByWebSocket(pClient);
    wmd->clearChannels();
    wmd->addChannels(arr);
    QString str(tr("Channels: "));
    LOG(str.append(buildChannelListString(arr)));
    emit updateChannels();
}

QString QtWS::getChannelFromSocket(QWebSocket* pSocket)
{
    return pSocket->request().url().path();
}

bool QtWS::getChannelFromMessage(QString* message, QString* channel)
{
    QString msg(*message);
    QStringList arr = msg.split("\n");
    *channel = arr[0];
    arr.pop_front();
    msg = arr.join("\n");
    *message = msg.toUtf8();
    return arr.length() > 0;
}
