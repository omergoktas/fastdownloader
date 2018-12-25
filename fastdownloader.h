#ifndef FASTDOWNLOADER_H
#define FASTDOWNLOADER_H

#include <QObject>
#include <QUrl>

class FastDownloaderPrivate;
class FastDownloader : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(FastDownloader)

public:
    explicit FastDownloader(const QUrl& url,
                            int numberOfSimultaneousConnections = 5,
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

    bool progress(int chunk = -1) const;

public slots:
    void start();
    void stop();
    void abort();

private:
    QUrl m_url;
    int m_numberOfSimultaneousConnections;
};

#endif // FASTDOWNLOADER_H