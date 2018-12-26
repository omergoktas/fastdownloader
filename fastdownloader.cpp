#include <fastdownloader_p.h>

FastDownloaderPrivate::FastDownloaderPrivate() : QObjectPrivate()
    , manager(new QNetworkAccessManager)
    , running(false)
{

}

bool FastDownloaderPrivate::resolveUrl()
{
    QNetworkReply* reply = manager->get(makeRequest(true));
    if (!reply)
        return false;

    Segment initialSegment;
    initialSegment.reply = reply;
    connectSegment(initialSegment);
    return true;
}

QNetworkRequest FastDownloaderPrivate::makeRequest(bool initial) const
{
    Q_Q(const FastDownloader);
    QNetworkRequest request;
    request.setUrl(resolvedUrl);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, initial);
    request.setRawHeader("User-Agent", "FastDownloader");
    request.setMaximumRedirectsAllowed(initial ? q->maxRedirectsAllowed() : 0);
    return request;
}

void FastDownloaderPrivate::connectSegment(const Segment& segment)
{
    Q_Q(FastDownloader);

    QObject::connect(segment.reply, SIGNAL(downloadProgress(qint64,qint64)),
                     q, SLOT(_q_downloadProgress(qint64,qint64)));
    QObject::connect(segment.reply, SIGNAL(error(QNetworkReply::NetworkError)),
                     q, SLOT(error(QNetworkReply::NetworkError)));
    QObject::connect(segment.reply, SIGNAL(sslErrors(QList<QSslError>)),
                     q, SLOT(_q_sslErrors(QList<QSslError>)));
    QObject::connect(segment.reply, SIGNAL(finished()),
                     q, SLOT(_q_finished()));
    QObject::connect(segment.reply, SIGNAL(redirected(QUrl)),
                     q, SLOT(_q_redirected(QUrl)));
    QObject::connect(segment.reply, SIGNAL(readyRead()),
                     q, SLOT(_q_readyRead()));

    segments.append(segment);
}

void FastDownloaderPrivate::_q_redirected(const QUrl& url)
{

}

void FastDownloaderPrivate::_q_finished()
{

}

void FastDownloaderPrivate::_q_readyRead()
{

}

void FastDownloaderPrivate::_q_error(QNetworkReply::NetworkError code)
{

}

void FastDownloaderPrivate::_q_sslErrors(const QList<QSslError>& errors)
{

}

void FastDownloaderPrivate::_q_downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{

}

FastDownloader::FastDownloader(const QUrl& url, int segmentSize, QObject* parent)
    : QObject(*(new FastDownloaderPrivate), parent)
    , m_url(url)
    , m_segmentSize(segmentSize)
    , m_maxRedirectsAllowed(5)
{
}

FastDownloader::FastDownloader(QObject* parent) : FastDownloader(QUrl(), 5, parent)
{
}

FastDownloader::~FastDownloader()
{
}

QUrl FastDownloader::resolvedUrl() const
{
    Q_D(const FastDownloader);
    return d->resolvedUrl;
}

QUrl FastDownloader::url() const
{
    return m_url;
}

void FastDownloader::setUrl(const QUrl& url)
{
    m_url = url;
}

int FastDownloader::segmentSize() const
{
    return m_segmentSize;
}

void FastDownloader::setSegmentSize(int segmentSize)
{
    m_segmentSize = segmentSize;
}

bool FastDownloader::isRunning() const
{

}

qint64 FastDownloader::bytesAvailable(int segment) const
{

}

qint64 FastDownloader::skip(int segment, qint64 maxSize)
{

}

qint64 FastDownloader::read(int segment, char* data, qint64 maxSize)
{

}

QByteArray FastDownloader::read(int segment, qint64 maxSize)
{

}

QByteArray FastDownloader::readAll(int segment)
{

}

qint64 FastDownloader::readLine(int segment, char* data, qint64 maxSize)
{

}

QByteArray FastDownloader::readLine(int segment, qint64 maxSize)
{

}

bool FastDownloader::atEnd(int segment) const
{

}

QString FastDownloader::errorString(int segment) const
{

}

bool FastDownloader::start()
{
    Q_D(FastDownloader);

    if (isRunning()) {
        qWarning("A download is already in progress");
        return false;
    }

    if (m_segmentSize > MAX_SEGMENTS) {
        qWarning("Segment size exceeds maximum number of segments allowed");
        return false;
    }

    if (!m_url.isValid()) {
        qWarning("Url is not valid");
        return false;
    }

    d->running = true;
    d->resolvedUrl = m_url;

    return d->resolveUrl();
}

void FastDownloader::stop()
{

}

void FastDownloader::close()
{

}

void FastDownloader::abort()
{

}

FastDownloader::FastDownloader::FastDownloader(FastDownloaderPrivate& dd, QObject* parent)
    : QObject(dd, parent)
{
}

int FastDownloader::maxRedirectsAllowed() const
{
    return m_maxRedirectsAllowed;
}

void FastDownloader::setMaxRedirectsAllowed(int maxRedirectsAllowed)
{
    m_maxRedirectsAllowed = maxRedirectsAllowed;
}

#include "moc_fastdownloader.cpp"