// BESDapResponse.cc

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

#include "BESDapResponse.h"
#include "BESContextManager.h"
#include "BESConstraintFuncs.h"
#include "BESDataNames.h"
#include "BESError.h"
#include "BESDebug.h"

using std::endl;
using std::string;
using std::ostream;

#define MODULE "dap"
#define prolog std::string("BESDapResponse::").append(__func__).append("() - ")

 /** @brief Extract the dap protocol from the setContext information
  * This method checks four contexts: dap_explicit_containers, dap_format and
  * xdap_accept, and xml:base
  *
  * If given, the boolean value of dap_explicit_containers is used. If that's
  * not given then look for dap_format and if that's not given default to
  * true. The OLFS should always send this to make Hyrax work the way DAP
  * clients expect.
  *
  * xdap_accept is the value of the DAP that clients can grok. It defaults to
  * "2.0"
  *
  * @note This value will be passed on to the DDS so that it can correctly
  * build versions of the DDX which are specified by DAP 3.x and 4.x
  */
void BESDapResponse::read_contexts()
{
    bool found = false;
    string context_key;
    string context_value;
    BESDEBUG(MODULE,prolog << "BEGIN" << endl);

    // d_explicit_containers is false by default
    context_key = "dap_explicit_containers";
    context_value = BESContextManager::TheManager()->get_context(context_key, found);
    //BESDEBUG(MODULE,prolog << context_key << ": \"" << context_value  << "\" found: " << found << endl);
    if (found) {
        if (context_value == "yes")
            d_explicit_containers = true;
        else if (context_value == "no")
            d_explicit_containers = false;
        else
            throw BESError("dap_explicit_containers must be yes or no",
            BES_SYNTAX_USER_ERROR, __FILE__, __LINE__);
    }
    else {
        context_key = "dap_format";
        context_value = BESContextManager::TheManager()->get_context(context_key, found);
        //BESDEBUG(MODULE,prolog << context_key << ": \"" << context_value  << "\" found: " << found << endl);
        if (found) {
            if (context_value == "dap2")
                d_explicit_containers = false;
            else
                d_explicit_containers = true;
        }
    }
    BESDEBUG(MODULE,prolog << "d_explicit_containers: " <<  (d_explicit_containers?"true":"false") << endl);

    context_key = "xdap_accept";
    context_value = BESContextManager::TheManager()->get_context(context_key, found);
    //BESDEBUG(MODULE,prolog << context_key << ": \"" << context_value  << "\" found: " << found << endl);
    if (found) d_dap_client_protocol = context_value;
    BESDEBUG(MODULE,prolog << "d_dap_client_protocol: " <<  d_dap_client_protocol << endl);

    context_key = "xml:base";
    context_value = BESContextManager::TheManager()->get_context(context_key, found);
    //BESDEBUG(MODULE,prolog << context_key << ": \"" << context_value  << "\" found: " << found << endl);
    if (found) d_request_xml_base = context_value;
    BESDEBUG(MODULE,prolog << "d_request_xml_base: " <<  d_request_xml_base << endl);

    BESDEBUG(MODULE,prolog << "END" << endl);
}

/** @brief See get_explicit_containers()

 @see get_explicit_containers()
 @see get_dap_client_protocol()
 @deprecated
 @return true if dap2 format, false otherwise
 */
bool BESDapResponse::is_dap2()
{
    return !d_explicit_containers;
}

/** @brief set the constraint depending on the context
 *
 * If the context is dap2 then the constraint will be the constraint of
 * the current container. If not dap2 and we have multiple containers
 * then the constraint of the current container must be added to the
 * current post constraint
 *
 * @param dhi The BESDataHandlerInterface of the request. THis holds the
 * current container and the current post constraint
 */
void BESDapResponse::set_constraint(BESDataHandlerInterface &dhi)
{
    if (dhi.container) {
        if (is_dap2()) {
            dhi.data[POST_CONSTRAINT] = dhi.container->get_constraint();
        }
        else {
            BESConstraintFuncs::post_append(dhi);
        }
    }
}

/** @brief set the constraint depending on the context
 *
 * If the context is dap2 then the constraint will be the constraint of
 * the current container. If not dap2 and we have multiple containers
 * then the constraint of the current container must be added to the
 * current post constraint
 *
 * @param dhi The BESDataHandlerInterface of the request. THis holds the
 * current container and the current post constraint
 */
void BESDapResponse::set_dap4_constraint(BESDataHandlerInterface &dhi)
{
    if (dhi.container) {
        dhi.data[DAP4_CONSTRAINT] = dhi.container->get_dap4_constraint();
    }
}

/** @brief set the constraint depending on the context
 *
 * If the context is dap2 then the constraint will be the constraint of
 * the current container. If not dap2 and we have multiple containers
 * then the constraint of the current container must be added to the
 * current post constraint
 *
 * @param dhi The BESDataHandlerInterface of the request. THis holds the
 * current container and the current post constraint
 */
void BESDapResponse::set_dap4_function(BESDataHandlerInterface &dhi)
{
    if (dhi.container) {
        dhi.data[DAP4_FUNCTION] = dhi.container->get_dap4_function();
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the das object
 * created
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDapResponse::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESDapResponse::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "d_explicit_containers: " << d_explicit_containers << endl;
    strm << BESIndent::LMarg << "d_dap_client_protocol: " << d_dap_client_protocol << endl;
    BESIndent::UnIndent();

    BESIndent::UnIndent();
}

