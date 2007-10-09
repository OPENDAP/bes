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
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
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
    bool stat = false;
    if (find_catalog(catalog->get_catalog_name()) == 0) {
#if 0
        _catalogs[catalog->get_catalog_name()] = catalog;
#endif
	string name = catalog->get_catalog_name() ;
        std::pair<const std::string, BESCatalog*> p =
	    std::make_pair( name, catalog ) ;
        stat = _catalogs.insert(p).second;
#if 0
        stat = true;
#endif
    }
    return stat;
}

/** @brief remove from the list and delete the specified catalog
 *
 * Search the list for the catalog with the given name. If the
 * catalog exists, remove it from the list and delete it.
 *
 * @param catalog_name name of the catalog to delete
 * @return true if successfully removed and deleted, false otherwise
 * @see BESCatalog
 */
bool
BESCatalogList::del_catalog( const string &catalog_name )
{
    bool ret = false ;
    BESCatalog *cat = 0 ;
    BESCatalogList::catalog_iter i ;
    i = _catalogs.find( catalog_name ) ;
    if( i != _catalogs.end() )
    {
	cat = (*i).second;
	_catalogs.erase( i ) ;
	delete cat ;
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

/** @brief show the contents of the catalog given the specified container
 *
 * This method adds information about the specified container to the
 * informational object specified.
 *
 * If there is only one catalog registered then the container is a node
 * within that one catalog.
 *
 * if there are more than one catalog registered then then:
 * - if the specified container is empty, display the list of catalogs.
 *   tag attributes include "catalogRoot".
 * - if not empty then the specified container must begin with the name
 *   of the catalog followed by a colon. The remainder of the container
 *   specified is the node within that catalog.
 *
 * If coi is catalog then if the specified container is a collection
 * then display the elements in the collection. If coi is info then
 * display information about only the specified container and not
 * its contents if a collection.
 *
 * @param container node to display, empty means root
 * @param coi is the request to include collections or just the specified
 * container
 * @param info informational object to add information to
 * @throws BESHandlerException if more than one catalog and no catalog
 * specified; if the specified catalog does not exist; if the container
 * within the catalog does not exist.
 */
void
BESCatalogList::show_catalog( const string &container,
			      const string &coi,
			      BESInfo *info )
{
    bool done = false ;
    // if there is only one catalog then go to it to show the catalog
    if( _catalogs.size() == 1 )
    {
	catalog_citer i = _catalogs.begin() ;
	BESCatalog *catalog = (*i).second ;
	done = catalog->show_catalog( container, coi, info ) ;
    }
    else if( _catalogs.size() != 0 )
    {
	// This means there are more than one catalog. If the specified container
	// is empty then display the names of the catalogs
	if( container.empty() )
	{
	    map<string,string> a1 ;
	    a1["thredds_collection"] = "\"true\"" ;
	    a1["isData"] = "\"false\"" ;
	    info->begin_tag( "dataset", &a1 ) ;
	    info->add_tag( "name", "/" ) ;

	    a1["catalogRoot"] = "\"true\"" ;
	    catalog_citer i = _catalogs.begin() ;
	    catalog_citer e = _catalogs.end() ;
	    for( ; i != e; i++ )
	    {
		string name = (*i).first ;
		BESCatalog *catalog = (*i).second ;
		info->begin_tag( "dataset", &a1 ) ;
		info->add_tag( "name", name ) ;
		info->end_tag( "dataset" ) ;
	    }

	    info->end_tag( "dataset" ) ;

	    done = true ;
	}
	else
	{
	    // if a container is specified then the name of the catalog
	    // comes first, followed by a colon. If no colon, then no
	    // catalog specified, which means error
	    string::size_type colon = container.find( ":" ) ;
	    if( colon == string::npos )
	    {
		string serr = "Multiple catalogs present but none specified in request" ;
		throw BESHandlerException( serr, __FILE__, __LINE__ ) ;
	    }
	    else
	    {
		// there is a colon. The name is the part before the colon.
		string name = container.substr( 0, colon ) ;
		string rest = container.substr( colon+1, container.length() - colon ) ;
		BESCatalog *catalog = _catalogs[ name ] ;
		if( catalog )
		{
		    done = catalog->show_catalog( rest, coi, info ) ;
		}
		else
		{
		    string serr = "The catalog " + name + " does not exist." ;
		    throw BESHandlerException( serr, __FILE__, __LINE__ ) ;
		}
	    }
	}
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
    }
}

/** @brief returns the singleton BESCatalogList instance
 */
BESCatalogList *
BESCatalogList::TheCatalogList()
{
    if( _instance == 0 )
    {
	_instance = new BESCatalogList ;
    }
    return _instance ;
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

