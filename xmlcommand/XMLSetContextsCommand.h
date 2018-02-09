// XMLSetContextsCommand.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2018 University Corporation for Atmospheric Research
// Author: James Gallagher <jgallagher@opendap.org>
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

#ifndef XMLSetContextsCommand_h
#define XMLSetContextsCommand_h 1

#include "BESXMLCommand.h"
#include "BESDataHandlerInterface.h"

namespace bes {

class XMLSetContextsCommand: public BESXMLCommand {
public:
    XMLSetContextsCommand(const BESDataHandlerInterface &base_dhi) : BESXMLCommand(base_dhi) { }
    virtual ~XMLSetContextsCommand() { }

    virtual void parse_request(xmlNode *node);

    virtual bool has_response() {
        return false;
    }

    virtual void dump(ostream &strm) const;

    static BESXMLCommand *CommandBuilder(const BESDataHandlerInterface &base_dhi);
};

} // namespace bes

#endif // XMLSetContextsCommand_h

