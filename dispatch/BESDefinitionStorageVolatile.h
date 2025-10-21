// BESDefinitionStorageVolatile.h

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

#ifndef BESDefinitionStorageVolatile_h_
#define BESDefinitionStorageVolatile_h_ 1

#include <string>
#include <map>

#include "BESDefinitionStorage.h"

class BESDefine;
class BESInfo;

/** @brief provides volatile storage for a specific definition/view of
 * different containers including contraints and aggregation.
 *
 * An implementation of the abstract interface BESDefinitionStorage
 * provides volatile storage for a definition, or view, of a set of data
 * including possibly constraints on each of those containers and possibly
 * aggregation of those containers.
 *
 * @see BESDefine
 * @see BESDefinitionStorageList
 */
class BESDefinitionStorageVolatile: public BESDefinitionStorage {
private:
	std::map<std::string, BESDefine *> _def_list;
    typedef std::map<std::string, BESDefine *>::const_iterator Define_citer;
    typedef std::map<std::string, BESDefine *>::iterator Define_iter;
public:
    /** @brief create an instance of BESDefinitionStorageVolatile with the give
     * name.
     *
     * @param name name of this persistence store
     */
    BESDefinitionStorageVolatile(const std::string &name) :
        BESDefinitionStorage(name)
    {
    }

    virtual ~BESDefinitionStorageVolatile();

    virtual BESDefine * look_for(const std::string &def_name);

    virtual bool add_definition(const std::string &def_name, BESDefine *d);

    virtual bool del_definition(const std::string &def_name);
    virtual bool del_definitions();

    virtual void show_definitions(BESInfo &info);

    void dump(std::ostream &strm) const override;
};

#endif // BESDefinitionStorageVolatile_h_

