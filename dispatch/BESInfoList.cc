// BESInfoList.cc

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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <mutex>

#include "BESInfo.h"
#include "BESInfoList.h"
#include "TheBESKeys.h"

using std::endl;
using std::ostream;
using std::string;

#define BES_DEFAULT_INFO_TYPE "txt"

BESInfoList::BESInfoList() {}

bool BESInfoList::add_info_builder(const string &info_type, p_info_builder info_builder) {

    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESInfoList::Info_citer i;
    i = _info_list.find(info_type);
    if (i == _info_list.end()) {
        _info_list[info_type] = info_builder;
        return true;
    }
    return false;
}

bool BESInfoList::rem_info_builder(const string &info_type) {

    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESInfoList::Info_iter i;
    i = _info_list.find(info_type);
    if (i != _info_list.end()) {
        _info_list.erase(i);
        return true;
    }
    return false;
}

BESInfo *BESInfoList::build_info() {

    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    string info_type = "";
    bool found = false;
    TheBESKeys::TheKeys()->get_value("BES.Info.Type", info_type, found);

    if (!found || info_type == "")
        info_type = BES_DEFAULT_INFO_TYPE;

    BESInfoList::Info_citer i;
    i = _info_list.find(info_type);
    if (i != _info_list.end()) {
        p_info_builder p = (*i).second;
        if (p) {
            return p(info_type);
        }
    }
    return 0;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the registered
 * BESInfo builders and the default values of the BESInfo objects created.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESInfoList::dump(ostream &strm) const {

    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    strm << BESIndent::LMarg << "BESInfoList::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    if (_info_list.size()) {
        strm << BESIndent::LMarg << "registered builders:" << endl;
        BESIndent::Indent();
        BESInfoList::Info_citer i = _info_list.begin();
        BESInfoList::Info_citer ie = _info_list.end();
        for (; i != ie; i++) {
            p_info_builder p = (*i).second;
            if (p) {
                BESInfo *info = p("dump");
                info->dump(strm);
                delete info;
            } else {
                strm << BESIndent::LMarg << "builder is null" << endl;
            }
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "registered builders: none" << endl;
    }
    BESIndent::UnIndent();
}

BESInfoList *BESInfoList::TheList() {
    static BESInfoList infoList;
    return &infoList;
}
