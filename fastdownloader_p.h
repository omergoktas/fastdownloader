#ifndef FASTDOWNLOADER_P_H
#define FASTDOWNLOADER_P_H

#include <fastdownloader.h>
#include <private/qobject_p.h>

struct Segment
{
    QNetworkReply* reply;
};

class FastDownloaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(FastDownloader)
public:
    FastDownloaderPrivate();

    bool resolveUrl();
    QNetworkRequest makeRequest(bool initial) const;
    void connectSegment(const Segment& segment);

    QScopedPointer<QNetworkAccessManager> manager;
    bool running;
    QUrl resolvedUrl;
    QList<Segment> segments;

    void _q_redirected(const QUrl& url);
    void _q_finished();
    void _q_readyRead();
    void _q_error(QNetworkReply::NetworkError code);
    void _q_sslErrors(const QList<QSslError>& errors);
    void _q_downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
};

#endif // FASTDOWNLOADER_P_H