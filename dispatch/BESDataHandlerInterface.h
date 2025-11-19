// BESDataHandlerInterface.h

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

#ifndef BESDataHandlerInterface_h_
#define BESDataHandlerInterface_h_ 1

#include <iostream>
#include <list>
#include <map>
#include <string>

class BESResponseHandler;
class BESResponseObject;
class BESInfo;

#include "BESContainer.h"
#include "BESInternalError.h"
#include "BESObj.h"
#include "BESResponseHandler.h"

/** @brief Structure storing information used by the BES to handle the request

 This information is used throughout the BES framework to handle the
 request and to also store information for logging and reporting.
 */

class BESDataHandlerInterface : public BESObj {
    std::ostream *output_stream = nullptr;

    // I tried adding a complete 'clone the dhi' method to see if that
    // would address the problem we're seeing on OSX 10.9. It didn't, but
    // we're not done yet, so maybe this will be useful still. jhrg 4/16/14
    void clone(const BESDataHandlerInterface &copy_from);

public:
    using data_citer = std::map<std::string, std::string>::const_iterator;

    BESResponseHandler *response_handler = nullptr;

    std::list<BESContainer *> containers;
    std::list<BESContainer *>::iterator containers_iterator;

    /** @brief pointer to the current container in this interface
     */
    BESContainer *container = nullptr;

    /** @brief the response object requested, e.g. das, dds
     */
    std::string action;
    std::string action_name;
    bool executed = false;

    /** @brief request protocol, such as HTTP
     */
    std::string transmit_protocol; // FIXME Not used? jhrg 5/30/18

    /** @brief the map of string data that will be required for the current
     * request.
     */
    std::map<std::string, std::string> data;

    /** @brief error information object
     */
    BESInfo *error_info = nullptr;

    BESDataHandlerInterface() = default;

    // These were causing multiple compiler warnings, so I removed the implementations since
    // it's clear they are private to be disallowed from auto generation for now
    // and this just not declaring an impl solves it.  (mpj 2/26/10)
    //
    // I implemented these - and made clone() private, since that's a more common pattern.
    // jhrg 4/18/14
    BESDataHandlerInterface(const BESDataHandlerInterface &from);
    BESDataHandlerInterface &operator=(const BESDataHandlerInterface &rhs);

    // Added 5/11/22 based on valgrind output. jhrg
    ~BESDataHandlerInterface() override { clean(); }

    /// deprecated
    void make_copy(const BESDataHandlerInterface &copy_from);

    void clean();

    void set_output_stream(std::ostream *strm) {
        if (output_stream) {
            std::string err = "output stream has already been set";
            throw BESInternalError(err, __FILE__, __LINE__);
        }
        output_stream = strm;
    }

    std::ostream &get_output_stream() const {
        if (!output_stream)
            throw BESInternalError("output stream has not yet been set, cannot use", __FILE__, __LINE__);
        return *output_stream;
    }

    BESResponseObject *get_response_object();

    /** @brief set the container pointer to the first container in the containers list
     */
    void first_container() {
        containers_iterator = containers.begin();
        if (containers_iterator != containers.end())
            container = (*containers_iterator);
        else
            container = nullptr;
    }

    /** @brief set the container pointer to the next * container in the list, null if at the end or no containers in
     * list
     */
    void next_container() {
        ++containers_iterator;
        if (containers_iterator != containers.end())
            container = (*containers_iterator);
        else
            container = nullptr;
    }

    const std::map<std::string, std::string> &data_c() const { return data; }

    void dump(std::ostream &strm) const override;
};

#endif //  BESDataHandlerInterface_h_
