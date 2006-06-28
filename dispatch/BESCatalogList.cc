// BESCatalogList.cc

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

#include "BESCatalogList.h"
#include "BESCatalog.h"
#include "BESResponseException.h"
#include "BESInfo.h"
#include "BESHandlerException.h"

BESCatalogList *BESCatalogList::_instance = 0 ;

BESCatalogList::~BESCatalogList()
{
    catalog_iterator i = _catalogs.begin() ;
    catalog_iterator e = _catalogs.end() ;
    for( ; i != e; i++ )
    {
	BESCatalog *catalog = (*i) ;
	if( catalog ) delete catalog ;
    }
}

void
BESCatalogList::add_catalog( BESCatalog *catalog )
{
    _catalogs.push_back( catalog ) ;
}

void
BESCatalogList::show_catalog( const string &container,
			   const string &coi,
			   BESInfo *info )
{
    catalog_iterator i = _catalogs.begin() ;
    catalog_iterator e = _catalogs.end() ;
    bool done = false ;
    for( ; i != e && done == false; i++ )
    {
	BESCatalog *catalog = (*i) ;
	done = catalog->show_catalog( container, coi, info ) ;
    }
    if( done == false )
    {
	string serr ;
	if( container != "" )
	{
	    serr = (string)"Unable to find catalog information for container "
		   + container ;
	}
	else
	{
	    serr = "Unable to find catalog information for root" ;
	}
	throw BESHandlerException( serr, __FILE__, __LINE__ ) ;
	//info->add_exception( "Error", serr, __FILE__, __LINE__ ) ;
    }
}

BESCatalogList *
BESCatalogList::TheCatalogList()
{
    if( _instance == 0 )
    {
	_instance = new BESCatalogList ;
    }
    return _instance ;
}

