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

/**
 * @brief Add a persistent store pointer to the list.
 *
 * Each persistent store has a name (retrieved via getPersistenceName()).
 * If a persistent store pointer is already in the list with that name,
 * the new pointer is not added. Otherwise, the pointer is added to the
 * end of the list with a reference count of 1.
 *
 * The list does NOT take ownership of the pointer 'cp'. Caller manages lifetime.
 * The persistent stores are searched in the order in which they were added.
 *
 * @param cp Non-owning pointer to the persistent store to add to the list.
 * @return True if successfully added, false otherwise (e.g., cp is null,
 * or a store with the same name already exists).
 * @see BESContainerStorage
 * @see BESContainerStorageList::StorageEntry
 */
bool BESContainerStorageList::add_persistence(BESContainerStorage *cp)
{
    // Lock the mutex for thread safety during access and modification
    std::lock_guard<std::recursive_mutex> lock(d_cache_lock_mutex);

    // --- Pre-conditions ---
    // 1. Check for null input pointer. Cannot add a null store.
    if (!cp) {
        return false;
    }

    // 2. Get the name from the provided storage object.
    // This assumes cp is valid and getPersistenceName() returns a valid reference.
    // Handle potential exceptions from getPersistenceName if necessary.
    const std::string& new_name = cp->get_name();

    // --- Check for Duplicates ---
    // 3. Iterate through the existing entries to find if a store with the
    //    same name already exists.
    //    We use the findEntryIterator helper or std::find_if directly.
    const auto it = find_entry_iterator(new_name); // Use the private helper for consistency

    // If the iterator is not the end iterator, a store with this name was found.
    if (it != d_storage_entries.end()) {
        // Store with this name already exists, do not add.
        return false;
    }

    // --- Add New Entry ---
    // 4. If no store with the same name exists, add the new pointer to the
    //    end of the vector.
    //    Use emplace_back to construct the StorageEntry directly in the vector.
    //    The StorageEntry constructor takes the raw pointer 'cp'.
    d_storage_entries.emplace_back(cp);

    // 5. Set the reference count for the newly added entry to 1.
    //    This follows the logic of the original linked-list implementation.
    //    Access the newly added element using back().
    d_storage_entries.back().reference_count = 1;

    // 6. Return true to indicate the store was successfully added.
    return true;
}

