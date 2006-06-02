// DefinitionStorageList.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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

#ifndef I_DefinitionStorageList_H
#define I_DefinitionStorageList_H 1

#include <string>

using std::string ;

class DefinitionStorage ;
class DODSDefine ;
class DODSInfo ;

#define PERSISTENCE_VOLATILE "volatile"

/** @brief Provides a mechanism for accessing definitions from
 * different definition stores registered with this server.
 *
 * This class provides a mechanism for users to access definitions
 * from different definition stores, such as from a MySQL database, a file, or
 * volatile stores.
 *
 * Users can add different DefinitionStorage instances to this
 * persistent list. Then, when a user looks for a definition, that search
 * goes through the list of persistent stores in the order they were added
 * to this list.
 *
 * @see DefinitionStorage
 * @see DODSDefine
 * @see DefinitionStorageException
 */
class DefinitionStorageList
{
private:
    static DefinitionStorageList * _instance ;

    typedef struct _persistence_list
    {
	DefinitionStorage *_persistence_obj ;
	DefinitionStorageList::_persistence_list *_next ;
    } persistence_list ;

    DefinitionStorageList::persistence_list *_first ;
protected:
				DefinitionStorageList() ;
public:
    virtual			~DefinitionStorageList() ;

    virtual bool		add_persistence( DefinitionStorage *p ) ;
    virtual bool		del_persistence( const string &persist_name ) ;
    virtual DefinitionStorage *	find_persistence( const string &persist_name ) ;

    virtual DODSDefine *	look_for( const string &def_name ) ;

    virtual void		show_definitions( DODSInfo &info ) ;

    static DefinitionStorageList *TheList() ;
} ;

#endif // I_DefinitionStorageList_H

