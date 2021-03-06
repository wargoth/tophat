/*
Copyright_License {

  Top Hat Soaring Glide Computer - http://www.tophatsoaring.org/
  Copyright (C) 2000-2016 The Top Hat Soaring Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_IO_TEXT_FILE_HPP
#define XCSOAR_IO_TEXT_FILE_HPP

#include "Charset.hpp"

#include <tchar.h>

class TLineReader;

/**
 * Opens the specified file and returns a LineReader.
 *
 * @param path the file path, either real or inside a zip file
 * @param cs the character set of the input file
 * @return a TLineReader which must be deleted by the caller; nullptr if
 * an error occurred opening the file
 */
TLineReader *
OpenTextFile(const TCHAR *path,
             Charset cs = Charset::UTF8);

#endif
