/****************************************************************************
**
** Copyright (C) 2019 Ömer Göktaş
** Contact: omergoktas.com
**
** This file is part of the FastDownloader library.
**
** The FastDownloader is free software: you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public License
** version 3 as published by the Free Software Foundation.
**
** The FastDownloader is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with the FastDownloader. If not, see
** <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include <fastdownloader_p.h>
#include <QRandomGenerator>

FastDownloaderPrivate::FastDownloaderPrivate() : QObjectPrivate()
  , manager(new QNetworkAccessManager)
  , running(false)
  , resolved(false)
  , simultaneousDownloadPossible(false)
  , contentLength(0)
  , totalBytesReceived(0)
  , error(QNetworkReply::NoError)
{
}

int FastDownloaderPrivate::generateUniqueId() const
{
    static const int min = std::numeric_limits<int>::min();
    static const int max = std::numeric_limits<int>::max();
    int id;
    do {
        id = QRandomGenerator::global()->bounded(min, max);
    } while(connectionExists(id));
    return id;
}

bool FastDownloaderPrivate::downloadCompleted() const
{
    if (nextPortionAvailable())
        return false;
    for (Connection* connection : connections) {
        if (connection->reply->isRunning())
            return false;
    }
    return true;
}

bool FastDownloaderPrivate::connectionExists(int id) const
{
    for (Connection* connection : connections) {
        if (connection->id == id)
            return true;
    }
    return false;
}

bool FastDownloaderPrivate::nextPortionAvailable() const
{
    Q_Q(const FastDownloader);

    if (simultaneousDownloadPossible
            && q->chunkSizeLimit() > 0
            && q->numberOfSimultaneousConnections() > 1) {
        qint64 bytesTotal = 0;
        for (Connection* connection : connections)
            bytesTotal += connection->bytesTotal;
        if (bytesTotal < contentLength)
            return true;
        else
            return false;
    } else {
        return false;
    }
}

qint64 FastDownloaderPrivate::nextPortionPosition() const
{
    if (!nextPortionAvailable())
        return -1;

    qint64 nextPos = 0;
    for (Connection* connection : connections) {
        qint64 end = connection->head + connection->bytesTotal;
        if (end > nextPos)
            nextPos = end;
    }

    return nextPos;
}

qint64 FastDownloaderPrivate::untargetedDataSize() const
{
    qint64 bytesTotal = 0;
    for (Connection* connection : connections)
        bytesTotal += connection->bytesTotal;
    return contentLength - bytesTotal;
}

FastDownloaderPrivate::Connection* FastDownloaderPrivate::connectionFor(int id) const
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

QList<FastDownloaderPrivate::Connection> FastDownloaderPrivate::createFakeCopyForActiveConnections() const
{
    QList<FastDownloaderPrivate::Connection> fakeCopy;
    for (Connection* connection : connections) {
        if (connection->reply->isRunning()) {
            Connection c;
            c.bytesReceived = connection->bytesReceived;
            c.bytesTotal = connection->bytesTotal;
            c.id = connection->id;
            fakeCopy.append(c);
        }
    }
    return fakeCopy;
}

void FastDownloaderPrivate::free()
{
    const QList<Connection*> copy(connections);
    for (Connection* connection : copy)
        deleteConnection(connection);
}

void FastDownloaderPrivate::reset()
{
    running = true;
    resolved = false;
    simultaneousDownloadPossible = false;
    resolvedUrl.clear();
    contentLength = 0;
    totalBytesReceived = 0;
    error = QNetworkReply::NoError;
}

void FastDownloaderPrivate::startSimultaneousDownloading()
{
    Q_Q(const FastDownloader);

    if (!running
            || !resolved
            || !simultaneousDownloadPossible
            || q->numberOfSimultaneousConnections() < 2) {
        return;
    }

    int i = 0;
    qint64 begin = 0;
    qint64 end = -1;

    do {
        begin = end + 1;

        qint64 slice = 0;
        if (i == q->numberOfSimultaneousConnections() - 1)
            slice = contentLength - begin;
        else
            slice = contentLength / q->numberOfSimultaneousConnections();

        if (q->chunkSizeLimit() > 0)
            end = begin + qMin(q->chunkSizeLimit(), slice) - 1;
        else
            end = begin + slice - 1;

        createConnection(resolvedUrl, begin, end);
    } while(++i < q->numberOfSimultaneousConnections());
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

    if (!isInitial) {
        connection->head = begin;
        connection->bytesTotal = end - begin + 1;
    }

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

qint64 FastDownloaderPrivate::testContentLength(const FastDownloaderPrivate::Connection* connection)
{
    Q_ASSERT(connection && connection->reply);
    QVariant contentLength = connection->reply->header(QNetworkRequest::ContentLengthHeader);
    if (contentLength.isNull() || !contentLength.isValid())
        return -1;
    return contentLength.toLongLong();
}

bool FastDownloaderPrivate::testSimultaneousDownload(const FastDownloaderPrivate::Connection* connection)
{
    Q_ASSERT(connection && connection->reply);

    return connection->reply->hasRawHeader("Accept-Ranges")
            && connection->reply->hasRawHeader("Content-Length")
            && connection->reply->rawHeader("Accept-Ranges") == "bytes"
            && connection->reply->rawHeader("Content-Length").toLongLong() > connection->reply->bytesAvailable()
            && connection->reply->rawHeader("Content-Length").toLongLong() >= FastDownloader::MIN_SIMULTANEOUS_CONTENT_SIZE;
}

void FastDownloaderPrivate::_q_finished()
{
    Q_Q(FastDownloader);

    Connection* connection = connectionFor(q->sender());
    const int id = connection->id;
    const bool downloadFinished = downloadCompleted();
    const QNetworkReply::NetworkError error = connection->reply->error();

    if (downloadFinished && error == QNetworkReply::NoError) {
        running = false;
        free();
    }

    emit q->finished(id);

    if (error != QNetworkReply::NoError) {
        q->abort();
        return;
    }

    if (downloadFinished) {
        emit q->finished();
        return;
    }

    qint64 nextPos = nextPortionPosition();
    if (nextPos > 0) {
        qint64 nextSize = untargetedDataSize();
        if (nextSize >= 2 * q->chunkSizeLimit())
            nextSize = q->chunkSizeLimit();
        createConnection(resolvedUrl, nextPos, nextPos + nextSize - 1);
    }
}

void FastDownloaderPrivate::_q_readyRead()
{
    Q_Q(FastDownloader);

    Connection* connection = connectionFor(q->sender());

    const qint64 prevBytesReceived = connection->bytesReceived;
    connection->bytesReceived = connection->pos + connection->reply->bytesAvailable();
    totalBytesReceived += connection->bytesReceived - prevBytesReceived;

    if (resolved) {
        emit q->readyRead(connection->id);
    } else {
        resolved = true;
        resolvedUrl = connection->reply->url();
        contentLength = testContentLength(connection);
        simultaneousDownloadPossible = testSimultaneousDownload(connection);

        emit q->resolved(resolvedUrl);

        if (connection->reply->isRunning()
                && simultaneousDownloadPossible
                && q->numberOfSimultaneousConnections() > 1) {
            totalBytesReceived = 0;
            deleteConnection(connection);
            startSimultaneousDownloading();
        } else {
            connection->bytesTotal = contentLength;
            emit q->readyRead(connection->id);
        }
    }
}

void FastDownloaderPrivate::_q_redirected(const QUrl& url)
{
    Q_Q(FastDownloader);

    if (resolved) {
        qWarning("WARNING: Suspicious redirection rejected");
        q->abort();
        return;
    }

    emit q->redirected(url);
}

void FastDownloaderPrivate::_q_error(QNetworkReply::NetworkError code)
{
    Q_Q(FastDownloader);
    if (code != QNetworkReply::NoError)
        error = code;
    emit q->error(connectionFor(q->sender())->id, code);
}

void FastDownloaderPrivate::_q_sslErrors(const QList<QSslError>& errors)
{
    Q_Q(FastDownloader);
    emit q->sslErrors(connectionFor(q->sender())->id, errors);
}

void FastDownloaderPrivate::_q_downloadProgress(qint64 /*bytesReceived*/, qint64 /*bytesTotal*/)
{
    Q_Q(FastDownloader);
    Connection* connection = connectionFor(q->sender());
    emit q->downloadProgress(connection->id, connection->bytesReceived, connection->bytesTotal);
    if (connection->reply->error() == QNetworkReply::NoError)
        emit q->downloadProgress(totalBytesReceived, contentLength);
}

