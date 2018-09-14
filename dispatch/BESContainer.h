// BESContainer.h

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

#ifndef BESContainer_h_
#define BESContainer_h_ 1

#include <list>
#include <string>

using std::list;
using std::string;

#include "BESObj.h"

/** @brief A container is something that holds data. I.E. a netcdf file or a
 * database entry
 *
 * A symbolic name is a name that represents a set of data, such as
 * a file, and the type of data, such as cedar, netcdf, hdf, etc...
 * Associated with this symbolic name during run time is a constraint
 * expression used to constrain the data and attributes desired from
 * the container.
 *
 * The symbolic name is looked up in persistence, such as a MySQL database,
 * a file, or in volatile memory. The information retrieved from the persistent
 * source is saved in the BESContainer and is used to execute the request
 * from the client.
 *
 * @note Many data items used with the BES are files and are referenced
 * relative to a configured _Data Root Directory_. When a Container is
 * added to a store, if that store uses the BESContainerStorageVolatile
 * storage type, the 'real name' will be transformed to the full pathname
 * of the file on disk for the current BES. It's useful to have access to
 * the original relative pathname provided by the client/user so I've
 * added a field (d_relative_name) to hold that information. jhrg 5/22/18
 *
 * @see BESContainerStorage
 */
class BESContainer: public BESObj {
private:
    string d_symbolic_name;     ///< The name of the container
    string d_real_name;         ///< The full name of the thing (filename, database table name, ...)
    string d_relative_name;     ///< The name relative to the Data Root dir
    string d_container_type;    ///< The handler that can read this kind of data (e.g., HDF5)

    string d_constraint;
    string d_dap4_constraint;
    string d_dap4_function;

    string d_attributes;     ///< See DefinitionStorageList, XMLDefineCommand

protected:
    BESContainer()
    {
    }

    /** @brief construct a container with the given symbolic name, real name
     * and container type.
     *
     * @note The relative name filed is set to "", which is the best this code can
     * do, but not really very good. Child classes that know the data root should
     * fix the value.
     *
     * @param sym_name symbolic name
     * @param real_name real name of the container, such as a file name
     * @param type type of data represented by this container, such as netcdf
     */
    BESContainer(const string &sym_name, const string &real_name, const string &type) :
        d_symbolic_name(sym_name), d_real_name(real_name), d_relative_name(""), d_container_type(type),
        d_constraint(""), d_dap4_constraint(""), d_dap4_function(""), d_attributes("")
    {
    }

    BESContainer(const BESContainer &copy_from);

    void _duplicate(BESContainer &copy_to);

public:

    virtual ~BESContainer()
    {
    }

    /** @brief pure abstract method to duplicate this instances of BESContainer
     */
    virtual BESContainer * ptr_duplicate() = 0;

    /** @brief set the constraint for this container
     *
     * @param s constraint
     */
    void set_constraint(const string &s)
    {
        d_constraint = s;
    }

    /** @brief set the constraint for this container
     *
     * @param s constraint
     */
    void set_dap4_constraint(const string &s)
    {
        d_dap4_constraint = s;
    }

    /** @brief set the constraint for this container
     *
     * @param s constraint
     */
    void set_dap4_function(const string &s)
    {
        d_dap4_function = s;
    }

    /** @brief set the real name for this container, such as a file name
     * if reading a data file.
     *
     * @param real_name real name, such as the file name
     */
    void set_real_name(const string &real_name)
    {
        d_real_name = real_name;
    }

    /// @brief Set the relative name of the object in this container
    void set_relative_name(const std::string &relative) {
        d_relative_name = relative;
    }

    /** @brief set the type of data that this container represents, such
     * as cedar or netcdf.
     *
     * @param type type of data, such as cedar or netcdf
     */
    void set_container_type(const string &type)
    {
        d_container_type = type;
    }

    /** @brief set desired attributes for this container
     *
     * @param attrs attributes desired to access for this container
     */
    void set_attributes(const string &attrs)
    {
        d_attributes = attrs;
    }

    /** @brief retrieve the real name for this container, such as a
     * file name.
     *
     * @return real name, such as file name
     */
    string get_real_name() const
    {
        return d_real_name;
    }

    /// @brief Get the relative name of the object in this container
    std::string get_relative_name() const {
        return d_relative_name;
    }

    /** @brief retrieve the constraint expression for this container
     *
     * @return constraint expression for this execution for the symbolic name
     */
    string get_constraint() const
    {
        return d_constraint;
    }

    /** @brief retrieve the constraint expression for this container
     *
     * @return constraint expression for this execution for the symbolic name
     */
    string get_dap4_constraint() const
    {
        return d_dap4_constraint;
    }

    /** @brief retrieve the constraint expression for this container
     *
     * @return constraint expression for this execution for the symbolic name
     */
    string get_dap4_function() const
    {
        return d_dap4_function;
    }

    /** @brief retrieve the symbolic name for this container
     *
     * @return symbolic name for this container
     */
    string get_symbolic_name() const
    {
        return d_symbolic_name;
    }

    /** @brief retrieve the type of data this container holds, such as
     * cedar or netcdf.
     *
     * @return type of data this container represents, such as cedar or
     * netcdf
     */
    string get_container_type() const
    {
        return d_container_type;
    }


    /** @brief retrieve the attributes desired from this container
     *
     * @return attributes desired from this container
     */
    string get_attributes() const
    {
        return d_attributes;
    }

    /** @brief returns the true name of this container
     *
     * The true name of this container might be an uncompressed file name
     * from the compressed file name represented by the real name of this
     * container. This method would uncompress the real name and return the
     * uncompressed file name. Another example is where the real name
     * represents a WCS request. The access method would make the WCS
     * request and return the name of the resulting file.
     *
     * @return name of file to access
     */
    virtual string access() = 0;
    virtual bool release() = 0;

    virtual void dump(ostream &strm) const;
};

#endif // BESContainer_h_
