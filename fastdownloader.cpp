#include <fastdownloader_p.h>

FastDownloaderPrivate::FastDownloaderPrivate() : QObjectPrivate()
  , manager(new QNetworkAccessManager)
  , running(false)
  , resolved(false)
  , parallelDownloadPossible(false)
{
}

bool FastDownloaderPrivate::resolveUrl()
{
    QNetworkReply* reply = manager->get(makeRequest(true));
    if (!reply)
        return false;

    auto initialSegment = new Segment;
    initialSegment->index = 0;
    initialSegment->reply = reply;
    segments.append(initialSegment);

    connectSegment(initialSegment);

    return true;
}

inline bool FastDownloaderPrivate::isParallelDownloadPossible(const QNetworkReply* reply) const
{
    return reply
            && reply->hasRawHeader("Accept-Ranges")
            && reply->hasRawHeader("Content-Length")
            && reply->rawHeader("Accept-Ranges") == "bytes"
            && reply->rawHeader("Content-Length").toLongLong() > FastDownloader::MIN_CONTENT_SIZE;
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

void FastDownloaderPrivate::connectSegment(const Segment* segment) const
{
    Q_Q(const FastDownloader);
    QObject::connect(segment->reply, SIGNAL(downloadProgress(qint64,qint64)),
                     q, SLOT(_q_downloadProgress(qint64,qint64)));
    QObject::connect(segment->reply, SIGNAL(error(QNetworkReply::NetworkError)),
                     q, SLOT(_q_error(QNetworkReply::NetworkError)));
    QObject::connect(segment->reply, SIGNAL(sslErrors(QList<QSslError>)),
                     q, SLOT(_q_sslErrors(QList<QSslError>)));
    QObject::connect(segment->reply, SIGNAL(finished()),
                     q, SLOT(_q_finished()));
    QObject::connect(segment->reply, SIGNAL(redirected(QUrl)),
                     q, SLOT(_q_redirected(QUrl)));
    QObject::connect(segment->reply, SIGNAL(readyRead()),
                     q, SLOT(_q_readyRead()));
}

void FastDownloaderPrivate::_q_redirected(const QUrl& url)
{
    Q_Q(FastDownloader);

    if (resolved) {
        qWarning("WARNING: Suspicious redirection is going to be rejected");
        q->close();
        return;
    }

    resolvedUrl = url;

    emit q->redirected(url);
}

void FastDownloaderPrivate::_q_readyRead()
{
    Q_Q(FastDownloader);

    auto reply = qobject_cast<QNetworkReply*>(q->sender());
    Q_ASSERT(reply);

    if (!resolved) {
        Q_ASSERT(reply == segments.first()->reply);
        resolved = true;
        parallelDownloadPossible = isParallelDownloadPossible(reply);

        if (!parallelDownloadPossible)
            qWarning("WARNING: Parallel download is not possible");

        Segment* segment = getSegment(reply);
        Q_ASSERT(segment);

        segment->bytesReceived = segment->reply->bytesAvailable();

        emit q->resolved(resolvedUrl);
        emit q->readyRead(segment->index);
    }
}

void FastDownloaderPrivate::_q_finished()
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

FastDownloader::FastDownloader::FastDownloader(FastDownloaderPrivate& dd, QObject* parent)
    : QObject(dd, parent)
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
    if (isRunning()) {
        qWarning("FastDownloader::setUrl: Cannot set, a download is already in progress");
        return;
    }

    m_url = url;
}

int FastDownloader::segmentSize() const
{
    return m_segmentSize;
}

void FastDownloader::setSegmentSize(int segmentSize)
{
    if (isRunning()) {
        qWarning("FastDownloader::setSegmentSize: Cannot set, a download is already in progress");
        return;
    }

    m_segmentSize = segmentSize;
}

int FastDownloader::maxRedirectsAllowed() const
{
    return m_maxRedirectsAllowed;
}

void FastDownloader::setMaxRedirectsAllowed(int maxRedirectsAllowed)
{
    if (isRunning()) {
        qWarning("FastDownloader::setMaxRedirectsAllowed: "
                 "Cannot set, a download is already in progress");
        return;
    }

    m_maxRedirectsAllowed = maxRedirectsAllowed;
}

bool FastDownloader::isRunning() const
{
    Q_D(const FastDownloader);
    return d->running;
}

