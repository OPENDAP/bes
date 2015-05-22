// BESInterface.cc

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

#include "config.h"

#include <string>
#include <sstream>
#include <iostream>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

using std::string;
using std::ostringstream;
using std::bad_alloc;
using std::cout;

#include "BESInterface.h"

#include "TheBESKeys.h"
#include "BESResponseHandler.h"
#include "BESAggFactory.h"
#include "BESAggregationServer.h"
#include "BESReporterList.h"

#include "BESExceptionManager.h"

#include "BESDataNames.h"

#include "BESDebug.h"
#include "BESStopWatch.h"
#include "BESInternalError.h"
#include "BESInternalFatalError.h"

#include "BESLog.h"

list < p_bes_init > BESInterface::_init_list;
list < p_bes_end > BESInterface::_end_list;

BESInterface::BESInterface( ostream *output_stream )
    : _strm( output_stream ),
      _transmitter( 0 )
{
    if( !output_stream )
    {
	string err = "output stream must be set in order to output responses" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
}

BESInterface::~BESInterface()
{
}

/** @brief Executes the given request to generate a specified response object

    Execute the request by:
    1. initializing BES
    2. validating the request, make sure all elements are present
    3. build the request plan (ie filling in the BESDataHandlerInterface)
    4. execute the request plan using the BESDataHandlerInterface
    5. transmit the resulting response object
    6. log the status of the execution
    7. notify the reporters of the request
    8. end the request, which allows developers to add callbacks to notify
    them of the end of the request

    If an exception is thrown in any of these steps the exception is handed
    over to the exception manager in order to generate the proper response.
    Control is returned back to the calling method if an exception is thrown
    and it is the responsibility of the calling method to call finish_with_error
    in order to transmit the error message back to the client.

    @return status of the execution of the request, 0 if okay, !0 otherwise
    @see initialize()
    @see validate_data_request()
    @see build_data_request_plan()
    @see execute_data_request_plan()
    @see finish_no_error()
    @see finish_with_error()
    @see transmit_data()
    @see log_status()
    @see report_request()
    @see end_request()
    @see exception_manager()
 */
int
BESInterface::execute_request( const string &from )
{

	BESStopWatch sw;
	if (BESISDEBUG( TIMING_LOG ))
		sw.start("BESInterface::execute_request",_dhi->data[REQUEST_ID]);

	if( !_dhi )
	{
		string err = "DataHandlerInterface can not be null" ;
		throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
	_dhi->set_output_stream( _strm ) ;
	_dhi->data[REQUEST_FROM] = from ;

	pid_t thepid = getpid() ;
	ostringstream ss ;
	ss << thepid ;
	_dhi->data[SERVER_PID] = ss.str() ;

	int status = 0;

	// We split up the calls for the reason that if we catch an
	// exception during the initialization, building, execution, or response
	// transmit of the request then we can transmit the exception/error
	// information.
	try {
		initialize();

		*(BESLog::TheLog()) << _dhi->data[SERVER_PID]
										  << " from " << _dhi->data[REQUEST_FROM]
																	<< " request received" << endl ;

		validate_data_request();
		build_data_request_plan() ;
		execute_data_request_plan();
		/* These two functions are now being called inside
		 * execute_data_request_plan as they are really a part of executing
		 * the request and not separate.
	invoke_aggregation();
	transmit_data();
		 */
		_dhi->executed = true ;
	}
	catch( BESError & ex )
	{
		return exception_manager( ex ) ;
	}
	catch( bad_alloc & )
	{
		string serr = "BES out of memory" ;
		BESInternalFatalError ex( serr, __FILE__, __LINE__ ) ;
		return exception_manager( ex ) ;
	}
	catch(...) {
		string serr = "An undefined exception has been thrown" ;
		BESInternalError ex( serr, __FILE__, __LINE__ ) ;
		return exception_manager( ex ) ;
	}

	return finish( status ) ;
}

int
BESInterface::finish( int status )
{
    try
    {
	// if there was an error duriing initialization, validation,
	// execution or transmit of the response then we need to transmit
	// the error information. Once printed, delete the error
	// information since we are done with it.
	if( _dhi->error_info )
	{
	    transmit_data();
	    delete _dhi->error_info ;
	    _dhi->error_info = 0 ;
	}
    }
    catch( BESError &ex )
    {
        status = exception_manager( ex ) ;
    }
    catch( bad_alloc & )
    {
        string serr = "BES out of memory" ;
        BESInternalFatalError ex( serr, __FILE__, __LINE__ ) ;
        status = exception_manager( ex ) ;
    }
    catch(...)
    {
        string serr = "An undefined exception has been thrown" ;
        BESInternalError ex( serr, __FILE__, __LINE__ ) ;
        status = exception_manager( ex ) ;
    }

    // If there is error information then the transmit of the error failed,
    // print it to standard out. Once printed, delete the error
    // information since we are done with it.
    if( _dhi->error_info )
    {
        _dhi->error_info->print( cout ) ;
	delete _dhi->error_info ;
	_dhi->error_info = 0 ;
    }

    // if there is a problem with the rest of these steps then all we will
    // do is log it to the BES log file and not handle the exception with
    // the exception manager.
    try
    {
        log_status();
    }
    catch( BESError &ex )
    {
	(*BESLog::TheLog()) << "Problem logging status: " << ex.get_message()
	                    << endl ;
    }
    catch( ... )
    {
	(*BESLog::TheLog()) << "Unknown problem logging status" << endl ;
    }

    try
    {
        report_request();
    }
    catch( BESError &ex )
    {
	(*BESLog::TheLog()) << "Problem reporting request: " << ex.get_message()
	                    << endl ;
    }
    catch( ... )
    {
	(*BESLog::TheLog()) << "Unknown problem reporting request" << endl ;
    }

    try
    {
        end_request();
    }
    catch( BESError &ex )
    {
	(*BESLog::TheLog()) << "Problem ending request: " << ex.get_message()
	                    << endl ;
    }
    catch( ... )
    {
	(*BESLog::TheLog()) << "Unknown problem ending request" << endl ;
    }

    return status ;
}

int
BESInterface::finish_with_error( int status )
{
    if( !_dhi->error_info )
    {
	// there wasn't an error ... so now what?
	string serr = "Finish_with_error called with no error object" ;
	BESInternalError ex( serr, __FILE__, __LINE__ ) ;
	status = exception_manager( ex ) ;
    }

    return finish( status ) ;
}

void
BESInterface::add_init_callback(p_bes_init init)
{
    _init_list.push_back(init);
}

/** @brief Initialize the BES object
 *
 *  This method must be called by all derived classes as it will initialize
 *  the environment
 */
void
BESInterface::initialize()
{
	BESStopWatch sw;
	if (BESISDEBUG( TIMING_LOG ))
		sw.start("BESInterface::initialize",_dhi->data[REQUEST_ID]);

    BESDEBUG("bes", "Initializing request: " << _dhi->data[DATA_REQUEST] << " ... " << endl ) ;
    bool do_continue = true;
    init_iter i = _init_list.begin();

    for( ; i != _init_list.end() && do_continue == true; i++ )
    {
        p_bes_init p = *i ;
        do_continue = p( *_dhi ) ;
    }

    if( !do_continue )
    {
        BESDEBUG("bes", "FAILED" << endl) ;
        string se = "Initialization callback failed, exiting";
        throw BESInternalError( se, __FILE__, __LINE__ ) ;
    }
    else
    {
        BESDEBUG("bes", "OK" << endl) ;
    }
}

/** @brief Validate the incoming request information
 */
void
BESInterface::validate_data_request()
{
	//BESStopWatch sw;
	//if (BESISDEBUG( TIMING_LOG ))
    //		sw.start("BESInterface::validate_data_request");

}

/** @brief Execute the data request plan

    Given the information in the BESDataHandlerInterface, execute the
    request. To do this we simply find the response handler given the action
    in the BESDataHandlerInterface and tell it to execute.

    If no BESResponseHandler can be found given the action then an
    exception is thrown.

    @see BESDataHandlerInterface
    @see BESResponseHandler
    @see BESResponseObject
 */
void
BESInterface::execute_data_request_plan()
{
	BESStopWatch sw;
	if (BESISDEBUG( TIMING_LOG ))
		sw.start("BESInterface::execute_data_request_plan",_dhi->data[REQUEST_ID]);

	;

    BESDEBUG("bes", "Executing request: " << _dhi->data[DATA_REQUEST] << " ... " << endl ) ;
    BESResponseHandler *rh = _dhi->response_handler ;
    if( rh )
    {
        rh->execute( *_dhi ) ;
    }
    else
    {
        BESDEBUG("bes", "FAILED" << endl) ;
        string se = "The response handler \"" + _dhi->action
		    + "\" does not exist" ;
        throw BESInternalError( se, __FILE__, __LINE__ ) ;
    }
    BESDEBUG("bes", "OK" << endl) ;

    // Now we need to do the post processing piece of executing the request
    invoke_aggregation();

    // And finally, transmit the response of this request
    transmit_data();
}

/** @brief Aggregate the resulting response object
 */
void
BESInterface::invoke_aggregation()
{
	BESStopWatch sw;
	if (BESISDEBUG( TIMING_LOG ))
		sw.start("BESInterface::invoke_aggregation",_dhi->data[REQUEST_ID]);

    if( _dhi->data[AGG_CMD] != "" )
    {
        BESDEBUG("bes", "aggregating with: " << _dhi->data[AGG_CMD] << " ...  "<< endl ) ;
        BESAggregationServer *agg =
            BESAggFactory::TheFactory()->find_handler( _dhi->data[AGG_HANDLER] );
        if( agg )
	{
            agg->aggregate( *_dhi ) ;
        }
	else
	{
            BESDEBUG("bes", "FAILED" << endl) ;
            string se = "The aggregation handler " + _dhi->data[AGG_HANDLER]
                + "does not exist" ;
            throw BESInternalError( se, __FILE__, __LINE__ ) ;
        }
        BESDEBUG("bes", "OK" << endl) ;
    }
}

/** @brief Transmit the resulting response object

    The derived classes are responsible for specifying a transmitter object
    for use in transmitting the response object. Again, the
    BESResponseHandler knows how to transmit itself.

    If no response handler or no response object or no transmitter is
    specified then do nothing here.

    @see BESResponseHandler
    @see BESResponseObject
    @see BESTransmitter
 */
void
BESInterface::transmit_data()
{
	BESStopWatch sw;
	if (BESISDEBUG( TIMING_LOG ))
		sw.start("BESInterface::transmit_data",_dhi->data[REQUEST_ID]);

    BESDEBUG("bes", "Transmitting request: " << _dhi->data[DATA_REQUEST] << endl) ;
    if (_transmitter)
    {
        if( _dhi->error_info )
	{
	    ostringstream strm ;
	    _dhi->error_info->print( strm ) ;
	    (*BESLog::TheLog()) << strm.str() << endl ;
	    BESDEBUG( "bes", "  transmitting error info using transmitter ... "
			     << endl << strm.str() << endl ) ;
            _dhi->error_info->transmit( _transmitter, *_dhi ) ;
        }
	else if( _dhi->response_handler )
	{
	    BESDEBUG( "bes", "  transmitting response using transmitter ...  " << endl ) ;
            _dhi->response_handler->transmit( _transmitter, *_dhi ) ;
        }
    }
    else
    {
        if( _dhi->error_info )
	{
	    BESDEBUG( "bes", "  transmitting error info using cout ... " << endl ) ;
            _dhi->error_info->print( cout ) ;
        }
	else
	{
	    BESDEBUG( "bes", "  Unable to transmit the response ... FAILED " << endl ) ;
	    string err = "Unable to transmit the response, no transmitter" ;
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
    }
    BESDEBUG("bes", "OK" << endl) ;
}

/** @brief Log the status of the request
 */
void
BESInterface::log_status()
{
}

/** @brief Report the request and status of the request to
 * BESReporterList::TheList()

    If interested in reporting the request and status of the request then
    one must register a BESReporter with BESReporterList::TheList().

    If no BESReporter objects are registered then nothing happens.

    @see BESReporterList
    @see BESReporter
 */
void
BESInterface::report_request()
{
    BESDEBUG( "bes", "Reporting on request: " << _dhi->data[DATA_REQUEST]
		     << " ... " << endl ) ;

    BESReporterList::TheList()->report( *_dhi ) ;

    BESDEBUG( "bes", "OK" << endl ) ;
}

void
BESInterface::add_end_callback( p_bes_end end )
{
    _end_list.push_back( end ) ;
}

/** @brief End the BES request
 *
 *  This method allows developers to add callbacks at the end of a request,
 *  to do any cleanup or do any extra work at the end of a request
 */
void
BESInterface::end_request()
{
    BESDEBUG("bes", "Ending request: " << _dhi->data[DATA_REQUEST] << " ... " << endl ) ;
    end_iter i = _end_list.begin();
    for( ; i != _end_list.end(); i++ )
    {
        p_bes_end p = *i ;
        p( *_dhi ) ;
    }

    // now clean up any containers that were used in the request, release
    // the resource
    _dhi->first_container() ;
    while( _dhi->container )
    {
    	BESDEBUG("bes", "Calling BESContainer::release()" << endl);
	_dhi->container->release() ;
	_dhi->next_container() ;
    }

    BESDEBUG("bes", "OK" << endl) ;
}

/** @brief Clean up after the request
 */
void
BESInterface::clean()
{
    if( _dhi )
	_dhi->clean() ;
}

/** @brief Manage any exceptions thrown during the whole process

    Specific responses are generated given a specific Exception caught. If
    additional exceptions are thrown within derived systems then implement
    those in the derived exception_manager methods. This is a catch-all
    manager and should be called once derived methods have caught their
    exceptions.

    @param e BESError to be managed
    @return status after exception is handled
    @see BESError
 */
int
BESInterface::exception_manager( BESError &e )
{
    return BESExceptionManager::TheEHM()->handle_exception( e, *_dhi ) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * BESDataHandlerInterface, the BESTransmitter being used, and the number of
 * initialization and termimation callbacks.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESInterface::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "BESInterface::dump - ("
        << (void *) this << ")" << endl;
    BESIndent::Indent();

    if (_init_list.size()) {
        strm << BESIndent::LMarg << "termination functions:" << endl;
        BESIndent::Indent();
        init_iter i = _init_list.begin();
        for (; i != _init_list.end(); i++) {
	    // TODO ISO C++ forbids casting between pointer-to-function and pointer-to-object
	    // ...also below
            strm << BESIndent::LMarg << (void *) (*i) << endl;
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "termination functions: none" << endl;
    }

    if (_end_list.size()) {
        strm << BESIndent::LMarg << "termination functions:" << endl;
        BESIndent::Indent();
        end_iter i = _end_list.begin();
        for (; i != _end_list.end(); i++) {
            strm << BESIndent::LMarg << (void *) (*i) << endl;
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "termination functions: none" << endl;
    }

    strm << BESIndent::LMarg << "data handler interface:" << endl;
    BESIndent::Indent();
    _dhi->dump(strm);
    BESIndent::UnIndent();

    if (_transmitter) {
        strm << BESIndent::LMarg << "transmitter:" << endl;
        BESIndent::Indent();
        _transmitter->dump(strm);
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "transmitter: not set" << endl;
    }
    BESIndent::UnIndent();
}
