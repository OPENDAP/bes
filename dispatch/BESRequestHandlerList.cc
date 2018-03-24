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

#include "BESRequestHandlerList.h"
#include "BESRequestHandler.h"
#include "BESInternalError.h"
#include "BESDataNames.h"

BESRequestHandlerList *BESRequestHandlerList::_instance = 0;

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
    if (find_handler(handler_name) == 0) {
        _handler_list[handler_name] = handler_object;
        return true;
    }
    return false;
}

/** @brief remove and return the specified request handler
 *
 * Finds, removes and returns the specified request handler. if the handler
 * exists then it is removed from the list, but not deleted. Deleting the
 * request handler is the responsability of the caller. The request handler is
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
    BESRequestHandler *ret = 0;
    BESRequestHandlerList::Handler_iter i;
    i = _handler_list.find(handler_name);
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
    BESRequestHandlerList::Handler_citer i;
    i = _handler_list.find(handler_name);
    if (i != _handler_list.end()) {
        return (*i).second;
    }
    return 0;
}

/** @brief return an iterator pointing to the first request handler in the
 * list
 *
 * @return a constant iterator pointing to the first request handler in the
 * list
 * @see BESRequestHandler
 */
BESRequestHandlerList::Handler_citer BESRequestHandlerList::get_first_handler()
{
    return _handler_list.begin();
}

/** @brief return a constant iterator pointing to the end of the list
 *
 * @return a constant iterator pointing to the end of the list
 * @see BESRequestHandler
 */
BESRequestHandlerList::Handler_citer BESRequestHandlerList::get_last_handler()
{
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
    string ret = "";
    bool first_name = true;
    BESRequestHandlerList::Handler_citer i = _handler_list.begin();
    for (; i != _handler_list.end(); i++) {
        if (!first_name) ret += ", ";
        ret += (*i).first;
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
    BESRequestHandlerList::Handler_citer i = get_first_handler();
    BESRequestHandlerList::Handler_citer ie = get_last_handler();
    for (; i != ie; i++) {
        BESRequestHandler *rh = (*i).second;
        p_request_handler_method p = rh->find_handler(dhi.action);
        if (p) {
            p(dhi);
        }
    }
}

/** @brief Execute a single method that will fill in the response object
 * rather than iterating over the list of containers or request handlers.
 *
 * This method is for requests of a single type of data. The request is passed
 * off to the request handler for the first container in the data handler
 * interface. It is up to this request handlers method for the specified
 * response object type to fill in the response object. It can iterate over
 * the containers in the data handler interface, for example.
 *
 * @note This method is not currently used. jhrg 2/23/16
 *
 * @param dhi data handler interface that contains the necessary information
 * to fill in the response object
 * @throws BESInternalError if the request handler cannot be found for the
 * first containers data type or if the request handler cannot fill in the
 * specified response object.
 * @see BESDataHandlerInterface
 * @see BESContainer
 * @see BESResponseObject
 */
void BESRequestHandlerList::execute_once(BESDataHandlerInterface &dhi)
{
    dhi.first_container();
    execute_current(dhi);
}

/** @brief Execute a single method for the current container that will fill
 * in the response object rather than iterating over the list of containers
 * or request handlers.
 *
 * The request is passed * off to the request handler for the current
 * container in the data handler interface.
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
    if (dhi.container) {
        // FIXME: This needs to happen here, but really should be done
        // in the get_container_type method in the container class if it
        // needs to happen. But those methods are not virtual and would
        // require a release of all modules.
        dhi.container->access();

        // 'find_handler' below could be renamed find_handler_for_container_type(). jhrg 2/8/18
        BESRequestHandler *rh = find_handler((dhi.container->get_container_type()));
        if (!rh)
            throw BESInternalError(string("The data handler '") + dhi.container->get_container_type() + "' does not exist",
                __FILE__, __LINE__);

        // 'find_handler' below could be renamed find_handler_method_for_response(). jhrg 2/8/18
        // Here's an example from the CSVRequestHandler:
        //     add_handler(DAS_RESPONSE, CSVRequestHandler::csv_build_das);
        // in the following 'p' will point to CSVRequestHandler::csv_build_das if
        // dhi.action is the string "get.das" (the value of the symbol DAS_RESPONSE)
        p_request_handler_method request_handler_method = rh->find_handler(dhi.action);
        if (!request_handler_method) {
            // TODO This should not be an internal error - it's really a configuration error
            // jhrg 2/20/15
            throw BESInternalError(string("Request handler for '") + dhi.container->get_container_type()
                + "' does not handle the response type '" + dhi.action + "'", __FILE__, __LINE__);
        }

        request_handler_method(dhi); // This is where the request handler method is called

        // TODO these are never used
#if 0
        if (dhi.container) {
            // This is (likely) for reporting. May not be used... jhrg 2/20/15
            string c_list = dhi.data[REAL_NAME_LIST];
            if (!c_list.empty()) c_list += ", ";
            c_list += dhi.container->get_real_name();
            dhi.data[REAL_NAME_LIST] = c_list;
        }
#endif
#if 0
        // if we can't find the function, see if there is a catch all
        // function that handles or redirects the request.
        //
        // TODO NB: There are no instances of catch.all handlers.
        // jhrg 2/20/15
        if (!p) {
            p = rh->find_handler( BES_REQUEST_HANDLER_CATCH_ALL);
        }
#endif
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
    strm << BESIndent::LMarg << "BESRequestHandlerList::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (_handler_list.size()) {
        strm << BESIndent::LMarg << "registered handlers:" << endl;
        BESIndent::Indent();
        BESRequestHandlerList::Handler_citer i = _handler_list.begin();
        BESRequestHandlerList::Handler_citer ie = _handler_list.end();
        for (; i != ie; i++) {
            BESRequestHandler *rh = (*i).second;
            rh->dump(strm);
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
    if (_instance == 0) {
        _instance = new BESRequestHandlerList;
    }
    return _instance;
}

