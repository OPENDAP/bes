// OPENDAP_RESPONSEResponseHandler.cc

// Copyright (c) 2013 OPeNDAP, Inc. Author: James Gallagher
// <jgallagher@opendap.org>, Patrick West <pwest@opendap.org>
// Nathan Potter <npotter@opendap.org>
//                                                                            
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2.1 of
// the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 U\ SA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI.
// 02874-0112.
#include "OPENDAP_RESPONSEResponseHandler.h"
#include "BESInfoList.h"
#include "BESInfo.h"

OPENDAP_RESPONSEResponseHandler::OPENDAP_RESPONSEResponseHandler( const string &name )
    : BESResponseHandler( name )
{
}

OPENDAP_RESPONSEResponseHandler::~OPENDAP_RESPONSEResponseHandler( )
{
}

void
OPENDAP_RESPONSEResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    // This is an example. Here you would build the BESResponseObject
    // object and set it to the _response protected data member
    BESInfo *info = BESInfoList::TheList()->build_info() ;
    _response = info ;

    // Here is where your code would fill in the new response object
}

void
OPENDAP_RESPONSEResponseHandler::transmit( BESTransmitter *transmitter,
                               BESDataHandlerInterface &dhi )
{
    // Here is where you would transmit your response object using the
    // specified transmitter object. This is the example using the BESInfo
    // response object
    if( _response )
    {
	BESInfo *info = dynamic_cast<BESInfo *>( _response ) ;
	info->transmit( transmitter, dhi ) ;
    }
}

void
OPENDAP_RESPONSEResponseHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "OPENDAP_RESPONSEResponseHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESResponseHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESResponseHandler *
OPENDAP_RESPONSEResponseHandler::OPENDAP_RESPONSEResponseBuilder( const string &name )
{
    return new OPENDAP_RESPONSEResponseHandler( name ) ;
}

