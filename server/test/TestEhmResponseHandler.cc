// TestEhmResponseHandler.cc

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

#include "TestEhmResponseHandler.h"
#include "TestException.h"

TestEhmResponseHandler::TestEhmResponseHandler( string name )
    :BESResponseHandler( name )
{
}

TestEhmResponseHandler::~TestEhmResponseHandler( )
{
}

/** @brief executes the command 'test sig;'
 *
 * @param dhi structure that holds request and response information
 * @throws BESResponseException if there is a problem building the
 * response object
 * @see _BESDataHandlerInterface
 * @see BESRequestHandlerList
 */
void
TestEhmResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    throw TestException( "TestEhmResponseHandler::execute" ) ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESTransmitter
 * @see _BESDataHandlerInterface
 */
void
TestEhmResponseHandler::transmit( BESTransmitter *transmitter,
				  BESDataHandlerInterface &dhi )
{
}

BESResponseHandler *
TestEhmResponseHandler::TestEhmResponseBuilder( string handler_name )
{
    return new TestEhmResponseHandler( handler_name ) ;
}

