// DefinitionStorageVolatile.cc

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

#include "DefinitionStorageVolatile.h"
#include "DODSDefine.h"
#include "DODSInfo.h"

DefinitionStorageVolatile::~DefinitionStorageVolatile()
{
}

/** @brief looks for a definition in this volatile store with the
 * given name
 *
 * @param def_name name of the definition to look for
 * @return definition with the given name, NULL if not found
 */
DODSDefine *
DefinitionStorageVolatile::look_for( const string &def_name )
{
    Define_citer i ;
    i = _def_list.find( def_name ) ;
    if( i != _def_list.end() )
    {
	return (*i).second;
    }
    return NULL ;
}

/** @brief adds a given definition to this volatile storage
 *
 * This method adds a definition to the definition store
 *
 * @param d definition to add
 */
bool
DefinitionStorageVolatile::add_definition( const string &def_name,
                                           DODSDefine *d )
{
    if( look_for( def_name ) == NULL )
    {
	_def_list[def_name] = d ;
	return true ;
    }
    return false ;
}

/** @brief deletes a defintion with the given name from this volatile store
 *
 * This method deletes a definition from the definition store with the
 * given name.
 *
 * @param def_name name of the defintion to delete
 * @return true if successfully deleted and false otherwise
 */
bool
DefinitionStorageVolatile::del_definition( const string &def_name )
{
    bool ret = false ;
    Define_iter i ;
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

/** @brief deletes all defintions from the definition store
 *
 * @return true if successfully deleted and false otherwise
 */
bool
DefinitionStorageVolatile::del_definitions( )
{
    while( _def_list.size() != 0 )
    {
	Define_iter di = _def_list.begin() ;
	DODSDefine *d = (*di).second ;
	_def_list.erase( di ) ;
	if( d )
	{
	    delete d ;
	}
    }
}

/** @brief show the defintions stored in this store
 *
 * Add information to the passed information object about each of the
 * defintions stored within this defintion store. The information
 * added to the passed information objects includes the name of this
 * persistent store on the first line followed by the information for
 * each definition on the following lines, one per line.
 *
 * @param info information object to store the information in
 */
void
DefinitionStorageVolatile::show_definitions( DODSInfo &info )
{
    info.add_data( get_name() ) ;
    info.add_data( "\n" ) ;
    bool first = true ;
    Define_citer di = _def_list.begin() ;
    Define_citer de = _def_list.end() ;
    if( di == de )
    {
	info.add_data( "  No definitions are currently defined\n" ) ;
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

	    info.add_data( "  Definition: " + def_name + "\n" ) ;
	    info.add_data( "    Containers:\n" ) ;

	    DODSDefine::containers_iterator ci = def->first_container() ;
	    DODSDefine::containers_iterator ce = def->end_container() ;
	    for( ; ci != ce; ci++ )
	    {
		string sym = (*ci).get_symbolic_name() ;
		string real = (*ci).get_real_name() ;
		string type = (*ci).get_container_type() ;
		string con = (*ci).get_constraint() ;
		string attrs = (*ci).get_attributes() ;
		string line = (string)"      " + sym + "," + real + "," + type
				  + ",\"" + con + "\",\"" + attrs + "\"\n" ;
		info.add_data( line ) ;
	    }

	    info.add_data( "    Aggregation: " + def->aggregation_handler + ": " + def->aggregation_command + "\n" ) ;
	}
    }
}

