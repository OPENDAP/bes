// BESContainer.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <stdio.h>
#include <errno.h>
#include <fstream>
#include <sstream>

using std::ifstream ;
using std::ofstream ;
using std::ios_base ;
using std::ostringstream ;

#include "BESContainer.h"
#include "TheBESKeys.h"
#include "BESUncompressManager.h"
#include "BESCache.h"
#include "BESContainerStorageException.h"

BESContainer::BESContainer(const string &s)
    : _valid( false ),
      _real_name( "" ),
      _constraint( "" ),
      _symbolic_name( s ),
      _container_type( "" ),
      _attributes( "" )
{
}

BESContainer::BESContainer( const BESContainer &copy_from )
    : _valid( copy_from._valid ),
      _real_name( copy_from._real_name ),
      _constraint( copy_from._constraint ),
      _symbolic_name( copy_from._symbolic_name ),
      _container_type( copy_from._container_type ),
      _attributes( copy_from._attributes )
{
}

string
BESContainer::access()
{
    // This is easy ... create the cache using the different keys
    BESKeys *keys = TheBESKeys::TheKeys() ;
    BESCache cache( *keys, "BES.CacheDir", "BES.CachePrefix", "BES.CacheSize" );

    return BESUncompressManager::TheManager()->uncompress( _real_name, cache ) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this container.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESContainer::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESContainer::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "is valid: " << _valid << endl ;
    strm << BESIndent::LMarg << "symbolic name: " << _symbolic_name << endl ;
    strm << BESIndent::LMarg << "real name: " << _real_name << endl ;
    strm << BESIndent::LMarg << "data type: " << _container_type << endl ;
    strm << BESIndent::LMarg << "constraint: " << _constraint << endl ;
    strm << BESIndent::LMarg << "attributes: " << _attributes << endl ;
    BESIndent::UnIndent() ;
}

