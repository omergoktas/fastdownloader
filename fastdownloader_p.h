#ifndef FASTDOWNLOADER_P_H
#define FASTDOWNLOADER_P_H

#include <fastdownloader.h>
#include <private/qobject_p.h>

struct Segment
{
    int index = -1;
    qint64 bytesReceived = 0;
    QNetworkReply* reply = nullptr;
};

class FastDownloaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(FastDownloader)
public:
    FastDownloaderPrivate();

    bool resolveUrl();
    bool isParallelDownloadPossible(const QNetworkReply* reply) const;
    QNetworkRequest makeRequest(bool initial) const;
    void connectSegment(const Segment* segment) const;

    inline QNetworkReply* getReply(int segmentIndex) const
    { return segments.at(segmentIndex)->reply; }
    inline Segment* getSegment(const QNetworkReply* reply) const
    {
        for (Segment* seg : segments) {
            if (seg->reply == reply)
                return seg;
        }
        return nullptr;
    }

    QScopedPointer<QNetworkAccessManager> manager;
    bool running;
    bool resolved;
    bool parallelDownloadPossible;
    QUrl resolvedUrl;
    QList<Segment*> segments;

    void _q_redirected(const QUrl& url);
    void _q_finished();
    void _q_readyRead();
    void _q_error(QNetworkReply::NetworkError code);
    void _q_sslErrors(const QList<QSslError>& errors);
    void _q_downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
};

#endif // FASTDOWNLOADER_P_H