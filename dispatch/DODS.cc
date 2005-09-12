// DODS.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <unistd.h>
#include <string>

using std::string ;

#include "DODS.h"

#include "TheDODSAuthenticator.h"
#include "DODSStatusReturn.h"
#include "TheDODSKeys.h"
#include "TheResponseHandlerList.h"
#include "DODSResponseHandler.h"
#include "TheAggFactory.h"
#include "DODSAggregationServer.h"
#include "cgi_util.h"
#include "TheReporterList.h"

// exceptions
#include "OPeNDAPDatabaseException.h"
#include "DODSParserException.h"
#include "DODSContainerPersistenceException.h"
#include "DODSKeysException.h"
#include "DODSLogException.h"
#include "DODSHandlerException.h"
#include "DODSIncorrectRequestException.h"
#include "DODSAuthenticateException.h"
#include "DODSResponseException.h"
#include "DODSAggregationException.h"
#include "Error.h"
#include "OPeNDAPDataNames.h"

#define DEFAULT_ADMINISTRATOR "support@unidata.ucar.edu"

DODS::DODS()
    : _transmitter( 0 )
{
}

DODS::~DODS()
{
}

/** @brief Executes the given request to generate a specified response object

    Execute the request by:
    1. initializing DODS
    2. authenticating the user
    3. validating the request, make sure all elements are present
    4. build the request plan (ie filling in the DODSDataHandlerInterface)
    5. execute the request plan using the DODSDataHandlerInterface
    6. transmit the resulting response object
    7. log the status of the execution
    8. notify the reporters of the request

    If an exception is thrown in any of these steps the exception is handed
    over to the exception manager in order to generate the proper response.

    @return status of the execution of the request, 0 if okay, !0 otherwise
    @see initialize
    @see authenticate
    @see validate_data_request
    @see build_data_request_plan
    @see execute_data_request_plan
    @see transmit_data
    @see log_status
    @see report_request
    @see exception_manager
 */
int
DODS::execute_request()
{
    int status = 0 ;

    try
    {
	initialize() ;
	authenticate() ;
	validate_data_request() ;
	build_data_request_plan() ;
	execute_data_request_plan() ;
	invoke_aggregation() ;
	transmit_data() ;
	log_status() ;
	report_request() ;
    }
    catch( DODSException &ex )
    {
	status = exception_manager( ex ) ;
    }
    catch( Error &e )
    {
	if( _dhi.transmit_protocol == "HTTP" ) set_mime_text( stdout, dods_error ) ;
	fprintf( stdout, "%s\n", e.error_message().c_str() ) ;
    }
    catch( bad_alloc &b )
    {
	if( _dhi.transmit_protocol == "HTTP" ) set_mime_text( stdout, dods_error ) ;
	fprintf( stdout, "dods server out of memory.\n" ) ;
    }
    catch(...)
    {
	if( _dhi.transmit_protocol == "HTTP" ) set_mime_text( stdout, dods_error ) ;
	fprintf( stdout, "An undefined dods exception ocurred.\n" ) ;
    }

    return status;
}

/** @brief Initialize the DODS object
 *
 *  This method must be called by all derived classes as it will initialize
 *  the environment
 */
void
DODS::initialize()
{
}

/** @brief Authenticate the user

    Authentication MUST be implemented in order to continue. If no
    authentication is needed then link in the test_authenticator object
    which will simply authenticate without checking any credentials.

    @see DODSAuthenticate
 */
void
DODS::authenticate()
{
    // if can't authenticate then throw an authentication exception
    if( !TheDODSAuthenticator )
    {
	string s = "No means to authenticate user, exiting" ;
	DODSAuthenticateException ae ;
	ae.set_error_description( s ) ;
	throw ae;
    }
    TheDODSAuthenticator->authenticate( _dhi ) ;
}

/** @brief Validate the incoming request information
 */
void
DODS::validate_data_request()
{
}

/** @brief Build the data request plan.

    It is the responsibility of the derived class to build the request plan.
    In other words, the container list must be filled in and the action set
    in the DODSDataHandlerInterface structure.

    @see _DODSDataHandlerInterface
 */
void
DODS::build_data_request_plan()
{
}

/** @brief Execute the data request plan

    Given the information in the DODSDataHandlerInterface, execute the
    request. To do this we simply find the response handler given the action
    in the DODSDataHandlerInterface and tell it to execute.

    If no DODSResponseHandler can be found given the action then an
    exception is thrown.

    @see _DODSDataHandlerInterface
    @see DODSResponseHandler
    @see DODSResponseObject
 */
void
DODS::execute_data_request_plan()
{
    DODSResponseHandler *rh = _dhi.response_handler ;
    if( rh )
    {
	rh->execute( _dhi ) ;
    }
    else
    {
	DODSHandlerException he ;
	string se = "The response handler \"" + _dhi.action
		    + "\" does not exist" ;
	he.set_error_description( se ) ;
	throw he;
    }
}

/** @brief Aggregate the resulting response object
 */
