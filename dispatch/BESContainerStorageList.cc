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
//
// Updated to C++14 using Google Gemini jhrg 4/18/25

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
    std::lock_guard<std::recursive_mutex> lock(d_cache_lock_mutex);

    if (!cp) {
        return false;
    }

    const std::string& new_name = cp->get_name();

    // --- Check for Duplicates ---
    const auto it = find_entry_iterator(new_name);
    if (it != d_storage_entries.end()) {
        // Store with this name already exists, do not add.
        return false;
    }

    // --- Add New Entry ---
    // StorageEntry ctor sets reference_count to zero
    d_storage_entries.emplace_back(cp);
    d_storage_entries.back().reference_count = 1;

    return true;
}

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

/**
 * @brief Finds a persistence store by name.
 * @param persist_name The name of the persistence store to find.
 * @return Raw pointer to the found BESContainerStorage, or nullptr if not found. The caller
 * should not delete this pointer!
 */
BESContainerStorage* BESContainerStorageList::find_persistence(const std::string& persist_name) {
    std::lock_guard<std::recursive_mutex> lock(d_cache_lock_mutex);

    const auto it = find_entry_iterator(persist_name);

    if (it != d_storage_entries.end()) {
        return it->storage_obj.get(); // return raw pointer obtained from unique_ptr
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

/**
 * @brief Scans all container stores in order to find a container by symbolic name.
 * @param sym_name The symbolic name of the container to find.
 * @return Pointer to the found BESContainer, or nullptr if not found in any store.
 * The caller of this method assumes ownership of the BESContainer pointer returned.
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
