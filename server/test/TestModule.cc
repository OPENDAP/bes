// TestModule.cc

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

#include <iostream>

using std::endl ;

#include "TestModule.h"
#include "TestNames.h"
#include "DODSResponseHandlerList.h"
#include "TestSigResponseHandler.h"
#include "TestEhmResponseHandler.h"
#include "TestCommand.h"
#include "DODSRequestHandlerList.h"
#include "TestRequestHandler.h"
#include "TestException.h"
#include "DODS.h"

#include "DODSLog.h"

void
TestModule::initialize()
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Initializing Test Module" << endl;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << "test" << " request handler" << endl ;
    DODSRequestHandlerList::TheList()->add_handler( "test", new TestRequestHandler( "test" ) ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << TEST_SIG << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( TEST_SIG, TestSigResponseHandler::TestSigResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << TEST_EHM << " response handler" << endl;
    DODSResponseHandlerList::TheList()->add_handler( TEST_EHM, TestEhmResponseHandler::TestEhmResponseBuilder ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << TEST_RESPONSE << " command" << endl;
    OPeNDAPCommand *cmd = new TestCommand( TEST_RESPONSE ) ;
    OPeNDAPCommand::add_command( TEST_RESPONSE, cmd ) ;

    string cmd_name = string( TEST_RESPONSE ) + "." + TEST_SIG ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( TEST_RESPONSE ) + "." + TEST_EHM ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding Test exception callback" << endl ;
    DODS::add_ehm_callback( TestException::handleException ) ;
}

void
TestModule::terminate()
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Cleaing up Test Module" << endl;

    DODSRequestHandler *rh =
	DODSRequestHandlerList::TheList()->remove_handler( "test" ) ;
    if( rh ) delete rh ;

    OPeNDAPCommand *cmd = OPeNDAPCommand::rem_command( TEST_RESPONSE ) ;
    if( cmd ) delete cmd ;

    DODSResponseHandlerList::TheList()->remove_handler( TEST_SIG ) ;
    DODSResponseHandlerList::TheList()->remove_handler( TEST_EHM ) ;
}

extern "C"
{
    OPeNDAPAbstractModule *maker()
    {
	return new TestModule ;
    }
}

