// BESRequestHandlerList.h

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

#ifndef I_BESRequestHandlerList_h
#define I_BESRequestHandlerList_h 1

#include <map>
#include <string>

#include "BESObj.h"

#if 0
#include "BESDataHandlerInterface.h"
#endif

class BESRequestHandler;
class BESDataHandlerInterface;

/** @brief The list of registered request handlers for this server; a singleton
 *
 * For a type of data to be read by the BES, there must be a _request handler_
 * that can read those data. Typically, a request handler reads information
 * from files stored on the computer running the BES, although it is possible
 * for a request handler to read data from remote machines, other data services,
 * relational databases, et cetera.
 *
 * Instances of BESRequestHandler use the BESDataHandlerInterface object to
 * get a ResponseObject that they then 'fill in' with the 'requested' information.
 * The ResponseObject holds instances of other objects like DAS, DDS, DMR, or
 * version.
 *
 * The request handlers are registered with this request handler list using a
 * short std::string as the key value. Other parts of the BES can access this singleton
 * class to find a specific handler using that std::string.
 *
 * @see BESDapModule::initialize() to see where various handlers used to process
 * requests for DAP2 and DAP4 responses are registered along with the std::strings
 * that can be used to access the handlers using this class' `file_handler()` method.
 */
class BESRequestHandlerList: public BESObj {
private:
    static BESRequestHandlerList * _instance;
    std::map<std::string, BESRequestHandler *> _handler_list;

protected:
    BESRequestHandlerList(void)
    {
    }

public:
    virtual ~BESRequestHandlerList(void)
    {
    }

    typedef std::map<std::string, BESRequestHandler *>::const_iterator Handler_citer;
    typedef std::map<std::string, BESRequestHandler *>::iterator Handler_iter;

    virtual bool add_handler(const std::string &handler_name, BESRequestHandler * handler);
    virtual BESRequestHandler * remove_handler(const std::string &handler_name);
    virtual BESRequestHandler * find_handler(const std::string &handler_name);

    virtual Handler_citer get_first_handler();
    virtual Handler_citer get_last_handler();

    virtual std::string get_handler_names();

    virtual void execute_each(BESDataHandlerInterface &dhi);
    virtual void execute_all(BESDataHandlerInterface &dhi);
#if 0
    virtual void execute_once(BESDataHandlerInterface &dhi);
#endif
    virtual void execute_current(BESDataHandlerInterface &dhi);

    virtual void dump(std::ostream &strm) const;

    static BESRequestHandlerList *TheList();
};

#endif // I_BESRequestHandlerList_h

