#ifndef FASTDOWNLOADER_P_H
#define FASTDOWNLOADER_P_H

#include <fastdownloader.h>
#include <private/qobject_p.h>

struct Chunk
{
    quint32 id = 0;
    qint64 pos = -1;
    QByteArray data;
    QNetworkReply* reply = nullptr;
};

class FastDownloaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(FastDownloader)
public:
    FastDownloaderPrivate();

    bool chunkExists(quint32 id) const;
    void deleteChunk(Chunk* chunk);
    void createChunk(const QUrl& url, qint64 begin = -1, qint64 end = -1);
    void startParallelDownloading();
    quint32 generateUniqueId() const;
    Chunk* chunkFor(quint32 id) const;
    Chunk* chunkFor(const QObject* sender) const;

    static qint64 contentLength(const Chunk* chunk);
    static bool testParallelDownload(const Chunk* chunk);

    QScopedPointer<QNetworkAccessManager> manager;
    bool running;
    bool resolved;
    bool parallelDownloadPossible;
    QUrl resolvedUrl;
    qint64 bytesReceived;
    qint64 bytesTotal;
    QList<Chunk*> chunks;

    void _q_finished();
    void _q_readyRead();
    void _q_redirected(const QUrl& url);
    void _q_error(QNetworkReply::NetworkError code);
    void _q_sslErrors(const QList<QSslError>& errors);
    void _q_downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
};

#endif // FASTDOWNLOADER_P_H