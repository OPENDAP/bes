// BESDefinitionStorageList.cc

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

#include <iostream>
#include <mutex>
#include "config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

using std::endl;
using std::string;
using std::ostream;

#include "BESDefinitionStorageList.h"
#include "BESDefinitionStorage.h"
#include "BESDefine.h"
#include "BESInfo.h"

BESDefinitionStorageList *BESDefinitionStorageList::d_instance = nullptr;
static std::once_flag d_euc_init_once;

BESDefinitionStorageList::BESDefinitionStorageList() :
    _first(0)
{
}

BESDefinitionStorageList::~BESDefinitionStorageList()
{
    BESDefinitionStorageList::persistence_list *pl = _first;
    while (pl) {
        if (pl->_persistence_obj) {
            delete pl->_persistence_obj;
        }
        BESDefinitionStorageList::persistence_list *next = pl->_next;
        delete pl;
        pl = next;
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
 * @see BESDefinitionStorage
 */
bool BESDefinitionStorageList::add_persistence(BESDefinitionStorage *cp)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    bool ret = false;
    if (!_first) {
        _first = new BESDefinitionStorageList::persistence_list;
        _first->_persistence_obj = cp;
        _first->_reference = 1;
        _first->_next = 0;
        ret = true;
    }
    else {
        BESDefinitionStorageList::persistence_list *pl = _first;
        bool done = false;
        while (done == false) {
            if (pl->_persistence_obj->get_name() != cp->get_name()) {
                if (pl->_next) {
                    pl = pl->_next;
                }
                else {
                    pl->_next = new BESDefinitionStorageList::persistence_list;
                    pl->_next->_persistence_obj = cp;
                    pl->_next->_reference = 1;
                    pl->_next->_next = 0;
                    done = true;
                    ret = true;
                }
            }
            else {
                done = true;
                ret = false;
            }
        }
    }
    return ret;
}

/** @brief reference a persistent store in the list
 *
 * Informs the list that there is a reference to a definition store
 *
 * @param persist_name name of the persistent store to be referenced
 * @return true if successfully referenced, false if not found
 * @see BESDefinitionStorage
 */
bool BESDefinitionStorageList::ref_persistence(const string &persist_name)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    bool ret = false;
    BESDefinitionStorageList::persistence_list *pl = _first;

    bool done = false;
    while (done == false) {
        if (pl) {
            if (pl->_persistence_obj && pl->_persistence_obj->get_name() == persist_name) {
                ret = true;
                done = true;
                pl->_reference++;
            }
            else {
                pl = pl->_next;
            }
        }
        else {
            done = true;
        }
    }

    return ret;
}

/** @brief de-reference a persistent store in the list
 *
 * De-reference the specified store in the list. If this is the last
 * reference then the definition store is removed from the list and
 * deleted.
 *
 * @param persist_name name of the persistent store to be de-referenced
 * @return true if successfully de-referenced, false if not found
 * @see BESDefinitionStorage
 */
bool BESDefinitionStorageList::deref_persistence(const string &persist_name)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    bool ret = false;
    BESDefinitionStorageList::persistence_list *pl = _first;
    BESDefinitionStorageList::persistence_list *last = 0;

    bool done = false;
    while (done == false) {
        if (pl) {
            if (pl->_persistence_obj && pl->_persistence_obj->get_name() == persist_name) {
                ret = true;
                done = true;
                pl->_reference--;
                if (!pl->_reference) {
                    if (pl == _first) {
                        _first = _first->_next;
                    }
                    else {
                        if (!last) throw BESInternalError("ContainerStorageList last is null", __FILE__, __LINE__);
                        last->_next = pl->_next;
                    }
                    delete pl->_persistence_obj;
                    delete pl;
                    pl = 0;
                }
            }
            else {
                last = pl;
                pl = pl->_next;
            }
        }
        else {
            done = true;
        }
    }

    return ret;
}

/** @brief find the persistence store with the given name
 *
 * Returns the persistence store with the given name
 *
 * @param persist_name name of the persistent store to be found
 * @return the persistence store BESDefinitionStorage
 * @see BESDefinitionStorage
 */
BESDefinitionStorage *
BESDefinitionStorageList::find_persistence(const string &persist_name)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESDefinitionStorage *ret = NULL;
    BESDefinitionStorageList::persistence_list *pl = _first;
    bool done = false;
    while (done == false) {
        if (pl) {
            if (persist_name == pl->_persistence_obj->get_name()) {
                ret = pl->_persistence_obj;
                done = true;
            }
            else {
                pl = pl->_next;
            }
        }
        else {
            done = true;
        }
    }
    return ret;
}

/** @brief look for the specified definition in the list of defintion stores.
 *
 * Looks for a definition with the given name in the order in which
 * definition stores were added to the definition storage list.
 *
 * @param def_name name of the definition to find
 * @return defintion with the given name, null otherwise
 * @see BESDefinitionStorage
 * @see BESDefine
 */
BESDefine *
BESDefinitionStorageList::look_for(const string &def_name)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESDefine *ret_def = NULL;
    BESDefinitionStorageList::persistence_list *pl = _first;
    bool done = false;
    while (done == false) {
        if (pl) {
            ret_def = pl->_persistence_obj->look_for(def_name);
            if (ret_def) {
                done = true;
            }
            else {
                pl = pl->_next;
            }
        }
        else {
            done = true;
        }
    }
    return ret_def;
}

/** @brief show information for each definition in each persistence store
 *
 * For each definition in each persistent store, add infomation about each of
 * those definitions. The information added to the information object
 * includes the persistent store name, in the order the persistent
 * stores are searched, followed by a line for each definition within that
 * persistent store which includes the name of the definition, information
 * about each container used by that definition, the aggregation server
 * being used and the aggregation command being used if aggregation is
 * specified.
 *
 * @param info object to store the definition and persistent store information
 * @see BESInfo
 */
void BESDefinitionStorageList::show_definitions(BESInfo &info)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESDefinitionStorageList::persistence_list *pl = _first;
    bool first = true;
    while (pl) {
        if (!first) {
            // separate each store with a blank line
            info.add_break(1);
        }
        first = false;
        std::map<string, string, std::less<>> props;
        props["name"] = pl->_persistence_obj->get_name();
        info.begin_tag("store", &props);
        pl->_persistence_obj->show_definitions(info);
        info.end_tag("store");
        pl = pl->_next;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the list of
 * definition storage instaces registered with the list.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDefinitionStorageList::dump(ostream &strm) const
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    strm << BESIndent::LMarg << "BESDefinitionStorageList::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (_first) {
        strm << BESIndent::LMarg << "registered definition storage:" << endl;
        BESIndent::Indent();
        BESDefinitionStorageList::persistence_list *pl = _first;
        while (pl) {
            pl->_persistence_obj->dump(strm);
            pl = pl->_next;
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "registered definition storage: none" << endl;
    }
    BESIndent::UnIndent();
}

BESDefinitionStorageList *
BESDefinitionStorageList::TheList()
{
    std::call_once(d_euc_init_once,BESDefinitionStorageList::initialize_instance);
    return d_instance;
}

void BESDefinitionStorageList::initialize_instance() {
    d_instance = new BESDefinitionStorageList;
#ifdef HAVE_ATEXIT
    atexit(delete_instance);
#endif
}

void BESDefinitionStorageList::delete_instance() {
    delete d_instance;
    d_instance = 0;
}

