// BESRequestHandlerList.cc

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

#include "BESLog.h"
#include "BESRequestHandlerList.h"
#include "BESRequestHandler.h"
#include "BESInternalError.h"

using std::endl;
using std::ostream;
using std::string;

BESRequestHandlerList *BESRequestHandlerList::d_instance = nullptr;
static std::once_flag d_euc_init_once;

/** @brief add a request handler to the list of registered handlers for this
 * server
 *
 * @param handler_name name of the data type handled by this request handler
 * @param handler_object the request handler object that knows how to fill in
 * specific response objects
 * @return true if successfully added, false if already exists
 * @see BESRequestHandler
 */
bool BESRequestHandlerList::add_handler(const string &handler_name, BESRequestHandler *handler_object)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    if (find_handler(handler_name) == nullptr) {
        _handler_list[handler_name] = handler_object;
        return true;
    }
    return false;
}

/** @brief remove and return the specified request handler
 *
 * Finds, removes and returns the specified request handler. if the handler
 * exists then it is removed from the list, but not deleted. Deleting the
 * request handler is the responsibility of the caller. The request handler is
 * then returned to the caller. If not found, NULL is returned
 *
 * @param handler_name name of the data type request handler to be removed and
 * returned
 * @return returns the request handler if found, NULL otherwise
 * @see BESRequestHandler
 */
BESRequestHandler *
BESRequestHandlerList::remove_handler(const string &handler_name)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESRequestHandler *ret = nullptr;

    auto i = _handler_list.find(handler_name);
    if (i != _handler_list.end()) {
        ret = (*i).second;
        _handler_list.erase(i);
    }
    return ret;
}

/** @brief find and return the specified request handler
 *
 * @param handler_name name of the data type request handler
 * @return the request handler for the specified data type, NULL if not found
 * @see BESRequestHandler
 */
BESRequestHandler *
BESRequestHandlerList::find_handler(const string &handler_name)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    auto i = _handler_list.find(handler_name);
    if (i != _handler_list.end()) {
        return i->second;
    }
    return nullptr;
}

/** @brief return an iterator pointing to the first request handler in the
 * list
 *
 * @return a constant iterator pointing to the first request handler in the
 * list
 * @see BESRequestHandler
 */
BESRequestHandlerList::handler_citer BESRequestHandlerList::get_first_handler()
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    return _handler_list.begin();
}

/** @brief return a constant iterator pointing to the end of the list
 *
 * @return a constant iterator pointing to the end of the list
 * @see BESRequestHandler
 */
BESRequestHandlerList::handler_citer BESRequestHandlerList::get_last_handler()
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    return _handler_list.end();
}

/** @brief Returns a comma separated string of request handlers registered
 * with the server
 *
 * @return comma separated string of request handler names registered with the
 * server.
 * @see BESRequestHandler
 */
string BESRequestHandlerList::get_handler_names()
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    string ret;
    bool first_name = true;
    for (auto const &handler_pair: _handler_list) {
        if (!first_name) ret += ", ";
        ret += handler_pair.first;
        first_name = false;
    }
    return ret;
}

/** @brief for each container in the given data handler interface, execute the
 * given request
 *
 * For some response objects it is necessary to iterate over all of the
 * containers listed in the specified data handler interface. For each
 * container, get the type of data represented by that container, find the
 * request handler for that data type, find the method within that request
 * handler that knows how to handle the response object to be filled in, and
 * execute that method.
 *
 * @param dhi the data handler interface that contains the list of containers
 * to be iterated over
 * @throws BESInternalError if any one of the request handlers does not
 * know how to fill in the specified response object or if any one of the
 * request handlers does not exist.
 * @see BESDataHandlerInterface
 * @see BESContainer
 * @see BESRequestHandler
 * @see BESResponseObject
 */
void BESRequestHandlerList::execute_each(BESDataHandlerInterface &dhi)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    dhi.first_container();
    while (dhi.container) {
        execute_current(dhi);
        dhi.next_container();
    }
}

