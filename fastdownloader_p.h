#ifndef FASTDOWNLOADER_P_H
#define FASTDOWNLOADER_P_H

#include <fastdownloader.h>
#include <private/qobject_p.h>

class FastDownloaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(FastDownloader)

    struct Connection
    {
        quint32 id = 0;
        QNetworkReply* reply = nullptr;
        qint64 bytesReceived = 0; // Used for statistical reasons
    };

public:
    FastDownloaderPrivate();

    bool connectionExists(quint32 id) const;
    bool downloadCompleted() const;
    void deleteConnection(Connection* connection);
    void createConnection(const QUrl& url, qint64 begin = -1, qint64 end = -1);
    void startParallelDownloading();
    qint64 calculateTotalBytesReceived() const;
    quint32 generateUniqueId() const;
    Connection* connectionFor(quint32 id) const;
    Connection* connectionFor(const QObject* sender) const;

    static qint64 contentLength(const Connection* connection);
    static bool testParallelDownload(const Connection* connection);

    QScopedPointer<QNetworkAccessManager> manager;
    bool running;
    bool resolved;
    bool parallelDownloadPossible;
    QUrl resolvedUrl;
    qint64 bytesTotal;
    QList<Connection*> connections;

    void _q_finished();
    void _q_readyRead();
    void _q_redirected(const QUrl& url);
    void _q_error(QNetworkReply::NetworkError code);
    void _q_sslErrors(const QList<QSslError>& errors);
    void _q_downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
};

#endif // FASTDOWNLOADER_P_H