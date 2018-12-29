#include <fastdownloader_p.h>
#include <QRandomGenerator>

FastDownloaderPrivate::FastDownloaderPrivate() : QObjectPrivate()
  , manager(new QNetworkAccessManager)
  , running(false)
  , resolved(false)
  , parallelDownloadPossible(false)
  , bytesReceived(0)
  , bytesTotal(0)
{
}

void FastDownloaderPrivate::startParallelDownloading()
{
    Q_Q(const FastDownloader);

    if (!running
            || !resolved
            || !parallelDownloadPossible
            || q->numberOfParallelConnections() < 2) {
        return;
    }

    int i = 0;
    qint64 begin = 0;
    qint64 end = -1;

    do {
        begin = end + 1;

        qint64 slice = 0;
        if (i == q->numberOfParallelConnections() - 1)
            slice = bytesTotal - begin;
        else
            slice = bytesTotal / q->numberOfParallelConnections();

        if (q->chunkSizeLimit() > 0)
            end = begin + qMin(q->chunkSizeLimit(), slice) - 1;
        else
            end = begin + slice - 1;

        createChunk(resolvedUrl, begin, end);
    } while(++i < q->numberOfParallelConnections());
}

bool FastDownloaderPrivate::testParallelDownload(const Chunk* chunk)
{
    Q_ASSERT(chunk && chunk->reply);
    return chunk->reply->hasRawHeader("Accept-Ranges")
            && chunk->reply->hasRawHeader("Content-Length")
            && chunk->reply->rawHeader("Accept-Ranges") == "bytes"
            && chunk->reply->rawHeader("Content-Length").toLongLong() > chunk->reply->bytesAvailable()
            && chunk->reply->rawHeader("Content-Length").toLongLong() > FastDownloader::MIN_CONTENT_SIZE;
}

void FastDownloaderPrivate::_q_finished()
{
    Q_Q(FastDownloader);

    Chunk* chunk = chunkFor(q->sender());
    q->finished(chunk->id);

    if (downloadCompleted())
        q->finished();
}

void FastDownloaderPrivate::deleteChunk(Chunk* chunk)
{
    Q_Q(const FastDownloader);

    chunk->reply->disconnect(q);
    if (chunk->reply->isRunning())
        chunk->reply->abort();
    chunk->reply->deleteLater();
    chunks.removeOne(chunk);
    delete chunk;
}

void FastDownloaderPrivate::createChunk(const QUrl& url, qint64 begin, qint64 end)
{
    Q_Q(const FastDownloader);

    bool isInitial = begin < 0;

    QNetworkRequest request;
    request.setUrl(url);
    request.setSslConfiguration(q->sslConfiguration());
    request.setPriority(QNetworkRequest::HighPriority);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, isInitial);
    request.setHeader(QNetworkRequest::UserAgentHeader, "FastDownloader");
    request.setMaximumRedirectsAllowed(isInitial ? q->maxRedirectsAllowed() : 0);

    if (!isInitial) {
        QByteArray range = "bytes=";
        range.append(QByteArray::number(begin));
        range.append('-');
        range.append(QByteArray::number(end));
        request.setRawHeader("Range", range);
    }

    QNetworkReply* reply = manager->get(request);
    reply->setReadBufferSize(q->readBufferSize());

    auto chunk = new Chunk;
    chunk->id = generateUniqueId();
    chunk->reply = reply;
    chunk->pos = begin;

    QObject::connect(chunk->reply, SIGNAL(finished()),
                     q, SLOT(_q_finished()));
    QObject::connect(chunk->reply, SIGNAL(readyRead()),
                     q, SLOT(_q_readyRead()));
    QObject::connect(chunk->reply, SIGNAL(redirected(QUrl)),
                     q, SLOT(_q_redirected(QUrl)));
    QObject::connect(chunk->reply, SIGNAL(error(QNetworkReply::NetworkError)),
                     q, SLOT(_q_error(QNetworkReply::NetworkError)));
    QObject::connect(chunk->reply, SIGNAL(sslErrors(const QList<QSslError>&)),
                     q, SLOT(_q_sslErrors(const QList<QSslError>&)));
    QObject::connect(chunk->reply, SIGNAL(downloadProgress(qint64,qint64)),
                     q, SLOT(_q_downloadProgress(qint64,qint64)));

    chunks.append(chunk);
}