bool FastDownloader::isResolved() const
{
    Q_D(const FastDownloader);
    return d->resolved;
}

bool FastDownloader::isParallelDownloadPossible() const
{
    Q_D(const FastDownloader);
    return d->parallelDownloadPossible;
}

qint64 FastDownloader::bytesAvailable(int segment) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::bytesAvailable: No downloads in progress");
        return -1;
    }

    if (segment < 0 || segment >= d->segments.size()) {
        qWarning("FastDownloader::bytesAvailable: Invalid segment index");
        return -1;
    }

    return d->getReply(segment)->bytesAvailable();
}

qint64 FastDownloader::skip(int segment, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::skip: No downloads in progress");
        return -1;
    }

    if (segment < 0 || segment >= d->segments.size()) {
        qWarning("FastDownloader::skip: Invalid segment index");
        return -1;
    }

    return d->getReply(segment)->skip(maxSize);
}

qint64 FastDownloader::read(int segment, char* data, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::read: No downloads in progress");
        return -1;
    }

    if (segment < 0 || segment >= d->segments.size()) {
        qWarning("FastDownloader::read: Invalid segment index");
        return -1;
    }

    return d->getReply(segment)->read(data, maxSize);
}

QByteArray FastDownloader::read(int segment, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::read: No downloads in progress");
        return {};
    }

    if (segment < 0 || segment >= d->segments.size()) {
        qWarning("FastDownloader::read: Invalid segment index");
        return {};
    }

    return d->getReply(segment)->read(maxSize);
}

QByteArray FastDownloader::readAll(int segment) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::readAll: No downloads in progress");
        return {};
    }

    if (segment < 0 || segment >= d->segments.size()) {
        qWarning("FastDownloader::readAll: Invalid segment index");
        return {};
    }

    return d->getReply(segment)->readAll();
}

qint64 FastDownloader::readLine(int segment, char* data, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::readLine: No downloads in progress");
        return -1;
    }

    if (segment < 0 || segment >= d->segments.size()) {
        qWarning("FastDownloader::readLine: Invalid segment index");
        return -1;
    }

    return d->getReply(segment)->readLine(data, maxSize);
}

QByteArray FastDownloader::readLine(int segment, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::readLine: No downloads in progress");
        return {};
    }

    if (segment < 0 || segment >= d->segments.size()) {
        qWarning("FastDownloader::readLine: Invalid segment index");
        return {};
    }

    return d->getReply(segment)->readLine(maxSize);
}

bool FastDownloader::atEnd(int segment) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::atEnd: No downloads in progress");
        return true;
    }

    if (segment < 0 || segment >= d->segments.size()) {
        qWarning("FastDownloader::atEnd: Invalid segment index");
        return true;
    }

    return d->getReply(segment)->atEnd();
}

QString FastDownloader::errorString(int segment) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::errorString: No downloads in progress");
        return {};
    }

    if (segment < 0 || segment >= d->segments.size()) {
        qWarning("FastDownloader::errorString: Invalid segment index");
        return {};
    }

    return d->getReply(segment)->errorString();
}

void FastDownloader::ignoreSslErrors(int segment)
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::ignoreSslErrors: No downloads in progress");
        return;
    }

    if (segment < 0 || segment >= d->segments.size()) {
        qWarning("FastDownloader::ignoreSslErrors: Invalid segment index");
        return;
    }

    return d->getReply(segment)->ignoreSslErrors();
}

void FastDownloader::ignoreSslErrors(int segment, const QList<QSslError>& errors)
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::ignoreSslErrors: No downloads in progress");
        return;
    }

    if (segment < 0 || segment >= d->segments.size()) {
        qWarning("FastDownloader::ignoreSslErrors: Invalid segment index");
        return;
    }

    return d->getReply(segment)->ignoreSslErrors(errors);
}

bool FastDownloader::start()
{
    Q_D(FastDownloader);

    if (isRunning()) {
        qWarning("FastDownloader::start: A download is already in progress");
        return false;
    }

    if (m_segmentSize < 1 || m_segmentSize > MAX_SEGMENTS) {
        qWarning("FastDownloader::start: Segment size is incorrect, "
                 "It may exceeds maximum number of segments allowed");
        return false;
    }

    if (!m_url.isValid()) {
        qWarning("FastDownloader::start: Url is not valid");
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

#include "moc_fastdownloader.cpp"