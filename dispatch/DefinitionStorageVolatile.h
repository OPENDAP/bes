// DefinitionStorageVolatile.h

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

#ifndef DefinitionStorageVolatile_h_
#define DefinitionStorageVolatile_h_ 1

#include <string>
#include <map>

using std::string ;
using std::map ;

#include "DefinitionStorage.h"

class DODSDefine ;
class DODSInfo ;

/** @brief provides volatile storage for a specific definition/view of
 * different containers including contraints and aggregation.
 *
 * An implementation of the abstract interface DefinitionStorage
 * provides volatile storage for a definition, or view, of a set of data
 * including possibly constraints on each of those containers and possibly
 * aggregation of those containers.
 *
 * @see DODSDefine
 * @see DefinitionStorageList
 */
class DefinitionStorageVolatile : public DefinitionStorage
{
private:
    map< string, DODSDefine * > _def_list ;
    typedef map< string, DODSDefine * >::const_iterator Define_citer ;
    typedef map< string, DODSDefine * >::iterator Define_iter ;
public:
    /** @brief create an instance of DefinitionStorageVolatile with the give
     * name.
     *
     * @param name name of this persistence store
     */
    				DefinitionStorageVolatile( const string &name )
				    : DefinitionStorage( name ) {}

    virtual 			~DefinitionStorageVolatile() ;

    virtual DODSDefine * 	look_for( const string &def_name ) ;

    virtual bool		add_definition( const string &def_name,
                                                DODSDefine *d ) ;

    virtual bool		del_definition( const string &def_name ) ;
    virtual bool		del_definitions( ) ;

    virtual void		show_definitions( DODSInfo &info ) ;
};

#endif // DefinitionStorageVolatile_h_

