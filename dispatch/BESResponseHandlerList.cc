// BESResponseHandlerList.cc

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

#include "config.h"
#include <mutex>

#ifdef HAVE_STDLIB_H
#include <cstdlib>
#endif

#include "BESResponseHandlerList.h"

using std::endl;
using std::ostream;
using std::string;

BESResponseHandlerList::BESResponseHandlerList() = default;

/** @brief add a response handler to the list
 *
 * This method actually adds to the list a method that knows how to build a
 * response handler. For each request that comes in, the response name (such
 * as das or help or define) is looked up in this list and the method is used to
 * build a new response handler.
 *
 * @param handler_name name of the handler to add to the list
 * @param handler_method method that knows how to build the named response
 * handler
 * @return true if successfully added, false if it already exists
 * @see BESResponseHandler
 * @see BESResponseObject
 */
bool BESResponseHandlerList::add_handler(const string &handler_name, p_response_handler handler_method) {
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESResponseHandlerList::Handler_citer i = _handler_list.find(handler_name);
    if (i == _handler_list.end()) {
        _handler_list[handler_name] = handler_method;
        return true;
    }
    return false;
}

/** @brief removes a response handler from the list
 *
 * The method that knows how to build the specified response handler is
 * removed from the list.
 *
 * @param handler_name name of the handler build method to remove from the list
 * @return true if successfully removed, false if it doesn't exist in the list
 * @see BESResponseHandler
 */
bool BESResponseHandlerList::remove_handler(const string &handler_name) {
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESResponseHandlerList::Handler_iter i = _handler_list.find(handler_name);
    if (i != _handler_list.end()) {
        _handler_list.erase(i);
        return true;
    }
    return false;
}

/** @brief returns the response handler with the given name from the list
 *
 * This method looks up the build method with the given name in the list. If
 * it is found then the build method is invoked with the given handler name
 * and the response handler built with the build method is returned. If the
 * handler build method does not exist in the list then NULL is returned.
 *
 * @param handler_name name of the handler to build and return
 * @return a BESResponseHandler using the specified build method, or NULL if
 * it doesn't exist in the list.
 * @see BESResponseHandler
 */
BESResponseHandler *BESResponseHandlerList::find_handler(const string &handler_name) {
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESResponseHandlerList::Handler_citer i = _handler_list.find(handler_name);
    if (i != _handler_list.end()) {
        p_response_handler p = (*i).second;
        if (p) {
            return p(handler_name);
        }
    }
    return nullptr;
}

/** @brief returns the comma separated list of all response handlers currently registered with this server.
 *
 * Builds a comma separated list of response handlers registered with this
 * server and returns it to the caller.
 *
 * @return comma separated list of response handler names
 */
string BESResponseHandlerList::get_handler_names() {
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    string ret = "";
    bool first_name = true;
    BESResponseHandlerList::Handler_citer i = _handler_list.begin();
    for (; i != _handler_list.end(); i++) {
        if (!first_name)
            ret += ", ";
        ret += (*i).first;
        first_name = false;
    }
    return ret;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the names of the
 * registered response handlers.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESResponseHandlerList::dump(ostream &strm) const {
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    strm << BESIndent::LMarg << "BESResponseHandlerList::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    if (_handler_list.size()) {
        strm << BESIndent::LMarg << "registered response handlers:" << endl;
        BESIndent::Indent();
        BESResponseHandlerList::Handler_citer i = _handler_list.begin();
        BESResponseHandlerList::Handler_citer ie = _handler_list.end();
        for (; i != ie; i++) {
            strm << BESIndent::LMarg << (*i).first << endl;
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "registered response handlers: none" << endl;
    }
    BESIndent::UnIndent();
}

BESResponseHandlerList *BESResponseHandlerList::TheList() {
    static BESResponseHandlerList list;
    return &list;
}
