// BESWWWModule.cc

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

#include <iostream>

using std::endl;

#include "BESWWWModule.h"
#include "BESDebug.h"

#include "BESWWWNames.h"
#include "BESDapNames.h"
#include "BESResponseNames.h"
#include "BESResponseHandlerList.h"
#include "BESWWWResponseHandler.h"

#include "BESWWWRequestHandler.h"
#include "BESRequestHandlerList.h"

#include "BESDapService.h"

#include "BESWWWTransmit.h"
#include "BESTransmitter.h"
#include "BESReturnManager.h"
#include "BESTransmitterNames.h"

#include "BESXMLWWWGetCommand.h"

void
 BESWWWModule::initialize(const string & modname)
{
    BESDEBUG( "www", "Initializing OPeNDAP WWW module " << modname << endl ) ;

    BESDEBUG( "www", "    adding " << modname << " request handler" << endl ) ;
    BESRequestHandler *handler = new BESWWWRequestHandler( modname ) ;
    BESRequestHandlerList::TheList()->add_handler( modname, handler ) ;

    BESDEBUG( "www", "    adding " << WWW_RESPONSE
		     << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler(WWW_RESPONSE,
                                                   BESWWWResponseHandler::
                                                   WWWResponseBuilder);

    BESDEBUG( "www", "Adding to dap services" << endl ) ;
    BESDapService::add_to_dap_service( WWW_SERVICE,
		       "OPeNDAP HTML Form for data constraints and access" ) ;

    BESTransmitter *t =
        BESReturnManager::TheManager()->find_transmitter( DAP2_FORMAT ) ;
    if( t )
    {
	BESDEBUG( "www", "    adding basic " << WWW_TRANSMITTER
			 << " transmit function" << endl ) ;
        t->add_method( WWW_TRANSMITTER, BESWWWTransmit::send_basic_form ) ;
    }

    BESDEBUG( "www", "    adding " << WWW_RESPONSE << " command" << endl) ;
    /* old-style string command
    BESCommand *cmd = new BESWWWGetCommand(WWW_RESPONSE);
    BESCommand::add_command(WWW_RESPONSE, cmd);
    */
    BESXMLCommand::add_command( WWW_RESPONSE,
				BESXMLWWWGetCommand::CommandBuilder ) ;

    BESDEBUG( "www", "Adding www context to BESDebug" << endl ) ;
    BESDebug::Register( "www" ) ;

    BESDEBUG( "www", "Done Initializing OPeNDAP WWW module "
		     << modname << endl ) ;
}

void BESWWWModule::terminate(const string & modname)
{
    BESDEBUG( "www", "Cleaning OPeNDAP WWW module " << modname << endl ) ;

    BESDEBUG( "www", "    removing " << modname <<
		     " request handler " << endl ) ;
    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler( modname ) ;
    if( rh ) delete rh ;

    BESDEBUG( "www", "    removing " << WWW_RESPONSE
		     << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->remove_handler(WWW_RESPONSE);

    BESDEBUG( "www", "    removing " << WWW_RESPONSE << " command" << endl ) ;
    BESXMLCommand::del_command( WWW_RESPONSE ) ;

    BESTransmitter *t =
        BESReturnManager::TheManager()->find_transmitter( DAP2_FORMAT ) ;
    if( t )
    {
	BESDEBUG( "www", "    removing basic " << WWW_TRANSMITTER
			 << " transmit function" << endl ) ;
        t->remove_method(WWW_TRANSMITTER);
    }

    t = BESReturnManager::TheManager()->find_transmitter( DAP2_FORMAT ) ;
    if( t )
    {
	BESDEBUG( "www", "    removing http " << WWW_TRANSMITTER
			 << " transmit function" << endl ) ;
        t->remove_method(WWW_TRANSMITTER);
    }

    BESDEBUG( "www", "Done Cleaning OPeNDAP WWW module " << modname << endl ) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESWWWModule::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "BESWWWModule::dump - ("
        << (void *) this << ")" << endl;
}

extern "C" BESAbstractModule *maker() {
    return new BESWWWModule;
}
