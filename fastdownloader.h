#ifndef FASTDOWNLOADER_H
#define FASTDOWNLOADER_H

#include <fastdownloader_global.h>

#include <QObject>
#include <QUrl>
#include <QSslError>
#include <QNetworkReply>

class FastDownloaderPrivate;
class FASTDOWNLOADER_EXPORT FastDownloader : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(FastDownloader)
    Q_DECLARE_PRIVATE(FastDownloader)

    enum {
        // Note: QNetworkAccessManager queues the requests it receives. The number of requests
        // executed in parallel is dependent on the protocol. Currently, for the HTTP protocol
        // on desktop platforms, 6 requests are executed in parallel for one host/port combination.
        MAX_SEGMENTS = 6,

        // Give the initial segment a space for fetching some data. That helps when
        // downloading videos. If we let the initial segment to have some space to
        // download video data, the video can show up much faster on screen.
        INITIAL_LATENCY = 1000,

        // The minimum content size allowed for parallel download. Lesser sized data will
        // not be downloaded simultaneously.
        MIN_CONTENT_SIZE = 102400
    };

public:
    explicit FastDownloader(const QUrl& url, int segmentSize = 5, QObject* parent = nullptr);
    explicit FastDownloader(QObject* parent = nullptr);

    QUrl resolvedUrl() const;

    QUrl url() const;
    void setUrl(const QUrl& url);

    int segmentSize() const;
    void setSegmentSize(int segmentSize);

    int maxRedirectsAllowed() const;
    void setMaxRedirectsAllowed(int maxRedirectsAllowed);

    bool isRunning() const;
    bool isParallelDownloadPossible() const;

    qint64 readBufferSize() const;
    void setReadBufferSize(qint64 size);

    QSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QSslConfiguration& config);

    bool atEnd(int segment) const;

    qint64 bytesAvailable(int segment = -1) const;

    qint64 skip(int segment, qint64 maxSize) const;

    qint64 read(int segment, char* data, qint64 maxSize) const;
    QByteArray read(int segment, qint64 maxSize) const;
    QByteArray readAll(int segment) const;

    qint64 readLine(int segment, char* data, qint64 maxSize) const;
    QByteArray readLine(int segment, qint64 maxSize = 0) const;

    QString errorString(int segment) const;

    void ignoreSslErrors(int segment);
    void ignoreSslErrors(int segment, const QList<QSslError>& errors);

public slots:
    bool start();
    void stop();
    void close();
    void abort();

protected:
    FastDownloader(FastDownloaderPrivate& dd, QObject* parent);

signals:
    void done();
    void redirected(const QUrl& url);
    void finished(int segment);
    void readyRead(int segment);
    void error(int segment, QNetworkReply::NetworkError code);
    void sslErrors(int segment, const QList<QSslError>& errors);
    void downloadProgress(int segment, qint64 bytesReceived, qint64 bytesTotal);

private:
    Q_PRIVATE_SLOT(d_func(), void _q_redirected(const QUrl&))
    Q_PRIVATE_SLOT(d_func(), void _q_finished())
    Q_PRIVATE_SLOT(d_func(), void _q_readyRead())
    Q_PRIVATE_SLOT(d_func(), void _q_error(QNetworkReply::NetworkError))
    Q_PRIVATE_SLOT(d_func(), void _q_sslErrors(const QList<QSslError>&))
    Q_PRIVATE_SLOT(d_func(), void _q_downloadProgress(qint64, qint64))
    Q_PRIVATE_SLOT(d_func(), void _q_launchOtherSegments())

private:
    QUrl m_url;
    int m_segmentSize;
    int m_maxRedirectsAllowed;
    qint64 m_readBufferSize;
    QSslConfiguration m_sslConfiguration;
};

#endif // FASTDOWNLOADER_H