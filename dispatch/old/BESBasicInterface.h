// BESBasicInterface.h

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

#ifndef BESBasicInterface_h_
#define BESBasicInterface_h_ 1

#include <new>

#include "BESInterface.h"
#include "BESDataHandlerInterface.h"
#include "BESDataRequestInterface.h"

/** @brief Entry point into BES using string command requests

 The format of the request looks something like:

 <PRE>
 set container in catalog values c,nc/mplot.nc;
 define d as c;
 get das for d;
 </PRE>

 In this example a DAS object response is being requested. The DAS object
 is to be built from the definition 'd', where d is defined using the
 data container c. The data container c is created using the real file
 nc/mplot.nc

 BESBasicInterface uses BESParser to parse through the request string,
 building up a plan to be used during the execute method. Most
 implementations simply log information to the BESLog file before calling
 the parent class method.

 @see BESInterface
 @see BESParser
 */
class BESBasicInterface: public BESInterface {
protected:
    virtual void initialize();
    virtual void validate_data_request();
    virtual void build_data_request_plan();
    virtual void execute_data_request_plan();
    virtual void invoke_aggregation();
    virtual void transmit_data();
    virtual void log_status();
    virtual void clean();
public:
    BESBasicInterface(std::ostream *strm);
    virtual ~BESBasicInterface();

    virtual int execute_request(const std::string &from);

    virtual void dump(std::ostream &strm) const;
};

#endif // BESBasicInterface_h_

