#include "qtws.h"

QtWS::QtWS()
{
    m_pWebSocketBackbone = new QWebSocket();
    keepaliveTimer.setInterval(1000);
    connect(&keepaliveTimer, SIGNAL(timeout()), this, SLOT(sendKeepAlivePing()));

    startBackboneWatchdog();
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

QString QtWS::secureBackboneUrl(QString url)
{
    QUrl u1(url);
    QString res(QUrl(u1.scheme().append("://").append(u1.host()).append(":").append(
                         QString::number(u1.port())))
                    .toString());
    return res;
}

void QtWS::onSslErrors(const QList<QSslError>& errors)
{
    QString str("SSL error occurred: ");
    for (int i = 0; i < errors.length(); i++) {
        str.append(errors.at(i).errorString());
        str.append("\n");
    }
    qDebug() << str;
    qDebug() << "Trying to ignore the error(s) and go on....";

    m_pWebSocketBackbone->ignoreSslErrors();
}

void QtWS::sendKeepAlivePing()
{
    qDebug() << "PING";
    m_pWebSocketBackbone->ping();
}

void QtWS::startKeepAliveTimer()
{
    keepaliveTimer.start();
}

void QtWS::stopKeepAliveTimer()
{
    keepaliveTimer.stop();
}

void QtWS::startBackboneWatchdog()
{
    connect(m_pWebSocketBackbone, SIGNAL(connected()), this, SLOT(startKeepAliveTimer()));
    connect(m_pWebSocketBackbone, SIGNAL(disconnected()), this, SLOT(stopKeepAliveTimer()));
}
