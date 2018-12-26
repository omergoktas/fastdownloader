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

public:
    explicit FastDownloader(const QUrl& url, int segmentSize = 5, QObject* parent = nullptr);
    explicit FastDownloader(QObject* parent = nullptr);
    ~FastDownloader() override;

    enum { MAX_SEGMENTS = 6 };

    QUrl resolvedUrl() const;

    QUrl url() const;
    void setUrl(const QUrl& url);

    int segmentSize() const;
    void setSegmentSize(int segmentSize);

    int maxRedirectsAllowed() const;
    void setMaxRedirectsAllowed(int maxRedirectsAllowed);

    bool isRunning() const;

    qint64 bytesAvailable(int segment = -1) const;

    qint64 skip(int segment, qint64 maxSize);

    qint64 read(int segment, char* data, qint64 maxSize);
    QByteArray read(int segment, qint64 maxSize);
    QByteArray readAll(int segment);

    qint64 readLine(int segment, char* data, qint64 maxSize);
    QByteArray readLine(int segment, qint64 maxSize = 0);

    bool atEnd(int segment) const;

    QString errorString(int segment) const;

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

private:
    QUrl m_url;
    int m_segmentSize;
    int m_maxRedirectsAllowed;
};

#endif // FASTDOWNLOADER_H