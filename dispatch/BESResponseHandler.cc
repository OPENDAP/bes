// BESResponseHandler.cc

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

#include "BESResponseHandler.h"

#include "BESDataHandlerInterface.h"
#include "BESResponseObject.h"
#include "BESTransmitter.h"
#include <utility>

#include "TheBESKeys.h"

using std::endl;
using std::ostream;
using std::string;

const string annotation_service_url = "BES.AnnotationServiceURL";

BESResponseHandler::BESResponseHandler(string name) : d_response_name(std::move(name)) {
    d_annotation_service_url = TheBESKeys::read_string_key(annotation_service_url, "");
}

BESResponseHandler::~BESResponseHandler() { delete d_response_object; }

BESResponseObject *BESResponseHandler::get_response_object() { return d_response_object; }

BESResponseObject *BESResponseHandler::set_response_object(BESResponseObject *new_response) {
    BESResponseObject *curr_obj = d_response_object;
    d_response_object = new_response;
    return curr_obj;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the name of this
 * response handler and, if present, dumps the response object itself.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESResponseHandler::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESResponseHandler::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "response name: " << d_response_name << endl;
    if (d_response_object) {
        strm << BESIndent::LMarg << "response object:" << endl;
        BESIndent::Indent();
        d_response_object->dump(strm);
        BESIndent::UnIndent();
    } else {
        strm << BESIndent::LMarg << "response object: not set" << endl;
    }
    BESIndent::UnIndent();
}
