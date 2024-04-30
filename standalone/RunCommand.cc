// RunCommand.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.
//
// Copyright (c) 2014 OPeNDAP, Inc.
//
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
//
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>
//      hyoklee     Hyo-Kyung Lee <hyoklee@hdfgroup.org>

#include "config.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "BESXMLInterface.h"
#include "BESStopWatch.h"
#include "BESError.h"
#include "BESDebug.h"

#include "RunCommand.h"

using namespace std;

/** @brief Runs a single command in the BES Framework
 *
 * The response is written to the output stream if one is specified,
 * otherwise the output is ignored.
 *
 * @param  cmd  The BES request that is sent to the BES server to handle.
 * @param repeat Number of times to repeat the command
 * @throws BESError Thrown if there is a problem sending the request
 *                      to the server or a problem receiving the response
 *                      from the server.
 * @see    BESError
 */
void RunCommand::run(const std::string &cmd, ostream &strm)
{
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start("RunCommand::executeCommand");

    BESXMLInterface interface(cmd, strm);

    int status = interface->execute_request("standalone");

    strm << flush;

    // Put the call to finish() here because we're not sending chunked responses back
    // to a client over PPT. In the BESServerHandler.cc code, we must do that and hence,
    // break up the call to finish() for the error and no-error cases.
    status = interface->finish(status);

    if (status != 0) {
        // an error has occurred.
        switch (status) {
            case BES_INTERNAL_FATAL_ERROR: {
                cerr << "Status not OK, dispatcher returned value " << status << '\n';
                exit(1);
            }

            case BES_INTERNAL_ERROR:
            case BES_SYNTAX_USER_ERROR:
            case BES_FORBIDDEN_ERROR:
            case BES_NOT_FOUND_ERROR: {
                cerr << "Status not OK, dispatcher returned value " << status << ", continuing\n";
                break;
            }

            default:
                break;
        }
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void RunCommand::dump(ostream & strm) const
{
	strm << BESIndent::LMarg << "RunCommand::dump - (" << (void *) this << ")" << endl;
	BESIndent::Indent();
	strm << BESIndent::LMarg << "stream: " << (void *) _strm << endl;
	strm << BESIndent::LMarg << "stream created? " << _strmCreated << endl;
	BESIndent::UnIndent();
}
