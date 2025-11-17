// BESDefineResponseHandler.h

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

#ifndef I_BESDefineResponseHandler_h
#define I_BESDefineResponseHandler_h 1

#include "BESResponseHandler.h"

/** @brief response handler that creates a definition given container names
 * and optionally constraints and attributes for each of those containers.
 *
 * A request looks like:
 *
 * define &lt;def_name&gt; as &lt;container_list&gt;
 *
 * &nbsp;&nbsp;[where &lt;container_x&gt;.constraint="&lt;constraint&gt;"]
 *
 * &nbsp;&nbsp;[,&lt;container_x&gt;.attributes="&lt;attrs&gt;"]
 *
 * &nbsp;&nbsp;[aggregate by "&lt;aggregation_command&gt;"];
 *
 * where container_list is a list of containers representing points of data,
 * such as a file. For each container in the container_list the user can
 * specify a constraint and a list of attributes. You need not specify a
 * constraint for a given container or a list of attributes. If just
 * specifying a constraint then leave out the attributes. If just specifying a
 * list of attributes then leave out the constraint. For example:
 *
 * define d1 as container_1,container_2
 *
 * &nbsp;&nbsp;where container_1.constraint="constraint1"
 *
 * &nbsp;&nbsp;,container_2.constraint="constraint2"
 *
 * &nbsp;&nbsp;,container_2.attributes="attr1,attr2";
 *
 * It adds the new definition to the list. If a definition already exists
 * in the list with the given name then it is removed first and the new one is
 * added. The definition is currently available through the life of the server
 * process and is not persistent.
 *
 * @see BESDefine
 * @see DefintionStorageList
 * @see BESContainer
 * @see BESTransmitter
 */
class BESDefineResponseHandler : public BESResponseHandler {
public:
    BESDefineResponseHandler(const std::string &name);
    ~BESDefineResponseHandler(void) override;

    void execute(BESDataHandlerInterface &dhi) override;
    void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi) override;

    void dump(std::ostream &strm) const override;

    static BESResponseHandler *DefineResponseBuilder(const std::string &name);
};

#endif // I_BESDefineResponseHandler_h
