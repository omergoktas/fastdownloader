#ifndef FASTDOWNLOADER_P_H
#define FASTDOWNLOADER_P_H

#include <fastdownloader.h>
#include <private/qobject_p.h>

class FastDownloaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(FastDownloader)
public:
    FastDownloaderPrivate();
    ~FastDownloaderPrivate();

    FastDownloader* q_ptr;
};

#endif // FASTDOWNLOADER_P_H