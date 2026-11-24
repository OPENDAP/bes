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

#include <vector>

#include "BESInterface.h"
#include "BESDataHandlerInterface.h"

class BESXMLCommand;

/** @brief Entry point into BES using xml document requests

 @see BESInterface
 */
class BESXMLInterface: public BESInterface {
private:

    friend class XmlInterfaceTest;

    /// This matches all the XML commands to methods that parse them
    std::vector<BESXMLCommand*> d_xml_cmd_list;

    /// The command(s) to run bundled in an XML document.
    std::string d_xml_document;

    /// This is the DHI used to hold information parsed from the request.
    BESDataHandlerInterface d_xml_interface_dhi;

protected:
    void build_data_request_plan() override;

    void execute_data_request_plan() override;

    void transmit_data() override;

    void log_status() override;

    void clean() override;

    void log_the_command(); // added for just this code. jhrg 4/20/22

public:
    BESXMLInterface(const std::string &cmd, std::ostream *strm);

    ~BESXMLInterface() override;

    // Hack: To get the URL and CE for the error message (HYRAX-2334) we use
    // information sent by the OLFS. This is actually a feature of the OLFS
    // that is set with an option which is generally on for Hyrax, but can be
    // turned off (I think). Thus, this is a maybe subtle point of coupling
    // between the olfs and bes. jhrg 11/24/26
    static std::string olfs_log_url_path(const std::string &olfs_log_line);

    void dump(std::ostream &strm) const override;
};

#endif // BESXMLInterface_h_

