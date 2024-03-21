// BESServiceRegistry.cc

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

#include <mutex>

#include "BESServiceRegistry.h"
#include "BESInfo.h"
#include "BESInternalError.h"

using std::endl;
using std::ostream;
using std::string;
using std::map;
using std::list;

BESServiceRegistry *BESServiceRegistry::d_instance = nullptr ;
static std::once_flag d_euc_init_once;

BESServiceRegistry::BESServiceRegistry() {}

BESServiceRegistry::~BESServiceRegistry() {}

/** @brief Add a service to the BES
 *
 * @param name name of the service to be added
 * @throws BESInternalError if the service already exists
 */
void
BESServiceRegistry::add_service( const string &name ) 
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    map<string,map<string,service_cmd> >::iterator i = _services.find( name ) ;
    if( i == _services.end() )
    {
	map<string,service_cmd> cmds ;
	_services[name] = cmds ;
    }
    else
    {
	string err = (string)"The service " + name
		     + " has already been registered" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
}

/** @brief This function allows callers to add to a service that already
 * exists
 *
 * For example, the dap-server modules add to the dap service the three
 * commands ascii, html_form, and info_page. These three responses use the
 * three basic commands of the dap service, das, dds, and datadds.
 *
 * @param service name of the service to add the commands to
 * @param cmd the command being added to the service
 * @param cmd_descript the description of the command being added to the service
 * @param format the format of the command being added to the service
 * @throws BESInternalError if the service does not exist or the
 * command is already reigstered with the service
 */
void
BESServiceRegistry::add_to_service( const string &service,
				    const string &cmd,
				    const string &cmd_descript,
				    const string &format )
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    map<string,map<string,service_cmd> >::iterator si ;
    si = _services.find( service ) ;
    if( si != _services.end() )
    {
	map<string,service_cmd>::const_iterator ci ;
	ci = (*si).second.find( cmd ) ;
	if( ci != (*si).second.end() )
	{
	    string err = (string)"Attempting to add command "
			 + (*ci).first + " to the service "
			 + service + ", command alrady exists" ;
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
	service_cmd sc ;
	sc._description = cmd_descript ;
	sc._formats[format] = format ;
	(*si).second[cmd] = sc ;
    }
    else
    {
	string err = (string)"Attempting to add commands to the service "
		     + service + " that has not yet been registered" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
}

/** @brief add a format response to a command of a service
 *
 * @param service name of the service to add the format to
 * @param cmd the command in the service to add the format to
 * @param format the format to add
 * @throws BESInternalError if the service or command do not exist or if
 * the format has already been registered.
 */
void
BESServiceRegistry::add_format( const string &service,
				const string &cmd,
				const string &format )
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    map<string,map<string,service_cmd> >::iterator si ;
    si = _services.find( service ) ;
    if( si != _services.end() )
    {
	map<string,service_cmd>::iterator ci = (*si).second.find( cmd ) ;
	if( ci != (*si).second.end() )
	{
	    map<string,string>::iterator fi ;
	    fi = (*ci).second._formats.find( format ) ;
	    if( fi == (*ci).second._formats.end() )
	    {
		(*ci).second._formats[format] = format ;
	    }
	    else
	    {
		string err = (string)"Attempting to add format "
			     + format + " to command " + cmd
			     + " for service " + service
			     + " where the format has already been registered" ;
		throw BESInternalError( err, __FILE__, __LINE__ ) ;
	    }
	}
	else
	{
	    string err = (string)"Attempting to add a format " + format
			 + " to command " + cmd + " for service " + service
			 + " where the command has not been registered" ;
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
    }
    else
    {
	string err = (string)"Attempting to add a format " + format
		     + " to command " + cmd + " for a service " + service
		     + " that has not been registered" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
}

/** @brief remove a service from the BES
 *
 * This function removes the specified service from the BES, meaning that
 * the service is no longer provided by the BES. It also removes the service
 * from any handlers that registered to handle the service
 *
 * @param service name of the service to remove
 */
void
BESServiceRegistry::remove_service( const string &service )
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    map<string,map<string,service_cmd> >::iterator i ;
    i = _services.find( service ) ;
    if( i != _services.end() )
    {
	// erase the service from the registry
	_services.erase( i ) ;

	// remove the service from the _handles list as well, so that if
	// asked, the handlers no longer handler the service because it no
	// longer exists.
	map<string,map<string,string> >::iterator hi = _handles.begin() ;
	map<string,map<string,string> >::iterator he = _handles.end() ;
	for( ; hi != he; hi++ )
	{
	    map<string,string>::iterator hsi = (*hi).second.find( service ) ;
	    if( hsi != (*hi).second.end() )
	    {
		(*hi).second.erase( hsi ) ;
	    }
	}
    }
}

/** @brief Determines if a service and, optionally, a command and a
 * return format, is available
 *
 * Given a service name and possibly a command name and return format,
 * determine if that service is available and, if not empty, if the service
 * provides the specified command.
 *
 * @param service the name of the service in question
 * @param cmd the service command in question, defaults to empty string
 * @param format the service return format in question, defaults to empty string
 * @return true if the service is available and, if provided, the command is
 * provided by the service and, if format is provided, the return format
 * of the service and command
 */
bool
BESServiceRegistry::service_available( const string &service,
				       const string &cmd,
				       const string &format )
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    bool isit = false ;
    map<string,map<string,service_cmd> >::iterator si ;
    si = _services.find( service ) ;
    if( si != _services.end() )
    {
	if( !cmd.empty() )
	{
	    map<string,service_cmd>::iterator ci = (*si).second.find( cmd ) ;
	    if( ci != (*si).second.end() )
	    {
		if( !format.empty() )
		{
		    map<string,string>::iterator fi ;
		    fi = (*ci).second._formats.find( format ) ;
		    if( fi != (*ci).second._formats.end() )
		    {
			isit = true ;
		    }
		}
		else
		{
		    isit = true ;
		}
	    }
	}
	else
	{
	    isit = true ;
	}
    }
    return isit ;
}

