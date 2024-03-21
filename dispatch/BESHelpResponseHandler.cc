// BESHelpResponseHandler.cc

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
#include "BESHelpResponseHandler.h"
#include "BESInfoList.h"
#include "BESInfo.h"
#include "BESRequestHandlerList.h"
#include "BESRequestHandler.h"
#include "BESResponseNames.h"

using std::endl;
using std::ostream;
using std::string;
using std::map;

BESHelpResponseHandler::BESHelpResponseHandler( const string &name )
    : BESResponseHandler( name )
{
}

BESHelpResponseHandler::~BESHelpResponseHandler( )
{
}

/** @brief executes the command 'show help;' by returning general help
 * information as well as help information for all of the data request
 * handlers registered.
 *
 * The BESHelpResponseHandler first retreives general help information from help
 * files located in the file pointed to by either the key BES.Help.TXT if
 * the client is a basic text client or BES.Help.HTTP if the client is
 * HTML based. It then lists each of the data types registered to handle
 * requests (such as NetCDF, HDF, Cedar, etc...). Then for all data request
 * handlers registered with BESRequestHandlerList help information can be
 * added to the informational object.
 *
 * The response object BESHTMLInfo is created to store the help information.
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESHTMLInfo
 * @see BESRequestHandlerList
 */
void
BESHelpResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    BESInfo *info = BESInfoList::TheList()->build_info() ;
    d_response_object = info ;

    info->begin_response( HELP_RESPONSE_STR, dhi ) ;
    dhi.action_name = HELP_RESPONSE_STR ;

    map<string, string, std::less<>> attrs ;
    attrs["name"] = PACKAGE_NAME ;
    attrs["version"] = PACKAGE_VERSION ;
    info->begin_tag( "module", &attrs ) ;
    info->add_data_from_file( "BES.Help", "BES Help" ) ;
    info->end_tag( "module" ) ;

    // execute help for each registered request server
    info->add_break( 2 ) ;

    BESRequestHandlerList::TheList()->execute_all( dhi ) ;

    info->end_response() ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text or html, depending
 * on whether the client making the request can handle HTML information.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESHTMLInfo
 * @see BESTransmitter
 * @see BESDataHandlerInterface
 */
void
BESHelpResponseHandler::transmit( BESTransmitter *transmitter,
                               BESDataHandlerInterface &dhi )
{
    if( d_response_object )
    {
	BESInfo *info = dynamic_cast<BESInfo *>(d_response_object) ;
	if( !info )
	    throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
	info->transmit( transmitter, dhi ) ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESHelpResponseHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESHelpResponseHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESResponseHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESResponseHandler *
BESHelpResponseHandler::HelpResponseBuilder( const string &name )
{
    return new BESHelpResponseHandler( name ) ;
}

