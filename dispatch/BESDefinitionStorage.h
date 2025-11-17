// BESDefinitionStorage.h

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

#ifndef BESDefinitionStorage_h_
#define BESDefinitionStorage_h_ 1

#include <string>

#include "BESObj.h"

class BESDefine;
class BESInfo;

/** @brief provides persistent storage for a specific view of different
 * containers including contraints and aggregation.
 *
 * An implementation of the abstract interface BESDefinitionStorage
 * provides storage for a definition, or view, of a set of data including
 * possibly constraints on each of those containers and possibly aggregation
 * of those containers.
 *
 * An instance of a derived implementation has a name associated with it, in
 * case that there are multiple ways in which the information can be stored.
 * For example, the main persistent storage for containers could be a mysql
 * database, but a user could store temporary information in different files.
 * If the user wishes to remove one of these persistence stores they would
 * request that a named BESDefinitionStorage object be removed from the
 * list.
 *
 * @see BESDefine
 * @see BESDefinitionStorageList
 */
class BESDefinitionStorage : public BESObj {
protected:
    std::string _my_name;

public:
    /** @brief create an instance of BESDefinitionStorage with the give
     * name.
     *
     * @param name name of this persistence store
     */
    BESDefinitionStorage(const std::string &name) : _my_name(name) {};

    ~BESDefinitionStorage() override {};

    /** @brief retrieve the name of this persistent store
     *
     * @return name of this persistent store.
     */
    virtual const std::string &get_name() const { return _my_name; }

    /** @brief looks for a definition in this persistent store with the
     * given name
     *
     * @param def_name name of the definition to look for
     * @return definition with the given name, NULL if not found
     */
    virtual BESDefine *look_for(const std::string &def_name) = 0;

    /** @brief adds a given definition to this storage
     *
     * This method adds a definition to the definition store, taking
     * ownership of that definition. If the definition already exists, then
     * the definition is NOT added.
     *
     * @param def_name name of the definition to add
     * @param d definition to add
     * @return true if successfully added, false if already exists
     */
    virtual bool add_definition(const std::string &def_name, BESDefine *d) = 0;

    /** @brief deletes a defintion with the given name
     *
     * This method deletes a definition from the definition store with the
     * given name.
     *
     * @param def_name name of the defintion to delete
     * @return true if successfully deleted and false otherwise
     */
    virtual bool del_definition(const std::string &def_name) = 0;

    /** @brief deletes all defintions from the definition store
     *
     * @return true if successfully deleted and false otherwise
     */
    virtual bool del_definitions() = 0;

    /** @brief show the defintions stored in this store
     *
     * Add information to the passed information response object about each
     * of the defintions stored within this defintion store. The information
     * added to the passed information objects includes the name of this
     * persistent store on the first line followed by the information for
     * each definition on the following lines.
     *
     * @param info information response object to store the information in
     */
    virtual void show_definitions(BESInfo &info) = 0;

    /** @brief Displays debug information about this object
     *
     * @param strm output stream to use to dump the contents of this object
     */
    void dump(std::ostream &strm) const override = 0;
};

#endif // BESDefinitionStorage_h_
