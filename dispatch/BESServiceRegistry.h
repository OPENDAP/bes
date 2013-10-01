// BESServiceRegistry.h

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

#ifndef I_BESServiceRegistry_h
#define I_BESServiceRegistry_h 1

#include <string>
#include <map>
#include <list>

using std::string ;
using std::map ;
using std::list ;

#include "BESObj.h"

class BESInfo ;

/** @brief The service registry allows modules to register services with the
 * BES that they provide.
 *
 * For example, the dap service provides the das, dds, ddx, and dods
 * commands as part of its service. Data handlers, such as that provided in
 * the nc_module for netcdf data, can register that it can provide the dap
 * services by calling the handle_services call.
 *
 * Also provided by this class is the response body for the
 * &lt;showServices &gt;/ command, returning the list of services, the
 * commands provided by that services, and the description of those commands
 */
class BESServiceRegistry : public BESObj
{
private:
    typedef struct _service_cmd
    {
	string _description ;
	map<string,string> _formats ;
    } service_cmd ;
    static BESServiceRegistry *		_instance ;
    map<string,map<string,service_cmd> >_services ;
    map<string,map<string,string> >	_handles ;
protected:
				BESServiceRegistry(void) ;
public:
    virtual			~BESServiceRegistry(void) ;

    virtual void		add_service( const string &name ) ;
    virtual void		add_to_service( const string &service,
						const string &cmd,
						const string &cmd_descript,
						const string &format ) ;
    virtual void		add_format( const string &service,
					    const string &cmd,
					    const string &format ) ;

    virtual void		remove_service( const string &name ) ;

    virtual bool		service_available( const string &name,
						   const string &cmd = "",
						   const string &format = "" ) ;

    virtual void		handles_service( const string &handler,
						 const string &service ) ;
    
    virtual bool		does_handle_service( const string &handler,
						     const string &service ) ;
    virtual void		services_handled( const string &handler,
						  list<string> &services ) ;

    virtual void		show_services( BESInfo &info ) ;

    virtual void		dump( ostream &strm ) const ;

    static BESServiceRegistry *	TheRegistry() ;
};

#endif // I_BESServiceRegistry_h

