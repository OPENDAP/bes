// BESCmdInterface.cc

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

#include <unistd.h>
#include <iostream>
#include <sstream>

using std::endl ;
using std::stringstream ;

#include "BESCmdInterface.h"
#include "BESCmdParser.h"
#include "BESInterface.h"
#include "BESLog.h"
#include "BESDebug.h"
#include "BESBasicHttpTransmitter.h"
#include "BESReturnManager.h"
#include "BESTransmitException.h"
#include "BESAggFactory.h"
#include "BESAggregationServer.h"
#include "BESTransmitterNames.h"
#include "BESDataNames.h"

/** @brief Instantiate a BESCmdInterface object

    @param dri BESDSDataRequestInterface built using information from the
    apache module including the request and constraint passed as part of the
    URL.
    @see _BESDSDataRequestInterface
 */
BESCmdInterface::BESCmdInterface()
    : BESInterface()
{
}

BESCmdInterface::BESCmdInterface( const string &cmd )
{
    _dhi.data[DATA_REQUEST] = cmd ;
}

BESCmdInterface::~BESCmdInterface()
{
    clean() ;
}

/** @brief Override execute_request in order to register memory pool

    Once the memory pool is initialized hand over control to parent class to
    execute the request. Once completed, unregister the memory pool.

    This needs to be done here instead of the initialization method
    because???
 */
int
BESCmdInterface::execute_request( const string &from )
{
    return BESInterface::execute_request( from ) ;
}

/** @brief Initialize the BES

    Determines what transmitter this BES will be using to transmit response
    objects and then calls the parent initialization method in order to
    initialize all global variables.

    @see BESTransmitter
    @see _BESDataHandlerInterface
 */
void
BESCmdInterface::initialize()
{
    // dhi has already been filled in at this point, so let's set a default
    // transmitter given the protocol. The transmitter might change after
    // parsing a request and given a return manager to use. This is done in
    // build_data_plan.
    //
    // The reason I moved this from the build_data_plan method is because a
    // registered initialization routine might throw an exception and we
    // will need to transmit the exception info, which needs a transmitter.
    // If an exception happens before this then the exception info is just
    // printed to stdout (see BESInterface::transmit_data()). -- pcw 09/05/06
    string protocol = _dhi.transmit_protocol ;
    if( protocol != "HTTP" )
    {
	BESDEBUG( "Finding " << BASIC_TRANSMITTER << " transmitter ... " )
	_transmitter = BESReturnManager::TheManager()->find_transmitter( BASIC_TRANSMITTER ) ;
	if( !_transmitter )
	{
	    string s = (string)"Unable to find transmitter "
		       + BASIC_TRANSMITTER ;
	    throw BESTransmitException( s, __FILE__, __LINE__ ) ;
	}
	BESDEBUG( "OK" << endl )
    }
    else
    {
	BESDEBUG( "Finding " << HTTP_TRANSMITTER << " transmitter ... " )
	_transmitter = BESReturnManager::TheManager()->find_transmitter( HTTP_TRANSMITTER ) ;
	if( !_transmitter )
	{
	    string s = (string)"Unable to find transmitter "
		       + HTTP_TRANSMITTER ;
	    throw BESTransmitException( s, __FILE__, __LINE__ ) ;
	}
	BESDEBUG( "OK" << endl )
    }

    BESInterface::initialize() ;
}

/** @brief Validate the incoming request information
 */
void
BESCmdInterface::validate_data_request()
{
    BESInterface::validate_data_request() ;
}

/** @brief Build the data request plan using the BESCmdParser.

    @see BESCmdParser
 */
void
BESCmdInterface::build_data_request_plan()
{
    BESDEBUG( "building request plan for: "<< _dhi.data[DATA_REQUEST] << " ...")
    if( BESLog::TheLog()->is_verbose() )
    {
	*(BESLog::TheLog()) << _dhi.data[SERVER_PID]
			     << " from " << _dhi.data[REQUEST_FROM]
			     << " [" << _dhi.data[DATA_REQUEST] << "] building"
			     << endl ;
    }
    BESCmdParser::parse( _dhi.data[DATA_REQUEST], _dhi ) ;
    BESDEBUG( " OK" << endl )

    // The default _transmitter (either basic or http depending on the
    // protocol passed) has been set in initialize. If the parsed command
    // sets a RETURN_CMD (a different transmitter) then look it up here. If
    // it's set but not found then this is an error. If it's not set then
    // just use the defaults.
    if( _dhi.data[RETURN_CMD] != "" )
    {
	BESDEBUG( "Finding transmitter: " << _dhi.data[RETURN_CMD] << " ...  " )
	_transmitter = BESReturnManager::TheManager()->find_transmitter( _dhi.data[RETURN_CMD] ) ;
	if( !_transmitter )
	{
	    string s = (string)"Unable to find transmitter "
	               + _dhi.data[RETURN_CMD] ;
	    throw BESTransmitException( s, __FILE__, __LINE__ ) ;
	}
	BESDEBUG( "OK" << endl )
    }

    if( BESDebug::Debug() ) _dhi.dump( *(BESDebug::Get_strm()) ) ;
}

