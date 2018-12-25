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
    Q_DECLARE_PRIVATE(FastDownloader)

public:
    explicit FastDownloader(const QUrl& url,
                            int numberOfSimultaneousConnections = 5, // 6 max
                            QObject *parent = nullptr);
    explicit FastDownloader(QObject *parent = nullptr);

    QUrl url() const;
    void setUrl(const QUrl& url);

    int numberOfSimultaneousConnections() const;
    void setNumberOfSimultaneousConnections(int numberOfSimultaneousConnections);

    bool isStarted() const;
    bool isRunning() const;
    bool isResolved() const;
    bool isFinished() const;
    bool isSimultaneousDownloadAvailable() const;

    qreal progress(int chunk = -1) const;
    qint64 size(int chunk = -1) const;
    qint64 bytesAvailable(int chunk = -1) const;

    qint64 skip(int chunk, qint64 maxSize);

    qint64 read(int chunk, char* data, qint64 maxSize);
    QByteArray read(int chunk, qint64 maxSize);
    QByteArray readAll(int chunk);

    qint64 readLine(int chunk, char* data, qint64 maxSize);
    QByteArray readLine(int chunk, qint64 maxSize = 0);

    bool atEnd(int chunk) const;

    QString errorString(int chunk) const;

public slots:
    void start();
    void stop();
    void abort();

signals:
    void sslErrors(const QList<QSslError>& errors);
    void error(QNetworkReply::NetworkError code);
    void redirected(const QUrl &url);
    void progressChanged(int chunk, qint64 bytesReceived, qint64 bytesTotal);
    void readyRead(int chunk);
    void finished(int chunk);
    void resolved();
    void done();

private:
    QUrl m_url;
    int m_numberOfSimultaneousConnections;
};

#endif // FASTDOWNLOADER_H