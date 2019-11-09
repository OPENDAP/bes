// SocketUtilities.h

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

#ifndef SocketUtilities_h
#define SocketUtilities_h 1

#include <string>

class SocketUtilities
{
public:
    /**
      * Routine to convert a long int to the specified numeric base,
      * from 2 to 36. You must get sure the buffer val is big enough
      * to hold all the digits for val or this routine may be UNSAFE.
      * @param val the value to be converted.
      * @param buf A buffer where to place the conversion.  
      * @param base base number system to use
      * @return Pointer to the buffer buf.
      */

    static char *ltoa( long val, char *buf, int base ) ;

    /** 
      * Create a uniq name which is used to create a 
      * unique name for a Unix socket in a client .
      * or for creating a temporary file.
      * @return uniq name
      */
    static std::string create_temp_name() ;
} ;

#endif // SocketUtilities_h

// $Log: SocketUtilities.h,v $
