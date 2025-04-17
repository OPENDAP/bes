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

#include <vector>          // For std::vector
#include <string>          // For std::string
#include <mutex>           // For std::recursive_mutex
#include <ostream>         // For dump method parameter
#include <stdexcept>       // For potential exceptions (though none shown here yet)
#include <algorithm>       // For std::find_if
#include <memory>          // Although not using unique_ptr, still good practice include

#include "BESObj.h"
#include "BESContainerStorage.h"

class BESContainer;
class BESInfo;

/** @brief Provides a mechanism for accessing container information from
 * different container stores registered with this server.
 *
 * This class provides a mechanism for users to access container information
 * from different container stores, such as from a MySQL database, a file, or
 * volatile stores.
 *
 * Uses a list (vector) to manage different BESContainerStorage instances.
 * Searches for symbolic names proceed through the list of persistent stores in order.
 *
 * If a symbolic name is not found then a flag is checked (logic within methods)
 * to determine whether to simply log the fact or throw an exception.
 *
 * @see BESContainerStorage
 * @see BESContainer
 * @see BESContainerStorageException (Assuming this exception class exists)
 */
class BESContainerStorageList : public BESObj {
private:
    // Structure to hold the managed storage object and its application-level reference count.
    struct StorageEntry {
        std::unique_ptr<BESContainerStorage> storage_obj; // Manages ownership
        unsigned int reference_count = 0;                   // Application-level ref count

        // Constructor takes ownership of the provided storage object
        // Note: Prefer passing unique_ptr directly if possible in add_persistence.
        explicit StorageEntry(BESContainerStorage* obj)
                : storage_obj(obj)  // Takes ownership, starts with 0 refs
        {
            if (!obj) {
                // Handle null input if necessary, perhaps throw?
                // For now, allowing it, but unique_ptr will be null.
            }
        }

        // Constructor for moving a unique_ptr in (preferred)
        explicit StorageEntry(std::unique_ptr<BESContainerStorage> obj_ptr)
                : storage_obj(std::move(obj_ptr)) {}

        // Deleted copy operations because unique_ptr is not copyable
        StorageEntry(const StorageEntry&) = delete;
        StorageEntry& operator=(const StorageEntry&) = delete;

        // Default move operations are fine
        StorageEntry(StorageEntry&&) = default;
        StorageEntry& operator=(StorageEntry&&) = default;

        // Helper to get the name from the storage object.
        // Assumes BESContainerStorage has a method like get_name() const.
        // Replace 'get_name' with the actual method name if different.
        const std::string& get_name() const {
            if (storage_obj) {
                // return storage_obj->get_name(); // Hypothetical method call
                // If get_name() is not const, you might need a const_cast or redesign.
                // Let's assume a hypothetical getPersistenceName() const method exists:
                return storage_obj->get_name();
            }
            static const std::string empty_name = ""; // Return empty if no object
            return empty_name;
        }
    };

    mutable std::recursive_mutex d_cache_lock_mutex; // Mutex for thread safety
    std::vector<StorageEntry> d_storage_entries;     // Use std::vector to hold entries

    // Private helper to find an entry by name (implementation detail)
    // Returns an iterator to the found entry or d_storage_entries.end() if not found.
    // Note: Lock should be held by the caller.
    auto find_entry_iterator(const std::string& persist_name) {
        return std::find_if(d_storage_entries.begin(), d_storage_entries.end(),
                            [&persist_name](const StorageEntry& entry) {
                                // Check if storage_obj is valid before calling get_name()
                                return entry.storage_obj && entry.get_name() == persist_name;
                            });
    }

    // Const version of the helper
    auto find_entry_iterator(const std::string& persist_name) const {
        return std::find_if(d_storage_entries.begin(), d_storage_entries.end(),
                            [&persist_name](const StorageEntry& entry) {
                                return entry.storage_obj && entry.get_name() == persist_name;
                            });
    }


public:
    // Default constructor is acceptable
    BESContainerStorageList() = default;

    // Destructor is implicitly defaulted.
    // std::vector will destroy its elements.
    // StorageEntry's destructor calls std::unique_ptr's destructor, which deletes the object.
    ~BESContainerStorageList() override = default;

    // --- Singleton Pattern ---
    // Make the singleton non-copyable and non-movable
    BESContainerStorageList(const BESContainerStorageList&) = delete;
    BESContainerStorageList& operator=(const BESContainerStorageList&) = delete;
    BESContainerStorageList(BESContainerStorageList&&) = delete;
    BESContainerStorageList& operator=(BESContainerStorageList&&) = delete;

    /**
     * @brief Get the singleton instance of the list.
     * @return Pointer to the singleton BESContainerStorageList.
     */
    static BESContainerStorageList* TheList() {
        // C++11 guarantees thread-safe initialization of static locals
        static BESContainerStorageList instance;
        return &instance;
    }

    // It might make better sense to call these methods add_storage(), ref_storage(), ...
    // jhrg 4/17/25
    virtual bool add_persistence(BESContainerStorage *cp);
    virtual bool ref_persistence(const std::string &persist_name);
    virtual bool deref_persistence(const std::string &persist_name);
    virtual BESContainerStorage *find_persistence(const std::string &persist_name);

    virtual bool is_nice(); // Implementation depends on its original purpose

    // These methods look through every BESContainerStorage object in the list
    // looking for sym_name and return/delete the first one found. jhrg 4//17/25
    virtual BESContainer *look_for(const std::string &sym_name);
    virtual void delete_container(const std::string &sym_name);
    // This method also operates on all the containers in all the Storages. jhrg 4/17/25
    virtual void show_containers(BESInfo &info);

    /**
     * @brief Dumps the state of the storage list to an output stream.
     * @param strm The output stream.
     */
    void dump(std::ostream &strm) const override;

    /**
     * @brief Gets the number of registered persistence stores.
     * @return The count of stores.
     */
    size_t get_store_count() const noexcept { // Can be noexcept if mutex lock doesn't throw
        std::lock_guard<std::recursive_mutex> lock(d_cache_lock_mutex);
        return d_storage_entries.size();
    }
};

#endif // I_BESContainerStorageList_H

