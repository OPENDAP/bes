// BESUncompress3GZ.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef BESUncompress3GZ_h_
#define BESUncompress3GZ_h_ 1

#include <string>

#include "BESObj.h"

/** @brief Function to uncompress files with .gz extension
 *
 * The static function is responsible for uncompressing gz files. If the
 * uncompressed target file already exists then this function will overwrite
 * that file. The destination file must be already open for write or
 * read/write. This code will not close that file.
 *
 * If any errors occur during this operation then a
 * BESContainerStorageException will be thrown
 *
 * @param src the source file that is to be uncompressed
 * @param target the file descriptor of the target uncompressed file
 * @throws BESContainerStorageException if errors in uncompressing the file
 */
class BESUncompress3GZ : public BESObj {
public:
    static void uncompress(const std::string &src, int dest_fd);
};

#endif // BESUncompress3GZ_h_
