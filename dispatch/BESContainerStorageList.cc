// BESContainerStorageList.cc

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

#include "config.h"

#include <iostream>
#include <mutex>

#include "BESContainerStorageList.h"
#include "BESContainerStorage.h"
#include "BESSyntaxUserError.h"
#include "BESContainer.h"
#include "TheBESKeys.h"
#include "BESLog.h"
#include "BESInfo.h"

#include "BESDebug.h"

using std::endl;
using std::string;
using std::ostream;

BESContainerStorageList *BESContainerStorageList::d_instance = nullptr;
static std::once_flag d_euc_init_once;

BESContainerStorageList::BESContainerStorageList() :
        _first(0)
{
}

BESContainerStorageList::~BESContainerStorageList()
{
    BESContainerStorageList::persistence_list *pl = _first;
    while (pl) {
        if (pl->_persistence_obj) {
            delete pl->_persistence_obj;
        }
        BESContainerStorageList::persistence_list *next = pl->_next;
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
 * @see BESContainerStorage
 */
bool BESContainerStorageList::add_persistence(BESContainerStorage *cp)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    bool ret = false;
    if (!_first) {
        _first = new BESContainerStorageList::persistence_list;
        _first->_persistence_obj = cp;
        _first->_reference = 1;
        _first->_next = 0;
        ret = true;
    }
    else {
        BESContainerStorageList::persistence_list *pl = _first;
        bool done = false;
        while (done == false) {
            if (pl->_persistence_obj->get_name() != cp->get_name()) {
                if (pl->_next) {
                    pl = pl->_next;
                }
                else {
                    pl->_next = new BESContainerStorageList::persistence_list;
                    pl->_next->_reference = 1;
                    pl->_next->_persistence_obj = cp;
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

/** @brief refence the specified persistent store if in the list
 *
 * Increments the reference count of the persistent store in the
 * list. This lets the system know that there is a module that is
 * referencing the specified catalog
 *
 * @param persist_name name of the persistent store to be referenced
 * @return true if successfully referenced, false otherwise
 * @see BESContainerStorage
 */
bool BESContainerStorageList::ref_persistence(const string &persist_name)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    bool ret = false;
    BESContainerStorageList::persistence_list *pl = _first;

    bool done = false;
    while (done == false) {
        if (pl) {
            if (pl->_persistence_obj && pl->_persistence_obj->get_name() == persist_name) {
                done = true;
                ret = true;
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

/** @brief dereference a persistent store in the list.
 *
 * de-reference the names persistent store. If found, the reference
 * on the catalog is decremented and true is returned. If the
 * reference reaches zero then the catalog is removed. If not found
 * then false is returned.
 *
 * @param persist_name name of the persistent store to be
 * de-referenced
 * @return true if successfully de-referenced, false otherwise
 * @see BESContainerStorage
 */
bool BESContainerStorageList::deref_persistence(const string &persist_name)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    bool ret = false;
    BESContainerStorageList::persistence_list *pl = _first;
    BESContainerStorageList::persistence_list *last = 0;

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
                        if (!last)
                            throw BESInternalError("ContainerStorageList last is null", __FILE__, __LINE__);
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
 * @return the persistence store BESContainerStorage
 * @see BESContainerStorage
 */
BESContainerStorage *
BESContainerStorageList::find_persistence(const string &persist_name)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESContainerStorage *ret = NULL;
    BESContainerStorageList::persistence_list *pl = _first;
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

bool BESContainerStorageList::isnice()
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    bool ret = false;
    string key = "BES.Container.Persistence";
    bool found = false;
    string isnice;
    TheBESKeys::TheKeys()->get_value(key, isnice, found);
    if (isnice == "Nice" || isnice == "nice" || isnice == "NICE")
        ret = true;
    else
        ret = false;
    return ret;
}

/** @brief look for the specified container information in the list of
 * persistent stores.
 *
 * Look for the container with the specified symbolic name in the
 * BESContainerStorage instances. The first to find it wins.
 *
 * If the container information is not found then, depending on the value of
 * the key BES.Container.Persistence in the bes configuration file, an
 * exception is thrown or it is logged to the bes log file that it was not
 * found. If the key is set to Nice, nice, or NICE then information is logged
 * to the bes log file stating that the container information was not found.
 *
 * @param sym_name symbolic name of the container to look for
 * @return a new instances of BESContainer if found, else 0. The caller owns
 * the returned container and is responsible for deleting cleaning
 * @throws BESSyntaxUserError if container not found and strict
 * set in the bes configuration file for BES.Container.Persistence
 * @see BESContainerStorage
 * @see BESContainer
 * @see BESKeys
 * @see BESLog
 * @see BESSyntaxUserError
 */
BESContainer *
BESContainerStorageList::look_for(const string &sym_name)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESContainer *ret_container = 0;
    BESContainerStorageList::persistence_list *pl = _first;
    bool done = false;
    while (done == false) {
        if (pl) {
            ret_container = pl->_persistence_obj->look_for(sym_name);
            if (ret_container) {
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
    if (!ret_container) {
        string msg = (string) "Could not find the symbolic name " + sym_name;
        ERROR_LOG(msg << endl);
        if (!isnice()) {
            throw BESSyntaxUserError(msg, __FILE__, __LINE__);
        }
    }

    return ret_container;
}

/**
 * @brief scan all of the container stores and remove any containers called \arg syn_name
 *
 * Scan all of the Container Storage objects and looking for containers
 * called \arg sym_name and delete those. This method was added as a fix
 * for a bug where containers in one store were used because the <define>
 * command did not have the information it needed to search a particular
 * store and used the global 'look_for' method defined above.
 *
 * @note A better fix is to change the design of the Container and ContainerStorage
 * classes. jhrg 1/8/19
 *
 * @param sym_name The name of the container(s) to delete.
 */
void
BESContainerStorageList::delete_container(const std::string &sym_name)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESContainerStorageList::persistence_list *pl = _first;
    while (pl) {
        (void) pl->_persistence_obj->del_container(sym_name);

        pl = pl->_next;
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
 * @see BESInfo
 */
void BESContainerStorageList::show_containers(BESInfo &info)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESContainerStorageList::persistence_list *pl = _first;
    while (pl) {
        std::map<string, string, std::less<>> props;
        props["name"] = pl->_persistence_obj->get_name();
        info.begin_tag("store", &props);
        pl->_persistence_obj->show_containers(info);
        info.end_tag("store");
        pl = pl->_next;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * the container storage objects stored in this list.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESContainerStorageList::dump(ostream &strm) const
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    strm << BESIndent::LMarg << "BESContainerStorageList::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESContainerStorageList::persistence_list *pl = _first;
    if (pl) {
        strm << BESIndent::LMarg << "container storage:" << endl;
        BESIndent::Indent();
        while (pl) {
            pl->_persistence_obj->dump(strm);
            pl = pl->_next;
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "container storage: empty" << endl;
    }
    BESIndent::UnIndent();
}

BESContainerStorageList *
BESContainerStorageList::TheList()
{
    std::call_once(d_euc_init_once,BESContainerStorageList::initialize_instance);
    return d_instance;
}

void BESContainerStorageList::initialize_instance() {
    d_instance = new BESContainerStorageList;
#ifdef HAVE_ATEXIT
    atexit(delete_instance);
#endif
}

void BESContainerStorageList::delete_instance() {
    delete d_instance;
    d_instance = 0;
}

