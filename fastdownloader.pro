QT -= gui
TEMPLATE = lib
TARGE = fastdownloader
CONFIG += shared strict_c strict_c++ utf8_source hide_symbols
gcc:QMAKE_CXXFLAGS += -pedantic-errors
msvc:QMAKE_CXXFLAGS += -permissive-
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000
DEFINES += FASTDOWNLOADER_LIBRARY

unix {
    target.path = /usr/lib
    INSTALLS += target
}

include(fastdownloader.pri)