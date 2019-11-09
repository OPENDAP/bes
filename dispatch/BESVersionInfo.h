// BESVersionInfo.h

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

#ifndef BESVersionInfo_h_
#define BESVersionInfo_h_ 1

#include "BESInfo.h"

/** brief represents simple text information in a response object, such as
 * version and help information.
 *
 * Uses the default add_data and print methods, where the print method, if the
 * response is going to a browser, sets the MIME type to text.
 *
 * @see BESXMLInfo
 * @see BESResponseObject
 */
class BESVersionInfo: public BESInfo {
private:
    bool _inbes;
    bool _inhandler;
    BESInfo * _info;
    void add_version(const std::string &type, const std::string &name, const std::string &vers);
public:
    BESVersionInfo();
    virtual ~BESVersionInfo();

    virtual void add_library(const std::string &n, const std::string &v);
    virtual void add_module(const std::string &n, const std::string &v);
    virtual void add_service(const std::string &n, const std::list<std::string> &vers);

    virtual void begin_response(const std::string &response_name, BESDataHandlerInterface &dhi)
    {
        _info->begin_response(response_name, dhi);
    }
    virtual void end_response()
    {
        _info->end_response();
    }

    virtual void add_tag(const std::string &tag_name, const std::string &tag_data, std::map<std::string, std::string> *attrs = 0)
    {
        _info->add_tag(tag_name, tag_data, attrs);
    }
    virtual void begin_tag(const std::string &tag_name, std::map<std::string, std::string> *attrs = 0)
    {
        _info->begin_tag(tag_name, attrs);
    }
    virtual void end_tag(const std::string &tag_name)
    {
        _info->end_tag(tag_name);
    }

    virtual void add_data(const std::string &s)
    {
        _info->add_data(s);
    }
    virtual void add_space(unsigned long num_spaces)
    {
        _info->add_space(num_spaces);
    }
    virtual void add_break(unsigned long num_breaks)
    {
        _info->add_break(num_breaks);
    }
    virtual void add_data_from_file(const std::string &key, const std::string &name)
    {
        _info->add_data_from_file(key, name);
    }
    virtual void add_exception(BESError &e, const std::string &admin)
    {
        _info->add_exception(e, admin);
    }
    virtual void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi)
    {
        _info->transmit(transmitter, dhi);
    }
    virtual void print(std::ostream &strm)
    {
        _info->print(strm);
    }

    virtual void dump(std::ostream &strm) const;
};

#endif // BESVersionInfo_h_

