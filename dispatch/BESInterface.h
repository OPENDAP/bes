// BESInterface.h

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

#ifndef BESInterface_h_
#define BESInterface_h_ 1

#include <string>
#include <ostream>

#include "BESObj.h"

class BESError;
class BESTransmitter;
class BESDataHandlerInterface;

/** @brief Entry point into BES, building responses to given requests.

 BESInterface is an abstract class providing the entry point into the
 retrieval of information using the BES framework. There are eight
 steps to retrieving a response to a given request:

 <OL>
 <LI>initialize the BES environment</LI>
 <LI>validate the incoming information to make sure that all information
 is available to perform the query</LI>
 <LI>build the request plan to retrieve the information. A response can
 be generated for multiple files using multiple server types (cedar, cdf,
 netcdf, etc...)</LI>
 <LI>execute the request plan and build the response object</LI>
 <LI>transmit the response object</LI>
 <LI>log the status of the request</LI>
 <LI>send out report information that can be reported on by any number of
 reporters registered with the system</LI>
 <LI>end the request</LI>
 </OL>

 The way in which the response is generated is as follows.
 A BESResponseHandler is found that knows how to build the requested
 response object. The built-in response handlers are for the
 response objects das, dds, ddx, data, help, version. These response
 handlers are added to a response handler list during initialization.
 Additional response handlers can be added to this list. For example,
 in Cedar, response handlers are registered to build flat, tab, info,
 and stream responses.

 To build the response objects, a user can make many requests. For
 example, a das object can be built using as many different files as is
 requested, say for file1,file2,file3,file4. And each of these files
 could be of a different data type. For example, file1 and file3 could
 be cedar files, file2 could be a cdf file, and file4 could be a netcdf
 file.

 The structure that holds all the requested information is the
 BESDataHandlerInterface. It holds on to a list of containers, each of
 which has the data type (cedar, cdf, nph, etc...) and the file to be
 read. The BESDataHandlerInterface is built in the build request method.

 The response handlers know how to build the specified response object,
 such as DAS, DDS, help, status, version, etc...

 For each container in the BESDataHandlerInterface find the
 request handler (BESRequestHandler) for the container data type. Each
 request handler registers functions that know how to fill in a certain
 type of response (DAS, DDS, etc...). Find that function and invoke it. So,
 for example, there is a CedarRequestHandler class that registers functions
 that know how to fill in the different response objects from cedar files.

 Once the response object is filled, it is transmitted using a specified
 BESTransmitter.

 The status is then logged (default is to not log any status. It is up
 to derived classes of BESInterface to implement the log_status method.)

 The request and status are then reported. The default action is to
 pass off the reporting to BESReporterList::TheList(), which has a list of
 registered reporters and passes off the information to each of those
 reporters. For example, if the Cedar project wants to report on any
 cedar access, then it can register a reporter with
 BESReporterList::TheList().

 @see BESGlobalInit
 @see BESKeys
 @see BESResponseHandler
 @see BESRequestHandler
 @see BESTransmitter
 @see BESLog
 @see BESReporter
 */
class BESInterface: public BESObj {
private:
    std::ostream *d_strm {nullptr};
    int d_bes_timeout {0}; ///< Command timeout; can be overridden using setContext

protected:
    BESDataHandlerInterface *d_dhi_ptr {nullptr}; ///< Allocated by the child class
    BESTransmitter *d_transmitter {nullptr};  ///< The Transmitter to use for the result

    virtual void end_request();

    virtual void build_data_request_plan() = 0;

    virtual void execute_data_request_plan() = 0;

    virtual void transmit_data() = 0;

    virtual void log_status() = 0;

    virtual void clean() = 0;

    explicit BESInterface(std::ostream *strm);

    ~BESInterface() override = default;

    static int handleException(const BESError &e, BESDataHandlerInterface &dhi);

    void set_bes_timeout();
    void clear_bes_timeout();

public:
    // This is the point where BESServerHandler::execute(Connection *c) passes control
    // to the 'run the command' part of the server. jhrg 11/7/17
    virtual int execute_request(const std::string &from);

    virtual int finish(int status);

    void dump(std::ostream &strm) const override;
};

#endif // BESInterface_h_

