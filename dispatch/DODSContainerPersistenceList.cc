// DODSContainerPersistenceList.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

using std::endl ;

#include "DODSContainerPersistenceList.h"
#include "DODSContainerPersistence.h"
#include "DODSContainerPersistenceException.h"
#include "DODSContainer.h"
#include "TheDODSKeys.h"
#include "TheDODSLog.h"
#include "DODSInfo.h"

DODSContainerPersistenceList::DODSContainerPersistenceList()
    : _first( 0 )
{
}

DODSContainerPersistenceList::~DODSContainerPersistenceList()
{
    DODSContainerPersistenceList::persistence_list *pl = _first ;
    while( pl )
    {
	if( pl->_persistence_obj )
	{
	    delete pl->_persistence_obj ;
	}
	DODSContainerPersistenceList::persistence_list *next = pl->_next ;
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
 * @see DODSContainerPersistence
 */
bool
DODSContainerPersistenceList::add_persistence( DODSContainerPersistence *cp )
{
    bool ret = false ;
    if( !_first )
    {
	_first = new DODSContainerPersistenceList::persistence_list ;
	_first->_persistence_obj = cp ;
	_first->_next = 0 ;
	ret = true ;
    }
    else
    {
	DODSContainerPersistenceList::persistence_list *pl = _first ;
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
		    pl->_next = new DODSContainerPersistenceList::persistence_list ;
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
 * @see DODSContainerPersistence
 */
bool
DODSContainerPersistenceList::rem_persistence( const string &persist_name )
{
    bool ret = false ;
    DODSContainerPersistenceList::persistence_list *pl = _first ;
    DODSContainerPersistenceList::persistence_list *last = 0 ;

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
 * @return the persistence store DODSContainerPersistence
 * @see DODSContainerPersistence
 */
DODSContainerPersistence *
DODSContainerPersistenceList::find_persistence( const string &persist_name )
{
    DODSContainerPersistence *ret = NULL ;
    DODSContainerPersistenceList::persistence_list *pl = _first ;
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
DODSContainerPersistenceList::isnice()
{
    bool ret = false ;
    string key = "DODS.Container.Persistence" ;
    bool found = false ;
    string isnice = TheDODSKeys->get_key( key, found ) ;
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
 * DODSContainerPersistence instances then it is the responsibility of that
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
 * @see DODSContainerPersistence
 * @see DODSContainer
 * @see DODSKeys
 * @see DODSLog
 */
void
DODSContainerPersistenceList::look_for( DODSContainer &d )
{
    DODSContainerPersistenceList::persistence_list *pl = _first ;
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
	    (*TheDODSLog) << "Could not find the symbolic name "
	                  << d.get_symbolic_name().c_str() << endl ;
	else
	{
	    string s = (string)"Could not find the symbolic name "
	               + d.get_symbolic_name() ;
	    DODSContainerPersistenceException pe ;
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
DODSContainerPersistenceList::show_containers( DODSInfo &info )
{
    DODSContainerPersistenceList::persistence_list *pl = _first ;
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

// $Log: DODSContainerPersistenceList.cc,v $
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
