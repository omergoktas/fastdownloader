#ifndef FASTDOWNLOADER_H
#define FASTDOWNLOADER_H

#include <fastdownloader_global.h>

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

        // The minimum possible content size allowed for parallel downloads. Lesser sized data
        // will not be having parallel download feature enabled (100 Kb).
        MIN_CONTENT_SIZE = 102400
    };

public:
    explicit FastDownloader(const QUrl& url, int numberOfParallelConnections = 5, QObject* parent = nullptr);
    explicit FastDownloader(QObject* parent = nullptr);
    ~FastDownloader() override;

    qint64 contentLength() const;
    QUrl resolvedUrl() const;

    QUrl url() const;
    void setUrl(const QUrl& url);

    int numberOfParallelConnections() const;
    void setNumberOfParallelConnections(int numberOfParallelConnections);

    int maxRedirectsAllowed() const;
    void setMaxRedirectsAllowed(int maxRedirectsAllowed);

    bool isRunning() const;
    bool isResolved() const;
    bool isParallelDownloadPossible() const;

    qint64 connectionSizeLimit() const;
    void setConnectionSizeLimit(qint64 connectionSizeLimit);

    qint64 readBufferSize() const;
    void setReadBufferSize(qint64 size);

    QSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QSslConfiguration& config);

    QNetworkAccessManager* networkAccessManager() const;

    bool atEnd(int id) const;
    qint64 bytesAvailable(int id) const;
    qint64 peek(int id, char *data, qint64 maxSize);
    QByteArray peek(int id, qint64 maxSize);
    qint64 skip(int id, qint64 maxSize) const;
    qint64 read(int id, char* data, qint64 maxSize) const;
    QByteArray read(int id, qint64 maxSize) const;
    QByteArray readAll(int id) const;
    qint64 readLine(int id, char* data, qint64 maxSize) const;
    QByteArray readLine(int id, qint64 maxSize = 0) const;
    QString errorString(int id) const;
    void ignoreSslErrors(int id) const;
    void ignoreSslErrors(int id, const QList<QSslError>& errors) const;

public slots:
    bool start();
    void abort();

protected:
    FastDownloader(FastDownloaderPrivate& dd, QObject* parent);

signals:
    void finished();
    void finished(int id);
    void readyRead(int id);
    void redirected(const QUrl& url);
    void resolved(const QUrl& resolvedUrl);
    void error(int id, QNetworkReply::NetworkError code);
    void sslErrors(int id, const QList<QSslError>& errors);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadProgress(int id, qint64 bytesReceived, qint64 bytesTotal);

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
    qint64 m_connectionSizeLimit;
    qint64 m_readBufferSize;
    QSslConfiguration m_sslConfiguration;
};

#endif // FASTDOWNLOADER_H