/** @brief Execute the data request plan

    Simply calls the parent method. Prior to calling the parent method logs
    a message to the dods log file.

    @see BESLog
 */
void
BESCmdInterface::execute_data_request_plan()
{
    if( BESLog::TheLog()->is_verbose() )
    {
	*(BESLog::TheLog()) << _dhi.data[SERVER_PID]
			     << " from " << _dhi.data[REQUEST_FROM]
			     << " [" << _dhi.data[DATA_REQUEST] << "] executing"
			     << endl ;
    }
    BESInterface::execute_data_request_plan() ;
}

/** @brief Invoke the aggregation server, if there is one

    Simply calls the parent method. Prior to calling the parent method logs
    a message to the dods log file.

    @see BESLog
 */
void
BESCmdInterface::invoke_aggregation()
{
    if( _dhi.data[AGG_CMD] == "" )
    {
	if( BESLog::TheLog()->is_verbose() )
	{
	    *(BESLog::TheLog()) << _dhi.data[SERVER_PID]
				 << " from " << _dhi.data[REQUEST_FROM]
				 << " [" << _dhi.data[DATA_REQUEST] << "]"
				 << " not aggregating, command empty"
				 << endl ;
	}
    }
    else
    {
	BESAggregationServer *agg = BESAggFactory::TheFactory()->find_handler( _dhi.data[AGG_HANDLER] ) ;
	if( !agg )
	{
	    if( BESLog::TheLog()->is_verbose() )
	    {
		*(BESLog::TheLog()) << _dhi.data[SERVER_PID]
				     << " from " << _dhi.data[REQUEST_FROM]
				     << " [" << _dhi.data[DATA_REQUEST] << "]"
				     << " not aggregating, no handler"
				     << endl ;
	    }
	}
	else
	{
	    if( BESLog::TheLog()->is_verbose() )
	    {
		*(BESLog::TheLog()) << _dhi.data[SERVER_PID]
				     << " from " << _dhi.data[REQUEST_FROM]
				     << " [" << _dhi.data[DATA_REQUEST]
				     << "] aggregating" << endl ;
	    }
	    //This is done in BESInterface::invoke_aggregation
	    //agg->aggregate( _dhi ) ;
	}
    }
    BESInterface::invoke_aggregation() ;
}

/** @brief Transmit the response object

    Simply calls the parent method. Prior to calling the parent method logs
    a message to the dods log file.

    @see BESLog
 */
void
BESCmdInterface::transmit_data()
{
    if( BESLog::TheLog()->is_verbose() )
    {
	*(BESLog::TheLog()) << _dhi.data[SERVER_PID]
			     << " from " << _dhi.data[REQUEST_FROM]
			     << " [" << _dhi.data[DATA_REQUEST]
			     << "] transmitting" << endl ;
    }
    BESInterface::transmit_data() ;
} 

/** @brief Log the status of the request to the BESLog file

    @see BESLog
 */
void
BESCmdInterface::log_status()
{
    string result = "completed" ;
    if( _dhi.error_info )
	result = "failed" ;
    if( BESLog::TheLog()->is_verbose() )
    {
	*(BESLog::TheLog()) << _dhi.data[SERVER_PID]
			     << " from " << _dhi.data[REQUEST_FROM]
			     << " [" << _dhi.data[DATA_REQUEST] << "] "
			     << result << endl ;
    }
}

/** @brief Clean up after the request is completed

    Calls the parent method clean and then logs to the BESLog file saying
    that we are done and exiting the process. The exit actually takes place
    in the module code.

    @see BESLog
 */
void
BESCmdInterface::clean()
{
    BESInterface::clean() ;
    if( BESLog::TheLog()->is_verbose() )
    {
	*(BESLog::TheLog()) << _dhi.data[SERVER_PID]
			     << " from " << _dhi.data[REQUEST_FROM]
			     << " [" << _dhi.data[DATA_REQUEST] << "] cleaning"
			     << endl ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESCmdInterface::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESCmdInterface::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESInterface::dump( strm ) ;
    BESIndent::UnIndent() ;


}

