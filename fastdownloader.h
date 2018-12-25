#ifndef FASTDOWNLOADER_H
#define FASTDOWNLOADER_H

#include <QObject>
#include <QUrl>

class FastDownloader : public QObject
{
    Q_OBJECT
public:
    explicit FastDownloader(const QUrl& url,
                            int numberOfSimultaneousConnections = 5,
                            QObject *parent = nullptr);
    explicit FastDownloader(QObject *parent = nullptr);

    QUrl url() const;
    void setUrl(const QUrl& url);

    int numberOfSimultaneousConnections() const;
    void setNumberOfSimultaneousConnections(int numberOfSimultaneousConnections);

    bool progress(int chunk = -1) const;
    bool isResolved() const;
    bool isStarted() const;
    bool isRunning() const;
    bool isFinished() const;

public slots:
    void start();
    void stop();
    void abort();

private:
    QUrl m_url;
    int m_numberOfSimultaneousConnections;
};

#endif // FASTDOWNLOADER_H