#if 0
////// old
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
        _first->_next = nullptr;
        ret = true;
    }
    else {
        BESContainerStorageList::persistence_list *pl = _first;
        bool done = false;
        while (!done) {
            if (pl->_persistence_obj->get_name() != cp->get_name()) {
                if (pl->_next) {
                    pl = pl->_next;
                }
                else {
                    pl->_next = new BESContainerStorageList::persistence_list;
                    pl->_next->_reference = 1;
                    pl->_next->_persistence_obj = cp;
                    pl->_next->_next = nullptr;
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

#endif

/**
 * @brief Increments the reference count for a persistence store by name.
 * @param persist_name The name of the persistence store to reference.
 * @return True if the store was found and referenced, false otherwise.
 */
bool BESContainerStorageList::ref_persistence(const std::string &persist_name) {
    std::lock_guard<std::recursive_mutex> lock(d_cache_lock_mutex);

    auto it = find_entry_iterator(persist_name);

    if (it != d_storage_entries.end()) {
        it->reference_count++;
        return true;
    }

    return false;
}

#if 0

/** @brief reference the specified persistent store if in the list
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
    while (!done) {
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

#endif
/**
 * @brief Decrements the reference count for a persistence store by name.
 * @param persist_name The name of the persistence store to dereference.
 * @return True if the store was found and dereferenced, false otherwise.
 */
bool BESContainerStorageList::deref_persistence(const std::string &persist_name) {
    std::lock_guard<std::recursive_mutex> lock(d_cache_lock_mutex);

    auto it = find_entry_iterator(persist_name);

    if (it != d_storage_entries.end()) {
        if (it->reference_count > 0) {
            it->reference_count--;
            if (it->reference_count == 0) {
                // If reference count reaches zero, remove the entry.
                d_storage_entries.erase(it);
            }
            return true;
        }
    }
    return false;
}

#if 0

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
    BESContainerStorageList::persistence_list *last = nullptr;

    bool done = false;
    while (!done) {
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
                    pl = nullptr;
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

#endif
/**
 * @brief Finds a persistence store by name.
 * @param persist_name The name of the persistence store to find.
 * @return Raw pointer to the found BESContainerStorage, or nullptr if not found.
 * The returned pointer is non-owning; its lifetime is managed externally by the caller.
 */
BESContainerStorage* BESContainerStorageList::find_persistence(const std::string& persist_name) {
    std::lock_guard<std::recursive_mutex> lock(d_cache_lock_mutex);

    const auto it = find_entry_iterator(persist_name);

    if (it != d_storage_entries.end()) {
        return it->storage_obj;
    }

    return nullptr;
}

/**
 * @brief Checks some condition (unclear from original name/context).
 * @return Boolean result based on the check.
 */
bool BESContainerStorageList::is_nice() {
    std::lock_guard<std::recursive_mutex> lock(d_cache_lock_mutex);

    std::string key = "BES.Container.Persistence";
    bool found = false;
    std::string is_nice;
    TheBESKeys::TheKeys()->get_value(key, is_nice, found);

    return (is_nice == "Nice" || is_nice == "nice" || is_nice == "NICE");
}

#if 0

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

    BESContainerStorage *ret = nullptr;
    BESContainerStorageList::persistence_list *pl = _first;
    bool done = false;
    while (!done) {
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

bool BESContainerStorageList::is_nice()
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

#endif

/**
 * @brief Scans all container stores in order to find a container by symbolic name.
 * @param sym_name The symbolic name of the container to find.
 * @return Pointer to the found BESContainer, or nullptr if not found in any store.
 * Ownership likely belongs to the BESContainerStorage that provided it.
 */
BESContainer* BESContainerStorageList::look_for(const std::string& sym_name) {
    std::lock_guard<std::recursive_mutex> lock(d_cache_lock_mutex);

    for (const auto& entry : d_storage_entries) {
        if (entry.storage_obj) {
            BESContainer* container = entry.storage_obj->look_for(sym_name);
            if (container) {
                return container;
            }
        }
    }

    std::string msg = "Could not find the symbolic name " + sym_name;
    ERROR_LOG(msg);
    if (!is_nice()) {
        throw BESSyntaxUserError(msg, __FILE__, __LINE__);
    }

    return nullptr;
}

/**
 * @brief Requests deletion of a container by symbolic name across all stores.
 * Iterates through stores and asks each to delete the container if found.
 * @param sym_name The symbolic name of the container to delete.
 */
void BESContainerStorageList::delete_container(const std::string& sym_name) {
    std::lock_guard<std::recursive_mutex> lock(d_cache_lock_mutex);

    for (const auto& entry : d_storage_entries) {
        if (entry.storage_obj) {
            entry.storage_obj->del_container(sym_name);
        }
    }
}

/**
 * @brief Populates BESInfo with details about the registered containers/stores.
 * @param info The BESInfo object to populate.
 */
void BESContainerStorageList::show_containers(BESInfo& info) {
    std::lock_guard<std::recursive_mutex> lock(d_cache_lock_mutex);

    for (const auto& entry : d_storage_entries) {
        if (entry.storage_obj) {
            std::map<std::string, std::string, std::less<>> props;
            props["name"] = entry.storage_obj->get_name();
            info.begin_tag("store", &props);
            entry.storage_obj->show_containers(info);
            info.end_tag("store");
        }
    }
}

#if 0

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
        string msg = "Could not find the symbolic name " + sym_name;
        ERROR_LOG(msg);
        if (!is_nice()) {
            throw BESSyntaxUserError(msg, __FILE__, __LINE__);
        }
    }

    return ret_container;
}

/**
 * @brief scan all the container stores and remove any containers called \arg syn_name
 *
 * Scan all the Container Storage objects and looking for containers
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

#endif

/**
 * @brief Dumps the state of the storage list to an output stream.
 * @param strm The output stream.
 */
void BESContainerStorageList::dump(std::ostream& strm) const {
    std::lock_guard<std::recursive_mutex> lock(d_cache_lock_mutex);

    strm << BESIndent::LMarg << "BESContainerStorageList::dump - (" << (void*)this << ")" << std::endl;
    BESIndent::Indent();

    if (!d_storage_entries.empty()) {
        strm << BESIndent::LMarg << "container storage:" << std::endl;
        BESIndent::Indent();
        for (const auto& entry : d_storage_entries) {
            if (entry.storage_obj) {
                entry.storage_obj->dump(strm);
            } else {
                strm << BESIndent::LMarg << "Storage entry with null pointer" << std::endl;
            }
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "container storage: empty" << std::endl;
    }
    BESIndent::UnIndent();
}

#if 0

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

#endif
