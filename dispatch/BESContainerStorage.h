// BESContainerStorage.h

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

#ifndef BESContainerStorage_h_
#define BESContainerStorage_h_ 1

#include <string>
#include <list>
#include <ostream>

#include "BESObj.h"

class BESContainer;
class BESInfo;

/** @brief provides persistent storage for data storage information
 * represented by a container.
 *
 * An implementation of the abstract interface BESContainerStorage
 * provides storage for information about accessing data of different data
 * types. The information is represented by a symbolic name. A user can
 * request a symbolic name that represents a certain container.
 *
 * For example, a symbolic name 'nc1' could represent the netcdf file
 * /usr/apache/htdocs/netcdf/datfile01.cdf.
 *
 * An instance of a derived implementation has a name associated with it, in
 * case that there are multiple ways in which the information can be stored.
 * For example, the main persistent storage for containers could be a mysql
 * database, but a user could store temporary information in different files.
 * If the user wishes to remove one of these persistence stores they would
 * request that a named BESContainerStorage object be removed from the
 * list.
 * 
 * @see BESContainer
 * @see BESContainerStorageList
 */
class BESContainerStorage: public BESObj {
protected:
    std::string _my_name;
    virtual void show_container(const std::string &sym_name, const std::string &real_name, const std::string &type, BESInfo &info);

public:
    /** @brief create an instance of BESContainerStorage with the given
     * name.
     *
     * @param name name of this persistence store
     */
    explicit BESContainerStorage(const std::string &name) : _my_name(name)
    {
    }

    ~BESContainerStorage() override = default;

    /** @brief retrieve the name of this persistent store
     *
     * @return name of this persistent store.
     */
    virtual const std::string & get_name() const
    {
        return _my_name;
    }

    /** @brief looks for a container in this persistent store
     *
     * This method looks for a container with the given symbolic name.
     *
     * @param sym_name The symbolic name of the container to look for
     * @return If sym_name is found, the BESContainer instance representing
     * that symbolic name, else NULL is returned.
     */
    virtual BESContainer * look_for(const std::string &sym_name) = 0;

    /** @brief adds a container with the provided information
     *
     * This method adds a container to the persistence store with the
     * specified information.
     *
     * @param sym_name The symbolic name for the container
     * @param real_name The real name for the container
     * @param type The type of data held by this container. This is
     * the handler that can be used to read the data
     * @deprecated
     */
    virtual void add_container(const std::string &sym_name, const std::string &real_name, const std::string &type) = 0;

    /**
     * @brief Add a container to the store
     *
     * @param c
     */
    virtual void add_container(BESContainer *c) = 0;

    /** @brief removes a container with the given symbolic name
     *
     * This method removes a container to the persistence store with the
     * given symbolic name. It deletes the container.
     *
     * @param s_name symbolic name for the container
     * @return true if successfully removed and false otherwise
     */
    virtual bool del_container(const std::string &s_name) = 0;

    /** @brief removes all container
     *
     * This method removes all containers from the persistent store. It does
     * not delete the real data behind the container.
     *
     * @return true if successfully removed and false otherwise
     */
    virtual bool del_containers() = 0;

    /** @brief determine if the given container is data and what services
     * are available for it
     *
     * @param inQuestion the container in question
     * @param provides an output parameter for storing the list of
     * services provided for this container
     */
    virtual bool isData(const std::string &inQuestion, std::list<std::string> &provides) = 0;

    /** @brief show the containers stored in this persistent store
     *
     * Add information to the passed information object about each of the
     * containers stored within this persistent store. The information
     * added to the passed information objects includes the name of this
     * persistent store on the first line followed by the symbolic name,
     * real name and data type for each container, one per line.
     *
     * @param info information object to store the information in
     */
    virtual void show_containers(BESInfo &info) = 0;

    /** @brief Displays debug information about this object
     *
     * @param strm output stream to use to dump the contents of this object
     */
    void dump(std::ostream &strm) const override = 0;
};

#endif // BESContainerStorage_h_
