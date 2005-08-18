// DODSApache.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

using std::endl ;

#include "DODSApache.h"
#include "DODSMemoryManager.h"

#include "TheDODSLog.h"
#include "TheDODSKeys.h"
#include "DODSMemoryGlobalArea.h"
#include "DODSParser.h"
#include "DODSStatusReturn.h"
#include "DODSIncorrectRequestException.h"
#include "cgi_util.h"
#include "DODSBasicHttpTransmitter.h"
#include "TheDODSReturnManager.h"
#include "DODSTransmitException.h"
#include "TheAggFactory.h"
#include "DODSAggregationServer.h"
#include "OPeNDAPDataNames.h"

#define DEFAULT_ADMINISTRATOR "support@unidata.ucar.edu"

/** @brief Instantiate a DODSApache object

    @param dri DODSDataRequestInterface built using information from the
    apache module including the request and constraint passed as part of the
    URL.
    @see _DODSDataRequestInterface
 */
DODSApache::DODSApache( const DODSDataRequestInterface &dri )
    : OPeNDAPCmdInterface()
{
    _dri = &dri ;
}

DODSApache::~DODSApache()
{
    clean() ;
}

/** @brief Override execute_request in order to register memory pool

    Once the memory pool is initialized hand over control to parent class to
    execute the request. Once completed, unregister the memory pool.

    This needs to be done here instead of the initialization method
    because???

    @see DODSMemoryGlobalArea
 */
