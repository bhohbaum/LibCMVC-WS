#include "qtws.h"
#include <QRandomGenerator>

QtWS* QtWS::instance = nullptr;

/**
 * @brief QtWS::QtWS
 */
QtWS::QtWS()
{
    Q_INIT_RESOURCE(qtws);

    m_keepaliveTimer.setInterval(28500);
    connect(&m_keepaliveTimer, SIGNAL(timeout()), this, SLOT(sendKeepAlivePing()));
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
        int ret = deflateInit2(&strm,
            qMax(-1, qMin(9, level)),
            Z_DEFLATED,
            GZIP_WINDOWS_BIT,
            8,
            Z_DEFAULT_STRATEGY);
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
    QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
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

    pClient->ignoreSslErrors();
}

/**
 * @brief QtWS::sendKeepAlivePing
 */
void QtWS::sendKeepAlivePing()
{
    for (int i = 0; i < m_pWebSocketBackbone.count(); i++) {
        LOG(wsInfo(tr("PING "), m_pWebSocketBackbone.at(i)));
        if (m_pWebSocketBackbone.at(i)->state() == QAbstractSocket::SocketState::ConnectedState) {
            m_pWebSocketBackbone.at(i)->ping();
        }
    }
}

/**
 * @brief QtWS::startKeepAliveTimer
 */
void QtWS::startKeepAliveTimer()
{
    m_keepaliveTimer.start();
}

/**
 * @brief QtWS::stopKeepAliveTimer
 */
void QtWS::stopKeepAliveTimer()
{
    m_keepaliveTimer.stop();
}

/**
 * @brief QtWS::startBackboneWatchdog
 */
void QtWS::startBackboneWatchdog()
{
    for (int i = 0; i < m_pWebSocketBackbone.count(); i++) {
        connect(m_pWebSocketBackbone.at(i), SIGNAL(connected()), this, SLOT(startKeepAliveTimer()));
        connect(m_pWebSocketBackbone.at(i),
            SIGNAL(disconnected()),
            this,
            SLOT(stopKeepAliveTimer()));
    }
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

    m_qtTranslator.load(QLocale::system(), QStringLiteral("qtbase_"));
    m_qtTranslatorLib.load(trFileLib);
    m_qtTranslatorClient.load(trFileClient);
    m_qtTranslatorServer.load(trFileServer);

    app->installTranslator(&m_qtTranslatorLib);
    app->installTranslator(&m_qtTranslatorClient);
    app->installTranslator(&m_qtTranslatorServer);
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
    LOG(wsInfo(tr("Client identified as another server, moving connection to backbone pool: "),
        pClient));
    m_clients.removeAll(pClient);
    m_backbones.removeAll(pClient);
    m_backbones << pClient;
    m_channelListUpdateTimer.stop();
    m_channelListUpdateTimer.singleShot(1000, this, SIGNAL(updateChannels()));
    //emit updateChannels();
    QString bbMessage("/bb-event");
    bbMessage.append("\n");
    bbMessage.append(CHANNEL_LIST_REQUEST);
    for (int i = 0; i < QtWS::getInstance()->m_backbones.count(); i++) {
        QtWS::getInstance()->m_backbones[i]->sendTextMessage(bbMessage);
    }
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
    m_channelListUpdateTimer.stop();
    m_channelListUpdateTimer.singleShot(1000, this, SIGNAL(updateChannels()));
    //emit updateChannels();
}

QString QtWS::getChannelFromSocket(QWebSocket* pSocket)
{
    return pSocket->request().url().path();
}

void QtWS::getChannelFromMessage(QByteArray* message, QString* channel)
{
    /*
    QString msg(*message), chan(*channel);
    getChannelFromMessage(&msg, &chan);
    message->clear();
    message->append(msg);
    *channel = chan;
    */
    QString msg(*message);
    QList<QByteArray> arr = message->split('\n');
    *channel = QString(arr[0]);
    arr.pop_front();
    *message = arr.join('\n');
}

void QtWS::getChannelFromMessage(QString* message, QString* channel)
{
    QString msg(*message);
    QStringList arr = msg.split("\n");
    *channel = arr[0];
    arr.pop_front();
    *message = arr.join("\n").toUtf8();
}

void QtWS::sendBackboneMessage(QWebSocket* pSocket,
    QString messageID,
    QString channel,
    QByteArray message,
    QtWS::MessageType type,
    bool compressionEnabled)
{
    QByteArray bbMessage;
    bbMessage.append(messageID.trimmed().toUtf8());
    bbMessage.append("\n");
    bbMessage.append(channel.trimmed().toUtf8());
    bbMessage.append("\n");
    bbMessage.append(message);
    QByteArray finalMessage(bbMessage);
    if (compressionEnabled) {
        QtWS::getInstance()->gzipCompress(bbMessage, finalMessage, 9);
    }
    if (type == MessageType::Text && !compressionEnabled) {
        pSocket->sendTextMessage(finalMessage);
    } else if (type == MessageType::Binary || compressionEnabled) {
        pSocket->sendBinaryMessage(finalMessage);
    }
}

void QtWS::sendClientMessage(QWebSocket* pSocket,
    QByteArray message,
    QtWS::MessageType type,
    bool compressionEnabled)
{
    QByteArray finalMessage(message);
    if (compressionEnabled) {
        QtWS::getInstance()->gzipCompress(message, finalMessage, 9);
    }
    if (type == MessageType::Text && !compressionEnabled) {
        pSocket->sendTextMessage(finalMessage);
    } else if (type == MessageType::Binary || compressionEnabled) {
        pSocket->sendBinaryMessage(finalMessage);
    }
}

void QtWS::parseBackboneMessage(QByteArray inputBuffer,
    QString* messageID,
    QString* channel,
    QByteArray* message,
    bool* compressionEnabled)
{
    QByteArray inputBufferInt(inputBuffer);
    *compressionEnabled = QtWS::getInstance()->gzipDecompress(inputBuffer, inputBufferInt);
    QList<QByteArray> arr = inputBuffer.split('\n');
    *messageID = QString(arr[0]);
    arr.pop_front();
    *channel = QString(arr[0]);
    arr.pop_front();
    *message = QByteArray(arr.join('\n'));
    if (m_debug) {
        if (!messageID->startsWith(MESSAGE_ID_TOKEN)) {
            LOG(tr("Could not find message id token in input data!"));
        }
    }
}

QString QtWS::generateMessageID()
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
                   .append(" ")
                   .append(QString::number(QRandomGenerator::global()->generate())));
    QString messageID = QString(MESSAGE_ID_TOKEN)
                            .append(QCryptographicHash::hash(ts.toUtf8(), QCryptographicHash::Algorithm::Md5)
                                        .toHex());
    return messageID;
}
