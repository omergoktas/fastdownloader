#include <fastdownloader_p.h>

FastDownloaderPrivate::FastDownloaderPrivate()
{

}

FastDownloaderPrivate::~FastDownloaderPrivate()
{

}

FastDownloader::FastDownloader(const QUrl& url, int numberOfSimultaneousConnections, QObject* parent)
    : QObject(parent)
    , m_url(url)
    , m_numberOfSimultaneousConnections(numberOfSimultaneousConnections)
{
}

FastDownloader::FastDownloader(QObject* parent) : FastDownloader(QUrl(), 5, parent)
{
}

QUrl FastDownloader::url() const
{
    return m_url;
}

void FastDownloader::setUrl(const QUrl& url)
{
    m_url = url;
}

int FastDownloader::numberOfSimultaneousConnections() const
{
    return m_numberOfSimultaneousConnections;
}

void FastDownloader::setNumberOfSimultaneousConnections(int numberOfSimultaneousConnections)
{
    m_numberOfSimultaneousConnections = numberOfSimultaneousConnections;
}

bool FastDownloader::isStarted() const
{

}

bool FastDownloader::isRunning() const
{

}

bool FastDownloader::isResolved() const
{

}

bool FastDownloader::isFinished() const
{

}

bool FastDownloader::progress(int chunk) const
{

}

void FastDownloader::start()
{

}

void FastDownloader::stop()
{

}

void FastDownloader::abort()
{

}