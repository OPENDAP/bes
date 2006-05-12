// DODS.cc

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

#include <unistd.h>
#include <string>

using std::string ;

#include "DODS.h"

#include "DODSStatusReturn.h"
#include "TheDODSKeys.h"
#include "DODSResponseHandler.h"
#include "OPeNDAPAggFactory.h"
#include "DODSAggregationServer.h"
#include "cgi_util.h"
#include "DODSReporterList.h"

#include "OPeNDAPDatabaseException.h"
#include "ContainerStorageException.h"
#include "DODSKeysException.h"
#include "DODSLogException.h"
#include "DODSHandlerException.h"
#include "DODSIncorrectRequestException.h"
#include "DODSResponseException.h"
#include "DODSAggregationException.h"
#include "Error.h"
#include "OPeNDAPDataNames.h"

#define DEFAULT_ADMINISTRATOR "support@unidata.ucar.edu"

list< p_opendap_init > DODS::_init_list ;
list< p_opendap_ehm > DODS::_ehm_list ;
list< p_opendap_end > DODS::_end_list ;

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
    2. validating the request, make sure all elements are present
    3. build the request plan (ie filling in the DODSDataHandlerInterface)
    4. execute the request plan using the DODSDataHandlerInterface
    5. transmit the resulting response object
    6. log the status of the execution
    7. notify the reporters of the request
    8. end the request, which allows developers to add callbacks to notify
    them of the end of the request

    If an exception is thrown in any of these steps the exception is handed
    over to the exception manager in order to generate the proper response.

    @return status of the execution of the request, 0 if okay, !0 otherwise
    @see initialize
    @see validate_data_request
    @see build_data_request_plan
    @see execute_data_request_plan
    @see transmit_data
    @see log_status
    @see report_request
    @see end_request
    @see exception_manager
 */
int
DODS::execute_request()
{
    int status = 0 ;

    try
    {
	initialize() ;
	validate_data_request() ;
	build_data_request_plan() ;
	execute_data_request_plan() ;
	invoke_aggregation() ;
	transmit_data() ;
	log_status() ;
	report_request() ;
	end_request() ;
    }
    catch( DODSException &ex )
    {
	status = exception_manager( ex ) ;
    }
    catch( Error &e )
    {
	if( _dhi.transmit_protocol == "HTTP" ) set_mime_text( stdout, dods_error ) ;
	fprintf( stdout, "%s\n", e.get_error_message().c_str() ) ;
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

void
DODS::add_init_callback( p_opendap_init init )
{
    _init_list.push_back( init ) ;
}

/** @brief Initialize the DODS object
 *
 *  This method must be called by all derived classes as it will initialize
 *  the environment
 */
void
DODS::initialize()
{
    bool do_continue = true ;
    init_iter i = _init_list.begin() ;
    for( i; i != _init_list.end() && do_continue == true; i++ )
    {
	p_opendap_init p = *i ;
	do_continue = p( _dhi ) ;
    }
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
	DODSAggregationServer *agg = OPeNDAPAggFactory::TheFactory()->find_handler( _dhi.data[AGG_HANDLER] ) ;
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

/** @brief Report the request and status of the request to
 * DODSReporterList::TheList()

    If interested in reporting the request and status of the request then
    one must register a DODSReporter with DODSReporterList::TheList().

    If no DODSReporter objects are registered then nothing happens.

    @see DODSReporterList
    @see DODSReporter
 */
void
DODS::report_request()
{
    DODSReporterList::TheList()->report( _dhi ) ;
}

void
DODS::add_end_callback( p_opendap_end end )
{
    _end_list.push_back( end ) ;
}

/** @brief End the OPeNDAP request
 *
 *  This method allows developers to add callbacks at the end of a request,
 *  to do any cleanup or do any extra work at the end of a request
 */
void
DODS::end_request()
{
    end_iter i = _end_list.begin() ;
    for( i; i != _end_list.end(); i++ )
    {
	p_opendap_end p = *i ;
	p( _dhi ) ;
    }
}

/** @brief Clean up after the request
 */
void
DODS::clean()
{
    if( _dhi.response_handler ) delete _dhi.response_handler ;
    _dhi.response_handler = 0 ;
}

void
DODS::add_ehm_callback( p_opendap_ehm ehm )
{
    _ehm_list.push_back( ehm ) ;
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
    @see OPeNDAPDatabaseException
    @see DODSMySQLQueryException
    @see ContainerStorageException
    @see DODSKeysException
    @see DODSLogException
    @see DODSHandlerException
    @see DODSResponseException
 */
int
DODS::exception_manager( DODSException &e )
{
    // Let's see if any of these exception callbacks can handle the
    // exception. The first callback that can handle the exception wins
    ehm_iter i = _ehm_list.begin() ;
    for( i; i != _ehm_list.end(); i++ )
    {
	p_opendap_ehm p = *i ;
	int handled = p( e, _dhi ) ;
	if( handled )
	{
	    return handled ;
	}
    }

    bool ishttp = false ;
    if( _dhi.transmit_protocol == "HTTP" )
	ishttp = true ;

    DODSIncorrectRequestException *ireqx=dynamic_cast<DODSIncorrectRequestException*>(&e);
    if (ireqx)
    {
	if( ishttp ) set_mime_text( stdout, dods_error ) ;
	bool found = false ;
	string administrator =
	    TheDODSKeys::TheKeys()->get_key( "OPeNDAP.ServerAdministrator", found ) ;
	if( administrator=="" )
	    fprintf( stdout, "%s %s %s\n",
			     "OPeNDAP: internal server error please contact",
			     DEFAULT_ADMINISTRATOR,
			     "with the following message:" ) ;
	else
	    fprintf( stdout, "%s %s %s\n",
			     "OPeNDAP: internal server error please contact",
			     administrator.c_str(),
			     "with the following message:" ) ;
	fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
	return DODS_REQUEST_INCORRECT;
    }
    OPeNDAPDatabaseException *ce=dynamic_cast<OPeNDAPDatabaseException*>(&e);
    if(ce)
    {
	if( ishttp ) set_mime_text( stdout, dods_error ) ;
	fprintf( stdout, "Reporting Database Exception.\n" ) ;
	fprintf( stdout, "%s\n", e.get_error_description().c_str() ) ;
	return OPENDAP_DATABASE_FAILURE;
    }
    ContainerStorageException *dpe=dynamic_cast<ContainerStorageException*>(&e);
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
	return DODS_DATA_HANDLER_PROBLEM;
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
	TheDODSKeys::TheKeys()->get_key( "OPeNDAP.ServerAdministrator", found ) ;
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
