// BESDDXResponseHandler.cc

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

#include <DDS.h>

#include "BESDDXResponseHandler.h"
#include "BESDASResponse.h"
#include "BESDDSResponse.h"
#include "BESDapNames.h"
#include "BESDataNames.h"
#include "BESRequestHandlerList.h"

#include "BESDebug.h"

using namespace libdap;

BESDDXResponseHandler::BESDDXResponseHandler(const string &name) :
        BESResponseHandler(name)
{
}

BESDDXResponseHandler::~BESDDXResponseHandler()
{
}

/** @brief executes the command 'get ddx for def_name;'
 *
 * For each container in the specified definition go to the request
 * handler for that container and have it first add to the OPeNDAP DDS response
 * object. Once the DDS object has been filled in, repeat the process but
 * this time for the OPeNDAP DAS response object. Then add the attributes from
 * the DAS object to the DDS object.
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESDDSResponse
 * @see BESDASResponse
 * @see BESRequestHandlerList
 */
void BESDDXResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    BESDEBUG( "dap", "Entering BESDDXResponseHandler::execute" << endl );

    dhi.action_name = DDX_RESPONSE_STR;
    // Create the DDS.
    // NOTE: It is the responsibility of the specific request handler to set
    // the BaseTypeFactory. It is set to NULL here
    DDS *dds = new DDS(NULL, "virtual");

    BESDDSResponse *bdds = new BESDDSResponse(dds);
    _response = bdds;
    _response_name = DDS_RESPONSE;
    dhi.action = DDS_RESPONSE;

    BESDEBUG( "bes", "about to set dap version to: " << bdds->get_dap_client_protocol() << endl);
    BESDEBUG( "bes", "about to set xml:base to: " << bdds->get_request_xml_base() << endl);

    // I added these two lines from BESDDXResponse. jhrg 10/05/09
    // Note that the get_dap_client_protocol(), ..., methods
    // are defined in BESDapResponse - these are not the methods of the
    // same name in DDS. 2/23/11 jhrg

    // Set the DAP protocol version requested by the client. 2/25/11 jhrg

    dhi.first_container();
    BESDEBUG("version", "Initial CE: " << dhi.container->get_constraint() << endl);
    dhi.container->set_constraint(dds->get_keywords().parse_keywords(dhi.container->get_constraint()));
    BESDEBUG("version", "CE after keyword processing: " << dhi.container->get_constraint() << endl);

    if (dds->get_keywords().has_keyword("dap")) {
        BESDEBUG("version",
                "Has keyword 'dap', setting version to: " << dds->get_keywords().get_keyword_value("dap") << endl);
        dds->set_dap_version(dds->get_keywords().get_keyword_value("dap"));
    }
    else if (!bdds->get_dap_client_protocol().empty()) {
        BESDEBUG("version",
                "Has non-empty dap version info in bdds, setting version to: " << bdds->get_dap_client_protocol() << endl);
        dds->set_dap_version(bdds->get_dap_client_protocol());
    }
    else {
        BESDEBUG("version", "Has no clue about dap version, using default." << endl);
    }

    dds->set_request_xml_base(bdds->get_request_xml_base());

    BESRequestHandlerList::TheList()->execute_each(dhi);

    dhi.action = DDX_RESPONSE;
    _response = bdds;

    BESDEBUG( "dap", "Leaving BESDDXResponseHandler::execute" << endl);
}

/** @brief transmit the response object built by the execute command
 *
 * If a response object was built then transmit it using the send_ddx method
 * on the specified transmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DDS
 * @see DAS
 * @see BESTransmitter
 * @see BESDataHandlerInterface
 */
void BESDDXResponseHandler::transmit(BESTransmitter * transmitter, BESDataHandlerInterface & dhi)
{
    if (_response) {
        transmitter->send_response(DDX_SERVICE, _response, dhi);
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDDXResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESDDXResponseHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
BESDDXResponseHandler::DDXResponseBuilder(const string &name)
{
    return new BESDDXResponseHandler(name);
}