/** @brief for all of the registered request handlers, execute the given
 * request
 *
 * In some cases, such as a version or help request, it is necessary to
 * iterate over all of the registered request handlers to fill in the response
 * object. If a request handler does not know how to fill in the response
 * object, i.e. doesn't handle the response type, then simply move on to the
 * next. No exception is thrown in this case.
 *
 * @note This method is currently _only_ used by the help and version
 * requests. jhrg 2/23/16
 *
 * @param dhi data handler interface that contains the necessary information
 * to fill in the response object.
 * @see BESDataHandlerInterface
 * @see BESRequestHandler
 * @see BESResponseObject
 */
void BESRequestHandlerList::execute_all(BESDataHandlerInterface &dhi)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    for (auto const &handler_pair: _handler_list) {
        auto p = handler_pair.second->find_method(dhi.action);
        if (p)
            p(dhi);
    }
}

/** @brief Execute a single method for the current container that will fill
 * in the response object rather than iterating over the list of containers
 * or request handlers.
 *
 * The request is passed off to the request handler for the current
 * container in the data handler interface.
 *
 * @note This is used only in one place (3/8/22) in the NCML module in
 * DDSLoader::loadInto().
 *
 * @param dhi data handler interface that contains the necessary information
 * to fill in the response object
 * @throws BESInternalError if the request handler cannot be found for the
 * current containers data type or if the request handler cannot fill in the
 * specified response object.
 * @see BESDataHandlerInterface
 * @see BESContainer
 * @see BESResponseObject
 */
void BESRequestHandlerList::execute_current(BESDataHandlerInterface &dhi)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    if (dhi.container) {
        // Patrick's comment: This needs to happen here, but really should be done
        // in the get_container_type method in the container class if it
        // needs to happen.
        //
        // This call will, for BESFileContainer, decompress and cache compressed files,
        // changing their extensions from, e.g., '.gz' to '.h5' and enabling the
        // get_container_type() method to function correctly. jhrg 5/31/18

        // TODO for the NgapOwnedContainer, this is a problem since these calls are
        //  expensive, even when the container is intelligently written (DMR++ documents
        //  can be many megabytes in size). Fix this mess so that get_container_type()
        //  works without all of the handlers calling access() twice. jhrg 2/18/25
        dhi.container->access();

        // Given the kind of thing in the DHI's container (netcdf file, ...) find the
        // RequestHandler that understands that and then find the method in that handler
        // that can process the DHI's action.
        BESRequestHandler *rh = find_handler((dhi.container->get_container_type()));
        if (!rh)
            throw BESInternalError(string("The data handler '") + dhi.container->get_container_type() + "' does not exist",
                __FILE__, __LINE__);

        p_request_handler_method request_handler_method = rh->find_method(dhi.action);
        if (!request_handler_method) {
            throw BESInternalError(string("Request handler for '") + dhi.container->get_container_type()
                + "' does not handle the response type '" + dhi.action + "'", __FILE__, __LINE__);
        }

        VERBOSE("Found handler '" + rh->get_name() + "' for item '" + dhi.container->get_symbolic_name() + "'.\n");

        request_handler_method(dhi); // This is where the request handler method is called
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * each of the registered request handlers.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESRequestHandlerList::dump(ostream &strm) const
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    strm << BESIndent::LMarg << "BESRequestHandlerList::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (!_handler_list.empty()) {
        strm << BESIndent::LMarg << "registered handlers:" << endl;
        BESIndent::Indent();
        for (auto const &handler_pair: _handler_list) {
            handler_pair.second->dump(strm);
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "registered handlers: none" << endl;
    }
    BESIndent::UnIndent();
}

BESRequestHandlerList *
BESRequestHandlerList::TheList()
{
    std::call_once(d_euc_init_once,BESRequestHandlerList::initialize_instance);
    return d_instance;
}

void BESRequestHandlerList::initialize_instance() {
    d_instance = new BESRequestHandlerList;
#ifdef HAVE_ATEXIT
    atexit(delete_instance);
#endif
}

void BESRequestHandlerList::delete_instance() {
    delete d_instance;
    d_instance = nullptr;
}



