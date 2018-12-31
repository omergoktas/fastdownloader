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

#ifndef FASTDOWNLOADER_H
#define FASTDOWNLOADER_H

#include <fastdownloader_global.h>

#include <QUrl>
#include <QSslError>
#include <QNetworkReply>

/*!
    Some notes:
    If you want to understand the logic more, read Qt Documentations of QNetworkAccessManager,
    QNetworkReply and QIODevice. I've tried to keep compliance with those. Almost the same logic.
    Each connection (chunk) identified with an unique id. You can reach out those chunks and their
    data by using their id. It is best to read data of a chunk, with its id, when readyRead signal
    is emitted. You can read it later, but make sure you read everything before an error state
    occur, or the last download connection is over (before the last finished signal is emitted)

    Internal data buffer is chopped whenever you read it (since we don't have a secondary buffer,
    the only buffer we have is kept and managed by QNetworkReply and I suggest you to review
    QNetworkReply's internal codes if you want to understand more about buffer cleansing)

    Also, all the internal connections and QNetworkReply instances are cleared, with all their
    buffers and data, right before the final "finished" signal is emitted on the last ongoing chunk.
    Same goes for abort function and any error state. You won't be able to read any data after
    calling abort, or any "error" signal is being emitted.
 */

class FastDownloaderPrivate;
class FASTDOWNLOADER_EXPORT FastDownloader : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(FastDownloader)
    Q_DECLARE_PRIVATE(FastDownloader)

public:
    enum {
        // Note: QNetworkAccessManager queues the requests it receives. The number of requests
        // executed in parallel is dependent on the protocol. Currently, for the HTTP protocol
        // on desktop platforms, 6 requests are executed in parallel for one host/port combination.
        MAX_SIMULTANEOUS_CONNECTIONS = 6,

        // The minimum possible chunk size allowed for simultaneous downloads. Lesser sized chunks are
        // not allowed (10 Kb). Zero (0) is allowed for chunk size limit and it means no limit.
        MIN_CHUNK_SIZE = 10240,

        // The minimum possible content size allowed for simultaneous downloads. Lesser sized data
        // will not be downloaded with simultaneous download feature (100 Kb).
        MIN_SIMULTANEOUS_CONTENT_SIZE = 102400
    };

public:
    explicit FastDownloader(const QUrl& url, int numberOfSimultaneousConnections = 5, QObject* parent = nullptr);
    explicit FastDownloader(QObject* parent = nullptr);
    ~FastDownloader() override;

    QUrl url() const;
    void setUrl(const QUrl& url);

    int numberOfSimultaneousConnections() const;
    void setNumberOfSimultaneousConnections(int numberOfSimultaneousConnections);

    int maxRedirectsAllowed() const;
    void setMaxRedirectsAllowed(int maxRedirectsAllowed);

    qint64 chunkSizeLimit() const;
    void setChunkSizeLimit(qint64 chunkSizeLimit);

    qint64 readBufferSize() const;
    void setReadBufferSize(qint64 size);

    QSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QSslConfiguration& config);

    QNetworkAccessManager* networkAccessManager() const;

    /*!
        Following functions are filled with necessary information
        right before the "resolved" signal is emitted.
    */
    QUrl resolvedUrl() const;
    qint64 contentLength() const;
    qint64 bytesReceived() const;
    QNetworkReply::NetworkError error() const;

    bool isError() const;
    bool isRunning() const;
    bool isFinished() const; // exists for convenience
    bool isResolved() const;
    bool isSimultaneousDownloadPossible() const;

    /*!
        Following functions should be used between the "resolved"
        and the last "finished" signals are being emitted (both
        signals are not included), or before any error occur.
    */
    bool atEnd(int id) const;

    qint64 head(int id) const;
    qint64 pos(int id) const;
    qint64 bytesAvailable(int id) const;

    qint64 peek(int id, char* data, qint64 maxSize);
    QByteArray peek(int id, qint64 maxSize);

    qint64 skip(int id, qint64 maxSize) const;

    qint64 read(int id, char* data, qint64 maxSize) const;
    QByteArray read(int id, qint64 maxSize) const;
    QByteArray readAll(int id) const;

    qint64 readLine(int id, char* data, qint64 maxSize) const;
    QByteArray readLine(int id, qint64 maxSize = 0) const;

    QString errorString(int id) const;

    void ignoreSslErrors(int id) const;
    void ignoreSslErrors(int id, const QList<QSslError>& errors) const;

public slots:
    bool start();
    void abort();

protected:
    FastDownloader(FastDownloaderPrivate& dd, QObject* parent);

signals:
    void finished();
    void finished(int id);
    void readyRead(int id);
    void redirected(const QUrl& url);
    void resolved(const QUrl& resolvedUrl);
    void error(int id, QNetworkReply::NetworkError code);
    void sslErrors(int id, const QList<QSslError>& errors);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadProgress(int id, qint64 bytesReceived, qint64 bytesTotal);

private:
    Q_PRIVATE_SLOT(d_func(), void _q_finished())
    Q_PRIVATE_SLOT(d_func(), void _q_readyRead())
    Q_PRIVATE_SLOT(d_func(), void _q_redirected(const QUrl&))
    Q_PRIVATE_SLOT(d_func(), void _q_error(QNetworkReply::NetworkError))
    Q_PRIVATE_SLOT(d_func(), void _q_sslErrors(const QList<QSslError>&))
    Q_PRIVATE_SLOT(d_func(), void _q_downloadProgress(qint64, qint64))

private:
    QUrl m_url;
    int m_numberOfSimultaneousConnections;
    int m_maxRedirectsAllowed;
    qint64 m_chunkSizeLimit;
    qint64 m_readBufferSize;
    QSslConfiguration m_sslConfiguration;
};

#endif // FASTDOWNLOADER_H