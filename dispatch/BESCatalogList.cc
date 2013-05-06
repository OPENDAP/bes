// BESCatalogList.cc

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <sstream>

using std::ostringstream ;

#include "BESCatalogList.h"
#include "BESCatalog.h"
#include "BESCatalogEntry.h"
#include "BESInfo.h"
#include "BESSyntaxUserError.h"
#include "TheBESKeys.h"
#include "BESNames.h"



static pthread_once_t instance_control = PTHREAD_ONCE_INIT;



BESCatalogList *BESCatalogList::_instance = 0 ;

/** @brief returns the singleton BESCatalogList instance. The pthreads library insures that only one instance
 * can be made in a process lifetime.
 */
BESCatalogList *
BESCatalogList::TheCatalogList()
{
    pthread_once(&instance_control, initialize_instance);
    return _instance;
}

/**
 * private static that only get's called once by dint of...    EXPLAIN
 */
void BESCatalogList::initialize_instance() {
    if (_instance == 0) {
        _instance = new BESCatalogList;
        atexit(delete_instance);
    }
}

/**
 * Private static function can only be called by friends andf pThreads code.
 */
void BESCatalogList::delete_instance() {
    delete _instance;
    _instance = 0;
}



/** @brief construct a catalog list
 *
 * @see BESCatalog
 */
BESCatalogList::BESCatalogList()
{
    bool found = false ;
    string key = "BES.Catalog.Default" ;
    try
    {
	TheBESKeys::TheKeys()->get_value( key, _default_catalog, found ) ;
    }
    catch( BESError & )
    {
	found = false ;
    }
    if( !found || _default_catalog.empty() )
    {
	_default_catalog = BES_DEFAULT_CATALOG ;
    }
}




/** @brief list destructor deletes all registered catalogs
 *
 * @see BESCatalog
 */
BESCatalogList::~BESCatalogList()
{
    catalog_iter i = _catalogs.begin() ;
    catalog_iter e = _catalogs.end() ;
    for( ; i != e; i++ )
    {
	BESCatalog *catalog = (*i).second ;
	if( catalog ) delete catalog ;
    }
}





/** @brief adds the speciifed catalog to the list
 *
 * @param catalog new catalog to add to the list
 * @return false if a catalog with the given catalog's name
 * already exists. Returns true otherwise.
 * @see BESCatalog
 */
bool
BESCatalogList::add_catalog(BESCatalog * catalog)
{
    bool result = false;
    if( catalog )
    {
	if (find_catalog(catalog->get_catalog_name()) == 0)
	{
#if 0
	    _catalogs[catalog->get_catalog_name()] = catalog;
#endif
	    string name = catalog->get_catalog_name() ;
	    std::pair<const std::string, BESCatalog*> p =
		std::make_pair( name, catalog ) ;
	    result = _catalogs.insert(p).second;
#if 0
	    result = true;
#endif
	}
    }
    return result;
}

/** @brief reference the specified catalog
 *
 * Search the list for the catalog with the given name. If the
 * catalog exists, reference it and return true. If not found then
 * return false.
 *
 * @param catalog_name name of the catalog to reference
 * @return true if successfully found and referenced, false otherwise
 * @see BESCatalog
 */
bool
BESCatalogList::ref_catalog( const string &catalog_name )
{
    bool ret = false ;
    BESCatalog *cat = 0 ;
    BESCatalogList::catalog_iter i ;
    i = _catalogs.find( catalog_name ) ;
    if( i != _catalogs.end() )
    {
	cat = (*i).second;
	cat->reference_catalog() ;
	ret = true ;
    }
    return ret ;
}

/** @brief de-reference the specified catalog and remove from list
 * if no longer referenced
 *
 * Search the list for the catalog with the given name. If the
 * catalog exists, de-reference it. If there are no more references
 * then remove the catalog from the list and delete it.
 *
 * @param catalog_name name of the catalog to de-reference
 * @return true if successfully de-referenced, false otherwise
 * @see BESCatalog
 */
bool
BESCatalogList::deref_catalog( const string &catalog_name )
{
    bool ret = false ;
    BESCatalog *cat = 0 ;
    BESCatalogList::catalog_iter i ;
    i = _catalogs.find( catalog_name ) ;
    if( i != _catalogs.end() )
    {
	cat = (*i).second;
	if( !cat->dereference_catalog() )
	{
	    _catalogs.erase( i ) ;
	    delete cat ;
	}
	ret = true ;
    }
    return ret ;
}

/** @brief find the catalog in the list with the specified name
 *
 * @param catalog_name name of the catalog to find
 * @return a BESCatalog with the given name if found, 0 otherwise
 * @see BESCatalog
 */
BESCatalog *
BESCatalogList::find_catalog( const string &catalog_name )
{
    BESCatalog *ret = 0 ;
    BESCatalogList::catalog_citer i ;
    i = _catalogs.find( catalog_name ) ;
    if( i != _catalogs.end() )
    {
	ret = (*i).second;
    }
    return ret ;
}

/** @brief show the list of catalogs
 *
 * This method adds information about the list of catalogs that exist
 *
 * If there is a problem accessing the requested node then the reason for
 * the problem must be included in the informational response, not an
 * exception thrown. This method will not throw an exception.
 *
 * @param coi is the request to include collections or just the specified
 * container
 * @param info informational object to add information to
 */
BESCatalogEntry *
BESCatalogList::show_catalogs( BESDataHandlerInterface &dhi,
			       BESCatalogEntry *entry,
			       bool show_default )
{
    BESCatalogEntry *myentry = entry ;
    if( !myentry )
    {
	myentry = new BESCatalogEntry( "/", "" ) ;
    }
    catalog_citer i = _catalogs.begin() ;
    catalog_citer e = _catalogs.end() ;
    for( ; i != e; i++ )
    {
	// if show_default is true then display all catalogs
	// if !show_default but this current catalog is not the default
	// then display
	if( show_default || (*i).first != default_catalog() )
	{
	    BESCatalog *catalog = (*i).second ;
	    catalog->show_catalog( "", SHOW_INFO_RESPONSE, myentry ) ;
	}
    }

    return myentry ;
}


/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the catalogs
 * registered in this list.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESCatalogList::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESCatalogList::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "default catalog: "
			     << _default_catalog << endl ;
    if( _catalogs.size() )
    {
	strm << BESIndent::LMarg << "catalog list:" << endl ;
	BESIndent::Indent() ;
	catalog_citer i = _catalogs.begin() ;
	catalog_citer e = _catalogs.end() ;
	for( ; i != e; i++ )
	{
	    BESCatalog *catalog = (*i).second ;
	    strm << BESIndent::LMarg << (*i).first << catalog << endl ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "catalog list: empty" << endl ;
    }
    BESIndent::UnIndent() ;
}

