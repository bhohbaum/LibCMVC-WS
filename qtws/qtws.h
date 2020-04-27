#ifndef QTWS_H
#define QTWS_H

#include "zlib.h"
#include <QByteArray>

#define GZIP_WINDOWS_BIT 15 + 16 // gzip only decoding
#define GZIP_OR_ZLIB_WIN_BIT 15 + 32 // auto zlib or gzip decoding
#define GZIP_CHUNK_SIZE 32 * 1024

class QtWS {
public:
    QtWS();

    //static QByteArray gUncompress(const QByteArray& data);

    static bool gzipCompress(QByteArray input, QByteArray& output, int level = -1);
    static bool gzipDecompress(QByteArray input, QByteArray& output);
};

#endif // QTWS_H