/** @brief The specified handler can handle the specified service
 *
 * This function lets the BES know that the specifie dhandler can handle the
 * services provided by service. For example, the netcdf_handler registeres
 * that it can handle the dap services, meaning that it can fill in the DAP
 * responses.
 *
 * @param handler name of the data handler being registered
 * @param service name of the service the handler now provides
 * @throws BESInternalError if the service does not exist
 */
void
BESServiceRegistry::handles_service( const string &handler,
				     const string &service )
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    map<string,map<string,service_cmd> >::iterator si ;
    si = _services.find( service ) ;
    if( si == _services.end() )
    {
	string err = (string)"Registering a handler to handle service "
		     + service + " that has not yet been registered" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    map<string,map<string,string> >::iterator hi = _handles.find( handler ) ;
    if( hi == _handles.end() )
    {
	map<string,string> services ;
	services[service] = service ;
	_handles[handler] = services ;
    }
    else
    {
	map<string,string>::iterator ci = (*hi).second.find( service ) ;
	if( ci == (*hi).second.end() )
	{
	    (*hi).second[service] = service ;
	}
    }
}

/** @brief Asks if the specified handler can handle the specified service
 *
 * Does the specified handler provide the specified service.
 *
 * @param handler name of the handler to check
 * @param service name of the service the handler might handle
 * @returns true if the handler does provide the service, false otherwise
 */
bool
BESServiceRegistry::does_handle_service( const string &handler,
					 const string &service )
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    bool handled = false ;
    map<string,map<string,string> >::iterator hi = _handles.find( handler ) ;
    if( hi != _handles.end() )
    {
	map<string,string>::iterator si = (*hi).second.find( service ) ;
	if( si != (*hi).second.end() )
	{
	    handled = true ;
	}
    }
    return handled ;
}

/** @brief returns the list of servies provided by the handler in question
 *
 * If the handler is not found, then the services list will be empty
 *
 * @param handler name of the handler to inquire about
 * @param services out parameter that will hold the services provided by the
 * handler
 */
void
BESServiceRegistry::services_handled( const string &handler,
				      list<string> &services )
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    map<string,map<string,string> >::iterator hi = _handles.find( handler ) ;
    if( hi != _handles.end() )
    {
	map<string,string>::const_iterator si = (*hi).second.begin() ;
	map<string,string>::const_iterator se = (*hi).second.end() ;
	for( ; si != se; si++ )
	{
	    services.push_back( (*si).second ) ;
	}
    }
}

