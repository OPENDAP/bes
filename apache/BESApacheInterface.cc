// BESApacheInterface.cc

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
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <iostream>

using std::endl ;

#include "BESApacheInterface.h"
#include "BESMemoryManager.h"

#include "BESLog.h"
#include "TheBESKeys.h"
#include "BESMemoryGlobalArea.h"
#include "BESStatusReturn.h"
#include "BESIncorrectRequestException.h"
#include "cgi_util.h"
#include "BESBasicHttpTransmitter.h"
#include "BESTransmitException.h"
#include "BESAggregationServer.h"
#include "BESDataNames.h"

#define DEFAULT_ADMINISTRATOR "support@unidata.ucar.edu"

/** @brief Instantiate a BESApacheInterface object

    @param dri BESDataRequestInterface built using information from the
    apache module including the request and constraint passed as part of the
    URL.
    @see _BESDataRequestInterface
 */
BESApacheInterface::BESApacheInterface( const BESDataRequestInterface &dri )
    : BESCmdInterface()
{
    _dri = &dri ;
}

BESApacheInterface::~BESApacheInterface()
{
    clean() ;
}

/** @brief Override execute_request in order to register memory pool

    Once the memory pool is initialized hand over control to parent class to
    execute the request. Once completed, unregister the memory pool.

    This needs to be done here instead of the initialization method
    because???

    @see BESMemoryGlobalArea
 */
int
BESApacheInterface::execute_request()
{
    BESMemoryManager::register_global_pool() ; 

    int status = BESCmdInterface::execute_request() ;

    if( !BESMemoryManager::unregister_global_pool() )
	return BES_TERMINATE_IMMEDIATE ;

    return status;
}

/** @brief Find and set the user from the cookie set in the browser

    The cookie is of the format "name1=val1;name2=val2,...,namen=valn"

    Find the cookie with the name defined in OPENDAP_USER_COOKIE and get the
    value of that key.
 */
void
find_user_from_cookie( const char *cookie, string &user )
{
    if( cookie )
    {
	string s_cookie = cookie ;
	string var = "OpenDAP.remoteuser=" ;
	int user_var = s_cookie.find( var ) ;
	if( user_var >= 0 )
	{
	    string s_user_var = s_cookie.substr( user_var + var.length(),
	                                         s_cookie.length() ) ;
	    int semi = s_user_var.find( ";" ) ;
	    if( semi < 0 )
	    {
		user = s_user_var ;
	    }
	    else
	    {
		user = s_user_var.substr( 0, semi ) ;
	    }
	}
    }
}

/** @brief Initialize the BES object from the apache environment

    First calls the parent initialization method in order to initialize all
    global variables.

    Once this is completed the BESDataHandlerInterface is initialized given
    the BESDataRequestInterface constructed within the module code.

    This includes retreiving the user information from the cookie created on
    the client side in the browser. The cookie name is defined in
    OPENDAP_USER_COOKIE above.

    Also creates the BESBasicHttpTransmitter object in order to transmit
    the response object via http, setting the mime type and other header
    information for the response.

    @see BESGlobalInit
 */
void
BESApacheInterface::initialize()
{
    BESMemoryManager::initialize_memory_pool() ;

    string https = _dri->server_protocol ;
    std::string::size_type http = https.find("HTTP");
    if( http == string::npos )
    {
	_dhi.transmit_protocol = _dri->server_protocol ;
    }
    else
    {
	_dhi.transmit_protocol = "HTTP" ;
    }

    _dhi.data[USER_ADDRESS] = _dri->user_address ;
    _dhi.data[DATA_REQUEST] = _dri->request ;

    string user = "undef" ;
    if( _dri->cookie )
    {
	find_user_from_cookie( _dri->cookie, user ) ;
    }

    _dhi.data[USER_NAME] = user ;
    _dhi.data[USER_TOKEN] = _dri->token ;

    if( BESLog::TheLog() && BESLog::TheLog()->is_verbose() )
    {
	*(BESLog::TheLog()) << "Data Request Interface:" << endl ;
	*(BESLog::TheLog()) << "    server_name = " << _dri->server_name << endl ;
	*(BESLog::TheLog()) << "    server_address = " << _dri->server_address << endl ;
	*(BESLog::TheLog()) << "    server_protocol = " << _dri->server_protocol << endl ;
	*(BESLog::TheLog()) << "    server_port = " << _dri->server_port << endl ;
	*(BESLog::TheLog()) << "    script_name = " << _dri->script_name << endl ;
	*(BESLog::TheLog()) << "    user_address = " << _dri->user_address << endl ;
	*(BESLog::TheLog()) << "    user_agent = " << _dri->user_agent << endl ;
	*(BESLog::TheLog()) << "    request = " << _dri->request << endl ;
	if( _dri->cookie )
	    *(BESLog::TheLog()) << "    cookie = " << _dri->cookie << endl ;
	else
	    *(BESLog::TheLog()) << "    cookie = no cookie set" << endl ;
    }

    BESCmdInterface::initialize() ;
}

/** @brief Validate the information in the BESDataRequestInterface
 */
