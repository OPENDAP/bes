/*
 * BESXMLWriter.h
 *
 *  Created on: Jul 28, 2010
 *      Author: jimg
 */

// Copyright (c) 2013 OPeNDAP, Inc. Author: James Gallagher
// <jgallagher@opendap.org>, Patrick West <pwest@opendap.org>
// Nathan Potter <npotter@opendap.org>
//                                                                            
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2.1 of
// the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 U\ SA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI.
// 02874-0112.
#ifndef XMLWRITER_H_
#define XMLWRITER_H_

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include <string>

class BESXMLWriter {
private:
    // Various xml writer stuff
    xmlTextWriterPtr d_writer;
    xmlBufferPtr d_doc_buf;
    bool d_started;
    bool d_ended;
    std::string d_ns_uri;

    std::string d_doc;

    void m_cleanup() ;

public:
    BESXMLWriter();
    virtual ~BESXMLWriter();

    xmlTextWriterPtr get_writer() { return d_writer; }
    // string get_ns_uri() const { return d_ns_uri; }
    const char *get_doc();
};

#endif /* XMLWRITER_H_ */