/** @brief fills in the response object for the &lt;showService /&gt; request
 *
 * This function is called to fill in the response object for the
 * &lt;showServices /&gt; request. It adds each of the services and the
 * commands provided by that service.
 *
 * @param info The BES informational object that will hold the response
 */
void
BESServiceRegistry::show_services( BESInfo &info )
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    auto si = _services.begin() ;
    auto se = _services.end() ;
    for( ; si != se; si++ )
    {
	map<string, string, std::less<>> props ;
	props["name"] = (*si).first ;
	info.begin_tag( "serviceDescription", &props ) ;
        auto ci = (*si).second.begin() ;
        auto ce = (*si).second.end() ;
	for( ; ci != ce; ci++ )
	{
	    map<string, string, std::less<>> cprops ;
	    cprops["name"] = (*ci).first ;
	    info.begin_tag( "command", &cprops ) ;
	    info.add_tag( "description", (*ci).second._description ) ;
        auto fi = (*ci).second._formats.begin() ;
        auto fe = (*ci).second._formats.end() ;
	    for( ; fi != fe; fi++ )
	    {
		map<string, string, std::less<>> fprops ;
		fprops["name"] = (*fi).first ;
		info.add_tag( "format", "", &fprops ) ;
	    }
	    info.end_tag( "command" ) ;
	}
	info.end_tag( "serviceDescription" ) ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * the service registry 
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESServiceRegistry::dump( ostream &strm ) const
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    strm << BESIndent::LMarg << "BESServiceRegistry::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "registered services" << endl ;
    BESIndent::Indent() ;
    map<string,map<string,service_cmd> >::const_iterator si ;
    si = _services.begin() ;
    map<string,map<string,service_cmd> >::const_iterator se ;
    se = _services.end() ;
    for( ; si != se; si++ )
    {
	strm << BESIndent::LMarg << (*si).first << endl ;
	BESIndent::Indent() ;
	map<string,service_cmd>::const_iterator ci = (*si).second.begin() ;
	map<string,service_cmd>::const_iterator ce = (*si).second.end() ;
	for( ; ci != ce; ci++ )
	{
	    strm << BESIndent::LMarg << (*ci).first << endl ;
	    BESIndent::Indent() ;
	    strm << BESIndent::LMarg << "description: "
		 << (*ci).second._description << endl ;
	    strm << BESIndent::LMarg << "formats:" << endl ;
	    BESIndent::Indent() ;
	    map<string,string>::const_iterator fi ;
	    fi = (*ci).second._formats.begin() ;
	    map<string,string>::const_iterator fe ;
	    fe = (*ci).second._formats.end() ;
	    for( ; fi != fe; fi++ )
	    {
		strm << BESIndent::LMarg << (*fi).first << endl ;
	    }
	    BESIndent::UnIndent() ;
	    BESIndent::UnIndent() ;
	}
	BESIndent::UnIndent() ;
    }
    BESIndent::UnIndent() ;
    strm << BESIndent::LMarg << "services provided by handler" << endl ;
    BESIndent::Indent() ;
    map<string,map<string,string> >::const_iterator hi = _handles.begin() ;
    map<string,map<string,string> >::const_iterator he = _handles.end() ;
    for( ; hi != he; hi++ )
    {
	strm << BESIndent::LMarg << (*hi).first ;
	map<string,string>::const_iterator hsi = (*hi).second.begin() ;
	map<string,string>::const_iterator hse = (*hi).second.end() ;
	bool isfirst = true ;
	for( ; hsi != hse; hsi++ )
	{
	    if( !isfirst ) strm << ", " ;
	    else strm << ": " ;
	    strm << (*hsi).first ;
	    isfirst = false ;
	}
	strm << endl ;
    }
    BESIndent::UnIndent() ;
    BESIndent::UnIndent() ;
}

BESServiceRegistry *
BESServiceRegistry::TheRegistry()
{
    std::call_once(d_euc_init_once,BESServiceRegistry::initialize_instance);
    return d_instance;
}

void BESServiceRegistry::initialize_instance() {
    d_instance = new BESServiceRegistry;
#ifdef HAVE_ATEXIT
    atexit(delete_instance);
#endif
}

void BESServiceRegistry::delete_instance() {
    delete d_instance;
    d_instance = 0;
}

