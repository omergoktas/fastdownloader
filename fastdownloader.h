#ifndef FASTDOWNLOADER_H
#define FASTDOWNLOADER_H

#include <QObject>
#include <QUrl>
#include <QSslError>
#include <QNetworkReply>

class FastDownloaderPrivate;
class FastDownloader : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(FastDownloader)
    Q_DECLARE_PRIVATE(FastDownloader)

public:
    explicit FastDownloader(const QUrl& url, int segmentSize = 5, QObject* parent = nullptr);
    explicit FastDownloader(QObject* parent = nullptr);
    ~FastDownloader() override;

    enum { MAX_SEGMENTS = 6 };

    QUrl url() const;
    void setUrl(const QUrl& url);

    int segmentSize() const;
    void setSegmentSize(int segmentSize);

    int maxRedirectsAllowed() const;
    void setMaxRedirectsAllowed(int maxRedirectsAllowed);

    bool isRunning() const;
    bool isResolved() const;

    qreal progress(int segment = -1) const;
    qint64 size(int segment = -1) const;
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
    void resolved();
    void finished(int segment);
    void readyRead(int segment);
    void progressChanged(int segment, qint64 bytesReceived, qint64 bytesTotal);
    void redirected(const QUrl &url);
    void error(QNetworkReply::NetworkError code);
    void sslErrors(const QList<QSslError>& errors);

private:
    QUrl m_url;
    int m_segmentSize;
    int m_maxRedirectsAllowed;
};

#endif // FASTDOWNLOADER_H