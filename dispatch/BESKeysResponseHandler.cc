// BESKeysResponseHandler.cc

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

#include "BESKeysResponseHandler.h"
#include "TheBESKeys.h"
#include "BESInfo.h"

BESKeysResponseHandler::BESKeysResponseHandler( string name )
    : BESResponseHandler( name )
{
}

BESKeysResponseHandler::~BESKeysResponseHandler( )
{
}

/** @brief executes the command 'show keys;' by returning the list of
 * all key/value pairs defined in the OPeNDAP initialization file.
 *
 * This response handler knows how to retrieve the list of keys retrieved from
 * the OPeNDAP initialization file and stored in TheBESKeys. A BESInfo
 * informational response object is built to hold all of the key/value pairs.
 *
 * The information is returned, one key per line, like:
 *
 * key: "&lt;key_name&gt;", value: "&lt;key_value&gt"
 *
 * @param dhi structure that holds request and response information
 * @throws BESHandlerException if there is a problem building the
 * response object
 * @throws BESResponseException upon fatal error building the response
 * object
 * @see _BESDataHandlerInterface
 * @see BESInfo
 * @see TheBESKeys
 */
void
BESKeysResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    BESInfo *info = new BESInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;

    info->add_data( (string)"List of currently defined keys "
                    + " from the initialization file "
                    + TheBESKeys::TheKeys()->keys_file_name() + "\n" ) ;

    BESKeys::Keys_citer ki = TheBESKeys::TheKeys()->keys_begin() ;
    BESKeys::Keys_citer ke = TheBESKeys::TheKeys()->keys_end() ;
    for( ; ki != ke; ki++ )
    {
	string line = (string)"key: \"" + (*ki).first
	              + "\", value:\"" + (*ki).second
		      + "\"\n" ;
	info->add_data( line ) ;
    }
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text using the specified
 * transmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESInfo
 * @see BESTransmitter
 * @see _BESDataHandlerInterface
 */
void
BESKeysResponseHandler::transmit( BESTransmitter *transmitter,
                                  BESDataHandlerInterface &dhi )
{
    if( _response )
    {
	BESInfo *info = dynamic_cast<BESInfo *>(_response) ;
	transmitter->send_text( *info, dhi );
    }
}

BESResponseHandler *
BESKeysResponseHandler::KeysResponseBuilder( string handler_name )
{
    return new BESKeysResponseHandler( handler_name ) ;
}

