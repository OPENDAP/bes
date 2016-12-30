// BESXMLInterface.h

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

#ifndef BESXMLInterface_h_
#define BESXMLInterface_h_ 1

#include <new>
#include <vector>

using std::new_handler;
using std::bad_alloc;
using std::vector;

#include "BESInterface.h"
#include "BESXMLUtils.h"

class BESXMLCommand;

/** @brief Entry point into BES using xml document requests

 @see BESInterface
 */
// class BESXMLInterface: public BESBasicInterface {
class BESXMLInterface: public BESInterface {
private:
    vector<BESXMLCommand *> _cmd_list;
    BESDataHandlerInterface _base_dhi;
protected:
    virtual void initialize();
    virtual void validate_data_request();
    virtual void build_data_request_plan();
    virtual void execute_data_request_plan();
    virtual void invoke_aggregation();
    virtual void transmit_data();
    virtual void log_status();
    virtual void report_request();
    virtual void clean();
public:
    BESXMLInterface(const string &cmd, ostream *strm);
    virtual ~BESXMLInterface();

    virtual int execute_request(const string &from);

    virtual void dump(ostream &strm) const;
};

#endif // BESXMLInterface_h_

