// BESDMRResponse.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Author: Patrick West <pwest@rpi.edu>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "BESDMRResponse.h"
#include "BESDebug.h"

using std::endl;
using std::string;
using std::ostream;

#define MODULE "dap"
#define prolog std::string("BESDMRResponse::").append(__func__).append("() - ")

BESDMRResponse::BESDMRResponse(DMR *dmr) : BESDapResponse(), _dmr(dmr) {
    string xml_base = get_request_xml_base();
    _dmr->set_request_xml_base(xml_base);
    BESDEBUG(MODULE, prolog << "_dmr->request_xml_base(): \"" << _dmr->request_xml_base()
        << "\" (dmr: " << (void *) _dmr << ")" << endl);
}

BESDMRResponse::~BESDMRResponse() {
    delete _dmr ;
}

/** @brief set the container in the DAP response object
 *
 * @param cn name of the current container being operated on
 */
void BESDMRResponse::set_container(const string &/*cn*/)
{
    // FIXME Add container support
#if 0
    if( _dmr && get_explicit_containers() )
    {
        _dmr->container_name( cn );
    }
#endif
}

/** @brief clear the container in the DAP response object
 */
void BESDMRResponse::clear_container()
{
#if 0
    if( _dmr )
    {
        _dmr->container_name( "" );
    }
#endif
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the das object
 * created
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDMRResponse::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESDMRResponse::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (_dmr) {
        strm << BESIndent::LMarg << "DMR:" << endl;
        BESIndent::Indent();
        BESIndent::SetIndent(BESIndent::GetIndent());
        _dmr->dump(strm);
        BESIndent::Reset();
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "DMR: null" << endl;
    }
    BESIndent::UnIndent();
}

