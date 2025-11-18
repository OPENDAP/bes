// BESTransmitter.cc

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

#include <algorithm>
#include <iostream>
#include <string>
#include <unistd.h>

#include "BESContextManager.h"
#include "BESDataHandlerInterface.h"
#include "BESInternalError.h"
#include "BESResponseObject.h"

#include "BESDebug.h"
#include "BESInfo.h"
#include "BESUtil.h"
#include "TheBESKeys.h"

#include "BESTransmitter.h"

using std::endl;
using std::ostream;
using std::string;

bool BESTransmitter::add_method(string method_name, p_transmitter trans_method) {
    BESTransmitter::_method_citer i;
    i = _method_list.find(method_name);
    if (i == _method_list.end()) {
        _method_list[method_name] = trans_method;
        return true;
    }
    return false;
}

bool BESTransmitter::remove_method(string method_name) {
    BESTransmitter::_method_iter i;
    i = _method_list.find(method_name);
    if (i != _method_list.end()) {
        _method_list.erase(i);
        return true;
    }
    return false;
}

p_transmitter BESTransmitter::find_method(string method_name) {
    BESTransmitter::_method_citer i;
    i = _method_list.find(method_name);
    if (i != _method_list.end()) {
        p_transmitter p = (*i).second;
        return p;
    }
    return nullptr;
}

void BESTransmitter::send_response(const string &method_name, BESResponseObject *response,
                                   BESDataHandlerInterface &dhi) {
    p_transmitter p = find_method(method_name);
    if (p) {
        p(response, dhi);
    } else {
        throw BESInternalError(string("Unable to transmit response, no transmitter for ") + method_name, __FILE__,
                               __LINE__);
    }
}

void BESTransmitter::send_text(BESInfo &info, BESDataHandlerInterface &dhi) {
    bool found = false;
    string context = "transmit_protocol";
    string protocol = BESContextManager::TheManager()->get_context(context, found);
    if (protocol == "HTTP") {
        if (info.is_buffered()) {
            BESUtil::set_mime_text(dhi.get_output_stream());
        }
    }
    info.print(dhi.get_output_stream());
}

void BESTransmitter::send_html(BESInfo &info, BESDataHandlerInterface &dhi) {
    bool found = false;
    string context = "transmit_protocol";
    string protocol = BESContextManager::TheManager()->get_context(context, found);
    if (protocol == "HTTP") {
        if (info.is_buffered()) {
            BESUtil::set_mime_html(dhi.get_output_stream());
        }
    }
    info.print(dhi.get_output_stream());
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the names of
 * register transmit methods
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESTransmitter::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESTransmitter::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    if (_method_list.size()) {
        strm << BESIndent::LMarg << "registered methods:" << endl;
        BESIndent::Indent();
        _method_citer i = _method_list.begin();
        _method_citer ie = _method_list.end();
        for (; i != ie; i++) {
            strm << BESIndent::LMarg << (*i).first << ": " << (void *)(*i).second << endl;
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "registered methods: none" << endl;
    }
    BESIndent::UnIndent();
}