FastDownloader::FastDownloader(const QUrl& url, int numberOfSimultaneousConnections, QObject* parent)
    : QObject(*(new FastDownloaderPrivate), parent)
    , m_url(url)
    , m_numberOfSimultaneousConnections(numberOfSimultaneousConnections)
    , m_maxRedirectsAllowed(5)
    , m_chunkSizeLimit(0)
    , m_readBufferSize(0)
    , m_sslConfiguration(QSslConfiguration::defaultConfiguration())
{
}

FastDownloader::FastDownloader(QObject* parent) : FastDownloader(QUrl(), 5, parent)
{
}

FastDownloader::FastDownloader(FastDownloaderPrivate& dd, QObject* parent)
    : QObject(dd, parent)
{
}

FastDownloader::~FastDownloader()
{
    Q_D(const FastDownloader);
    if (d->running)
        abort();
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

int FastDownloader::numberOfSimultaneousConnections() const
{
    return m_numberOfSimultaneousConnections;
}

void FastDownloader::setNumberOfSimultaneousConnections(int numberOfSimultaneousConnections)
{
    Q_D(const FastDownloader);

    if (d->running) {
        qWarning("FastDownloader::setNumberOfSimultaneousConnections: "
                 "Cannot set, a download is already in progress");
        return;
    }

    m_numberOfSimultaneousConnections = numberOfSimultaneousConnections;
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

QNetworkAccessManager* FastDownloader::networkAccessManager() const
{
    Q_D(const FastDownloader);
    return d->manager.data();
}

QUrl FastDownloader::resolvedUrl() const
{
    Q_D(const FastDownloader);
    return d->resolvedUrl;
}

qint64 FastDownloader::contentLength() const
{
    Q_D(const FastDownloader);
    return d->contentLength;
}

qint64 FastDownloader::bytesReceived() const
{
    Q_D(const FastDownloader);
    return d->totalBytesReceived;
}

QNetworkReply::NetworkError FastDownloader::error() const
{
    Q_D(const FastDownloader);
    return d->error;
}

bool FastDownloader::isError() const
{
    Q_D(const FastDownloader);
    return d->error != QNetworkReply::NoError;
}

bool FastDownloader::isRunning() const
{
    Q_D(const FastDownloader);
    return d->running;
}

bool FastDownloader::isFinished() const
{
    Q_D(const FastDownloader);
    return !d->running;
}

bool FastDownloader::isResolved() const
{
    Q_D(const FastDownloader);
    return d->resolved;
}

bool FastDownloader::isSimultaneousDownloadPossible() const
{
    Q_D(const FastDownloader);
    return d->simultaneousDownloadPossible;
}

bool FastDownloader::atEnd(int id) const
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

qint64 FastDownloader::head(int id) const
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

    return d->connectionFor(id)->head;
}

qint64 FastDownloader::pos(int id) const
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

    return d->connectionFor(id)->pos;
}

