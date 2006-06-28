// BESInfoList.cc

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

#include "BESInfoList.h"
#include "BESInfo.h"
#include "TheBESKeys.h"

#define BES_DEFAULT_INFO_TYPE "txt"

BESInfoList *BESInfoList::_instance = 0 ;

BESInfoList::BESInfoList()
{
}

BESInfoList::~BESInfoList()
{
}

bool
BESInfoList::add_info_builder( const string &info_type,
			       p_info_builder info_builder )
{
    BESInfoList::Info_citer i ;
    i = _info_list.find( info_type ) ;
    if( i == _info_list.end() )
    {
	_info_list[info_type] = info_builder ;
	return true ;
    }
    return false ;
}

bool
BESInfoList::rem_info_builder( const string &info_type )
{
    BESInfoList::Info_iter i ;
    i = _info_list.find( info_type ) ;
    if( i != _info_list.end() )
    {
	_info_list.erase( i ) ;
	return true ;
    }
    return false ;
}

BESInfo *
BESInfoList::build_info( )
{
    string info_type = "" ;
    bool found = false ;
    try
    {
	info_type = TheBESKeys::TheKeys()->get_key( "OPeNDAP.Info.Type", found ) ;
    }
    catch( ... )
    {
	info_type = "" ;
    }

    if( !found || info_type == "" )
	info_type = BES_DEFAULT_INFO_TYPE ;

    BESInfoList::Info_citer i ;
    i = _info_list.find( info_type ) ;
    if( i != _info_list.end() )
    {
	p_info_builder p = (*i).second ;
	if( p )
	{
	    return p( info_type ) ;
	}
    }
    return 0 ;
}

BESInfoList *
BESInfoList::TheList()
{
    if( _instance == 0 )
    {
	_instance = new BESInfoList ;
    }
    return _instance ;
}

