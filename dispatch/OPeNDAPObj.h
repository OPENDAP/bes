// OPeNDAPObj.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org> and Jose Garcia <jgarcia@ucar.org>
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

/** @brief top level OPeNDAP object to house generic methods
 */

#ifndef A_OPeNDAPObj_h
#define A_OPeNDAPObj_h 1

#include <iostream>

using std::ostream ;

/** @brief Base object for bes objects
 *
 * A base object for any OPeNDAP objects in bes to use. Provides simple
 * methods for dumping the contents of the object.
 */

class OPeNDAPObj
{
public:
    /** @brief Default constructor
     *
     */
    virtual		~OPeNDAPObj() {}

    /** @brief dump the contents of this object to the specified ostream
     *
     * This method is implemented by all derived classes to dump their
     * contents, in other words, any state they might have, private variables,
     * etc...
     *
     * The inline function below can be used to dump the contents of an
     * OPeNDAOObj object. For example, the object Animal is derived from
     * OPeNDAPObj. A user could do the following:
     *
     * Animal *a = new dog( "Sparky" ) ;
     * cout << a << endl ;
     *
     * And the dump method for dog could display the name passed into the
     * constructor, the (this) pointer of the object, etc...
     *
     * @param strm C++ i/o stream to dump the object to
     */
    virtual void	dump( ostream &strm ) const = 0 ;
} ;

/** @brief dump the contents of the specified object to the specified ostream
 *
 * This inline method uses the dump method of the OPeNDAPObj instance passed
 * to it. This allows a user to dump the contents of an object instead of just
 * getting the pointer value of the object.
 *
 * For example, the object Animal is derived from OPeNDAPObj. A user could
 * do the following:
 *
 * Animal *a = new dog( "Sparky" ) ;
 * cout << a << endl ;
 *
 * And the dump method for dog could display the name passed into the
 * constructor, the (this) pointer of the object, etc...
 *
 * @param strm C++ i/o stream to dump the object to
 * @param obj The OPeNDAPObj to dump
 */
inline ostream &
operator<<( ostream &strm, const OPeNDAPObj &obj )
{
    obj.dump( strm ) ;
    return strm ;
}

#endif // A_OPeNDAPObj_h

// $Log: OPeNDAPObj.h,v $
