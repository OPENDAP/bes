// DODSDefineList.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org> and Jose Garcia <jgarcia@ucar.org>
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

#include "DODSDefineList.h"
#include "DODSDefine.h"
#include "DODSInfo.h"
#include "DODSContainer.h"

DODSDefineList *DODSDefineList::_instance = 0 ;

bool
DODSDefineList::add_def( const string &def_name,
			     DODSDefine *def )
{
    if( find_def( def_name ) == 0 )
    {
	_def_list[def_name] = def ;
	return true ;
    }
    return false ;
}

bool
DODSDefineList::remove_def( const string &def_name )
{
    bool ret = false ;
    DODSDefineList::Define_iter i ;
    i = _def_list.find( def_name ) ;
    if( i != _def_list.end() )
    {
	DODSDefine *d = (*i).second;
	_def_list.erase( i ) ;
	delete d ;
	ret = true ;
    }
    return ret ;
}

void
DODSDefineList::remove_defs( )
{
    while( _def_list.size() != 0 )
    {
	DODSDefineList::Define_iter di = _def_list.begin() ;
	DODSDefine *d = (*di).second ;
	_def_list.erase( di ) ;
	if( d )
	{
	    delete d ;
	}
    }
}

DODSDefine *
DODSDefineList::find_def( const string &def_name )
{
    DODSDefineList::Define_citer i ;
    i = _def_list.find( def_name ) ;
    if( i != _def_list.end() )
    {
	return (*i).second;
    }
    return 0 ;
}

void
DODSDefineList::show_definitions( DODSInfo &info )
{
    bool first = true ;
    DODSDefineList::Define_citer di = _def_list.begin() ;
    DODSDefineList::Define_citer de = _def_list.end() ;
    if( di == de )
    {
	info.add_data( "No definitions are currently defined\n" ) ;
    }
    else
    {
	for( ; di != de; di++ )
	{
	    string def_name = (*di).first ;
	    DODSDefine *def = (*di).second ;

	    if( !first )
	    {
		info.add_data( "\n" ) ;
	    }
	    first = false ;

	    info.add_data( "Definition: " + def_name + "\n" ) ;
	    info.add_data( "Containers:\n" ) ;

	    DODSDefine::containers_iterator ci = def->first_container() ;
	    DODSDefine::containers_iterator ce = def->end_container() ;
	    for( ; ci != ce; ci++ )
	    {
		string sym = (*ci).get_symbolic_name() ;
		string real = (*ci).get_real_name() ;
		string type = (*ci).get_container_type() ;
		string con = (*ci).get_constraint() ;
		string attrs = (*ci).get_attributes() ;
		string line = sym + "," + real + "," + type
				  + ",\"" + con + "\",\"" + attrs + "\"\n" ;
		info.add_data( line ) ;
	    }

	    info.add_data( "Aggregation: " + def->aggregation_command + "\n" ) ;
	}
    }
}

DODSDefineList *
DODSDefineList::TheList()
{
    if( _instance == 0 )
    {
	_instance = new DODSDefineList ;
    }
    return _instance ;
}

// $Log: DODSDefineList.cc,v $
// Revision 1.3  2005/03/17 19:25:29  pwest
// string parameters changed to const references. remove_def now deletes the definition and returns true if removed or false otherwise. Added method remove_defs to remove all definitions
//
// Revision 1.2  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
