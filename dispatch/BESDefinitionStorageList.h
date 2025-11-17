// BESDefinitionStorageList.h

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

#ifndef I_BESDefinitionStorageList_H
#define I_BESDefinitionStorageList_H 1

#include <mutex>
#include <string>

#include "BESObj.h"

class BESDefinitionStorage;
class BESDefine;
class BESInfo;

#ifndef DEFAULT
#define DEFAULT "default"
#endif

#ifndef CATALOG
#define CATALOG "catalog"
#endif

/** @brief Provides a mechanism for accessing definitions from
 * different definition stores registered with this server.
 *
 * This class provides a mechanism for users to access definitions
 * from different definition stores, such as from a MySQL database, a file, or
 * volatile stores.
 *
 * Users can add different BESDefinitionStorage instances to this
 * persistent list. Then, when a user looks for a definition, that search
 * goes through the list of persistent stores in the order they were added
 * to this list.
 *
 * @see BESDefinitionStorage
 * @see BESDefine
 * @see BESDefinitionStorageException
 */
class BESDefinitionStorageList : public BESObj {
private:
    mutable std::recursive_mutex d_cache_lock_mutex;

    static void initialize_instance();
    static void delete_instance();

    typedef struct _persistence_list {
        BESDefinitionStorage *_persistence_obj;
        unsigned int _reference;
        BESDefinitionStorageList::_persistence_list *_next;
    } persistence_list;

    BESDefinitionStorageList::persistence_list *_first;
    BESDefinitionStorageList();

public:
    ~BESDefinitionStorageList() override = default;

    BESDefinitionStorageList(const BESDefinitionStorageList &) = delete;
    BESDefinitionStorageList &operator=(const BESDefinitionStorageList &) = delete;

    virtual bool add_persistence(BESDefinitionStorage *p);
    virtual bool ref_persistence(const std::string &persist_name);
    virtual bool deref_persistence(const std::string &persist_name);
    virtual BESDefinitionStorage *find_persistence(const std::string &persist_name);

    virtual BESDefine *look_for(const std::string &def_name);

    virtual void show_definitions(BESInfo &info);

    void dump(std::ostream &strm) const override;

    static BESDefinitionStorageList *TheList();
};

#endif // I_BESDefinitionStorageList_H
