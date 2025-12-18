// BESXMLSetContainerCommand.h

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

#ifndef A_BESXMLSetContainerCommand_h
#define A_BESXMLSetContainerCommand_h 1

#include "BESXMLCommand.h"
#include "BESDataHandlerInterface.h"


/**
 * @brief Parse the setContainer element in a request document
 *
 * This command will make a new Container object that binds a URL Path
 * to the name on some data store the BES can access. If no data store
 * is given in the command, the default 'catalog,' the local POSIX file
 * system, is assumed. Confusingly, this default catalog is named 'catalog.'
 *
 * @todo This command can be shortened like the setContexts command to use
 * the NullResponseHandler since it does not normally return data to the
 * BES's client (i.e., it's response handler uses the SilentInfo response
 * object).
 *
 */
class BESXMLSetContainerCommand: public BESXMLCommand {
public:
    BESXMLSetContainerCommand(const BESDataHandlerInterface &base_dhi);
    virtual ~BESXMLSetContainerCommand()
    {
    }

    virtual void parse_request(xmlNode *node);

    virtual bool has_response()
    {
        return false;
    }

    void dump(std::ostream &strm) const override;

    static BESXMLCommand * CommandBuilder(const BESDataHandlerInterface &base_dhi);
};

#endif // A_BESXMLSetContainerCommand_h

