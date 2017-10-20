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

#include "BESContextManager.h"
#include "BESInfo.h"

BESContextManager *BESContextManager::_instance = 0;

/** @brief set context in the BES
 *
 * @param name name of the context
 * @param value value the context is to take
 */
void BESContextManager::set_context(const string &name, const string &value)
{
    _context_list[name] = value;
}

/** @brief set context in the BES
 *
 * @param name name of the context
 * @param value value the context is to take
 */
void BESContextManager::unset_context(const string &name)
{
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
    string ret = "";
    found = false;
    BESContextManager::Context_iter i;
    i = _context_list.find(name);
    if (i != _context_list.end()) {
        ret = (*i).second;
        found = true;
    }
    return ret;
}

/** @brief Adds all context and their values to the given informational
 * object
 */
void BESContextManager::list_context(BESInfo &info)
{
    string name;
    string value;
    map<string, string> props;
    BESContextManager::Context_citer i = _context_list.begin();
    BESContextManager::Context_citer e = _context_list.end();
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
    strm << BESIndent::LMarg << "BESContextManager::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (_context_list.size()) {
        strm << BESIndent::LMarg << "current context:" << endl;
        BESIndent::Indent();
        BESContextManager::Context_citer i = _context_list.begin();
        BESContextManager::Context_citer ie = _context_list.end();
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
    if (_instance == 0) {
        _instance = new BESContextManager;
    }
    return _instance;
}

