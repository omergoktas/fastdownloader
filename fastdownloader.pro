QT          -= gui
TEMPLATE     = lib
TARGET       = fastdownloader
CONFIG      += shared dll strict_c++

DEFINES     += FASTDOWNLOADER_LIBRARY
DEFINES     += QT_DEPRECATED_WARNINGS
DEFINES     += QT_DISABLE_DEPRECATED_BEFORE=0x060000

unix {
    target.path = /usr/lib
    INSTALLS += target
}

osx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/Frameworks/
}

include(fastdownloader.pri)