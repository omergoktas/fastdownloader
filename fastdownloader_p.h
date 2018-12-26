#ifndef FASTDOWNLOADER_P_H
#define FASTDOWNLOADER_P_H

#include <fastdownloader.h>
#include <private/qobject_p.h>

struct Segment
{
    int index;

};

class FastDownloaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(FastDownloader)
public:
    FastDownloaderPrivate();

    bool resolveUrl();
    QNetworkRequest makeRequest() const;

    QScopedPointer<QNetworkAccessManager> manager;
    bool resolved;
    QUrl resolvedUrl;

};

#endif // FASTDOWNLOADER_P_H