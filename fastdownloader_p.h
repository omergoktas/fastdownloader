#ifndef FASTDOWNLOADER_P_H
#define FASTDOWNLOADER_P_H

#include <fastdownloader.h>
#include <private/qobject_p.h>
#include <QQueue>

struct Chunk
{
    bool cleanUp = false;
    quint32 id = 0;
    qint64 begin = -1;
    qint64 end = -1;
    qint64 bytesReceived = -1;
    QByteArray data;
    QNetworkReply* reply = nullptr;
};

class FastDownloaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(FastDownloader)
public:
    FastDownloaderPrivate();

    bool resolveUrl();
    bool isParallelDownloadPossible(const QNetworkReply* reply) const;
    void connectChunk(const Chunk* chunk) const;
    bool chunkExists(quint32 id) const;
    int generateUniqueId() const;
    Chunk* chunkFor(const QObject* sender) const;
    QNetworkRequest makeRequest(bool initial) const;

    QScopedPointer<QNetworkAccessManager> manager;
    bool running;
    bool parallelDownloadPossible;
    QUrl resolvedUrl;
    QQueue<Chunk*> chunks;

    void _q_redirected(const QUrl& url);
    void _q_readyRead();
    void _q_error(QNetworkReply::NetworkError code);
    void _q_sslErrors(const QList<QSslError>& errors);
    void _q_downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void _q_activateOtherConnections();
};

#endif // FASTDOWNLOADER_P_H