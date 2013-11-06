// PPTMarkFinder.cc

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

#include "PPTMarkFinder.h"
#include "BESInternalError.h"

PPTMarkFinder::PPTMarkFinder( unsigned char *mark, int markLength )
    : _markIndex( 0 ),
      _markLength( markLength )
{
    // Lets get sure we will not overrun the buffer
    if (markLength > PPTMarkFinder_Buffer_Size)
        throw BESInternalError( "mark given will overrun internal buffer.",
                            __FILE__, __LINE__ ) ;
    memcpy((void*) _mark, (void*) mark, (markLength * sizeof(unsigned char)) );
}

bool
PPTMarkFinder::markCheck( unsigned char b )
{
    if( _mark[_markIndex] == b )
    {
	_markIndex++;
	if( _markIndex == _markLength )
	{
	    _markIndex = 0;
	    return true ;
	}
    }
    else
    {
	_markIndex = 0 ;
    }

    return false ;
}

