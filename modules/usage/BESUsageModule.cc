// BESUsageModule.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu>
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

#include "config.h"

#include <iostream>

#include "BESUsageModule.h"

#include "BESUsageNames.h"
#include "BESDapNames.h"
#include "BESResponseNames.h"
#include "BESResponseHandlerList.h"

#include "BESUsageRequestHandler.h"
#include "BESRequestHandlerList.h"

#include "BESUsageResponseHandler.h"

#include "BESDapService.h"

#include "BESUsageTransmit.h"
#include "BESTransmitter.h"
#include "BESReturnManager.h"
#include "BESTransmitterNames.h"

#include "BESDebug.h"


using std::endl ;
using std::ostream;
using std::string;


void
BESUsageModule::initialize( const string &modname )
{
    BESDEBUG( "usage", "Initializing OPeNDAP Usage module "
		       << modname << endl ) ;

    BESDEBUG( "usage", "    adding " << modname <<
		       " request handler" << endl ) ;
    BESRequestHandler *handler = new BESUsageRequestHandler( modname ) ;
    BESRequestHandlerList::TheList()->add_handler( modname, handler ) ;

    BESDEBUG( "usage", "    adding " << Usage_RESPONSE
		       << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( Usage_RESPONSE, BESUsageResponseHandler::UsageResponseBuilder ) ;

    BESDEBUG( "usage", "Adding to dap  services" << endl ) ;
    BESDapService::add_to_dap_service( Usage_SERVICE,
				       "OPeNDAP Data Information Page" ) ;

    BESTransmitter *t = BESReturnManager::TheManager()->find_transmitter( DAP_FORMAT ) ;
    if( t )
    {
	BESDEBUG( "usage", "    adding basic " << Usage_TRANSMITTER
			   << " transmitter" << endl ) ;
	t->add_method( Usage_TRANSMITTER, BESUsageTransmit::send_basic_usage ) ;
    }

    BESDEBUG( "usage", "    adding usage debug context" << endl ) ;
    BESDebug::Register( "usage" ) ;

    BESDEBUG( "usage", "Done Initializing OPeNDAP Usage module"
		       << modname << endl ) ;
}

void
BESUsageModule::terminate( const string &modname )
{
    BESDEBUG( "usage", "Cleaning OPeNDAP usage module " << modname << endl ) ;

    BESDEBUG( "usage", "    removing " << modname << " request handler "
		       << endl ) ;
    BESRequestHandler *rh =
	BESRequestHandlerList::TheList()->remove_handler( modname ) ;
    if( rh ) delete rh ;

    BESDEBUG( "usage", "    removing " << Usage_RESPONSE
		       << " response handler " << endl ) ;
    BESResponseHandlerList::TheList()->remove_handler( Usage_RESPONSE ) ;

    BESTransmitter *t =
	BESReturnManager::TheManager()->find_transmitter( DAP_FORMAT ) ;
    if( t )
    {
	BESDEBUG( "usage", "    removing basic " << Usage_TRANSMITTER
			   << " transmitter" << endl ) ;
	t->remove_method( Usage_TRANSMITTER ) ;
    }

    t = BESReturnManager::TheManager()->find_transmitter( DAP_FORMAT ) ;
    if( t )
    {
	BESDEBUG( "usage", "    removing http " << Usage_TRANSMITTER
			   << " transmitter" << endl ) ;
	t->remove_method( Usage_TRANSMITTER ) ;
    }

    BESDEBUG( "usage", "Done Cleaning OPeNDAP usage module "
		       << modname << endl ) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESUsageModule::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESUsageModule::dump - ("
			     << (void *)this << ")" << endl ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new BESUsageModule ;
    }
}

