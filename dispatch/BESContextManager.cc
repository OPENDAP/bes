// BESContextManager.cc

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

#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <mutex>

#include "BESContextManager.h"
#include "BESInfo.h"
#include "BESDebug.h"

using std::endl;
using std::string;
using std::ostream;

BESContextManager *BESContextManager::d_instance = nullptr;
static std::once_flag d_euc_init_once;

#define MODULE "context"
#define prolog std::string("BESContextManager::").append(__func__).append("() - ")

BESContextManager::BESContextManager() {}

/** @brief set context in the BES
 *
 * @param name name of the context
 * @param value value the context is to take
 */
void BESContextManager::set_context(const string &name, const string &value)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESDEBUG(MODULE, prolog << "name=\"" << name << "\", value=\"" << value << "\"" << endl);
    _context_list[name] = value;
}

/** @brief set context in the BES
 *
 * @param name name of the context
 * @param value value the context is to take
 */
void BESContextManager::unset_context(const string &name)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESDEBUG(MODULE, prolog << "name=\"" << name << "\"" << endl);
    _context_list.erase(name);
}

/** @brief retrieve the value of the specified context from the BES
 *
 * Finds the specified context and returns its value
 *
 * @param name name of the context to retrieve
 * @param found the value of this parameter is set to indicate whether the
 * context was found or not. An empty string could be a valid value
 * @return the value of the requested context, empty string if not found
 */
string BESContextManager::get_context(const string &name, bool &found)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    string ret;
    found = false;
    // Use find() instead of operator[] to avoid inserting a default value at key
    // if it does not exist int he map. jhrg 2/26/25
    auto const i = _context_list.find(name);
    if (i != _context_list.end()) {
        ret = i->second;
        found = true;
    }

    BESDEBUG(MODULE, prolog << "name=\"" << name << "\", found=\"" << found << "\" value:\"" << ret << "\"" << endl);

    return ret;
}

/**
 * @brief Get the value of the given context and return it as an integer
 * @author jhrg 5/30/18
 * @param name The context name
 * @param found True if the context was found, false otherwise.
 * @return The context value as an integer. Returns 0 and found == false
 * if the context \arg name was not found. If the \arg name was found but
 * the value is the empty string, return 0.
 */
int BESContextManager::get_context_int(const string &name, bool &found)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    string value = BESContextManager::TheManager()->get_context(name, found);

    if (!found || value.empty()) return 0;

    char *endptr;
    errno = 0;
    int val = strtol(value.c_str(), &endptr, /*int base*/10);
    if (val == 0 && errno > 0) {
        throw BESInternalError(string("Error reading an integer value for the context '") + name + "': " + strerror(errno),
            __FILE__, __LINE__);
    }
    BESDEBUG(MODULE, prolog << "name=\"" << name << "\", found=\"" << found << "\" value: \"" << val << "\"" << endl);

    return val;
}

uint64_t BESContextManager::get_context_uint64(const string &name, bool &found)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    string value = BESContextManager::TheManager()->get_context(name, found);

    if (!found || value.empty()) return 0;

    char *endptr;
    errno = 0;
    uint64_t val = strtoull(value.c_str(), &endptr, /*int base*/10);
    if (val == 0 && errno > 0) {
        throw BESInternalError(string("Error reading an integer value for the context '") + name + "': " + strerror(errno),
                               __FILE__, __LINE__);
    }
    BESDEBUG(MODULE, prolog << "name=\"" << name << "\", found=\"" << found << "\" value: \"" << val << "\"" << endl);
    return val;
}




/** @brief Adds all context and their values to the given informational
 * object
 */
void BESContextManager::list_context(BESInfo &info)
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    string name;
    string value;
    std::map<string, string, std::less<>> props;
    auto i = _context_list.begin();
    auto e = _context_list.end();
    for (; i != e; i++) {
        props.clear();
        name = (*i).first;
        value = (*i).second;
        props["name"] = name;
        info.add_tag("context", value, &props);
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * each of the context values
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESContextManager::dump(ostream &strm) const
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    strm << BESIndent::LMarg << prolog << "(this: " << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (_context_list.size()) {
        strm << BESIndent::LMarg << "current context:" << endl;
        BESIndent::Indent();
        auto i = _context_list.begin();
        auto ie = _context_list.end();
        for (; i != ie; i++) {
            strm << BESIndent::LMarg << (*i).first << ": " << (*i).second << endl;
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "no context" << endl;
    }
    BESIndent::UnIndent();
}

BESContextManager *
BESContextManager::TheManager()
{
    static BESContextManager manager;
    // std::call_once(d_euc_init_once,BESContextManager::initialize_instance);
    return &manager;
}

void BESContextManager::initialize_instance() {
    d_instance = new BESContextManager;
#ifdef HAVE_ATEXIT
    atexit(delete_instance);
#endif
}

void BESContextManager::delete_instance() {
    delete d_instance;
    d_instance = 0;
}