void
DODS::invoke_aggregation()
{
    if( _dhi.data[AGG_CMD] != "" )
    {
	DODSAggregationServer *agg =
	    TheAggFactory->find_handler( _dhi.data[AGG_HANDLER] ) ;
	if( agg )
	{
	    agg->aggregate( _dhi ) ;
	}
    }
}

/** @brief Transmit the resulting response object

    The derived classes are responsible for specifying a transmitter object
    for use in transmitting the response object. Again, the
    DODSResponseHandler knows how to transmit itself.

    If no response handler or no response object or no transmitter is
    specified then do nothing here.

    @see DODSResponseHandler
    @see DODSResponseObject
    @see DODSTransmitter
 */
void
DODS::transmit_data()
{
    if( _dhi.response_handler && _transmitter )
    {
	_dhi.response_handler->transmit( _transmitter, _dhi ) ;
    }
}

/** @brief Log the status of the request
 */
void
DODS::log_status()
{
}

/** @brief Report the request and status of the request to TheReporterList

    If interested in reporting the request and status of the request then
    one must register a DODSReporter with TheReporterList.

    If no DODSReporter objects are registered then nothing happens.

    @see TheReporterList
    @see DODSReporter
 */
void
DODS::report_request()
{
    if( TheReporterList ) TheReporterList->report( _dhi ) ;
}

/** @brief Clean up after the request
 */
void
DODS::clean()
{
    if( _dhi.response_handler ) delete _dhi.response_handler ;
    _dhi.response_handler = 0 ;
}

/** @brief Manage any exceptions thrown during the whole process

    Specific responses are generated given a specific Exception caught. If
    additional exceptions are thrown within derived systems then implement
    those in the derived exception_manager methods. This is a catch-all
    manager and should be called once derived methods have caught their
    exceptions.

    @param e DODSException to be managed
    @return status after exception is handled
    @see DODSException
    @see DODSIncorrectException
    @see DODSAuthenticateException
    @see OPeNDAPDatabaseException
    @see DODSMySQLQueryException
    @see DODSParserException
    @see DODSContainerPersistenceException
    @see DODSKeysException
    @see DODSLogException
    @see DODSHandlerException
    @see DODSResponseException
 */
