#include <fastdownloader_p.h>
#include <QRandomGenerator>

FastDownloaderPrivate::FastDownloaderPrivate() : QObjectPrivate()
  , manager(new QNetworkAccessManager)
  , running(false)
  , resolved(false)
  , parallelDownloadPossible(false)
  , contentLength(-1)
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
            slice = contentLength - begin;
        else
            slice = contentLength / q->numberOfParallelConnections();

        if (q->connectionSizeLimit() > 0)
            end = begin + qMin(q->connectionSizeLimit(), slice) - 1;
        else
            end = begin + slice - 1;

        createConnection(resolvedUrl, begin, end);
    } while(++i < q->numberOfParallelConnections());
}

qint64 FastDownloaderPrivate::calculateTotalBytesReceived() const
{
    qint64 total = 0;
    for (Connection* connection : connections)
        total += connection->bytesReceived;
    return total;
}

bool FastDownloaderPrivate::testParallelDownload(const FastDownloaderPrivate::Connection* connection)
{
    Q_ASSERT(connection && connection->reply);

    return connection->reply->hasRawHeader("Accept-Ranges")
            && connection->reply->hasRawHeader("Content-Length")
            && connection->reply->rawHeader("Accept-Ranges") == "bytes"
            && connection->reply->rawHeader("Content-Length").toLongLong() > connection->reply->bytesAvailable()
            && connection->reply->rawHeader("Content-Length").toLongLong() > FastDownloader::MIN_CONTENT_SIZE;
}

void FastDownloaderPrivate::_q_finished()
{
    Q_Q(FastDownloader);

    Connection* connection = connectionFor(q->sender());
    q->finished(connection->id);

    if (downloadCompleted())
        q->finished();

    if (connection->reply->error() != QNetworkReply::NoError) {
        q->close(); // WARNING: That may also trigger an error, and maybe a "finished" too,
        // and there might a race condition occur
    }
}

void FastDownloaderPrivate::deleteConnection(FastDownloaderPrivate::Connection* connection)
{
    Q_Q(const FastDownloader);
    connection->reply->disconnect(q);
    if (connection->reply->isRunning())
        connection->reply->abort();
    connection->reply->deleteLater();
    connections.removeOne(connection);
    delete connection;
}

void FastDownloaderPrivate::createConnection(const QUrl& url, qint64 begin, qint64 end)
{
    Q_Q(const FastDownloader);

    const bool isInitial = begin < 0;

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

    auto connection = new Connection;
    connection->id = generateUniqueId();
    connection->reply = reply;

    QObject::connect(connection->reply, SIGNAL(finished()),
                     q, SLOT(_q_finished()));
    QObject::connect(connection->reply, SIGNAL(readyRead()),
                     q, SLOT(_q_readyRead()));
    QObject::connect(connection->reply, SIGNAL(redirected(QUrl)),
                     q, SLOT(_q_redirected(QUrl)));
    QObject::connect(connection->reply, SIGNAL(error(QNetworkReply::NetworkError)),
                     q, SLOT(_q_error(QNetworkReply::NetworkError)));
    QObject::connect(connection->reply, SIGNAL(sslErrors(const QList<QSslError>&)),
                     q, SLOT(_q_sslErrors(const QList<QSslError>&)));
    QObject::connect(connection->reply, SIGNAL(downloadProgress(qint64,qint64)),
                     q, SLOT(_q_downloadProgress(qint64,qint64)));

    connections.append(connection);
}

quint32 FastDownloaderPrivate::generateUniqueId() const
{
    quint32 id;
    do {
        id = QRandomGenerator::global()->generate();
    } while(connectionExists(id));
    return id;
}

qint64 FastDownloaderPrivate::testContentLength(const FastDownloaderPrivate::Connection* connection)
{
    Q_ASSERT(connection && connection->reply);
    QVariant contentLength = connection->reply->header(QNetworkRequest::ContentLengthHeader);
    if (contentLength.isNull() || !contentLength.isValid())
        return -1;
    return contentLength.toLongLong();
}

bool FastDownloaderPrivate::connectionExists(quint32 id) const
{
    for (Connection* connection : connections) {
        if (connection->id == id)
            return true;
    }
    return false;
}

