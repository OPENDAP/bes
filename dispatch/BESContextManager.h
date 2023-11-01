// BESContextManager.h

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

#ifndef I_BESContextManager_h
#define I_BESContextManager_h 1

#include <map>
#include <string>
#include <mutex>

#include "BESObj.h"

class BESInfo;

/** @brief maintains the list of registered request handlers for this server
 *
 * For a type of data to be handled by the BES the data type must
 * registered a request handler with the server. This request handler knows
 * how to fill in specific response objects, such as DAS, DDS, help, version,
 * etc... The request handlers are registered with this request handler list.
 */
class BESContextManager: public BESObj {
private:
    static BESContextManager * d_instance;
    mutable std::recursive_mutex d_cache_lock_mutex;

    static void initialize_instance();
    static void delete_instance();

    std::map<std::string, std::string> _context_list;

public:
    BESContextManager();
    virtual ~BESContextManager();

    typedef std::map<std::string, std::string>::const_iterator Context_citer;
    typedef std::map<std::string, std::string>::iterator Context_iter;

    virtual void set_context(const std::string &name, const std::string &value);
    virtual void unset_context(const std::string &name);
    virtual std::string get_context(const std::string &name, bool &found);
    virtual int get_context_int(const std::string &name, bool &found);
    virtual uint64_t get_context_uint64(const std::string &name, bool &found);

    virtual void list_context(BESInfo &info);

    virtual void dump(std::ostream &strm) const;

    static BESContextManager * TheManager();
};

#endif // I_BESContextManager_h

