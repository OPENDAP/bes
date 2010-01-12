// BESDataHandlerInterface.cc

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

#include "BESDataHandlerInterface.h"
#include "BESContainer.h"
#include "BESResponseHandler.h"
#include "BESInfo.h"
#include "BESIndent.h"

/** @brief make a copy of only some data from specified object
 *
 * makes a copy of only some of the data members in a
 * BESDataHandlerInterface. The container list and response handler should
 * not be copied. Each BESDataHandlerInterface should represent a
 * request/response, so each one should have it's own response handler.
 *
 * @param copy_from object to copy informatioon from
 */
void
BESDataHandlerInterface::make_copy( const BESDataHandlerInterface &copy_from )
{
    this->data = copy_from.data ;
    this->output_stream = copy_from.output_stream ;
    this->transmit_protocol = copy_from.transmit_protocol ;
}

/** @brief clean up any information created within this data handler
 * interface
 *
 * It is the job of the BESDataHandlerInterface to clean up the response
 * handler
 *
 */
void
BESDataHandlerInterface::clean()
{
    if( response_handler )
    {
	delete response_handler ;
    }
    response_handler = 0 ;
}

/** @brief returns the response object using the response handler
 *
 * If the response handler is set for this request then return the
 * response object for the request using that response handler
 *
 * @return The response object for this request
 */
BESResponseObject *
BESDataHandlerInterface::get_response_object()
{
    BESResponseObject *response = 0 ;

    if( response_handler )
    {
	response = response_handler->get_response_object() ;
    }
    return response ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * each of the data members held
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDataHandlerInterface::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDataHandlerInterface::dump" << endl ;
    BESIndent::Indent() ;
    if( response_handler )
    {
	strm << BESIndent::LMarg << "response handler:" << endl ;
	BESIndent::Indent() ;
	response_handler->dump( strm ) ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "response handler: not set" << endl ;
    }

    if( container )
    {
	strm << BESIndent::LMarg << "current container:" << endl ;
	BESIndent::Indent() ;
	container->dump( strm ) ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "current container: not set" << endl ;
    }

    if( containers.size() )
    {
	strm << BESIndent::LMarg << "container list:" << endl ;
	BESIndent::Indent() ;
	list<BESContainer *>::const_iterator i = containers.begin() ;
	list<BESContainer *>::const_iterator ie = containers.end() ;
	for( ; i != ie; i++ )
	{
	    (*i)->dump( strm ) ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "container list: empty" << endl ;
    }

    strm << BESIndent::LMarg << "action: " << action << endl ;
    strm << BESIndent::LMarg << "action name: " << action_name << endl ;
    strm << BESIndent::LMarg << "transmit protocol: " << transmit_protocol << endl ;
    if( data.size() )
    {
	strm << BESIndent::LMarg << "data:" << endl ;
	BESIndent::Indent() ;
	data_citer i = data.begin() ;
	data_citer ie = data.end() ;
	for( ; i != ie; i++ )
	{
	    strm << BESIndent::LMarg << (*i).first << ": "
				     << (*i).second << endl ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "data: none" << endl ;
    }

    if( error_info )
    {
	strm << BESIndent::LMarg << "error info:" << endl ;
	BESIndent::Indent() ;
	error_info->dump( strm ) ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "error info: null" << endl ;
    }
    BESIndent::UnIndent() ;
}

