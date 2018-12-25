QT          += core-private network
CONFIG      += c++14

DEFINES     += FASTDOWNLOADER_INCLUDE_STATIC

DEPENDPATH  += $$PWD
INCLUDEPATH += $$PWD

SOURCES     += $$PWD/fastdownloader.cpp
HEADERS     += $$PWD/fastdownloader.h \
               $$PWD/fastdownloader_p.h \
               $$PWD/fastdownloader_global.h