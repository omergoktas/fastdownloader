#include <fastdownloader_p.h>

FastDownloaderPrivate::FastDownloaderPrivate() : QObjectPrivate()
    , manager(new QNetworkAccessManager)
    , resolved(false)
{

}

bool FastDownloaderPrivate::resolveUrl()
{
    Q_Q(FastDownloader);

    resolvedUrl = q->url();

    QNetworkReply* reply = manager->get(makeRequest());

    QObject::connect(reply, &QNetworkReply::downloadProgress,
                             [=] (qint64 bytesReceived, qint64 bytesTotal) {
        qDebug() << bytesReceived << bytesTotal << "\t\t\t"
                 << QString("%%1").arg(QString::number(100 * ((qreal)bytesReceived / bytesTotal)));
    });

    QObject::connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
                     [=] (QNetworkReply::NetworkError e) {
        qDebug() << e;
    });
    QObject::connect(reply, &QNetworkReply::sslErrors,
                     [=](const QList<QSslError>& e) {
        qDebug() << e;
        reply->ignoreSslErrors();
    });

    QObject::connect(reply, &QNetworkReply::finished, [=] {
        qDebug() << "bitti lo!" << reply->readAll().size();
    });
    QObject::connect(reply, &QNetworkReply::redirected, [=] (const QUrl& url) {
        //qDebug() << "redirected: " << url;
    });
    QObject::connect(reply, &QIODevice::readyRead, [=] {
        //qDebug() << "gelen var";
        static bool first = false;
        if (first)
            return;
        first = true;

        qDebug() << reply->rawHeaderPairs();
//        qDebug() << reply->readAll().size();
    });
}

QNetworkRequest FastDownloaderPrivate::makeRequest() const
{
    Q_Q(const FastDownloader);
    QNetworkRequest request;
    request.setUrl(resolvedUrl);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setRawHeader("User-Agent", "FastDownloader");
    request.setMaximumRedirectsAllowed(q->maxRedirectsAllowed());
    return request;
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

bool FastDownloader::isResolved() const
{

}

qreal FastDownloader::progress(int segment) const
{

}

qint64 FastDownloader::size(int segment) const
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