int
DODSApache::execute_request()
{
    DODSMemoryManager::register_global_pool() ; 

    int status = OPeNDAPCmdInterface::execute_request() ;

    if( !DODSMemoryManager::unregister_global_pool() )
	return DODS_TERMINATE_IMMEDIATE ;

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

/** @brief Initialize the DODS object from the apache environment

    First calls the parent initialization method in order to initialize all
    global variables.

    Once this is completed the DODSDataHandlerInterface is initialized given
    the DODSDataRequestInterface constructed within the module code.

    This includes retreiving the user information from the cookie created on
    the client side in the browser. The cookie name is defined in
    OPENDAP_USER_COOKIE above.

    Also creates the DODSBasicHttpTransmitter object in order to transmit
    the response object via http, setting the mime type and other header
    information for the response.

    @see DODSGlobalInit
 */
void
DODSApache::initialize()
{
    OPeNDAPCmdInterface::initialize() ;

    DODSMemoryManager::initialize_memory_pool() ;

    string https = _dri->server_protocol ;
    int http = https.find("HTTP");
    if( http < 0 )
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

    if( TheDODSLog && TheDODSLog->is_verbose() )
    {
	*(TheDODSLog) << "Data Request Interface:" << endl ;
	*(TheDODSLog) << "    server_name = " << _dri->server_name << endl ;
	*(TheDODSLog) << "    server_address = " << _dri->server_address << endl ;
	*(TheDODSLog) << "    server_protocol = " << _dri->server_protocol << endl ;
	*(TheDODSLog) << "    server_port = " << _dri->server_port << endl ;
	*(TheDODSLog) << "    script_name = " << _dri->script_name << endl ;
	*(TheDODSLog) << "    user_address = " << _dri->user_address << endl ;
	*(TheDODSLog) << "    user_agent = " << _dri->user_agent << endl ;
	*(TheDODSLog) << "    request = " << _dri->request << endl ;
	if( _dri->cookie )
	    *(TheDODSLog) << "    cookie = " << _dri->cookie << endl ;
	else
	    *(TheDODSLog) << "    cookie = no cookie set" << endl ;
    }
}

/** @brief Validate the information in the DODSDataRequestInterface
 */
void
DODSApache::validate_data_request()
{
    if (!_dri->server_name)
	throw DODSIncorrectRequestException("undefined server name");
    if(!_dri->server_address)
	throw DODSIncorrectRequestException("undefined server address");
    if(!_dri->server_protocol)
	throw DODSIncorrectRequestException("undefined server protocol");
    if(!_dri->server_port)
	throw DODSIncorrectRequestException("undefined server port");
    if(!_dri->script_name)
	throw DODSIncorrectRequestException("undefined script name");
    if(!_dri->user_address)
	throw DODSIncorrectRequestException("undefined user address");
    if(!_dri->user_agent)
	throw DODSIncorrectRequestException("undefined user agent");
    if(!_dri->request)
	throw DODSIncorrectRequestException("undefined request");
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

    @param e DODSException to be handled. If this method does not handle the
    exception then it is passed to the parent class exception_manager method
    to be handled.
    @see DODSException
 */
int
DODSApache::exception_manager(DODSException &e)
{
    bool ishttp = false ;
    if( _dhi.transmit_protocol == "HTTP" )
	ishttp = true ;

    DODSIncorrectRequestException *ireqx=dynamic_cast<DODSIncorrectRequestException*>(&e);
    if (ireqx)
    {
	if (e.get_error_description()=="undefined request")
	{
	    // Everything is OK but  DODSDataRequestInterface::request is null.
	    if( ishttp )
	    {
		welcome_browser();
	    }
	}
	else
	{
	    return OPeNDAPCmdInterface::exception_manager( e ) ;
	}
	return DODS_REQUEST_INCORRECT;
    }
    return OPeNDAPCmdInterface::exception_manager( e ) ;
}

void
DODSApache::welcome_browser()
{
    string who = _dri->user_address ;
    string agent = _dri->user_agent ;
    if( TheDODSLog )
	(*TheDODSLog) << "Incoming request from " << who.c_str() << " using " << agent.c_str() << endl;

    // see if request comes from the Netscape or the HotJava...
    int mo=agent.find("Mozilla");
    int ho=agent.find("HotJava");
    if ((mo<0)&&(ho<0)) // No, sorry. For you just a message and good bye :-(
    {
	set_mime_text( stdout, unknown_type ) ;
	bool found = false ;
	string administrator =
	    TheDODSKeys->get_key( "DODS.ServerAdministrator", found ) ;
	if(administrator=="")
	    fprintf( stdout, "%s %s %s\n",
			     "DODS: internal server error please contact",
			     DEFAULT_ADMINISTRATOR,
			     "with the following message:" ) ;
	else
	    fprintf( stdout, "%s %s %s\n",
			     "DODS: internal server error please contact",
			     administrator.c_str(),
			     "with the following message:" ) ;
	fprintf( stdout, "%s %s\n",
			 "DODS: can not interact with browser", agent.c_str() ) ;
    }
    else // Yes, _agent contains the signature of a browser               
    {
	bool found = false ;
	string method =
	    TheDODSKeys->get_key( "DODS.DefaultResponseMethod", found ) ;
	if( (method!="GET") && (method!="POST") )
	{
	    set_mime_text( stdout, dods_error ) ;
	    found = false ;
	    string administrator =
		TheDODSKeys->get_key( "DODS.ServerAdministrator", found ) ;
	    if(administrator=="")
		fprintf( stdout, "%s %s %s\n",
				 "DODS: internal server error please contact",
				 DEFAULT_ADMINISTRATOR,
				 "with the following message:" ) ;
	    else
		fprintf( stdout, "%s %s %s\n",
				 "DODS: internal server error please contact",
				 administrator.c_str(),
				 "with the following message:" ) ;
	    fprintf( stdout, "%s %s\n",
			     "DODS: fatal, can not get/understand the key",
			     "DODS.DefaultResponseMethod" ) ;
	}
	else
	{
	    //set_mime_text( stdout, unknown_type ) ;
	    fprintf( stdout, "HTTP/1.0 200 OK\n" ) ;
	    fprintf( stdout, "Content-type: text/html\n\n" ) ;
	    fflush( stdout ) ;

	    fprintf( stdout, "<HTML>\n" ) ;
	    fprintf( stdout, "<HEAD>\n" ) ;
	    fprintf( stdout, "<TITLE> Request to the DODS server</TITLE>\n" ) ;
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
	    fprintf( stdout, "<input type=\"submit\" value=\"Submit to DODS\">\n" ) ;
	    fprintf( stdout, "<input type=\"reset\" value=\"Clean Text Field\">\n" ) ;
	    fprintf( stdout, "</form>\n" ) ;
	    fprintf( stdout, "</body>\n" ) ;
	    fprintf( stdout, "</html>\n" ) ;
	}
    }
}

// $Log: DODSApache.cc,v $
// Revision 1.6  2005/03/26 00:33:33  pwest
// fixed aggregation server invoke problems. Other commands use the aggregation command but no aggregation is needed, so set aggregation to empty string when done
//
// Revision 1.5  2005/02/09 19:50:09  pwest
// was trying to print null pointer and looking for transmitter in return manager
//
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.3  2004/12/15 17:39:03  pwest
// Added doxygen comments
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
