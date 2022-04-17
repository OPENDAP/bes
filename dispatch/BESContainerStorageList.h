// BESContainerStorageList.h

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

#ifndef I_BESContainerStorageList_H
#define I_BESContainerStorageList_H 1

#include <string>
#include <mutex>

#include "BESObj.h"

class BESContainerStorage;
class BESContainer;
class BESInfo;

#ifndef DEFAULT
#define DEFAULT "default"
#endif

#ifndef CATALOG
#define CATALOG "catalog"
#endif

/** @brief Provides a mechanism for accessing container information from
 * different container stores registered with this server.
 *
 * This class provides a mechanism for users to access container information
 * from different container stores, such as from a MySQL database, a file, or
 * volatile stores.
 *
 * Users can add different BESContainerStorage instances to this
 * persistent list. Then, when a user looks for a symbolic name, that search
 * goes through the list of persistent stores in order.
 *
 * If the symbolic name is not found then a flag is checked to determine
 * whether to simply log the fact that the symbolic name was not found, or to
 * throw an exception of type BESContainerStorageException.
 *
 * @see BESContainerStorage
 * @see BESContainer
 * @see BESContainerStorageException
 */
class BESContainerStorageList: public BESObj {
private:
    static BESContainerStorageList * d_instance;
    mutable std::recursive_mutex d_cache_lock_mutex;

    typedef struct _persistence_list {
        BESContainerStorage *_persistence_obj;
        unsigned int _reference;
        BESContainerStorageList::_persistence_list *_next;
    } persistence_list;

    BESContainerStorageList::persistence_list *_first;

    static void initialize_instance();
    static void delete_instance();

public:
    BESContainerStorageList();
    virtual ~BESContainerStorageList();

    virtual bool add_persistence(BESContainerStorage *p);
    virtual bool ref_persistence(const std::string &persist_name);
    virtual bool deref_persistence(const std::string &persist_name);
    virtual BESContainerStorage *find_persistence(const std::string &persist_name);
    virtual bool isnice();

    // These methods scan all of the container stores. Currently, this is used
    // by both <setContainer> and <define>. However, a better design would disentangle
    // the ContainerStorage from the Container creation. jhrg 1/8/19
    virtual BESContainer *look_for(const std::string &sym_name);
    virtual void delete_container(const std::string &sym_name);

    virtual void show_containers(BESInfo &info);

    virtual void dump(std::ostream &strm) const;

    static BESContainerStorageList *TheList();
};

#endif // I_BESContainerStorageList_H

