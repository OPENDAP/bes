// BESTextInfo.h

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

#ifndef BESTextInfo_h_
#define BESTextInfo_h_ 1

#include "BESInfo.h"

/**
 * @brief represents simple text information in a response object, such as
 * version and help information.
 *
 * Uses the default add_data and print methods, where the print method, if the
 * response is going to a browser, sets the MIME type to text.
 *
 * @see BESInfo
 * @see BESResponseObject
 */
class BESTextInfo: public BESInfo {
private:
    std::string _indent;
    bool _ishttp;
    bool _header;
public:
    explicit BESTextInfo(bool ishttp = false);
    BESTextInfo(const std::string &key, std::ostream *strm, bool strm_owned, bool ishttp = false);
    ~BESTextInfo() override;

    void begin_response(const std::string &response_name, BESDataHandlerInterface &dhi) override;

    void add_tag(const std::string &tag_name,
                         const std::string &tag_data,
                         std::map<std::string, std::string, std::less<>> *attrs = nullptr) override;

    void begin_tag(const std::string &tag_name,
                           std::map<std::string, std::string, std::less<>> *attrs = nullptr) override;
    void end_tag(const std::string &tag_name) override;

    void add_data(const std::string &s) override;
    void add_space(unsigned long num_spaces) override;
    void add_break(unsigned long num_breaks) override;

    void add_data_from_file(const std::string &key, const std::string &name) override;
    void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi) override;

    void dump(std::ostream &strm) const override;

    static BESInfo *BuildTextInfo(const std::string &info_type);
};

#endif // BESTextInfo_h_

