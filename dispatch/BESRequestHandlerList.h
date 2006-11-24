// BESRequestHandlerList.h

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

#ifndef I_BESRequestHandlerList_h
#define I_BESRequestHandlerList_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

#include "BESObj.h"
#include "BESDataHandlerInterface.h"

class BESRequestHandler ;

/** @brief maintains the list of registered request handlers for this server
 *
 * For a type of data to be handled by the BES the data type must
 * registered a request handler with the server. This request handler knows
 * how to fill in specific response objects, such as DAS, DDS, help, version,
 * etc... The request handlers are registered with this request handler list.
 */
class BESRequestHandlerList : public BESObj
{
private:
    static BESRequestHandlerList *	_instance ;
    map< string, BESRequestHandler * > _handler_list ;
protected:
				BESRequestHandlerList(void) {}
public:
    virtual			~BESRequestHandlerList(void) {}

    typedef map< string, BESRequestHandler * >::const_iterator Handler_citer ;
    typedef map< string, BESRequestHandler * >::iterator Handler_iter ;

    virtual bool		add_handler( string handler_name,
					 BESRequestHandler * handler ) ;
    virtual BESRequestHandler *remove_handler( string handler_name ) ;
    virtual BESRequestHandler *find_handler( string handler_name ) ;

    virtual Handler_citer	get_first_handler() ;
    virtual Handler_citer	get_last_handler() ;

    virtual string		get_handler_names() ;

    virtual void		execute_each( BESDataHandlerInterface &dhi ) ;
    virtual void		execute_all( BESDataHandlerInterface &dhi ) ;
    virtual void		execute_once( BESDataHandlerInterface &dhi ) ;

    virtual void		dump( ostream &strm ) const ;

    static BESRequestHandlerList *TheList() ;
};

#endif // I_BESRequestHandlerList_h

