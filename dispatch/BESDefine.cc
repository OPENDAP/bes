// BESDefine.cc

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

#include "BESDefine.h"

using std::endl;
using std::ostream;

BESDefine::~BESDefine()
{
    // delete all of the containers in my list, they belong to me
    while( _containers.size() != 0 )
    {
	BESDefine::containers_iter ci = _containers.begin() ;
	BESContainer *c = (*ci) ;
	_containers.erase( ci ) ;
	if( c )
	{
	    delete c ;
	}
    }
}

void
BESDefine::add_container( BESContainer *container )
{
    _containers.push_back( container ) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with contents of the
 * definition, including the containers in this definition and aggregation
 * information.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDefine::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDefine::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    if( _containers.size() )
    {
	strm << BESIndent::LMarg << "container list:" << endl ;
	BESIndent::Indent() ;
	BESDefine::containers_citer i = _containers.begin() ;
	BESDefine::containers_citer ie = _containers.end() ;
	for( ; i != ie; i++ )
	{
	    const BESContainer *c = (*i) ;
	    c->dump( strm ) ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "container list: empty" << endl ;
    }
    strm << BESIndent::LMarg << "aggregation command: " << _agg_cmd << endl ;
    strm << BESIndent::LMarg << "aggregation server: " << _agg_handler << endl ;
    BESIndent::UnIndent() ;
}

