// DODSContainerPersistenceList.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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

#ifndef I_DODSContainerPersistenceList_H
#define I_DODSContainerPersistenceList_H 1

#include <string>

using std::string ;

class DODSContainerPersistence ;
class DODSContainer ;
class DODSInfo ;

#define PERSISTENCE_VOLATILE "volatile"

/** @brief Provides a mechanism for accessing container information from
 * different persistent stores.
 *
 * This class provides a mechanism for users to access container information
 * from different persistent stores, such as from a MySQL database, a file, or
 * in memory.
 *
 * Users can add different DODSContainerPersistence instances to this
 * persistent list. Then, when a user looks for a symbolic name, that search
 * goes through the list of persistent stores in order.
 *
 * If the symbolic name is found then it is the responsibility of the
 * DODSContainerPersistence instances to fill in the container information in
 * the specified DODSContainer object.
 *
 * If the symbolic name is not found then a flag is checked to determine
 * whether to simply log the fact that the symbolic name was not found, or to
 * throw an exception of type DODSContainerPersistenceException.
 *
 * @see DODSContainerPersistence
 * @see DODSContainer
 * @see DODSContainerPersistenceException
 */
class DODSContainerPersistenceList
{
private:
    static DODSContainerPersistenceList * _instance ;

    typedef struct _persistence_list
    {
	DODSContainerPersistence *_persistence_obj ;
	DODSContainerPersistenceList::_persistence_list *_next ;
    } persistence_list ;

    DODSContainerPersistenceList::persistence_list *_first ;

    bool			isnice() ;
protected:
				DODSContainerPersistenceList() ;
public:
    virtual			~DODSContainerPersistenceList() ;

    virtual bool		add_persistence( DODSContainerPersistence *p ) ;
    virtual bool		rem_persistence( const string &persist_name ) ;
    virtual DODSContainerPersistence *find_persistence( const string &persist_name ) ;

    virtual void		look_for( DODSContainer &d ) ;

    virtual void		show_containers( DODSInfo &info ) ;

    static DODSContainerPersistenceList *TheList() ;
} ;

#endif // I_DODSContainerPersistenceList_H

// $Log: DODSContainerPersistenceList.h,v $
// Revision 1.5  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.3  2004/12/15 17:36:01  pwest
//
// Changed the way in which the parser retrieves container information, going
// instead to ThePersistenceList, which goes through the list of container
// persistence instances it has.
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
