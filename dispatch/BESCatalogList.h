// BESCatalogList.h

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

#ifndef BESCatalogList_h_
#define BESCatalogList_h_ 1

#include <map>
#include <string>

using std::map ;
using std::string ;

#include "BESObj.h"
#include "BESDataHandlerInterface.h"

class BESCatalog ;
class BESCatalogEntry ;

#define BES_DEFAULT_CATALOG "catalog"

/** @brief List of all registered catalogs
 *
 * BESCatalog objecgts can be registered with this list. The BES
 * allows for multiple catalogs. Most installations will have a 
 * single catalog registered.
 *
 * Catalogs have a uniq name
 *
 * If there is only one catalog then the display of the root will
 * be the display of that catalogs root.
 *
 * If there are more than one catalogs registered then the view of
 * the root will display the list of catalogs registered. To view
 * the contents of a specific catalog begin each container name with
 * the name of the catalog followed by a colon.
 *
 * show catalog for "cedar_catalog:/instrument/5340/year/2004/";
 *
 * @see BESCatalog
 */
class BESCatalogList : public BESObj
{
private:
    map<string, BESCatalog *>	_catalogs ;
    string			_default_catalog ;
    static BESCatalogList *	_instance ;

    static void initialize_instance();
    static void delete_instance();

    virtual ~BESCatalogList();

    friend class BESCatalogListUnitTest;

protected:
    BESCatalogList();


public:
    typedef map<string,BESCatalog *>::iterator catalog_iter ;
    typedef map<string,BESCatalog *>::const_iterator catalog_citer ;

    static BESCatalogList * TheCatalogList() ;


    virtual int			num_catalogs() { return _catalogs.size() ; }
    virtual string		default_catalog() { return _default_catalog ; }

    virtual bool		add_catalog( BESCatalog *catalog ) ;
    virtual bool		ref_catalog( const string &catalog_name ) ;
    virtual bool		deref_catalog( const string &catalog_name );
    virtual BESCatalog *	find_catalog( const string &catalog_name ) ;
    virtual BESCatalogEntry *	show_catalogs( BESDataHandlerInterface &dhi,
					       BESCatalogEntry *entry,
					       bool show_default = true ) ;
    
    virtual catalog_iter	first_catalog() { return _catalogs.begin() ; }
    virtual catalog_iter	end_catalog() { return _catalogs.end() ; }

    virtual void		dump( ostream &strm ) const ;

} ;

#endif // BESCatalogList_h_

