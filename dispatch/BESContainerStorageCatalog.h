// BESContainerStorageCatalog.h

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

#ifndef BESContainerStorageCatalog_h_
#define BESContainerStorageCatalog_h_ 1

#include <list>

using std::list ;

#include "BESContainerStorageVolatile.h"

class BESCatalogUtils ;

/** @brief implementation of BESContainerStorage that represents a
 * regular expression means of determining a data type.
 *
 * When a container is added to this container storage, the file extension
 * is used to determine the type of data using a set of regular expressions.
 * The regular expressions are retrieved from the BES initialization
 * file using TheBESKeys. It also gets the root directory for where the
 * files exist. This way, the user need not know the root directory or the
 * type of data represented by the file.
 *
 * Catalog.&lt;name&gt;.RootDirectory is the key
 * representing the base directory where the files are physically located.
 * The real_name of the container is determined by concatenating the file
 * name to the base directory.
 *
 * Catalog.&lt;name&gt;.TypeMatch is the key
 * representing the regular expressions. This key is formatted as follows:
 *
 * &lt;data type&gt;:&lt;reg exp&gt;;&lt;data type&gt;:&lt;reg exp&gt;;
 *
 * For example: cedar:cedar\/.*\.cbf;cdf:cdf\/.*\.cdf;
 *
 * The first would match anything that might look like: cedar/datfile01.cbf
 *
 * &lt;name&gt; is the name of this container storage, so you could have
 * multiple container stores using regular expressions.
 *
 * The containers are stored in a volatile list.
 *
 * @see BESContainerStorage
 * @see BESContainer
 * @see BESKeys
 */
class BESContainerStorageCatalog : public BESContainerStorageVolatile
{
private:
    const BESCatalogUtils *	_utils ;
public:
    				BESContainerStorageCatalog( const string &n ) ;
    virtual			~BESContainerStorageCatalog() ;

    virtual void		add_container( const string &s_name,
                                               const string &r_name,
					       const string &type ) ;
    bool			isData( const string &inQuestion,
    					list<string> &provides ) ;

    virtual void		dump( ostream &strm ) const ;
};

#endif // BESContainerStorageCatalog_h_

