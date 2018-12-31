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
        qint64 head = 0;
        qint64 pos = 0;
        qint64 bytesReceived = 0;
        qint64 bytesTotal = 0;
        QNetworkReply* reply = nullptr;
    };

public:
    FastDownloaderPrivate();

    int generateUniqueId() const;
    bool downloadCompleted() const;
    bool connectionExists(int id) const;

    Connection* connectionFor(int id) const;
    Connection* connectionFor(const QObject* sender) const;
    QList<Connection> createFakeCopyForActiveConnections() const;

    void free();
    void reset();
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
    QNetworkReply::NetworkError error;
    QList<Connection*> connections;

    void _q_finished();
    void _q_readyRead();
    void _q_redirected(const QUrl& url);
    void _q_error(QNetworkReply::NetworkError code);
    void _q_sslErrors(const QList<QSslError>& errors);
    void _q_downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
};

#endif // FASTDOWNLOADER_P_H