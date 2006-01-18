// ContainerStorageList.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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

#include <iostream>

using std::endl ;

#include "ContainerStorageList.h"
#include "ContainerStorage.h"
#include "ContainerStorageException.h"
#include "DODSContainer.h"
#include "TheDODSKeys.h"
#include "DODSLog.h"
#include "DODSInfo.h"

ContainerStorageList *ContainerStorageList::_instance = 0 ;

ContainerStorageList::ContainerStorageList()
    : _first( 0 )
{
}

ContainerStorageList::~ContainerStorageList()
{
    ContainerStorageList::persistence_list *pl = _first ;
    while( pl )
    {
	if( pl->_persistence_obj )
	{
	    delete pl->_persistence_obj ;
	}
	ContainerStorageList::persistence_list *next = pl->_next ;
	delete pl ;
	pl = next ;
    }
}

/** @brief Add a persistent store to the list
 *
 * Each persistent store has a name. If a persistent store already exists in
 * the list with that name then the persistent store is not added. Otherwise
 * the store is added to the list.
 *
 * The persistent stores are searched in the order in which they were added.
 *
 * @param cp persistent store to add to the list
 * @return true if successfully added, false otherwise
 * @see ContainerStorage
 */
bool
ContainerStorageList::add_persistence( ContainerStorage *cp )
{
    bool ret = false ;
    if( !_first )
    {
	_first = new ContainerStorageList::persistence_list ;
	_first->_persistence_obj = cp ;
	_first->_next = 0 ;
	ret = true ;
    }
    else
    {
	ContainerStorageList::persistence_list *pl = _first ;
	bool done = false ;
	while( done == false )
	{
	    if( pl->_persistence_obj->get_name() != cp->get_name() )
	    {
		if( pl->_next )
		{
		    pl = pl->_next ;
		}
		else
		{
		    pl->_next = new ContainerStorageList::persistence_list ;
		    pl->_next->_persistence_obj = cp ;
		    pl->_next->_next = 0 ;
		    done = true ;
		    ret = true ;
		}
	    }
	    else
	    {
		done = true ;
		ret = false ;
	    }
	}
    }
    return ret ;
}

/** @brief remove a persistent store from the list
 *
 * Removes the named persistent store from the list.
 *
 * @param persist_name name of the persistent store to be removed
 * @return true if successfully removed, false otherwise
 * @see ContainerStorage
 */
bool
ContainerStorageList::rem_persistence( const string &persist_name )
{
    bool ret = false ;
    ContainerStorageList::persistence_list *pl = _first ;
    ContainerStorageList::persistence_list *last = 0 ;

    bool done = false ;
    while( done == false )
    {
	if( pl )
	{
	    if( pl->_persistence_obj->get_name() == persist_name )
	    {
		ret = true ;
		done = true ;
		if( pl == _first )
		{
		    _first = _first->_next ;
		}
		else
		{
		    last->_next = pl->_next ;
		}
		delete pl->_persistence_obj ;
		delete pl ;
	    }
	    else
	    {
		last = pl ;
		pl = pl->_next ;
	    }
	}
	else
	{
	    done = true ;
	}
    }

    return ret ;
}

/** @brief find the persistence store with the given name
 *
 * Returns the persistence store with the given name
 *
 * @param persist_name name of the persistent store to be found
 * @return the persistence store ContainerStorage
 * @see ContainerStorage
 */
ContainerStorage *
ContainerStorageList::find_persistence( const string &persist_name )
{
    ContainerStorage *ret = NULL ;
    ContainerStorageList::persistence_list *pl = _first ;
    bool done = false ;
    while( done == false )
    {
	if( pl )
	{
	    if( persist_name == pl->_persistence_obj->get_name() )
	    {
		ret = pl->_persistence_obj ;
		done = true ;
	    }
	    else
	    {
		pl = pl->_next ;
	    }
	}
	else
	{
	    done = true ;
	}
    }
    return ret ;
}

bool
ContainerStorageList::isnice()
{
    bool ret = false ;
    string key = "DODS.Container.Persistence" ;
    bool found = false ;
    string isnice = TheDODSKeys::TheKeys()->get_key( key, found ) ;
    if( isnice == "Nice" || isnice == "nice" || isnice == "NICE" )
	ret = true ;
    else 
	ret = false ;
    return ret ;
}

/** @brief look for the specified container information in the list of
 * persistent stores.
 *
 * If the container information is found in one of the
 * ContainerStorage instances then it is the responsibility of that
 * instance to fill in the container information in the DODSContainer
 * instances passed.
 *
 * If the container information is not found then, depending on the value of
 * the key DODS.Container.Persistence in the dods initiailization file, an
 * exception is thrown or it is logged to the dods log file that it was not
 * found. If the key is set to Nice, nice, or NICE then information is logged
 * to the dods log file stating that the container information was not found.
 *
 * @param d container information to look for and, if found, to store the
 * container information in.
 * @see ContainerStorage
 * @see DODSContainer
 * @see DODSKeys
 * @see DODSLog
 */
void
ContainerStorageList::look_for( DODSContainer &d )
{
    ContainerStorageList::persistence_list *pl = _first ;
    bool done = false ;
    while( done == false )
    {
	if( pl )
	{
	    pl->_persistence_obj->look_for( d ) ;
	    if( d.is_valid() )
	    {
		done = true ;
	    }
	    else
	    {
		pl = pl->_next ;
	    }
	}
	else
	{
	    done = true ;
	}
    }
    if( d.is_valid() == false )
    {
	if( isnice() )
	    (*DODSLog::TheLog()) << "Could not find the symbolic name "
	                  << d.get_symbolic_name().c_str() << endl ;
	else
	{
	    string s = (string)"Could not find the symbolic name "
	               + d.get_symbolic_name() ;
	    ContainerStorageException pe ;
	    pe.set_error_description( s ) ;
	    throw pe;
	}
    }
}

/** @brief show information for each container in each persistence store
 *
 * For each container in each persistent store, add infomation about each of
 * those containers. The information added to the information object
 * includes the persistent store information, in the order the persistent
 * stores are searched for a container, followed by a line for each
 * container within that persistent store which includes the symbolic name,
 * the real name, and the data type, separated by commas.
 *
 * @param info object to store the container and persistent store information
 * @see DODSInfo
 */
void
ContainerStorageList::show_containers( DODSInfo &info )
{
    ContainerStorageList::persistence_list *pl = _first ;
    if( !pl )
    {
	info.add_data( "No persistence stores available\n" ) ;
    }
    bool first = true ;
    while( pl )
    {
	if( !first )
	{
	    // separate each store with a blank line
	    info.add_data( "\n" ) ;
	}
	first = false ;
	pl->_persistence_obj->show_containers( info ) ;
	pl = pl->_next ;
    }
}

ContainerStorageList *
ContainerStorageList::TheList()
{
    if( _instance == 0 )
    {
	_instance = new ContainerStorageList ;
    }
    return _instance ;
}

// $Log: ContainerStorageList.cc,v $
// Revision 1.6  2005/03/17 19:24:39  pwest
// in show_containers, if no persistence stores available then add message to response info object saying so
//
// Revision 1.5  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.3  2004/12/15 17:36:01  pwest
//
// Changed the way in which the parser retrieves container information, going
// instead to ThePersistenceList, which goes through the list of container
// persistence instances it has.
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
