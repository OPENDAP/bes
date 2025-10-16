// -*- mode: c++; c-basic-offset:4 -*-
//
// W10NModule.cc
//
// This file is part of BES w10n handler
//
// Copyright (c) 2015v OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include "config.h"

#include <iostream>

#include "BESDebug.h"
#include "BESResponseHandlerList.h"
#include "BESReturnManager.h"
#include "BESRequestHandler.h"
#include "BESRequestHandlerList.h"
#include "BESXMLCommand.h"

#include "W10NModule.h"
#include "W10NNames.h"
#include "W10nJsonTransmitter.h"
#include "W10nJsonRequestHandler.h"
#include "W10nShowPathInfoResponseHandler.h"
#include "W10nShowPathInfoCommand.h"
#include "w10n_utils.h"

#define RETURNAS_W10N "w10n"


void
W10NModule::initialize( const string &modname )
{
    BESDEBUG(W10N_DEBUG_KEY, "Initializing w10n Modules:" << endl ) ;

    BESRequestHandler *handler = new W10nJsonRequestHandler(modname);
    BESRequestHandlerList::TheList()->add_handler(modname, handler);



    BESDEBUG( W10N_DEBUG_KEY, "    adding " << W10N_SHOW_PATH_INFO_REQUEST << " command" << endl ) ;
    BESXMLCommand::add_command( W10N_SHOW_PATH_INFO_REQUEST, W10nShowPathInfoCommand::CommandBuilder ) ;

    BESDEBUG(W10N_DEBUG_KEY, "    adding " << W10N_SHOW_PATH_INFO_REQUEST_HANDLER_KEY << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( W10N_SHOW_PATH_INFO_REQUEST_HANDLER_KEY, W10nShowPathInfoResponseHandler::W10nShowPathInfoResponseBuilder ) ;

    BESDEBUG( W10N_DEBUG_KEY, "    adding " << RETURNAS_W10N << " transmitter" << endl );
    BESReturnManager::TheManager()->add_transmitter(RETURNAS_W10N, new W10nJsonTransmitter());


    BESDebug::Register(W10N_DEBUG_KEY);
    BESDEBUG(W10N_DEBUG_KEY, "Done Initializing w10n Modules." << endl ) ;
}

void
W10NModule::terminate( const string & /*modname*/ )
{
    BESDEBUG(W10N_DEBUG_KEY, "Removing w10n Modules:" << endl ) ;

    BESReturnManager::TheManager()->del_transmitter(RETURNAS_W10N);
   //  BESResponseHandlerList::TheList()->remove_handler( SHOW_PATH_INFO_RESPONSE ) ;

    BESDEBUG(W10N_DEBUG_KEY, "Done Removing w10n Modules." << endl ) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
W10NModule::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "W10NModule::dump - ("
			     << (void *)this << ")" << std::endl ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new W10NModule ;
    }
}

