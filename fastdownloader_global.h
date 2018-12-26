#ifndef FASTDOWNLOADER_GLOBAL_H
#define FASTDOWNLOADER_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(FASTDOWNLOADER_LIBRARY)
#  define FASTDOWNLOADER_EXPORT Q_DECL_EXPORT
#elif defined(FASTDOWNLOADER_INCLUDE_STATIC)
#  define FASTDOWNLOADER_EXPORT
#else
#  define FASTDOWNLOADER_EXPORT Q_DECL_IMPORT
#endif

#endif // FASTDOWNLOADER_GLOBAL_H