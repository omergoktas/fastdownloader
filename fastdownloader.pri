##**************************************************************************
##
## Copyright (C) 2019 Ömer Göktaş
## Contact: omergoktas.com
##
## This file is part of the FastDownloader library.
##
## The FastDownloader is free software: you can redistribute it and/or
## modify it under the terms of the GNU Lesser General Public License
## version 3 as published by the Free Software Foundation.
##
## The FastDownloader is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public
## License along with the FastDownloader. If not, see
## <https://www.gnu.org/licenses/>.
##
##**************************************************************************

QT          += core-private network
CONFIG      += c++11

DEFINES     += FASTDOWNLOADER_INCLUDE_STATIC

DEPENDPATH  += $$PWD
INCLUDEPATH += $$PWD

SOURCES     += $$PWD/fastdownloader.cpp
HEADERS     += $$PWD/fastdownloader.h \
               $$PWD/fastdownloader_p.h \
               $$PWD/fastdownloader_global.h