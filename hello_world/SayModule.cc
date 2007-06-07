// SayModule.cc

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

#include <iostream>

using std::endl ;

#include "SayModule.h"
#include "BESRequestHandlerList.h"
#include "SayRequestHandler.h"
#include "BESLog.h"
#include "BESResponseHandlerList.h"
#include "BESResponseNames.h"
#include "BESCommand.h"
#include "SayResponseNames.h"
#include "SayResponseHandler.h"
#include "SayCommand.h"
#include "BESReporterList.h"
#include "SayReporter.h"
#include "BESDebug.h"

void
SayModule::initialize( const string &modname )
{
    BESDEBUG( "Initializing Say Handler:" << endl )

    BESDEBUG( "    adding " << modname << " request handler" << endl )
    BESRequestHandlerList::TheList()->add_handler( modname, new SayRequestHandler( modname ) ) ;

    BESDEBUG( "    adding " << say_RESPONSE << " response handler" << endl )
    BESResponseHandlerList::TheList()->add_handler( say_RESPONSE, SayResponseHandler::SayResponseBuilder ) ;

    BESDEBUG( "    adding " << say_RESPONSE << " command" << endl )
    BESCommand *cmd = new SayCommand( say_RESPONSE ) ;
    BESCommand::add_command( say_RESPONSE, cmd ) ;

    BESDEBUG( "    adding Say reporter" << endl )
    BESReporterList::TheList()->add_reporter( modname, new SayReporter ) ;



    // INIT_END
}

void
SayModule::terminate( const string &modname )
{
    BESDEBUG( "Removing Say Handlers" << endl )
    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler( modname ) ;
    if( rh ) delete rh ;

    BESDEBUG( "    removing " << say_RESPONSE << " response handler" << endl )
    BESResponseHandlerList::TheList()->remove_handler( say_RESPONSE ) ;

    BESDEBUG( "    removing " << say_RESPONSE << " command" << endl )
    BESCommand::del_command( say_RESPONSE ) ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new SayModule ;
    }
}

void
SayModule::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "SayModule::dump - ("
			     << (void *)this << ")" << endl ;
}

