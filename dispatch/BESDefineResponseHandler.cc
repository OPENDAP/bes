// BESDefineResponseHandler.cc

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

#include <iostream>

using std::endl;

#include "BESDefineResponseHandler.h"

#if 0
#include "BESSilentInfo.h"
#endif


#include "BESDefine.h"
#include "BESDefinitionStorageList.h"
#include "BESDefinitionStorage.h"
#include "BESDataNames.h"
#include "BESSyntaxUserError.h"
#include "BESResponseNames.h"
#include "BESDataHandlerInterface.h"

BESDefineResponseHandler::BESDefineResponseHandler(const string &name) :
    BESResponseHandler(name)
{
}

BESDefineResponseHandler::~BESDefineResponseHandler()
{
}

/** @brief executes the command to create a new definition.
 *
 * A BESDefine object is created and added to the list of definitions. If
 * a definition already exists with the given name then it is removed and
 * the new one added.
 *
 * The BESDefine object is created using the containers, constraints,
 * attribute lists and aggregation command parsed in the parse method.
 *
 * @todo Roll this command's execute() method into the XMLDefineCommand::parse_request()
 * method using the NullResponseObject.
 *
 * @param dhi structure that holds request and response information
 * @throws BESSyntaxUserError if the store name specified does not exist
 * @see BESDataHandlerInterface
 * @see BESDefine
 * @see DefintionStorageList
 */
void BESDefineResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    dhi.action_name = DEFINE_RESPONSE_STR;
#if 0
    BESInfo *info = new BESSilentInfo();
    d_response_object = info;
#endif

    string def_name = dhi.data[DEF_NAME];
    string store_name = dhi.data[STORE_NAME];


    if (store_name == "") store_name = DEFAULT;

    BESDefinitionStorage *store = BESDefinitionStorageList::TheList()->find_persistence(store_name);
    if (store) {
        store->del_definition(def_name);

        BESDefine *dd = new BESDefine;
        dhi.first_container();
        while (dhi.container) {
            dd->add_container(dhi.container);
            dhi.next_container();
        }

        // TODO Now no-ops. Aggreagtion removed. jhrg 2/11/18
#if 0
        dd->set_agg_cmd(dhi.data[AGG_CMD]);
        dd->set_agg_handler(dhi.data[AGG_HANDLER]);
        dhi.data[AGG_CMD] = "";
        dhi.data[AGG_HANDLER] = "";
#endif

        store->add_definition(def_name, dd);
    }
    else {
        throw BESSyntaxUserError(string("Unable to add definition '") + def_name + "' to '" + store_name
                                 + "' store. Store does not exist", __FILE__, __LINE__);
    }
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * A BESInfo response object was built in the execute command to inform
 * the user whether the definition was successfully added or replaced. The
 * method send_text is called on the BESTransmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESInfo
 * @see BESTransmitter
 * @see BESDataHandlerInterface
 */
void BESDefineResponseHandler::transmit(BESTransmitter */*transmitter*/, BESDataHandlerInterface &/*dhi*/)
{
#if 0
	if (d_response_object) {
        BESInfo *info = dynamic_cast<BESInfo *>(d_response_object);
        if (!info) throw BESInternalError("cast error", __FILE__, __LINE__);
        info->transmit(transmitter, dhi);
    }
#endif
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDefineResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESDefineResponseHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
BESDefineResponseHandler::DefineResponseBuilder(const string &name)
{
    return new BESDefineResponseHandler(name);
}
