#include <fastdownloader_p.h>
#include <QTimer>
#include <QRandomGenerator>

FastDownloaderPrivate::FastDownloaderPrivate() : QObjectPrivate()
  , manager(new QNetworkAccessManager)
  , running(false)
  , parallelDownloadPossible(false)
{
}

bool FastDownloaderPrivate::resolveUrl()
{
    Q_Q(const FastDownloader);
    QNetworkReply* reply = manager->get(makeRequest(true));
    if (!reply)
        return false;

    reply->setReadBufferSize(q->readBufferSize());

    auto chunk = new Chunk;
    chunk->id = generateUniqueId();
    chunk->reply = reply;
    chunks.append(chunk);

    connectChunk(chunk);

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
    request.setSslConfiguration(q->sslConfiguration());
    request.setPriority(QNetworkRequest::HighPriority);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, initial);
    request.setHeader(QNetworkRequest::UserAgentHeader, "FastDownloader");
    request.setMaximumRedirectsAllowed(initial ? q->maxRedirectsAllowed() : 0);
    return request;
}

void FastDownloaderPrivate::connectChunk(const Chunk* chunk) const
{
    Q_Q(const FastDownloader);
    QObject::connect(chunk->reply, SIGNAL(downloadProgress(qint64,qint64)),
                     q, SLOT(_q_downloadProgress(qint64,qint64)));
    QObject::connect(chunk->reply, SIGNAL(error(QNetworkReply::NetworkError)),
                     q, SLOT(_q_error(QNetworkReply::NetworkError)));
    QObject::connect(chunk->reply, SIGNAL(sslErrors(QList<QSslError>)),
                     q, SLOT(_q_sslErrors(QList<QSslError>)));
    QObject::connect(chunk->reply, SIGNAL(redirected(QUrl)),
                     q, SLOT(_q_redirected(QUrl)));
    QObject::connect(chunk->reply, SIGNAL(readyRead()),
                     q, SLOT(_q_readyRead()));
}

bool FastDownloaderPrivate::chunkExists(quint32 id) const
{
    for (Chunk* chunk : chunks) {
        if (chunk->id == id)
            return true;
    }
    return false;
}

quint32 FastDownloaderPrivate::generateUniqueId() const
{
    quint32 id;
    do {
        id = QRandomGenerator::global()->generate();
    } while(chunkExists(id));
    return id;
}

Chunk* FastDownloaderPrivate::chunkFor(const QObject* sender) const
{
    const auto reply = qobject_cast<const QNetworkReply*>(sender);
    if (!reply)
        return nullptr;

    for (Chunk* chunk : chunks) {
        if (chunk->reply == reply)
            return chunk;
    }

    return nullptr;
}

void FastDownloaderPrivate::_q_redirected(const QUrl& url)
{
    Q_Q(FastDownloader);

    Chunk* chunk = chunkFor(q->sender());
    Q_ASSERT(chunk && chunk == chunks.first());

    if (chunk->bytesReceived >= 0) {
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

    Chunk* chunk = chunkFor(q->sender());
    Q_ASSERT(chunk);

    if (chunks.first() == chunk && chunk->bytesReceived < 0) {
        parallelDownloadPossible = isParallelDownloadPossible(chunk->reply);
        chunk->bytesReceived = chunk->reply->bytesAvailable();
        chunk->data = chunk->reply->readAll();
        QTimer::singleShot(FastDownloader::INITIAL_LATENCY, q, SLOT(_q_activateOtherChunks()));
    } else {
        chunk->bytesReceived += chunk->reply->bytesAvailable();
        chunk->data += chunk->reply->readAll();
    }

    emit q->readyRead(chunk->index);
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

void FastDownloaderPrivate::_q_activateOtherConnections()
{

}

FastDownloader::FastDownloader(const QUrl& url, int numberOfParallelConnections, QObject* parent)
    : QObject(*(new FastDownloaderPrivate), parent)
    , m_url(url)
    , m_numberOfParallelConnections(numberOfParallelConnections)
    , m_maxRedirectsAllowed(5)
    , m_chunkSizeLimit(0)
    , m_readBufferSize(0)
{
}

FastDownloader::FastDownloader(QObject* parent) : FastDownloader(QUrl(), 5, parent)
{
}

FastDownloader::FastDownloader::FastDownloader(FastDownloaderPrivate& dd, QObject* parent)
    : QObject(dd, parent)
{
}

qint64 FastDownloader::chunkSizeLimit() const
{
    return m_chunkSizeLimit;
}

void FastDownloader::setChunkSizeLimit(const qint64& chunkSizeLimit)
{
    if (isRunning()) {
        qWarning("FastDownloader::setChunkSizeLimit: Cannot set, a download is already in progress");
        return;
    }
    m_chunkSizeLimit = chunkSizeLimit;
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

int FastDownloader::numberOfParallelConnections() const
{
    return m_numberOfParallelConnections;
}

void FastDownloader::setNumberOfParallelConnections(int numberOfParallelConnections)
{
    if (isRunning()) {
        qWarning("FastDownloader::setNumberOfParallelConnections: "
                 "Cannot set, a download is already in progress");
        return;
    }

    m_numberOfParallelConnections = numberOfParallelConnections;
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

bool FastDownloader::isParallelDownloadPossible() const
{
    Q_D(const FastDownloader);
    return d->parallelDownloadPossible;
}

qint64 FastDownloader::readBufferSize() const
{
    return m_readBufferSize;
}

void FastDownloader::setReadBufferSize(qint64 size)
{
    Q_D(const FastDownloader);
    m_readBufferSize = size;
    if (isRunning()) {
        for (Chunk* chunk : d->chunks)
            chunk->reply->setReadBufferSize(m_readBufferSize);
    }
}

QSslConfiguration FastDownloader::sslConfiguration() const
{
    return m_sslConfiguration;
}

void FastDownloader::setSslConfiguration(const QSslConfiguration& config)
{
    Q_D(const FastDownloader);
    m_sslConfiguration = config;
    if (isRunning()) {
        for (Chunk* chunk : d->chunks)
            chunk->reply->setSslConfiguration(m_sslConfiguration);
    }
}

qint64 FastDownloader::bytesAvailable(int chunk) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::bytesAvailable: No downloads in progress");
        return -1;
    }

    if (chunk < 0 || chunk >= d->chunks.size()) {
        qWarning("FastDownloader::bytesAvailable: Invalid chunk index");
        return -1;
    }

    return d->replyFor(chunk)->bytesAvailable();
}

qint64 FastDownloader::skip(int chunk, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::skip: No downloads in progress");
        return -1;
    }

    if (chunk < 0 || chunk >= d->chunks.size()) {
        qWarning("FastDownloader::skip: Invalid chunk index");
        return -1;
    }

    return d->replyFor(chunk)->skip(maxSize);
}

qint64 FastDownloader::read(int chunk, char* data, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::read: No downloads in progress");
        return -1;
    }

    if (chunk < 0 || chunk >= d->chunks.size()) {
        qWarning("FastDownloader::read: Invalid chunk index");
        return -1;
    }

    return d->replyFor(chunk)->read(data, maxSize);
}