quint32 FastDownloaderPrivate::generateUniqueId() const
{
    quint32 id;

    do {
        id = QRandomGenerator::global()->generate();
    } while(chunkExists(id));

    return id;
}

qint64 FastDownloaderPrivate::contentLength(const Chunk* chunk)
{
    Q_ASSERT(chunk && chunk->reply);
    QVariant contentLength = chunk->reply->header(QNetworkRequest::ContentLengthHeader);
    if (contentLength.isNull() || !contentLength.isValid())
        return -1;
    return contentLength.toLongLong();
}

bool FastDownloaderPrivate::chunkExists(quint32 id) const
{
    for (Chunk* chunk : chunks) {
        if (chunk->id == id)
            return true;
    }

    return false;
}

bool FastDownloaderPrivate::downloadCompleted() const
{
    for (Chunk* chunk : chunks) {
        if (chunk->reply->isRunning())
            return false;
    }

    return true;
}

Chunk* FastDownloaderPrivate::chunkFor(quint32 id) const
{
    for (Chunk* chunk : chunks) {
        if (chunk->id == id)
            return chunk;
    }

    return nullptr;
}

Chunk* FastDownloaderPrivate::chunkFor(const QObject* sender) const
{
    const auto reply = qobject_cast<const QNetworkReply*>(sender);
    Q_ASSERT(reply);

    for (Chunk* chunk : chunks) {
        if (chunk->reply == reply)
            return chunk;
    }

    Q_ASSERT(0);

    return nullptr;
}

void FastDownloaderPrivate::_q_redirected(const QUrl& url)
{
    Q_Q(FastDownloader);

    if (resolved) {
        qWarning("WARNING: Suspicious redirection is going to be rejected");
        q->close(); // TODO: Are we gonna keep the data exists?
        return;
    }

    emit q->redirected(url);
}

void FastDownloaderPrivate::_q_readyRead()
{
    Q_Q(FastDownloader);

    Chunk* chunk = chunkFor(q->sender());

    if (resolved) {
        bytesReceived += chunk->reply->bytesAvailable();
        chunk->data += chunk->reply->readAll();
        emit q->readyRead(chunk->id);
    } else {
        resolved = true;
        resolvedUrl = chunk->reply->url();
        bytesTotal = contentLength(chunk);
        parallelDownloadPossible = testParallelDownload(chunk);

        emit q->resolved(resolvedUrl);

        if (chunk->reply->isRunning()
                && parallelDownloadPossible
                && q->numberOfParallelConnections() > 1) {
            deleteChunk(chunk);
            startParallelDownloading();
        } else {
            bytesReceived += chunk->reply->bytesAvailable();
            chunk->pos = 0;
            chunk->data = chunk->reply->readAll();
            emit q->readyRead(chunk->id);
        }
    }
}

void FastDownloaderPrivate::_q_error(QNetworkReply::NetworkError code)
{
    Q_Q(FastDownloader);

    Chunk* chunk = chunkFor(q->sender());
    q->error(chunk->id, code);
}

void FastDownloaderPrivate::_q_sslErrors(const QList<QSslError>& errors)
{
    Q_Q(FastDownloader);

    Chunk* chunk = chunkFor(q->sender());
    q->sslErrors(chunk->id, errors);
}

void FastDownloaderPrivate::_q_downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_Q(FastDownloader);

    Chunk* chunk = chunkFor(q->sender());
    q->downloadProgress(chunk->id, bytesReceived, bytesTotal);
    q->downloadProgress(this->bytesReceived, this->bytesTotal);
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

qint64 FastDownloader::size() const
{
    Q_D(const FastDownloader);
    return d->bytesTotal;
}

FastDownloader::FastDownloader::FastDownloader(FastDownloaderPrivate& dd, QObject* parent)
    : QObject(dd, parent)
{
}

qint64 FastDownloader::chunkSizeLimit() const
{
    return m_chunkSizeLimit;
}