bool FastDownloaderPrivate::downloadCompleted() const
{
    for (Connection* connection : connections) {
        if (connection->reply->isRunning())
            return false;
    }
    return true;
}

FastDownloaderPrivate::Connection* FastDownloaderPrivate::connectionFor(quint32 id) const
{
    for (Connection* connection : connections) {
        if (connection->id == id)
            return connection;
    }
    return nullptr;
}

FastDownloaderPrivate::Connection* FastDownloaderPrivate::connectionFor(const QObject* sender) const
{
    const auto reply = qobject_cast<const QNetworkReply*>(sender);
    Q_ASSERT(reply);

    for (Connection* connection : connections) {
        if (connection->reply == reply)
            return connection;
    }

    Q_ASSERT(0);
    return nullptr;
}

void FastDownloaderPrivate::_q_redirected(const QUrl& url)
{
    Q_Q(FastDownloader);

    /*!
       NOTE: Unproper error expression, no errorString either.
             May Qt handles the same situation though.
    */
    if (resolved) {
        qWarning("WARNING: Suspicious redirection rejected");
        q->abort(); // Triggers an OperationCanceledError in anyway
        return;
    }

    emit q->redirected(url);
}

void FastDownloaderPrivate::_q_readyRead()
{
    Q_Q(FastDownloader);

    Connection* connection = connectionFor(q->sender());

    if (resolved) {
        emit q->readyRead(connection->id);
    } else {
        resolved = true;
        resolvedUrl = connection->reply->url();
        contentLength = testContentLength(connection);
        parallelDownloadPossible = testParallelDownload(connection);

        emit q->resolved(resolvedUrl);

        if (connection->reply->isRunning()
                && parallelDownloadPossible
                && q->numberOfParallelConnections() > 1) {
            deleteConnection(connection);
            startParallelDownloading();
        } else {
            emit q->readyRead(connection->id);
        }
    }
}

void FastDownloaderPrivate::_q_error(QNetworkReply::NetworkError code)
{
    Q_Q(FastDownloader);
    q->error(connectionFor(q->sender())->id, code);
}

void FastDownloaderPrivate::_q_sslErrors(const QList<QSslError>& errors)
{
    Q_Q(FastDownloader);
    q->sslErrors(connectionFor(q->sender())->id, errors);
}

void FastDownloaderPrivate::_q_downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_Q(FastDownloader);
    Connection* connection = connectionFor(q->sender());
    connection->bytesReceived = bytesReceived;
    q->downloadProgress(connection->id, bytesReceived, bytesTotal);
    q->downloadProgress(calculateTotalBytesReceived(), contentLength);
}

FastDownloader::FastDownloader(const QUrl& url, int numberOfParallelConnections, QObject* parent)
    : QObject(*(new FastDownloaderPrivate), parent)
    , m_url(url)
    , m_numberOfParallelConnections(numberOfParallelConnections)
    , m_maxRedirectsAllowed(5)
    , m_connectionSizeLimit(0)
    , m_readBufferSize(0)
{
}

FastDownloader::FastDownloader(QObject* parent) : FastDownloader(QUrl(), 5, parent)
{
}

qint64 FastDownloader::contentLength() const
{
    Q_D(const FastDownloader);
    return d->contentLength;
}

FastDownloader::FastDownloader::FastDownloader(FastDownloaderPrivate& dd, QObject* parent)
    : QObject(dd, parent)
{
}

qint64 FastDownloader::connectionSizeLimit() const
{
    return m_connectionSizeLimit;
}

void FastDownloader::setConnectionSizeLimit(qint64 connectionSizeLimit)
{
    Q_D(const FastDownloader);

    if (d->running) {
        qWarning("FastDownloader::setConnectionSizeLimit: Cannot set, a download is already in progress");
        return;
    }

    m_connectionSizeLimit = connectionSizeLimit;
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
        for (FastDownloaderPrivate::Connection* connection : d->connections)
            connection->reply->setReadBufferSize(m_readBufferSize);
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
        for (FastDownloaderPrivate::Connection* connection : d->connections)
            connection->reply->setSslConfiguration(m_sslConfiguration);
    }
}

