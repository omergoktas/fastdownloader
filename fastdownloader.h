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
        MAX_PARALLEL_CONNECTIONS = 6,

        // The minimum content size allowed for parallel download. Lesser sized data will
        // not be downloaded simultaneously.
        MIN_CONTENT_SIZE = 102400
    };

public:
    explicit FastDownloader(const QUrl& url, int numberOfParallelConnections = 5, QObject* parent = nullptr);
    explicit FastDownloader(QObject* parent = nullptr);

    QUrl resolvedUrl() const;

    QUrl url() const;
    void setUrl(const QUrl& url);

    int numberOfParallelConnections() const;
    void setNumberOfParallelConnections(int numberOfParallelConnections);

    int maxRedirectsAllowed() const;
    void setMaxRedirectsAllowed(int maxRedirectsAllowed);

    bool isRunning() const;
    bool isParallelDownloadPossible() const;

    qint64 chunkSizeLimit() const;
    void setChunkSizeLimit(const qint64& chunkSizeLimit);

    qint64 readBufferSize() const;
    void setReadBufferSize(qint64 size);

    QSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QSslConfiguration& config);

    bool atEnd(int id) const;
    qint64 bytesAvailable(int id) const;
    qint64 skip(int id, qint64 maxSize) const;
    qint64 read(int id, char* data, qint64 maxSize) const;
    QByteArray read(int id, qint64 maxSize) const;
    QByteArray readAll(int id) const;
    qint64 readLine(int id, char* data, qint64 maxSize) const;
    QByteArray readLine(int id, qint64 maxSize = 0) const;
    QString errorString(int id) const;
    void ignoreSslErrors(int id);
    void ignoreSslErrors(int id, const QList<QSslError>& errors);

public slots:
    bool start();
    void stop();
    void close();
    void abort();

protected:
    FastDownloader(FastDownloaderPrivate& dd, QObject* parent);

signals:
    void finished();
    void finished(quint32 id);
    void readyRead(quint32 id);
    void redirected(const QUrl& url);
    void error(quint32 id, QNetworkReply::NetworkError code);
    void sslErrors(quint32 id, const QList<QSslError>& errors);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadProgress(quint32 id, qint64 bytesReceived, qint64 bytesTotal);

private:
    Q_PRIVATE_SLOT(d_func(), void _q_finished())
    Q_PRIVATE_SLOT(d_func(), void _q_readyRead())
    Q_PRIVATE_SLOT(d_func(), void _q_redirected(const QUrl&))
    Q_PRIVATE_SLOT(d_func(), void _q_error(QNetworkReply::NetworkError))
    Q_PRIVATE_SLOT(d_func(), void _q_sslErrors(const QList<QSslError>&))
    Q_PRIVATE_SLOT(d_func(), void _q_downloadProgress(qint64, qint64))

private:
    QUrl m_url;
    int m_numberOfParallelConnections;
    int m_maxRedirectsAllowed;
    qint64 m_chunkSizeLimit;
    qint64 m_readBufferSize;
    QSslConfiguration m_sslConfiguration;
};

#endif // FASTDOWNLOADER_H