void FastDownloader::setChunkSizeLimit(qint64 chunkSizeLimit)
{
    Q_D(const FastDownloader);

    if (d->running) {
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
    Q_D(const FastDownloader);

    if (d->running) {
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
    Q_D(const FastDownloader);

    if (d->running) {
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
    Q_D(const FastDownloader);

    if (d->running) {
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

qint64 FastDownloader::readBufferSize() const
{
    return m_readBufferSize;
}

void FastDownloader::setReadBufferSize(qint64 size)
{
    Q_D(const FastDownloader);

    m_readBufferSize = size;

    if (d->running) {
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

    if (d->running) {
        for (Chunk* chunk : d->chunks)
            chunk->reply->setSslConfiguration(m_sslConfiguration);
    }
}

qint64 FastDownloader::bytesAvailable(quint32 id) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::bytesAvailable: No downloads in progress");
        return -1;
    }

    if (!d->chunkExists(id)) {
        qWarning("FastDownloader::bytesAvailable: No such chunk matches with the id provided");
        return -1;
    }

    return d->chunkFor(id)->reply->bytesAvailable();
}

qint64 FastDownloader::skip(quint32 id, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::skip: No downloads in progress");
        return -1;
    }

    if (!d->chunkExists(id)) {
        qWarning("FastDownloader::skip: No such chunk matches with the id provided");
        return -1;
    }

    return d->chunkFor(id)->reply->skip(maxSize);
}

qint64 FastDownloader::read(quint32 id, char* data, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::read: No downloads in progress");
        return -1;
    }

    if (!d->chunkExists(id)) {
        qWarning("FastDownloader::read: No such chunk matches with the id provided");
        return -1;
    }

    return d->chunkFor(id)->reply->read(data, maxSize);
}

QByteArray FastDownloader::read(quint32 id, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::read: No downloads in progress");
        return {};
    }

    if (!d->chunkExists(id)) {
        qWarning("FastDownloader::read: No such chunk matches with the id provided");
        return {};
    }

    return d->chunkFor(id)->reply->read(maxSize);
}

QByteArray FastDownloader::readAll(quint32 id) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::readAll: No downloads in progress");
        return {};
    }

    if (!d->chunkExists(id)) {
        qWarning("FastDownloader::readAll: No such chunk matches with the id provided");
        return {};
    }

    return d->chunkFor(id)->reply->readAll();
}

qint64 FastDownloader::readLine(quint32 id, char* data, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::readLine: No downloads in progress");
        return -1;
    }

    if (!d->chunkExists(id)) {
        qWarning("FastDownloader::readLine: No such chunk matches with the id provided");
        return -1;
    }

    return d->chunkFor(id)->reply->readLine(data, maxSize);
}

QByteArray FastDownloader::readLine(quint32 id, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::readLine: No downloads in progress");
        return {};
    }

    if (!d->chunkExists(id)) {
        qWarning("FastDownloader::readLine: No such chunk matches with the id provided");
        return {};
    }

    return d->chunkFor(id)->reply->readLine(maxSize);
}

bool FastDownloader::atEnd(quint32 id) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::atEnd: No downloads in progress");
        return true;
    }

    if (!d->chunkExists(id)) {
        qWarning("FastDownloader::atEnd: No such chunk matches with the id provided");
        return true;
    }

    return d->chunkFor(id)->reply->atEnd();
}

QString FastDownloader::errorString(quint32 id) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::errorString: No downloads in progress");
        return {};
    }

    if (!d->chunkExists(id)) {
        qWarning("FastDownloader::errorString: No such chunk matches with the id provided");
        return {};
    }

    return d->chunkFor(id)->reply->errorString();
}

void FastDownloader::ignoreSslErrors(quint32 id) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::ignoreSslErrors: No downloads in progress");
        return;
    }

    if (!d->chunkExists(id)) {
        qWarning("FastDownloader::ignoreSslErrors: No such chunk matches with the id provided");
        return;
    }

    return d->chunkFor(id)->reply->ignoreSslErrors();
}

void FastDownloader::ignoreSslErrors(quint32 id, const QList<QSslError>& errors) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::ignoreSslErrors: No downloads in progress");
        return;
    }

    if (!d->chunkExists(id)) {
        qWarning("FastDownloader::ignoreSslErrors: No such chunk matches with the id provided");
        return;
    }

    return d->chunkFor(id)->reply->ignoreSslErrors(errors);
}

void FastDownloader::start()
{
    Q_D(FastDownloader);

    if (d->running) {
        qWarning("FastDownloader::start: A download is already in progress");
        return;
    }

    if (m_numberOfParallelConnections < 1
            || m_numberOfParallelConnections > MAX_PARALLEL_CONNECTIONS) {
        qWarning("FastDownloader::start: Number of parallel connections is incorrect, "
                 "It may exceeds maximum number of parallel connections allowed");
        return;
    }

    if (!m_url.isValid()) {
        qWarning("FastDownloader::start: Url is invalid");
        return;
    }

    d->running = true;
    d->createChunk(m_url);
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