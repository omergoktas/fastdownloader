# fastdownloader
Basic utility library based on Qt/C++ for fast, parallel downloads. It lets you download a file with more than one simultaneous connection.

## Basic usage
```cpp
#include <QCoreApplication>
#include <QBuffer>
#include <QFile>
#include <fastdownloader.h>

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    auto buffer = new QBuffer;
    auto file = new QFile("/Users/omergoktas/Desktop/Dragon and Toast.mp3"); //!!! Change this
    auto downloader = new FastDownloader(QUrl("https://bit.ly/2EXm5LF"));
    downloader->setNumberOfParallelConnections(5);

    if (!downloader->start()) {
        qWarning("Cannot start downloading for some reason");
        return 0;
    }

    buffer->open(QIODevice::WriteOnly);

    QObject::connect(downloader, &FastDownloader::readyRead, [=] (int id) {
        buffer->seek(downloader->head(id) + downloader->pos(id));
        buffer->write(downloader->readAll(id));
    });

    QObject::connect(downloader, qOverload<qint64,qint64>(&FastDownloader::downloadProgress),
                     [=] (qint64 bytesReceived, qint64 bytesTotal) {
        qWarning("Download Progress: %lld of %lld", bytesReceived, bytesTotal);
    });

    QObject::connect(downloader, qOverload<>(&FastDownloader::finished), [=] {
        buffer->close();
        if (downloader->bytesReceived() > 0) {
            file->open(QIODevice::WriteOnly);
            file->write(buffer->data());
            file->close();
        }
        qWarning("All done!");
        qWarning("Any errors: %s", downloader->isError() ? "yes" : "nope");
    });

    return a.exec();
}
```