int
DODS::exception_manager(DODSException &e)
{
    bool ishttp = false ;
    if( _dhi.transmit_protocol == "HTTP" )
	ishttp = true ;

    DODSIncorrectRequestException *ireqx=dynamic_cast<DODSIncorrectRequestException*>(&e);
    if (ireqx)
    {
	if( ishttp ) set_mime_text( stdout, dods_error ) ;
	bool found = false ;
	string administrator =
	    TheDODSKeys->get_key( "DODS.ServerAdministrator", found ) ;
	if( administrator=="" )
	    fprintf( stdout, "%s %s %s\n",
			     "DODS: internal server error please contact",
			     DEFAULT_ADMINISTRATOR,
			     "with the following message:" ) ;
	else
	    fprintf( stdout, "%s %s %s\n",
			     "DODS: internal server error please contact",
			     administrator.c_str(),
			     "with the following message:" ) ;
	fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
	return DODS_REQUEST_INCORRECT;
    }
    DODSAuthenticateException *ae = dynamic_cast<DODSAuthenticateException*>(&e);
    if(ae)
    {
	if( ishttp )
	{
	    set_mime_html( stdout, dods_error ) ;
	    fprintf( stdout, "<HTML>" ) ;
	    fprintf( stdout, "<HEAD></HEAD>" ) ;
	    fprintf( stdout, "<BODY BACKGROUND='http://cedarweb.hao.ucar.edu/images/Texture_lt_gray_004.jpg'>") ;
	    fprintf( stdout, "<TABLE BACKGROUND='http://cedarweb.hao.ucar.edu/images/Texture_lt_gray_004.jpg' BORDER='0' WIDTH='100%%' CELLPADDING='1' CELLSPACING='0'>" ) ;
	    fprintf( stdout, "<TR>" ) ;
	    fprintf( stdout, "<TD WIDTH='20%%' BACKGROUND='http://cedarweb.hao.ucar.edu/images/Texture_lt_gray_004.jpg'>" ) ;
	    fprintf( stdout, "<P ALIGN='center'>" ) ;
	    fprintf( stdout, "<A HREF='http://www.ucar.edu' TARGET='_blank'><IMG SRC='http://cedarweb.hao.ucar.edu/images/CedarwebUCAR.gif' ALT='UCAR' BORDER='0'><BR><FONT SIZE='2'>UCAR</FONT></A>" ) ;
	    fprintf( stdout, "</P>" ) ;
	    fprintf( stdout, "</TD>" ) ;
	    fprintf( stdout, "<TD WIDTH='80%%' BACKGROUND='http://cedarweb.hao.ucar.edu/images/Texture_lt_gray_004.jpg'>" ) ;
	    fprintf( stdout, "<P ALIGN='center'>" ) ;
	    fprintf( stdout, "<IMG BORDER='0' SRC='http://cedarweb.hao.ucar.edu/images/Cedarweb.jpg' ALT='CEDARweb'>" ) ;
	    fprintf( stdout, "</P>" ) ;
	    fprintf( stdout, "</TD>" ) ;
	    fprintf( stdout, "</TR>" ) ;
	    fprintf( stdout, "</TABLE>" ) ;
	    fprintf( stdout, "<BR />" ) ;
	    fprintf( stdout, "<BR />" ) ;
	    fprintf( stdout, "%s %s %s.\n",
	             "We were unable to authenticate your session",
		     "for user",
		     _dhi.data[USER_NAME].c_str() ) ;
	    fprintf( stdout, "<BR />\n" ) ;
	    fprintf( stdout, "<BR />\n" ) ;
	    fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
	    fprintf( stdout, "<BR />\n" ) ;
	    fprintf( stdout, "<BR />\n" ) ;
	    fprintf( stdout, "Please follow <A HREF=\"https://cedarweb.hao.ucar.edu:443/cgi-bin/ion-p?page=login.ion\" TARGET=\"NEW\">this link</A> to login.\n" ) ;
 	    fprintf( stdout, "Then refresh this page to get your data once you have logged in\n" ) ;
	    fprintf( stdout, "</BODY></HTML>" ) ;
	}
	else
	{
	    fprintf( stdout, "Reporting Authentication Exception.\n" ) ;
	    fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
	}
	return DODS_AUTHENTICATE_EXCEPTION;
    } 
    OPeNDAPDatabaseException *ce=dynamic_cast<OPeNDAPDatabaseException*>(&e);
    if(ce)
    {
	if( ishttp ) set_mime_text( stdout, dods_error ) ;
	fprintf( stdout, "Reporting Database Exception.\n" ) ;
	fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
	return OPENDAP_DATABASE_FAILURE;
    }
    DODSParserException *pe=dynamic_cast<DODSParserException*>(&e);
    if(pe)
    {
	if( ishttp ) set_mime_text( stdout, dods_error ) ;
	fprintf( stdout, "There is a parse error!\n" ) ;
	fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
	return DODS_PARSER_ERROR;
    }
    DODSContainerPersistenceException *dpe=dynamic_cast<DODSContainerPersistenceException*>(&e);
    if(dpe)
    {
	if( ishttp ) set_mime_text( stdout, dods_error ) ;
	fprintf( stdout, "Reporting persistence exception.\n" ) ;
	fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
	return DODS_CONTAINER_PERSISTENCE_ERROR;
    }  
    DODSKeysException *keysex=dynamic_cast<DODSKeysException*>(&e);
    if(keysex)
    {
	if( ishttp ) set_mime_text( stdout, dods_error ) ;
	fprintf( stdout, "Reporting keys exception.\n" ) ;
	fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
	return DODS_INITIALIZATION_FILE_PROBLEM;
    }  
    DODSLogException *logex=dynamic_cast<DODSLogException*>(&e);
    if(logex)
    {
	if( ishttp ) set_mime_text( stdout, dods_error ) ;
	fprintf( stdout, "Reporting log exception.\n" ) ;
	fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
	return DODS_LOG_FILE_PROBLEM;
    }
    DODSHandlerException *hanex=dynamic_cast <DODSHandlerException*>(&e);
    if(hanex)
    {
	if( ishttp ) set_mime_text( stdout, dods_error ) ;
	fprintf( stdout, "Reporting handler exception.\n" ) ;
	fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
	return DODS_DATA_HANDLER_FAILURE;
    }
    DODSResponseException *ranex=dynamic_cast <DODSResponseException*>(&e);
    if(ranex)
    {
	if( ishttp ) set_mime_text( stdout, dods_error ) ;
	fprintf( stdout, "Reporting response exception.\n" ) ;
	fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
	return DODS_DATA_HANDLER_FAILURE;
    }
    DODSAggregationException *aanex=dynamic_cast <DODSAggregationException*>(&e);
    if(aanex)
    {
	if( ishttp ) set_mime_text( stdout, dods_error ) ;
	fprintf( stdout, "Reporting response exception.\n" ) ;
	fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
	return DODS_AGGREGATION_EXCEPTION ;
    }
    if( ishttp ) set_mime_text( stdout, dods_error ) ;
    fprintf( stdout, "Reporting unknown exception.\n" ) ;
    bool found = false ;
    string administrator =
	TheDODSKeys->get_key( "DODS.ServerAdministrator", found ) ;
    fprintf( stdout, "Unmanaged DODS exception\n report to admin %s\n",
	     administrator.c_str() ) ;
    fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
    return DODS_TERMINATE_IMMEDIATE;
}

// $Log: DODS.cc,v $
// Revision 1.6  2005/04/19 17:55:41  pwest
// only set mime header if in http protocol
//
// Revision 1.5  2005/04/06 20:05:46  pwest
// added error description to message returned to user
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