qint64 FastDownloader::bytesAvailable(int id) const
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

qint64 FastDownloader::peek(int id, char* data, qint64 maxSize)
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::peek: No downloads in progress");
        return -1;
    }

    if (!d->connectionExists(id)) {
        qWarning("FastDownloader::peek: No such connection matches with the id provided");
        return -1;
    }

    return d->connectionFor(id)->reply->peek(data, maxSize);
}

QByteArray FastDownloader::peek(int id, qint64 maxSize)
{
    Q_D(const FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::peek: No downloads in progress");
        return {};
    }

    if (!d->connectionExists(id)) {
        qWarning("FastDownloader::peek: No such connection matches with the id provided");
        return {};
    }

    return d->connectionFor(id)->reply->peek(maxSize);
}

qint64 FastDownloader::skip(int id, qint64 maxSize) const
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

    FastDownloaderPrivate::Connection* connection = d->connectionFor(id);
    const qint64 length = connection->reply->skip(maxSize);
    if (length > 0)
        connection->pos += length;
    return length;
}

qint64 FastDownloader::read(int id, char* data, qint64 maxSize) const
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

    FastDownloaderPrivate::Connection* connection = d->connectionFor(id);
    const qint64 length = connection->reply->read(data, maxSize);
    if (length > 0)
        connection->pos += length;
    return length;
}

