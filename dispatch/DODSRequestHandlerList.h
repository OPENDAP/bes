// DODSRequestHandlerList.h

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

#ifndef I_DODSRequestHandlerList_h
#define I_DODSRequestHandlerList_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

#include "DODSDataHandlerInterface.h"

class DODSRequestHandler ;

/** @brief maintains the list of registered request handlers for this server
 *
 * For a type of data to be handled by an OPeNDAP-g server the data type must
 * registered a request handler with the server. This request handler knows
 * how to fill in specific response objects, such as DAS, DDS, help, version,
 * etc... The request handlers are registered with this request handler list.
 */
class DODSRequestHandlerList {
private:
    static DODSRequestHandlerList *	_instance ;
    map< string, DODSRequestHandler * > _handler_list ;
protected:
				DODSRequestHandlerList(void) {}
public:
    virtual			~DODSRequestHandlerList(void) {}

    typedef map< string, DODSRequestHandler * >::const_iterator Handler_citer ;
    typedef map< string, DODSRequestHandler * >::iterator Handler_iter ;

    virtual bool		add_handler( string handler_name,
					 DODSRequestHandler * handler ) ;
    virtual DODSRequestHandler *remove_handler( string handler_name ) ;
    virtual DODSRequestHandler *find_handler( string handler_name ) ;

    virtual Handler_citer	get_first_handler() ;
    virtual Handler_citer	get_last_handler() ;

    virtual string		get_handler_names() ;

    virtual void		execute_each( DODSDataHandlerInterface &dhi ) ;
    virtual void		execute_all( DODSDataHandlerInterface &dhi ) ;
    virtual void		execute_once( DODSDataHandlerInterface &dhi ) ;

    static DODSRequestHandlerList *TheList() ;
};

#endif // I_DODSRequestHandlerList_h

// $Log: DODSRequestHandlerList.h,v $
// Revision 1.4  2005/03/15 20:00:14  pwest
// added execute_once so that a single function can execute the request using all the containers instead of executing a function for each container. This is for requests that are handled by the same request type, for example, all containers are of type nc
//
// Revision 1.3  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
