// ContainerStorageVolatile.h

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

#ifndef ContainerStorageVolatile_h_
#define ContainerStorageVolatile_h_ 1

#include <map>
#include <string>

using std::map ;
using std::string ;

#include "ContainerStorage.h"

/** @brief implementation of ContainerStorage that stores containers
 * for the duration of this process.
 *
 * This implementation of ContainerStorage stores volatile
 * containers for the duration of this process. A list of containers is
 * stored in the object. The look_for method simply looks for the specified
 * symbolic name in the list of containers and returns if a match is found.
 * Containers can be added to this instance as long as the symbolic name
 * doesn't already exist.
 *
 * @see ContainerStorage
 * @see DODSContainer
 */
class ContainerStorageVolatile : public ContainerStorage
{
private:
    map< string, DODSContainer * > _container_list ;
public:
    				ContainerStorageVolatile( const string &n ) ;
    virtual			~ContainerStorageVolatile() ;

    typedef map< string, DODSContainer * >::const_iterator Container_citer ;
    typedef map< string, DODSContainer * >::iterator Container_iter ;
    virtual void		look_for( DODSContainer &d ) ;
    virtual void		add_container( const string &s_name,
                                               const string &r_name,
					       const string &type ) ;
    virtual bool		rem_container( const string &s_name ) ;

    virtual void		show_containers( DODSInfo &info ) ;
};

#endif // ContainerStorageVolatile_h_

// $Log: ContainerStorageVolatile.h,v $
// Revision 1.4  2005/03/17 19:23:58  pwest
// deleting the container in rem_container instead of returning the removed container, returning true if successfully removed and false otherwise
//
// Revision 1.3  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.2  2005/02/02 00:03:13  pwest
// ability to replace containers and definitions
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
