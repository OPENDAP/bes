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
 * @see BESContainerStorage
 */
class BESContainer: public BESObj {
private:
    string _symbolic_name;
    string _real_name;
    string _container_type;
    string _constraint;
    string _dap4_constraint;
    string _dap4_function;
    string _attributes;

protected:
    BESContainer()
    {
    }

    /** @brief construct a container with the given symbolic name, real name
     * and container type.
     *
     * @param sym_name symbolic name
     * @param real_name real name of the container, such as a file name
     * @param type type of data represented by this container, such as netcdf
     */
    BESContainer(const string &sym_name, const string &real_name, const string &type) :
            _symbolic_name(sym_name), _real_name(real_name), _container_type(type)
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
        _constraint = s;
    }

    /** @brief set the constraint for this container
     *
     * @param s constraint
     */
    void set_dap4_constraint(const string &s)
    {
        _dap4_constraint = s;
    }

    /** @brief set the constraint for this container
     *
     * @param s constraint
     */
    void set_dap4_function(const string &s)
    {
        _dap4_function = s;
    }

    /** @brief set the real name for this container, such as a file name
     * if reading a data file.
     *
     * @param real_name real name, such as the file name
     */
    void set_real_name(const string &real_name)
    {
        _real_name = real_name;
    }

    /** @brief set the type of data that this container represents, such
     * as cedar or netcdf.
     *
     * @param type type of data, such as cedar or netcdf
     */
    void set_container_type(const string &type)
    {
        _container_type = type;
    }

    /** @brief set desired attributes for this container
     *
     * @param attrs attributes desired to access for this container
     */
    void set_attributes(const string &attrs)
    {
        _attributes = attrs;
    }

    /** @brief retrieve the real name for this container, such as a
     * file name.
     *
     * @return real name, such as file name
     */
    string get_real_name() const
    {
        return _real_name;
    }

    /** @brief retrieve the constraint expression for this container
     *
     * @return constraint expression for this execution for the symbolic name
     */
    string get_constraint() const
    {
        return _constraint;
    }

    /** @brief retrieve the constraint expression for this container
     *
     * @return constraint expression for this execution for the symbolic name
     */
    string get_dap4_constraint() const
    {
        return _dap4_constraint;
    }

    /** @brief retrieve the constraint expression for this container
     *
     * @return constraint expression for this execution for the symbolic name
     */
    string get_dap4_function() const
    {
        return _dap4_function;
    }

    /** @brief retrieve the symbolic name for this container
     *
     * @return symbolic name for this container
     */
    string get_symbolic_name() const
    {
        return _symbolic_name;
    }

    /** @brief retrieve the type of data this container holds, such as
     * cedar or netcdf.
     *
     * @return type of data this container represents, such as cedar or
     * netcdf
     */
    string get_container_type() const
    {
        return _container_type;
    }

    /** @brief retrieve the attributes desired from this container
     *
     * @return attributes desired from this container
     */
    string get_attributes() const
    {
        return _attributes;
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
