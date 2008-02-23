// BESUncompressZ.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Denis Nadeau <dnadeau@pop600.gsfc.nasa.gov>
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      dnadeau     Denis Nadeau <dnadeau@pop600.gsfc.nasa.gov>

#ifndef BESUncompressZ_h_
#define BESUncompressZ_h_ 1

#include <string>

using std::string ;

#include "BESObj.h"

/** @brief Function to uncompress files with .Z extension
 *
 * The static function is responsible for uncompressing Z files. If the
 * uncompressed target file already exists then this function will overwrite
 * that file. If it doesn't already exist then it is created.
 *
 * If any errors occur during this operation then a
 * BESContainerStorageException will be thrown
 *
 * @param src the source file that is to be uncompressed
 * @param target the target uncompressed file
 * @return the target uncompressed file
 * @throws BESContainerStorageException if errors in uncompressing the file
 */
class BESUncompressZ : public BESObj
{
public:
    static void			uncompress( const string &src,
					    const string &target ) ;
};

#endif // BESUncompressZ_h_

