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

#ifndef FASTDOWNLOADER_GLOBAL_H
#define FASTDOWNLOADER_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(FASTDOWNLOADER_LIBRARY) // takes precedence when combined with others
#  define FASTDOWNLOADER_EXPORT Q_DECL_EXPORT
#elif defined(FASTDOWNLOADER_INCLUDE_STATIC)
#  define FASTDOWNLOADER_EXPORT
#else
#  define FASTDOWNLOADER_EXPORT Q_DECL_IMPORT
#endif

#endif // FASTDOWNLOADER_GLOBAL_H