QByteArray FastDownloader::read(int chunk, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::read: No downloads in progress");
        return {};
    }

    if (chunk < 0 || chunk >= d->chunks.size()) {
        qWarning("FastDownloader::read: Invalid chunk index");
        return {};
    }

    return d->replyFor(chunk)->read(maxSize);
}

QByteArray FastDownloader::readAll(int chunk) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::readAll: No downloads in progress");
        return {};
    }

    if (chunk < 0 || chunk >= d->chunks.size()) {
        qWarning("FastDownloader::readAll: Invalid chunk index");
        return {};
    }

    return d->replyFor(chunk)->readAll();
}

qint64 FastDownloader::readLine(int chunk, char* data, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::readLine: No downloads in progress");
        return -1;
    }

    if (chunk < 0 || chunk >= d->chunks.size()) {
        qWarning("FastDownloader::readLine: Invalid chunk index");
        return -1;
    }

    return d->replyFor(chunk)->readLine(data, maxSize);
}

QByteArray FastDownloader::readLine(int chunk, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::readLine: No downloads in progress");
        return {};
    }

    if (chunk < 0 || chunk >= d->chunks.size()) {
        qWarning("FastDownloader::readLine: Invalid chunk index");
        return {};
    }

    return d->replyFor(chunk)->readLine(maxSize);
}

bool FastDownloader::atEnd(int chunk) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::atEnd: No downloads in progress");
        return true;
    }

    if (chunk < 0 || chunk >= d->chunks.size()) {
        qWarning("FastDownloader::atEnd: Invalid chunk index");
        return true;
    }

    return d->replyFor(chunk)->atEnd();
}

QString FastDownloader::errorString(int chunk) const
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::errorString: No downloads in progress");
        return {};
    }

    if (chunk < 0 || chunk >= d->chunks.size()) {
        qWarning("FastDownloader::errorString: Invalid chunk index");
        return {};
    }

    return d->replyFor(chunk)->errorString();
}

void FastDownloader::ignoreSslErrors(int chunk)
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::ignoreSslErrors: No downloads in progress");
        return;
    }

    if (chunk < 0 || chunk >= d->chunks.size()) {
        qWarning("FastDownloader::ignoreSslErrors: Invalid chunk index");
        return;
    }

    return d->replyFor(chunk)->ignoreSslErrors();
}

void FastDownloader::ignoreSslErrors(int chunk, const QList<QSslError>& errors)
{
    Q_D(const FastDownloader);

    if (!isRunning()) {
        qWarning("FastDownloader::ignoreSslErrors: No downloads in progress");
        return;
    }

    if (chunk < 0 || chunk >= d->chunks.size()) {
        qWarning("FastDownloader::ignoreSslErrors: Invalid chunk index");
        return;
    }

    return d->replyFor(chunk)->ignoreSslErrors(errors);
}

bool FastDownloader::start()
{
    Q_D(FastDownloader);

    if (isRunning()) {
        qWarning("FastDownloader::start: A download is already in progress");
        return false;
    }

    if (m_numberOfParallelConnections < 1 || m_numberOfParallelConnections > MAX_PARALLEL_CONNECTIONS) {
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