// BESDapErrorInfo.h

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

#ifndef BESDapErrorInfo_h_
#define BESDapErrorInfo_h_ 1

#include <string>

#include "BESInfo.h"
#include <libdap/Error.h>

using namespace libdap;

/** @brief silent informational response object
 *
 * This class ignores any data added to an informational object and ignores
 * the print command. Basically, it is silent!
 *
 * @see BESResponseObject
 */
class BESDapErrorInfo: public BESInfo {
private:
    ErrorCode _error_code;
    std::string _error_msg;

public:
    BESDapErrorInfo() = delete;
    BESDapErrorInfo(ErrorCode ec, std::string msg);
    ~BESDapErrorInfo() override = default;

    void begin_response(const std::string &response_name, BESDataHandlerInterface &dhi) override;

    void add_tag(const std::string &tag_name, const std::string &tag_data, std::map<std::string, std::string> *attrs = nullptr) override;
    void begin_tag(const std::string &tag_name, std::map<std::string, std::string> *attrs = nullptr) override;
    void end_tag(const std::string &tag_name) override;

    void add_data(const std::string &s) override;
    void add_space(unsigned long num_spaces) override;
    void add_break(unsigned long num_breaks) override;

    void add_data_from_file(const std::string &key, const std::string &name) override;
    void add_exception(const BESError &e, const std::string &admin) override;
    void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi) override;
    void print(std::ostream &strm) override;

    void dump(std::ostream &strm) const override;
};

#endif // BESDapErrorInfo_h_

