// BESDataResponseHandler.cc

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

#include <cstdlib>
#include <cerrno>
#include <cstring>

#include <DDS.h>
#include <DataDDS.h>

#include "BESDataResponseHandler.h"
#include "BESDataDDSResponse.h"
#include "BESRequestHandlerList.h"
#include "BESDapNames.h"
#include "BESDataNames.h"
#include "BESContextManager.h"
#include "BESInternalError.h"
#include "BESDebug.h"

using namespace libdap;
using namespace std;

BESDataResponseHandler::BESDataResponseHandler( const string &name )
    : BESResponseHandler( name )
{
}

BESDataResponseHandler::~BESDataResponseHandler( )
{
}

/** @brief executes the command 'get data for &lt;def_name&gt;' by
 * executing the request for each container in the specified definition
 *
 * For each container in the specified defnition go to the request
 * handler for that container and have it add to the OPeNDAP DataDDS data
 * response object. The data response object is created within this method
 * and passed to the request handler list.
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESDataDDSResponse
 * @see BESRequestHandlerList
 * @see BESDefine
 */
void BESDataResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    dhi.action_name = DATA_RESPONSE_STR;
    // NOTE: It is the responsibility of the specific request handler to set
    // the BaseTypeFactory. It is set to NULL here
    DataDDS *dds = new DataDDS(NULL, "virtual");
    BESDataDDSResponse *bdds = new BESDataDDSResponse(dds);

    // Set the DAP protocol version requested by the client. 2/25/11 jhrg

    dhi.first_container();

    BESDEBUG("version", "Initial CE: " << dhi.container->get_constraint() << endl);

    // FIXME Keywords should not be used and this should be removed. jhrg 2/20/15
    dhi.container->set_constraint(dds->get_keywords().parse_keywords(dhi.container->get_constraint()));
    BESDEBUG("version", "CE after keyword processing: " << dhi.container->get_constraint() << endl);

    bool found;
    string response_size_limit = BESContextManager::TheManager()->get_context("max_response_size", found);
    if (found && !response_size_limit.empty()) {
        char *endptr;
        errno = 0;
        long rsl = strtol(response_size_limit.c_str(), &endptr, /*int base*/ 10);
        if (rsl == 0 && errno > 0) {
            string err = strerror(errno);
            delete dds; dds = 0;
            delete bdds; bdds = 0;
            throw BESInternalError("The responseSizeLimit context value ("
                    + response_size_limit + ") was bad: " + err, __FILE__, __LINE__);
       }

        dds->set_response_limit(rsl); // The default for this is zero
    }

    // FIXME This should be fixed too... (see above re keywords). jhrg 2/20/15
    if (dds->get_keywords().has_keyword("dap")) {
        dds->set_dap_version(dds->get_keywords().get_keyword_value("dap"));
    }
    else if (!bdds->get_dap_client_protocol().empty()) {
        dds->set_dap_version(bdds->get_dap_client_protocol());
    }

    _response = bdds;
    BESRequestHandlerList::TheList()->execute_each(dhi);
}

/** @brief transmit the response object built by the execute command
 *
 * If a response object was built then transmit it using the send_data
 * method on the specified transmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESDataDDSResponse
 * @see BESTransmitter
 * @see BESDataHandlerInterface
 */
void
BESDataResponseHandler::transmit( BESTransmitter *transmitter,
                                  BESDataHandlerInterface &dhi )
{
    if( _response )
    {
	transmitter->send_response( DATA_SERVICE, _response, dhi ) ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDataResponseHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDataResponseHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESResponseHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESResponseHandler *
BESDataResponseHandler::DataResponseBuilder( const string &name )
{
    return new BESDataResponseHandler( name ) ;
}

