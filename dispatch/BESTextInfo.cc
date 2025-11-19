// BESTextInfo.cc

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

#include <iostream>
#include <map>
#include <sstream>

using std::endl;
using std::map;
using std::ostream;
using std::ostringstream;
using std::string;

#include "BESTextInfo.h"
#include "BESUtil.h"

/** @brief constructs a basic text information response object.
 *
 * Text informational objects can also be sent as html with an html header.
 * We need to know this if the informationl is not buffered. If it isn't
 * buffered then we need to send that text header.
 *
 * @see BESInfo
 * @see BESResponseObject
 */
BESTextInfo::BESTextInfo(bool ishttp) : BESInfo(), _ishttp(ishttp), _header(false) {}

/** @brief constructs a basic text information response object.
 *
 * Uses the default specified key in the bes configuration file to
 * determine whether the information should be buffered or not.
 *
 * Text informational objects can also be sent as html with an html header.
 * We need to know this if the informationl is not buffered. If it isn't
 * buffered then we need to send that text header.
 *
 * @param key parameter from BES configuration file
 * @param strm if not buffered then use the passed stream
 * @param strm_owned if strm created (not cout or cerr for example) then
 * tells whether strm owned or not and can be deleted
 * @param ishttp tells whether this text information is html text and
 * therefor requires the html header
 *
 * @see BESInfo
 * @see BESResponseObject
 */
BESTextInfo::BESTextInfo(const string &key, ostream *strm, bool strm_owned, bool ishttp)
    : BESInfo(key, strm, strm_owned), _ishttp(ishttp), _header(false) {}

BESTextInfo::~BESTextInfo() = default;

/** @brief begin the informational response
 *
 * Because this is text informational object, no begin tags are needed
 *
 * @param response_name name of the response this information represents
 * @param dhi information about the request and response
 */
void BESTextInfo::begin_response(const string &response_name, BESDataHandlerInterface &dhi) {
    BESInfo::begin_response(response_name, dhi);
}

/** @brief add tagged information to the inforamtional response
 *
 * @param tag_name name of the tag to be added to the response
 * @param tag_data information describing the tag
 * @param attrs map of attributes to add to the tag
 */
void BESTextInfo::add_tag(const string &tag_name, const string &tag_data, map<string, string, std::less<>> *attrs) {
    add_data(_indent + tag_name + ": " + tag_data + "\n");
    if (attrs) {
        map<string, string>::const_iterator i = attrs->begin();
        map<string, string>::const_iterator e = attrs->end();
        for (; i != e; i++) {
            string name = (*i).first;
            string val = (*i).second;
            add_data(_indent + "    " + name + ": " + val + "\n");
        }
    }
}

/** @brief begin a tagged part of the information, information to follow
 *
 * @param tag_name name of the tag to begin
 * @param attrs map of attributes to begin the tag with
 */
void BESTextInfo::begin_tag(const string &tag_name, map<string, string, std::less<>> *attrs) {
    BESInfo::begin_tag(tag_name);
    add_data(_indent + tag_name + "\n");
    _indent += "    ";
    if (attrs) {
        map<string, string>::const_iterator i = attrs->begin();
        map<string, string>::const_iterator e = attrs->end();
        for (; i != e; i++) {
            string name = (*i).first;
            string val = (*i).second;
            add_data(_indent + name + ": " + val + "\n");
        }
    }
}

/** @brief end a tagged part of the informational response
 *
 * If the named tag is not the current tag then an error is thrown.
 *
 * @param tag_name name of the tag to end
 */
void BESTextInfo::end_tag(const string &tag_name) {
    BESInfo::end_tag(tag_name);
    if (_indent.size() >= 4)
        _indent = _indent.substr(0, _indent.size() - 4);
}

/** @brief add string data to the informational response
 *
 * @param s string data to add the informational response
 */
void BESTextInfo::add_data(const string &s) {
    if (_ishttp && !_header && !_buffered) {
        BESUtil::set_mime_text(*_strm);
        _header = true;
    }
    BESInfo::add_data(s);
}

/** @brief add a space to the informational response
 *
 * @param num_spaces the number of spaces to add to the information
 */
void BESTextInfo::add_space(unsigned long num_spaces) {
    string to_add;
    for (unsigned long i = 0; i < num_spaces; i++) {
        to_add += " ";
    }
    add_data(to_add);
}

/** @brief add a line break to the information
 *
 * @param num_breaks the number of line breaks to add to the information
 */
void BESTextInfo::add_break(unsigned long num_breaks) {
    string to_add;
    for (unsigned long i = 0; i < num_breaks; i++) {
        to_add += "\n";
    }
    add_data(to_add);
}

/** @brief add data from a file to the informational object
 *
 * This method simply adds a .TXT to the end of the key and passes the
 * request on up to the BESInfo parent class.
 *
 * @param key Key from the initialization file specifying the file to be
 * @param name A description of what is the information being loaded
 */
void BESTextInfo::add_data_from_file(const string &key, const string &name) {
    string newkey = key + ".TXT";
    BESInfo::add_data_from_file(newkey, name);
}

/** @brief transmit the text information as text
 *
 * use the send_text method on the transmitter to transmit the information
 * back to the client.
 *
 * @param transmitter The type of transmitter to use to transmit the info
 * @param dhi information to help with the transmission
 */
void BESTextInfo::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi) {
    transmitter->send_text(*this, dhi);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this text information object
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESTextInfo::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESTextInfo::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "has header been added? " << _header << endl;
    strm << BESIndent::LMarg << "indentation \"" << _indent << "\"" << endl;
    strm << BESIndent::LMarg << "is http? " << _ishttp << endl;
    BESInfo::dump(strm);
    BESIndent::UnIndent();
}

BESInfo *BESTextInfo::BuildTextInfo(const string & /*info_type*/) { return new BESTextInfo(); }
