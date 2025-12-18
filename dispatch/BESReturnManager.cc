// BESReturnManager.cc

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

#include <mutex>

#include "BESReturnManager.h"

using std::endl;
using std::ostream;
using std::string;

BESReturnManager *BESReturnManager::TheManager() {
    static BESReturnManager the_manager;
    return &the_manager;
}

bool BESReturnManager::add_transmitter(const string &name, BESTransmitter *transmitter) {
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    if (find_transmitter(name) == nullptr) {
        transmitter_list_[name] = transmitter;
        return true;
    }
    return false;
}

bool BESReturnManager::del_transmitter(const string &name) {
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    bool ret = false;

    auto i = transmitter_list_.find(name);
    if (i != transmitter_list_.end()) {
        auto *obj = i->second;
        transmitter_list_.erase(i);
        delete obj;
        ret = true;
    }
    return ret;
}

BESTransmitter *BESReturnManager::find_transmitter(const string &name) {
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    auto i = transmitter_list_.find(name);
    if (i != transmitter_list_.end()) {
        return i->second;
    }
    return nullptr;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the transmitters
 * registered with the return manager.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESReturnManager::dump(ostream &strm) const {
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    strm << BESIndent::LMarg << "BESReturnManager::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    if (!transmitter_list_.empty()) {
        strm << BESIndent::LMarg << "registered transmitters:" << endl;
        BESIndent::Indent();
        for (const auto &i : transmitter_list_) {
            strm << BESIndent::LMarg << i.first << endl;
            BESIndent::Indent();
            i.second->dump(strm);
            BESIndent::UnIndent();
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "registered transmitters: none" << endl;
    }
    BESIndent::UnIndent();
}