void
BESApacheInterface::validate_data_request()
{
    if (!_dri->server_name)
	throw BESIncorrectRequestException("undefined server name", __FILE__, __LINE__ );
    if(!_dri->server_address)
	throw BESIncorrectRequestException("undefined server address", __FILE__, __LINE__ );
    if(!_dri->server_protocol)
	throw BESIncorrectRequestException("undefined server protocol", __FILE__, __LINE__ );
    if(!_dri->server_port)
	throw BESIncorrectRequestException("undefined server port", __FILE__, __LINE__ );
    if(!_dri->script_name)
	throw BESIncorrectRequestException("undefined script name", __FILE__, __LINE__ );
    if(!_dri->user_address)
	throw BESIncorrectRequestException("undefined user address", __FILE__, __LINE__ );
    if(!_dri->user_agent)
	throw BESIncorrectRequestException("undefined user agent", __FILE__, __LINE__ );
    if(!_dri->request)
	throw BESIncorrectRequestException("undefined request", __FILE__, __LINE__ );
}

/** @brief Handle any exceptions generated from the request

    Captures if there is an error in the request format and builds a web
    page to allow the user to buid a request. If the request did not come
    from IE or Netscape then generate an error to the user. If the request
    contains a bad format, but a request is made, then generate an error
    message. Otherwise, if the request is empty, then generate a FORM to
    allow the user to enter a request string.

    All other exceptions are passed off to the parent exception manager to
    handle.

    @param e BESException to be handled. If this method does not handle the
    exception then it is passed to the parent class exception_manager method
    to be handled.
    @see BESException
 */
int
BESApacheInterface::exception_manager( BESException &e )
{
    bool ishttp = false ;
    if( _dhi.transmit_protocol == "HTTP" )
	ishttp = true ;

    BESIncorrectRequestException *ireqx=dynamic_cast<BESIncorrectRequestException*>(&e);
    if (ireqx)
    {
	if (e.get_message()=="undefined request")
	{
	    // Everything is OK but  BESDataRequestInterface::request is null.
	    if( ishttp )
	    {
		welcome_browser();
	    }
	}
	else
	{
	    return BESCmdInterface::exception_manager( e ) ;
	}
	return BES_REQUEST_INCORRECT;
    }
    return BESCmdInterface::exception_manager( e ) ;
}

void
BESApacheInterface::welcome_browser()
{
    string who = _dri->user_address ;
    string agent = _dri->user_agent ;
    if( BESLog::TheLog() )
	(*BESLog::TheLog()) << "Incoming request from " << who.c_str() << " using " << agent.c_str() << endl;

    // see if request comes from the Netscape or the HotJava...
    int mo=agent.find("Mozilla");
    int ho=agent.find("HotJava");
    if ((mo<0)&&(ho<0)) // No, sorry. For you just a message and good bye :-(
    {
	set_mime_text( stdout, unknown_type ) ;
	bool found = false ;
	string administrator =
	    TheBESKeys::TheKeys()->get_key( "BES.ServerAdministrator", found ) ;
	if(administrator=="")
	    fprintf( stdout, "%s %s %s\n",
			     "BES: internal server error please contact",
			     DEFAULT_ADMINISTRATOR,
			     "with the following message:" ) ;
	else
	    fprintf( stdout, "%s %s %s\n",
			     "BES: internal server error please contact",
			     administrator.c_str(),
			     "with the following message:" ) ;
	fprintf( stdout, "%s %s\n",
			 "BES: can not interact with browser", agent.c_str() ) ;
    }
    else // Yes, _agent contains the signature of a browser               
    {
	bool found = false ;
	string method =
	    TheBESKeys::TheKeys()->get_key( "BES.DefaultResponseMethod", found ) ;
	if( (method!="GET") && (method!="POST") )
	{
	    set_mime_text( stdout, dods_error ) ;
	    found = false ;
	    string administrator =
		TheBESKeys::TheKeys()->get_key( "BES.ServerAdministrator", found ) ;
	    if(administrator=="")
		fprintf( stdout, "%s %s %s\n",
				 "BES: internal server error please contact",
				 DEFAULT_ADMINISTRATOR,
				 "with the following message:" ) ;
	    else
		fprintf( stdout, "%s %s %s\n",
				 "BES: internal server error please contact",
				 administrator.c_str(),
				 "with the following message:" ) ;
	    fprintf( stdout, "%s %s\n",
			     "BES: fatal, can not get/understand the key",
			     "BES.DefaultResponseMethod" ) ;
	}
	else
	{
	    //set_mime_text( stdout, unknown_type ) ;
	    fprintf( stdout, "HTTP/1.0 200 OK\n" ) ;
	    fprintf( stdout, "Content-type: text/html\n\n" ) ;
	    fflush( stdout ) ;

	    fprintf( stdout, "<HTML>\n" ) ;
	    fprintf( stdout, "<HEAD>\n" ) ;
	    fprintf( stdout, "<TITLE> Request to the BES server</TITLE>\n" ) ;
	    fprintf( stdout, "<BODY>\n" ) ;
	    if (method=="GET")
		fprintf(stdout,"<form action=\"http://%s:%s%s\" method=get>\n",
			       _dri->server_name,
			       _dri->server_port,
			       _dri->script_name ) ;
	    else if (method=="POST")
		fprintf(stdout,"<form action=\"http://%s:%s%s\" method=post>\n",
			       _dri->server_name,
			       _dri->server_port,
			       _dri->script_name ) ;

	    fprintf( stdout, "<p>Request: <br><textarea name=\"request\" cols=85 rows=11 size=40,4 wrap=\"virtual\" ></textarea></p>\n" ) ;
	    fprintf( stdout, "<input type=\"submit\" value=\"Submit to BES\">\n" ) ;
	    fprintf( stdout, "<input type=\"reset\" value=\"Clean Text Field\">\n" ) ;
	    fprintf( stdout, "</form>\n" ) ;
	    fprintf( stdout, "</body>\n" ) ;
	    fprintf( stdout, "</html>\n" ) ;
	}
    }
}

