// BESObj.h

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

/** @brief top level BES object to house generic methods
 */

#ifndef A_BESObj_h
#define A_BESObj_h 1

#include <iostream>

#if 1
// This ripples through the code because it is included here. Some day this should be
// removed. jhrg 3/23/22
#include "BESIndent.h"
#endif

/** @brief Base object for bes objects
 *
 * A base object for any BES objects in bes to use. Provides simple
 * methods for dumping the contents of the object.
 */

class BESObj
{
public:
    /** @brief Default constructor
     *
     */
    virtual ~BESObj() = default;

    /** @brief dump the contents of this object to the specified ostream
     *
     * This method is implemented by all derived classes to dump their
     * contents, in other words, any state they might have, private variables,
     * etc...
     *
     * The inline function below can be used to dump the contents of an
     * OPeNDAPObj object. For example, the object Animal is derived from
     * BESObj. A user could do the following:
     *
     * Animal *a = new dog( "Sparky" ) ;
     * cout << a << endl ;
     *
     * And the dump method for dog could display the name passed into the
     * constructor, the (this) pointer of the object, etc...
     *
     * @param strm C++ i/o stream to dump the object to
     */
    virtual void dump( std::ostream &strm ) const = 0;
} ;

/** @brief dump the contents of the specified object to the specified ostream
 *
 * This inline method uses the dump method of the BESObj instance passed
 * to it. This allows a user to dump the contents of an object instead of just
 * getting the pointer value of the object.
 *
 * For example, the object Animal is derived from BESObj. A user could
 * do the following:
 *
 * Animal *a = new dog( "Sparky" ) ;
 * cout << a << endl ;
 *
 * And the dump method for dog could display the name passed into the
 * constructor, the (this) pointer of the object, etc...
 *
 * @param strm C++ i/o stream to dump the object to
 * @param obj The BESObj to dump
 */
inline std::ostream &
operator<<( std::ostream &strm, const BESObj &obj )
{
    obj.dump( strm ) ;
    return strm ;
}

#endif // A_BESObj_h

