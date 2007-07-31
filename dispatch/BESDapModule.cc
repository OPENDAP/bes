// BESDapModule.cc

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#include <iostream>

using std::endl ;

#include "BESDapModule.h"

#include "BESDapRequestHandler.h"
#include "BESRequestHandlerList.h"

#include "BESResponseNames.h"
#include "BESResponseHandlerList.h"

#include "BESDASResponseHandler.h"
#include "BESDDSResponseHandler.h"
#include "BESDataResponseHandler.h"
#include "BESDDXResponseHandler.h"

#include "BESCatalogResponseHandler.h"

#include "BESDapTransmit.h"
#include "BESTransmitter.h"
#include "BESReturnManager.h"
#include "BESTransmitterNames.h"

#include "BESDebug.h"
#include "BESException.h"
#include "BESExceptionManager.h"
#include "BESDapHandlerException.h"

void
BESDapModule::initialize( const string &modname )
{
    BESDEBUG( "dap", "Initializing DAP Modules:" << endl )

    BESDEBUG( "dap", "    adding " << modname << " request handler" << endl )
    BESRequestHandlerList::TheList()->add_handler( modname, new BESDapRequestHandler( modname ) ) ;

    BESDEBUG( "dap", "    adding " << DAS_RESPONSE << " response handler" << endl )
    BESResponseHandlerList::TheList()->add_handler( DAS_RESPONSE, BESDASResponseHandler::DASResponseBuilder ) ;

    BESDEBUG( "dap", "    adding " << DDS_RESPONSE << " response handler" << endl )
    BESResponseHandlerList::TheList()->add_handler( DDS_RESPONSE, BESDDSResponseHandler::DDSResponseBuilder ) ;

    BESDEBUG( "dap", "    adding " << DDX_RESPONSE << " response handler" << endl )
    BESResponseHandlerList::TheList()->add_handler( DDX_RESPONSE, BESDDXResponseHandler::DDXResponseBuilder ) ;

    BESDEBUG( "dap", "    adding " << DATA_RESPONSE << " response handler" << endl )
    BESResponseHandlerList::TheList()->add_handler( DATA_RESPONSE, BESDataResponseHandler::DataResponseBuilder ) ;

    BESDEBUG( "dap", "    adding " << CATALOG_RESPONSE << " response handler" << endl )
    BESResponseHandlerList::TheList()->add_handler( CATALOG_RESPONSE, BESCatalogResponseHandler::CatalogResponseBuilder ) ;

    BESDEBUG( "dap", "Initializing DAP Basic Transmitters:" << endl )
    BESTransmitter *t = BESReturnManager::TheManager()->find_transmitter( BASIC_TRANSMITTER ) ;
    if( t )
    {
	BESDEBUG( "dap", "    adding " << DAS_TRANSMITTER << endl )
	t->add_method( DAS_TRANSMITTER, BESDapTransmit::send_basic_das ) ;

	BESDEBUG( "dap", "    adding " << DDS_TRANSMITTER << endl )
	t->add_method( DDS_TRANSMITTER, BESDapTransmit::send_basic_dds ) ;

	BESDEBUG( "dap", "    adding " << DDX_TRANSMITTER << endl )
	t->add_method( DDX_TRANSMITTER, BESDapTransmit::send_basic_ddx ) ;

	BESDEBUG( "dap", "    adding " << DATA_TRANSMITTER << endl )
	t->add_method( DATA_TRANSMITTER, BESDapTransmit::send_basic_data ) ;
    }
    else
    {
	string err = (string)"Unable to initialize basic transmitter "
	             + "with dap transmit functions" ;
	throw BESException( err, __FILE__, __LINE__ ) ;
    }

    BESDEBUG( "dap", "Initializing DAP HTTP Transmitters:" << endl )
    t = BESReturnManager::TheManager()->find_transmitter( HTTP_TRANSMITTER ) ;
    if( t )
    {
	BESDEBUG( "dap", "    adding " << DAS_TRANSMITTER << endl )
	t->add_method( DAS_TRANSMITTER, BESDapTransmit::send_http_das ) ;

	BESDEBUG( "dap", "    adding " << DDS_TRANSMITTER << endl )
	t->add_method( DDS_TRANSMITTER, BESDapTransmit::send_http_dds ) ;

	BESDEBUG( "dap", "    adding " << DDX_TRANSMITTER << endl )
	t->add_method( DDX_TRANSMITTER, BESDapTransmit::send_http_ddx ) ;

	BESDEBUG( "dap", "    adding " << DATA_TRANSMITTER << endl )
	t->add_method( DATA_TRANSMITTER, BESDapTransmit::send_http_data ) ;
    }
    else
    {
	string err = (string)"Unable to initialize http transmitter "
	             + "with dap transmit functions" ;
	throw BESException( err, __FILE__, __LINE__ ) ;
    }

    BESDEBUG( "dap", "    adding dap exception handler" << endl )
    BESExceptionManager::TheEHM()->add_ehm_callback( BESDapHandlerException::handleException ) ;

    BESDEBUG( "dap", "    adding dap debug context" << endl )
    BESDebug::Register( "dap" ) ;

    BESDEBUG( "dap", "Done Initializing DAP Modules:" << endl )
}

void
BESDapModule::terminate( const string &modname )
{
    BESDEBUG( "dap", "Removing DAP Modules:" << endl )

    BESResponseHandlerList::TheList()->remove_handler( DAS_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( DDS_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( DDX_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( DATA_RESPONSE ) ;
    BESResponseHandlerList::TheList()->remove_handler( CATALOG_RESPONSE ) ;

    BESDEBUG( "dap", "Done Removing DAP Modules:" << endl )
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDapModule::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDapModule::dump - ("
			     << (void *)this << ")" << endl ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new BESDapModule ;
    }
}

