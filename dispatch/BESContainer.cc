// BESContainer.cc

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

#include "BESContainer.h"

/** @brief make a copy of the passed container
 *
 * @param copy_from The container to copy
 */
BESContainer::BESContainer(const BESContainer &copy_from) :
    BESObj(copy_from), _symbolic_name(copy_from._symbolic_name), _real_name(copy_from._real_name),
    d_relative_name(copy_from.d_relative_name), _container_type(copy_from._container_type),
    _constraint(copy_from._constraint), _dap4_constraint(copy_from._dap4_constraint),
    _dap4_function(copy_from._dap4_function), _attributes(copy_from._attributes)
{
}

/** @brief duplicate this instance into the passed container
 *
 * @param copy_to The container to copy this instance into
 */
void BESContainer::_duplicate(BESContainer &copy_to)
{
    copy_to._symbolic_name = _symbolic_name;
    copy_to._real_name = _real_name;
    copy_to.d_relative_name = d_relative_name;
    copy_to._constraint = _constraint;
    copy_to._dap4_constraint = _dap4_constraint;
    copy_to._dap4_function = _dap4_function;
    copy_to._container_type = _container_type;
    copy_to._attributes = _attributes;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this container.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESContainer::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESContainer::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "symbolic name: " << _symbolic_name << endl;
    strm << BESIndent::LMarg << "real name: " << _real_name << endl;
    strm << BESIndent::LMarg << "relative name: " << d_relative_name << endl;
    strm << BESIndent::LMarg << "data type: " << _container_type << endl;
    strm << BESIndent::LMarg << "constraint: " << _constraint << endl;
    strm << BESIndent::LMarg << "dap4_constraint: " << _dap4_constraint << endl;
    strm << BESIndent::LMarg << "dap4_function: " << _dap4_function << endl;
    strm << BESIndent::LMarg << "attributes: " << _attributes << endl;
    BESIndent::UnIndent();
}

