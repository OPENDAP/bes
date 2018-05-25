// TestModule.cc

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

#include <iostream>

using std::endl ;

#include "TestModule.h"
#include "TestNames.h"
#include "BESResponseHandlerList.h"
#include "TestSigResponseHandler.h"
#include "TestEhmResponseHandler.h"
#include "TestCommand.h"
#include "BESRequestHandlerList.h"
#include "TestRequestHandler.h"
#include "TestException.h"
#include "BESDapError.h"

#include "BESDebug.h"

void
TestModule::initialize( const string &modname )
{
    BESDEBUG( "test", "Initializing Test Module " << modname << endl ) ;

    BESDEBUG( "test", "    adding " << modname << " request handler" << endl ) ;
    BESRequestHandler *rqh = new TestRequestHandler( modname ) ;
    BESRequestHandlerList::TheList()->add_handler( modname, rqh ) ;

    BESDEBUG( "test", "    adding " << TEST_SIG << " response handler"
		      << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( TEST_SIG, TestSigResponseHandler::TestSigResponseBuilder ) ;

    BESDEBUG( "test", "    adding " << TEST_EHM << " response handler"
		      << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( TEST_EHM, TestEhmResponseHandler::TestEhmResponseBuilder ) ;

    BESDEBUG( "test", "    adding " << TEST_RESPONSE << " command" << endl ) ;
    BESXMLCommand::add_command( TEST_RESPONSE,
				TestCommand::CommandBuilder ) ;

    BESDEBUG( "test", "    adding Test exception callback" << endl ) ;
    BESDapError::TheDapHandler()->add_ehm_callback( TestException::handleException ) ;

    BESDEBUG( "test", "Done Initializing Test Module " << modname << endl ) ;
}

void
TestModule::terminate( const string &modname )
{
    BESDEBUG( "test", "Cleaing up Test Module " << modname << endl ) ;

    BESRequestHandler *rh =
	BESRequestHandlerList::TheList()->remove_handler( modname ) ;
    if( rh ) delete rh ;

    BESXMLCommand::del_command( TEST_RESPONSE ) ;

    BESResponseHandlerList::TheList()->remove_handler( TEST_SIG ) ;
    BESResponseHandlerList::TheList()->remove_handler( TEST_EHM ) ;

    BESDEBUG( "test", "Done Cleaing up Test Module " << modname << endl ) ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new TestModule ;
    }
}

