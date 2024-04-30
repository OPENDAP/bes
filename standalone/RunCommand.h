// RunCommand.h

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

#ifndef RunCommand_h
#define RunCommand_h 1

#include <iostream>
#include <string>
#include <fstream>

#include "BESObj.h"

/**
 * RunCommand is an object that handles the connection to, sending requests
 * to, and receiving response from a specified OpenDAP server running either
 * on this machine or another machine.
 * <p>
 * Requests to the OpenDAP server can be taken in different ways by the
 * RunCommand object.
 * <UL>
 * <LI>One request, ending with a semicolon.</LI>
 * <LI>Multiple requests, each ending with a semicolon.</LI>
 * <LI>Requests listed in a file, each request can span multiple lines in
 * the file and there can be more than one request per line. Each request
 * ends with a semicolon.</LI>
 * <LI>Interactive mode where the user inputs requests on the command line,
 * each ending with a semicolon, with multiple requests allowed per
 * line.</LI>
 * </UL>
 * <p>
 * Response from the requests can sent to any File or OutputStream as
 * specified by using the set_output methods. If no output is specified using
 * the set_output methods thent he output is ignored.
 *
 * Thread safety of this object has not yet been determined.
 *
 * @author Patrick West <A * HREF="mailto:pwest@hao.ucar.edu">pwest@hao.ucar.edu</A>
*/

class RunCommand : public BESObj {
private:
    std::ostream d_strm;

public:
    RunCommand() = default;
    ~RunCommand();
    RunCommand(const RunCommand &rhs) = delete;
    RunCommand &operator=(const RunCommand &rhs) = delete;
    RunCommand(RunCommand &&rhs) = delete;
    RunCommand &operator=(RunCommand &&rhs) = delete;


    void set_output(std::ostream &strm) { d_strm = strm; }

    [[nodiscard]] std::ostream &get_output() { return d_strm; }

    void run(const std::string &cmd);

    void dump(std::ostream &strm) const override;
};

#endif // RunCommand_h