QByteArray FastDownloader::read(int id, qint64 maxSize) const
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

    FastDownloaderPrivate::Connection* connection = d->connectionFor(id);
    const QByteArray& data = connection->reply->read(maxSize);
    if (data.size() > 0)
        connection->pos += data.size();
    return data;
}

QByteArray FastDownloader::readAll(int id) const
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

    FastDownloaderPrivate::Connection* connection = d->connectionFor(id);
    const QByteArray& data = connection->reply->readAll();
    if (data.size() > 0)
        connection->pos += data.size();
    return data;
}

qint64 FastDownloader::readLine(int id, char* data, qint64 maxSize) const
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

    FastDownloaderPrivate::Connection* connection = d->connectionFor(id);
    const qint64 length = connection->reply->readLine(data, maxSize);
    if (length > 0)
        connection->pos += length;
    return length;
}

QByteArray FastDownloader::readLine(int id, qint64 maxSize) const
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

    FastDownloaderPrivate::Connection* connection = d->connectionFor(id);
    const QByteArray& data = connection->reply->readLine(maxSize);
    if (data.size() > 0)
        connection->pos += data.size();
    return data;
}

QString FastDownloader::errorString(int id) const
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

void FastDownloader::ignoreSslErrors(int id) const
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

void FastDownloader::ignoreSslErrors(int id, const QList<QSslError>& errors) const
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

    if (m_numberOfSimultaneousConnections < 1
            || m_numberOfSimultaneousConnections > MAX_SIMULTANEOUS_CONNECTIONS) {
        qWarning("FastDownloader::start: Number of simultaneous connections is incorrect, "
                 "It may exceeds maximum number of simultaneous connections allowed");
        return false;
    }

    if (m_chunkSizeLimit < MIN_CHUNK_SIZE && m_chunkSizeLimit != 0) {
        qWarning("FastDownloader::start: Chunk size limit is too small.");
        return false;
    }

    if (!m_url.isValid()) {
        qWarning("FastDownloader::start: Url is invalid");
        return false;
    }

    d->reset();
    d->createConnection(m_url);

    return true;
}

void FastDownloader::abort()
{
    Q_D(FastDownloader);

    if (!d->running) {
        qWarning("FastDownloader::abort: No download is in progress to abort");
        return;
    }

    const QList<FastDownloaderPrivate::Connection>& fakeConnections = d->createFakeCopyForActiveConnections();

    d->error = QNetworkReply::OperationCanceledError;
    d->running = false;
    d->free();

    for (const FastDownloaderPrivate::Connection& fakeConnection : fakeConnections) {
        emit error(fakeConnection.id, d->error);
        emit downloadProgress(fakeConnection.id, fakeConnection.bytesReceived, fakeConnection.bytesTotal);
        emit finished(fakeConnection.id);
    }
    emit downloadProgress(d->totalBytesReceived, d->contentLength);
    emit finished();
}

#include "moc_fastdownloader.cpp"