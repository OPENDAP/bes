// BESResponseHandlerList.h

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

#ifndef I_BESResponseHandlerList_h
#define I_BESResponseHandlerList_h 1

#include <map>
#include <string>
#include <mutex>

#include "BESObj.h"

class BESResponseHandler;

typedef BESResponseHandler * (*p_response_handler)(const std::string &name);

/** @brief List of all registered response handlers for this server
 *
 * A BESResponseHandlerList allows the developer to add or remove named
 * response handlers from the list of handlers available for this server. For
 * example, when the request 'get das for d1;' a response handler named "das"
 * is looked up in the response handler list. Another example, 'show help;'
 * would look up a response handler named "show".
 *
 * What is actually added to the list are functions that instantiate the named
 * response handler, not the response handler themselves.
 *
 * @see BESResponseHandler
 * @see BESResponseObject
 */
class BESResponseHandlerList: public BESObj {
private:
    mutable std::recursive_mutex d_cache_lock_mutex;

    static void initialize_instance();
    static void delete_instance();

    std::map<std::string, p_response_handler> _handler_list;

    friend class resplistT;

    BESResponseHandlerList();

public:
    ~BESResponseHandlerList() override = default;

    BESResponseHandlerList(const BESResponseHandlerList&) = delete;
    BESResponseHandlerList& operator=(const BESResponseHandlerList&) = delete;

    typedef std::map<std::string, p_response_handler>::const_iterator Handler_citer;
    typedef std::map<std::string, p_response_handler>::iterator Handler_iter;

    virtual bool add_handler(const std::string &handler, p_response_handler handler_method);
    virtual bool remove_handler(const std::string &handler);
    virtual BESResponseHandler * find_handler(const std::string &handler);

    virtual std::string get_handler_names();

    void dump(std::ostream &strm) const override;

    static BESResponseHandlerList * TheList();
};

#endif // I_BESResponseHandlerList_h

