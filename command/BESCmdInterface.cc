// BESCmdInterface.cc

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
#include <sstream>

using std::endl ;
using std::stringstream ;

#include "BESCmdInterface.h"
#include "BESCmdParser.h"
#include "BESLog.h"
#include "BESDebug.h"
#include "BESBasicHttpTransmitter.h"
#include "BESReturnManager.h"
#include "BESSyntaxUserError.h"
#include "BESInternalError.h"
#include "BESAggFactory.h"
#include "BESAggregationServer.h"
#include "BESTransmitterNames.h"
#include "BESDataNames.h"

/** @brief Instantiate the BESCmdInterface object given the string command
 * and the output stream to use for the response object
 *
 * @param cmd BES string command
 * @param strm Ouput stream used to output the response object
 */
BESCmdInterface::BESCmdInterface( const string &cmd, ostream *strm )
    : BESBasicInterface( strm )
{
    _dhi = &_cmd_dhi ;
    _dhi->data[DATA_REQUEST] = cmd ;
}

BESCmdInterface::~BESCmdInterface()
{
    clean() ;
}

/** @brief Build the data request plan using the BESCmdParser.

    @see BESCmdParser
 */
void
BESCmdInterface::build_data_request_plan()
{
    BESDEBUG( "bes", "building request plan for: "<< _dhi->data[DATA_REQUEST]
		     << " ..." << endl )
    if( BESLog::TheLog()->is_verbose() )
    {
	*(BESLog::TheLog()) << _dhi->data[SERVER_PID]
			     << " from " << _dhi->data[REQUEST_FROM]
			     << " [" << _dhi->data[DATA_REQUEST] << "] building"
			     << endl ;
    }
    BESCmdParser::parse( _dhi->data[DATA_REQUEST], *_dhi ) ;
    BESDEBUG( "bes", " OK" << endl )

    BESBasicInterface::build_data_request_plan() ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESCmdInterface::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESCmdInterface::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESInterface::dump( strm ) ;
    BESIndent::UnIndent() ;


}

