// BESInfo.cc

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

#include <cerrno>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstring>

#include "BESInfo.h"
#include "TheBESKeys.h"
#include "BESInternalError.h"

using namespace std;

#define BES_INFO_FILE_BUFFER_SIZE 4096

/** @brief constructs a BESInfo object
 *
 * By default, informational responses are buffered, so the output stream is
 * created
 */
BESInfo::BESInfo() :
        _strm(0), _strm_owned(false), _buffered(true), _response_started(false) {
    _strm = new ostringstream;
    _strm_owned = true;
}

/** @brief constructs a BESInfo object
 *
 * If the passed key is set to true, True, TRUE, yes, Yes, or YES then the
 * information will be buffered, otherwise it will not be buffered.
 *
 * If the information is not to be buffered then the output stream is set to
 * standard output.
 *
 * @param key parameter from BES configuration file
 * @param strm if not buffered then use the passed stream to output information
 * @param strm_owned if stream was created (not cout or cerr for example) then either take
 * ownership or not
 */
BESInfo::BESInfo(const string &key, ostream *strm, bool strm_owned) :
        _strm(0), _strm_owned(false), _buffered(true), _response_started(false), _response_name("") {
    bool found = false;
    vector<string> vals;
    string b;
    TheBESKeys::TheKeys()->get_value(key, b, found);
    if (b == "true" || b == "True" || b == "TRUE" || b == "yes" || b == "Yes" || b == "YES") {
        _strm = new ostringstream;
        _strm_owned = true;
        _buffered = true;
        if (strm && strm_owned) delete strm;
    } else {
        if (!strm) {
            string s = "Informational response not buffered but no stream passed";
            throw BESInternalError(s, __FILE__, __LINE__);
        }
        _strm = strm;
        _strm_owned = strm_owned;
        _buffered = false;
    }
}

BESInfo::~BESInfo() {
    if (_strm && _strm_owned) {
        delete _strm;
        _strm = 0;
    }
}

/** @brief begin the informational response
 *
 * basic setup of the response from abstract class
 *
 * @param response_name name of the response this information represents
 * @param dhi information about the request and response
 */
void
BESInfo::begin_response(const string &response_name, map<string, string> */*attrs*/, BESDataHandlerInterface &/*dhi*/) {
    _response_started = true;
    _response_name = response_name;
}

/** @brief begin the informational response
 *
 * basic setup of the response from abstract class
 *
 * @param response_name name of the response this information represents
 * @param dhi information about the request and response
 */
void BESInfo::begin_response(const string &response_name, BESDataHandlerInterface &/*dhi*/) {
    _response_started = true;
    _response_name = response_name;
}

void BESInfo::end_response() {
    _response_started = false;
    if (_tags.size()) {
        string s = "Not all tags were ended in info response";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
}

void BESInfo::begin_tag(const string &tag_name, map<string, string> */*attrs*/) {
    _tags.push(tag_name);
}

void BESInfo::end_tag(const string &tag_name) {
    if (_tags.empty() || _tags.top() != tag_name) {
        string s = (string) "tag " + tag_name + " already ended or not started";
        throw BESInternalError(s, __FILE__, __LINE__);
    } else {
        _tags.pop();
    }
}

/** @brief add data to this informational object. If buffering is not set then
 * the information is output directly to the output stream.
 *
 * @param s information to be added to this informational response object
 */
void BESInfo::add_data(const string &s) {
    // If not buffered then we've been passed a stream to use.
    // If it is buffered then we created the stream.
    (*_strm) << s;
}

/** @brief add data from a file to the informational object.
 *
 * Adds data from a file to the informational object using the file
 * specified by the passed key string. The key is found from the bes
 * configuration file.
 *
 * If the key does not exist in the initialization file then this
 * information is added to the informational object, no excetion is thrown.
 *
 * If the file does not exist then this information is added to the
 * informational object, no exception is thrown.
 *
 * @param key Key from the initialization file specifying the file to be
 * @param name A description of what is the information being loaded
 */
void BESInfo::add_data_from_file(const string &key, const string &name) {
    bool found = false;
    string file;
    try {
        TheBESKeys::TheKeys()->get_value(key, file, found);
    }
    catch (...) {
        found = false;
    }
    if (found == false) {
        add_data(name + " file key " + key + " not found, information not available\n");
    } else {
        ifstream ifs(file.c_str());
        int myerrno = errno;
        if (!ifs) {
            string serr = name + " file " + file + " not found, information not available: ";
            char *err = strerror(myerrno);
            if (err)
                serr += err;
            else
                serr += "Unknown error";

            serr += "\n";

            add_data(serr);
        } else {
            char line[BES_INFO_FILE_BUFFER_SIZE];
            while (!ifs.eof()) {
                ifs.getline(line, BES_INFO_FILE_BUFFER_SIZE);
                if (!ifs.eof()) {
                    add_data(line);
                    add_data("\n");
                }
            }
            ifs.close();
        }
    }
}

/** @brief add exception information to this informational object
 *
 * Exception information is added differently to different informational
 * objects, such as html, xml, plain text. But, using the other methods of
 * this class we can take care of exceptions here.
 *
 * @param e The exception to add to the informational response object
 * @param admin The contact information for the person
 * responsible for this error
 */
void BESInfo::add_exception(const BESError &error, const string &admin) {
    begin_tag("BESError");
    ostringstream stype;
    stype << error.get_bes_error_type();
    add_tag("Type", stype.str());
    add_tag("Message", error.get_message());
    add_tag("Administrator", admin);

    error.add_my_error_details_to(*this);

    begin_tag("Location");
    add_tag("File", error.get_file());
    ostringstream sline;
    sline << error.get_line();
    add_tag("Line", sline.str());
    end_tag("Location");

    end_tag("BESError");
}

/** @brief print the information from this informational object to the
 * specified stream
 *
 * If the information was not buffered then this method does nothing,
 * otherwise the information is output to the specified ostream.
 *
 * @param strm output to this file descriptor if information buffered.
 */
void BESInfo::print(ostream &strm) {
    if (_buffered) {
        strm << ((ostringstream *) _strm)->str();
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with values of data
 * members.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESInfo::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESInfo::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "response name: " << _response_name << endl;
    strm << BESIndent::LMarg << "is it buffered? " << _buffered << endl;
    strm << BESIndent::LMarg << "has response been started? " << _response_started << endl;
    if (_tags.size()) {
        strm << BESIndent::LMarg << "tags:" << endl;
        BESIndent::Indent();
        stack<string> temp_tags = _tags;
        while (!temp_tags.empty()) {
            string tag = temp_tags.top();
            strm << BESIndent::LMarg << tag << endl;
            temp_tags.pop();
        }
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "tags: empty" << endl;
    }
    BESIndent::UnIndent();
}

