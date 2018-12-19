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
#include "BESResponseObject.h"
#include "BESDataHandlerInterface.h"
#include "BESTransmitter.h"

#include "TheBESKeys.h"

// Experimental: Annotation service URL. Set this parameter to the URL
// of an annotation service. If the value is not null, then a global
// attribute will be added to the DAS/DMR response for every dataset
// available using this server so that clients can access the annotation
// service. The name of the attribute will be 'Annotation'.

// BES.AnnotationServerURL = http://localhost:8083/Feedback/form

const string annotation_service_url = "BES.AnnotationServiceURL";

#if 0
// Experimental: Include the current dataset URL in the Query Parameter
// of the AnnotationServiceURL. This will make the value of the 'Annotation'
// attribute have the form: http://localhost:8083/Feedback/form?url=<url>.
// This has no effect if the BES.AnnotationServerURL parameter is null.

const string include_dataset_in_annotation_url = "BES.IncludeDatasetInAnnotationURL";
#endif

BESResponseHandler::BESResponseHandler(const string &name) :
    d_response_name(name), d_response_object(0)
{
    d_annotation_service_url = TheBESKeys::TheKeys()->read_string_key(annotation_service_url, "");
#if 0
    // see comment in header. jhrg 12/19/18
    d_include_dataset_in_annotation_url = TheBESKeys::TheKeys()->read_bool_key(include_dataset_in_annotation_url, false);
#endif
}

BESResponseHandler::~BESResponseHandler()
{
    delete d_response_object;
}


BESResponseObject *
BESResponseHandler::get_response_object()
{
    return d_response_object;
}

BESResponseObject *
BESResponseHandler::set_response_object(BESResponseObject *new_response)
{
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
void BESResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESResponseHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "response name: " << d_response_name << endl;
    if (d_response_object) {
        strm << BESIndent::LMarg << "response object:" << endl;
        BESIndent::Indent();
        d_response_object->dump(strm);
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "response object: not set" << endl;
    }
    BESIndent::UnIndent();
}