qint64 FastDownloader::bytesAvailable(quint32 id) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::bytesAvailable: No downloads in progress");
        return -1;
    }

    if (!d->connectionExists(id)) {
        qWarning("FastDownloader::bytesAvailable: No such connection matches with the id provided");
        return -1;
    }

    return d->connectionFor(id)->reply->bytesAvailable();
}

qint64 FastDownloader::skip(quint32 id, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::skip: No downloads in progress");
        return -1;
    }

    if (!d->connectionExists(id)) {
        qWarning("FastDownloader::skip: No such connection matches with the id provided");
        return -1;
    }

    return d->connectionFor(id)->reply->skip(maxSize);
}

qint64 FastDownloader::read(quint32 id, char* data, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::read: No downloads in progress");
        return -1;
    }

    if (!d->connectionExists(id)) {
        qWarning("FastDownloader::read: No such connection matches with the id provided");
        return -1;
    }

    return d->connectionFor(id)->reply->read(data, maxSize);
}

QByteArray FastDownloader::read(quint32 id, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::read: No downloads in progress");
        return {};
    }

    if (!d->connectionExists(id)) {
        qWarning("FastDownloader::read: No such connection matches with the id provided");
        return {};
    }

    return d->connectionFor(id)->reply->read(maxSize);
}

QByteArray FastDownloader::readAll(quint32 id) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::readAll: No downloads in progress");
        return {};
    }

    if (!d->connectionExists(id)) {
        qWarning("FastDownloader::readAll: No such connection matches with the id provided");
        return {};
    }

    return d->connectionFor(id)->reply->readAll();
}

qint64 FastDownloader::readLine(quint32 id, char* data, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::readLine: No downloads in progress");
        return -1;
    }

    if (!d->connectionExists(id)) {
        qWarning("FastDownloader::readLine: No such connection matches with the id provided");
        return -1;
    }

    return d->connectionFor(id)->reply->readLine(data, maxSize);
}

QByteArray FastDownloader::readLine(quint32 id, qint64 maxSize) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::readLine: No downloads in progress");
        return {};
    }

    if (!d->connectionExists(id)) {
        qWarning("FastDownloader::readLine: No such connection matches with the id provided");
        return {};
    }

    return d->connectionFor(id)->reply->readLine(maxSize);
}

bool FastDownloader::atEnd(quint32 id) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::atEnd: No downloads in progress");
        return true;
    }

    if (!d->connectionExists(id)) {
        qWarning("FastDownloader::atEnd: No such connection matches with the id provided");
        return true;
    }

    return d->connectionFor(id)->reply->atEnd();
}

QString FastDownloader::errorString(quint32 id) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::errorString: No downloads in progress");
        return {};
    }

    if (!d->connectionExists(id)) {
        qWarning("FastDownloader::errorString: No such connection matches with the id provided");
        return {};
    }

    return d->connectionFor(id)->reply->errorString();
}

void FastDownloader::ignoreSslErrors(quint32 id) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::ignoreSslErrors: No downloads in progress");
        return;
    }

    if (!d->connectionExists(id)) {
        qWarning("FastDownloader::ignoreSslErrors: No such connection matches with the id provided");
        return;
    }

    return d->connectionFor(id)->reply->ignoreSslErrors();
}

void FastDownloader::ignoreSslErrors(quint32 id, const QList<QSslError>& errors) const
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::ignoreSslErrors: No downloads in progress");
        return;
    }

    if (!d->connectionExists(id)) {
        qWarning("FastDownloader::ignoreSslErrors: No such connection matches with the id provided");
        return;
    }

    return d->connectionFor(id)->reply->ignoreSslErrors(errors);
}

bool FastDownloader::start()
{
    Q_D(FastDownloader);

    if (d->running) {
        qWarning("FastDownloader::start: A download is already in progress");
        return false;
    }

    if (m_numberOfParallelConnections < 1
            || m_numberOfParallelConnections > MAX_PARALLEL_CONNECTIONS) {
        qWarning("FastDownloader::start: Number of parallel connections is incorrect, "
                 "It may exceeds maximum number of parallel connections allowed");
        return false;
    }

    if (!m_url.isValid()) {
        qWarning("FastDownloader::start: Url is invalid");
        return false;
    }

    d->running = true;
    d->createConnection(m_url);

    return true;
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