// DAPCommandModule.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the DAP Data Access Protocol.

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

#include <iostream>

using std::endl ;
using std::cout ;

#include "DAPCommandModule.h"

#include "BESDapNames.h"
#include "BESDebug.h"

#include "BESCommand.h"

#include "BESCatalogCommand.h"

void
DAPCommandModule::initialize( const string &modname )
{
    BESDEBUG( "dap", "Initializing DAP Commands:" << endl )

    BESCommand *cmd = NULL ;

    BESDEBUG( "dap", "    adding " << DAS_RESPONSE << " command" << endl )
    BESCommand::add_command( DAS_RESPONSE, BESCommand::TermCommand ) ;

    BESDEBUG( "dap", "    adding " << DDS_RESPONSE << " command" << endl )
    BESCommand::add_command( DDS_RESPONSE, BESCommand::TermCommand ) ;

    BESDEBUG( "dap", "    adding " << DDX_RESPONSE << " command" << endl )
    BESCommand::add_command( DDX_RESPONSE, BESCommand::TermCommand ) ;

    BESDEBUG( "dap", "    adding " << DATA_RESPONSE << " command" << endl )
    BESCommand::add_command( DATA_RESPONSE, BESCommand::TermCommand ) ;

    BESDEBUG( "dap", "    adding " << CATALOG_RESPONSE << " command" << endl )
    cmd = new BESCatalogCommand( CATALOG_RESPONSE ) ;
    BESCommand::add_command( CATALOG_RESPONSE, cmd ) ;

    BESDEBUG( "dap", "    adding " << SHOW_INFO_RESPONSE << " command" << endl )
    cmd = new BESCatalogCommand( SHOW_INFO_RESPONSE ) ;
    BESCommand::add_command( SHOW_INFO_RESPONSE, cmd ) ;

    BESDEBUG( "dap", "Done Initializing DAP Commands:" << endl )
}

void
DAPCommandModule::terminate( const string &modname )
{
    BESDEBUG( "dap", "Removing DAP Commands" << endl )

    BESCommand::del_command( DAS_RESPONSE ) ;
    BESCommand::del_command( DDS_RESPONSE ) ;
    BESCommand::del_command( DDX_RESPONSE ) ;
    BESCommand::del_command( DATA_RESPONSE ) ;
    BESCommand::del_command( CATALOG_RESPONSE ) ;
    BESCommand::del_command( SHOW_INFO_RESPONSE ) ;

    BESDEBUG( "dap", "Done Removing DAP Commands" << endl )
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this class
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
DAPCommandModule::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "DAPCommandModule::dump - ("
			     << (void *)this << ")" << endl ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new DAPCommandModule ;
    }
}

