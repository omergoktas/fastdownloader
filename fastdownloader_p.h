#ifndef FASTDOWNLOADER_P_H
#define FASTDOWNLOADER_P_H

#include <fastdownloader.h>
#include <private/qobject_p.h>

class FastDownloaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(FastDownloader)

    struct Connection
    {
        int id = 0;
        qint64 pos = 0;
        qint64 bytesTotal = 0;
        qint64 bytesReceived = 0;
        QNetworkReply* reply = nullptr;
    };

public:
    FastDownloaderPrivate();

    int generateUniqueId() const;
    bool connectionExists(int id) const;
    bool downloadCompleted() const;
    Connection* connectionFor(int id) const;
    Connection* connectionFor(const QObject* sender) const;
    QList<Connection> fakeCopyForActiveConnections() const;
    void free();
    void startParallelDownloading();
    void deleteConnection(Connection* connection);
    void createConnection(const QUrl& url, qint64 begin = -1, qint64 end = -1);

    static qint64 testContentLength(const Connection* connection);
    static bool testParallelDownload(const Connection* connection);

    QScopedPointer<QNetworkAccessManager> manager;
    bool running;
    bool resolved;
    bool parallelDownloadPossible;
    QUrl resolvedUrl;
    qint64 contentLength;
    qint64 totalBytesReceived;
    QList<Connection*> connections;

    void _q_finished();
    void _q_readyRead();
    void _q_redirected(const QUrl& url);
    void _q_error(QNetworkReply::NetworkError code);
    void _q_sslErrors(const QList<QSslError>& errors);
    void _q_downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
};

#endif // FASTDOWNLOADER_P_H