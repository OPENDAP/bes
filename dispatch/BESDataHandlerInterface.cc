// BESDataHandlerInterface.cc

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

#include "BESDataHandlerInterface.h"
#include "BESContainer.h"
#include "BESIndent.h"
#include "BESInfo.h"
#include "BESResponseHandler.h"

using std::endl;
using std::list;
using std::ostream;

/** @brief make a copy of only some data from specified object
 *
 * makes a copy of only some of the data members in a
 * BESDataHandlerInterface. The container list and response handler should
 * not be copied. Each BESDataHandlerInterface should represent a
 * request/response, so each one should have it's own response handler.
 *
 * @note deprecated Now calls the new clone() method that copies the
 * whole object given that Patrick said he didn't know why it only copied
 * a few parts and it's not really copying that much. Note that the 'data'
 * field is copied/assigned even if copy_from.data and this->data are the
 * same object, which is an issue on clang 5
 *
 * @param copy_from object to copy information from
 */
void BESDataHandlerInterface::make_copy(const BESDataHandlerInterface &copy_from) { clone(copy_from); }

/** @brief Clone
 * Unlike make_copy(), make an exact copy of the BESDataHandlerInterface object.
 * The destination object it 'this'.
 *
 * @note This performs a shallow copy of the pointers unless those objects
 * implement operator=() and perform a deep copy.
 *
 * @param rhs The instance to clone
 */
void BESDataHandlerInterface::clone(const BESDataHandlerInterface &copy_from) {
#if 0
	// Added because this can be called from make_copy() which is public.
	// jhrg 4/18/14
    // I removed this because I think it's bogus but having it here confuses
    // coverity into thinking that parts of the object built be the copy ctor
    // might be uninitialized. All of the NCML handler tests pass with this
    // modification. jhrg 9/17/15
	if (this == &copy_from)
		return;
#endif
    output_stream = copy_from.output_stream;
    response_handler = copy_from.response_handler;

    containers = copy_from.containers;
    containers_iterator = copy_from.containers_iterator;
    container = copy_from.container;

    action = copy_from.action;
    action_name = copy_from.action_name;
    executed = copy_from.executed;

    transmit_protocol = copy_from.transmit_protocol;

    // I do this because clang 5 (on OSX 10.9) will remove everything from 'data'
    // if copy_from.data and 'data' are the same object. Since the ncml handler uses
    // make_copy() and may be using a reference to a DHI and/or the DHI's 'data' map,
    // I'm leaving the test in here. That is, it could be building a new DHI instance
    // that uses a reference to an existing 'data' field and then assign the original
    // DHI instance to the new one. The DHIs are different, but the 'data' map are
    // not. jhrg 4/18/14
    if (&data != &copy_from.data)
        data = copy_from.data;

    error_info = copy_from.error_info;
}

BESDataHandlerInterface::BESDataHandlerInterface(const BESDataHandlerInterface &from) { clone(from); }

BESDataHandlerInterface &BESDataHandlerInterface::operator=(const BESDataHandlerInterface &rhs) {
    if (&rhs == this) {
        return *this;
    }

    clone(rhs);

    return *this;
}

/** @brief clean up any information created within this data handler
 * interface
 *
 * It is the job of the BESDataHandlerInterface to clean up the response
 * handler
 *
 */
void BESDataHandlerInterface::clean() {
    if (response_handler) {
        delete response_handler;
    }
    response_handler = nullptr;
}

/** @brief returns the response object using the response handler
 *
 * If the response handler is set for this request then return the
 * response object for the request using that response handler
 *
 * @return The response object for this request
 */
BESResponseObject *BESDataHandlerInterface::get_response_object() {
    BESResponseObject *response = 0;

    if (response_handler) {
        response = response_handler->get_response_object();
    }
    return response;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * each of the data members held
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDataHandlerInterface::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESDataHandlerInterface::dump" << endl;
    BESIndent::Indent();
    if (response_handler) {
        strm << BESIndent::LMarg << "response handler:" << endl;
        BESIndent::Indent();
        response_handler->dump(strm);
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "response handler: not set" << endl;
    }

    if (container) {
        strm << BESIndent::LMarg << "current container:" << endl;
        BESIndent::Indent();
        container->dump(strm);
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "current container: not set" << endl;
    }

    if (containers.size()) {
        strm << BESIndent::LMarg << "container list:" << endl;
        BESIndent::Indent();
        list<BESContainer *>::const_iterator i = containers.begin();
        list<BESContainer *>::const_iterator ie = containers.end();
        for (; i != ie; i++) {
            (*i)->dump(strm);
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "container list: empty" << endl;
    }

    strm << BESIndent::LMarg << "action: " << action << endl;
    strm << BESIndent::LMarg << "action name: " << action_name << endl;
    strm << BESIndent::LMarg << "transmit protocol: " << transmit_protocol << endl;
    if (data.size()) {
        strm << BESIndent::LMarg << "data:" << endl;
        BESIndent::Indent();
        data_citer i = data.begin();
        data_citer ie = data.end();
        for (; i != ie; i++) {
            strm << BESIndent::LMarg << (*i).first << ": " << (*i).second << endl;
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "data: none" << endl;
    }

    if (error_info) {
        strm << BESIndent::LMarg << "error info:" << endl;
        BESIndent::Indent();
        error_info->dump(strm);
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "error info: null" << endl;
    }
    BESIndent::UnIndent();
}
