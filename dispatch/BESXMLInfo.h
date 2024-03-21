// BESXMLInfo.h

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

#ifndef BESXMLInfo_h_
#define BESXMLInfo_h_ 1

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include "BESInfo.h"

/** @brief represents an xml formatted response object
 *
 * An informational response object that is formated as an XML document.
 *
 * @see BESInfo
 * @see BESResponseObject
 */
class BESXMLInfo: public BESInfo {
private:
    xmlTextWriterPtr _writer;
    xmlBufferPtr _doc_buf;
    bool _started;
    bool _ended;
    std::string _doc;

    void cleanup();
protected:
    virtual void begin_tag(const std::string &tag_name, const std::string &ns, const std::string &uri, std::map<std::string, std::string> *attrs = 0);
public:
    BESXMLInfo();
    virtual ~BESXMLInfo();

    virtual void begin_response(const std::string &response_name, BESDataHandlerInterface &dhi);
    virtual void begin_response(const std::string &response_name, std::map<std::string, std::string, std::less<>> *attrs, BESDataHandlerInterface &dhi);
    virtual void end_response();

    virtual void add_tag(const std::string &tag_name, const std::string &tag_data, std::map<std::string, std::string> *attrs = 0);
    virtual void begin_tag(const std::string &tag_name, std::map<std::string, std::string> *attrs = 0);
    virtual void end_tag(const std::string &tag_name);

    virtual void add_data(const std::string &s);
    virtual void add_space(unsigned long num_spaces);
    virtual void add_break(unsigned long num_breaks);

    virtual void add_data_from_file(const std::string &key, const std::string &name);
    virtual void print(std::ostream &strm);
    virtual void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi);

    virtual void dump(std::ostream &strm) const;

    static BESInfo *BuildXMLInfo(const std::string &info_type);
};

#endif // BESXMLInfo_h_

