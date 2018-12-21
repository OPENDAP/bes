// BesWWWGetCommand.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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

#include "BESWWWGetCommand.h"
#include "BESTokenizer.h"
#include "BESResponseHandlerList.h"
#include "BESDefinitionStorageList.h"
#include "BESDefinitionStorage.h"
#include "BESDefine.h"
#include "BESSyntaxUserError.h"
#include "BESDataNames.h"
#include "BESWWWNames.h"

/** @brief knows how to parse a get request
 *
 * This class knows how to parse a get request, building a sub response
 * handler that actually knows how to build the requested response
 * object, such as das, dds, data, ddx, etc...
 *
 * A get request looks like:
 *
 * get &lt;response_type&gt; for &lt;def_name&gt; [return as &lt;ret_name&gt;;
 *
 * where response_type is the type of response being requested, for example
 * das, dds, dods.
 * where def_name is the name of the definition that has already been created,
 * like a view into the data
 * where ret_name is the method of transmitting the response. This is
 * optional.
 *
 * This parse method creates the sub response handler, retrieves the
 * definition information and finds the return object if one is specified.
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws BESSyntaxUserError if there is a problem parsing the request
 */
BESResponseHandler *BESWWWGetCommand::
parse_request(BESTokenizer & tokenizer, BESDataHandlerInterface & dhi)
{
    string def_name;
    string url;

    BESResponseHandler *retResponse =
        BESResponseHandlerList::TheList()->find_handler(_cmd);
    if (!retResponse) {
        string err("Command ");
        err += _cmd;
        err += " does not have a registered response handler";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }
    dhi.action = _cmd;

    string my_token = parse_options(tokenizer, dhi);
    if (my_token != "for") {
        tokenizer.parse_error(my_token + " not expected\n");
    } else {
        def_name = tokenizer.get_next_token();

        my_token = tokenizer.get_next_token();
        if (my_token == "using") {
            url = tokenizer.get_next_token();

            my_token = tokenizer.get_next_token();
            if (my_token != ";") {
                tokenizer.parse_error(my_token +
                                      " not expected, expecting ';'");
            }
        } else {
            tokenizer.parse_error(my_token +
                                  " not expected, expecting \"return\" or ';'");
        }
    }

    // FIX: should this be using dot notation? Like get das for volatile.d ;
    // Or do it like the containers, just find the first one available? Same
    // question for containers then?
    /*
       string store_name = DEFAULT ;
       BESDefinitionStorage *store =
       BESDefinitionStorageList::TheList()->find_def( store_name ) ;
       if( !store )
       {
       throw BESSyntaxUserError( (string)"Unable to find definition store " + store_name ) ;
       }
     */

    BESDefine *d = BESDefinitionStorageList::TheList()->look_for(def_name);
    if (!d) {
        string s = (string) "Unable to find definition " + def_name;
        throw BESSyntaxUserError(s, __FILE__, __LINE__);
    }

    BESDefine::containers_citer i = d->first_container();
    BESDefine::containers_citer ie = d->end_container();
    while (i != ie) {
        dhi.containers.push_back(*i);
        i++;
    }
    dhi.data[AGG_CMD] = d->get_agg_cmd();
    dhi.data[AGG_HANDLER] = d->get_agg_handler();
    dhi.data[WWW_URL] = url;

    return retResponse;
}
