// BESAsciiTransmit.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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

#include <memory>

#include <libdap/BaseType.h>
#include <libdap/Sequence.h>
#include <libdap/ConstraintEvaluator.h>
#include <libdap/D4Group.h>
#include <libdap/DMR.h>
#include <D4ConstraintEvaluator.h>
#include <libdap/D4BaseTypeFactory.h>
#include <libdap/ServerFunctionsList.h>
#include <D4FunctionEvaluator.h>
#include <libdap/crc.h>
#include <libdap/InternalErr.h>
#include <libdap/util.h>
#include <libdap/escaping.h>
#include <libdap/mime_util.h>

#include <BESUtil.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDapTransmit.h>
#include <BESContainer.h>
#include <BESDataDDSResponse.h>
#include <BESDMRResponse.h>
#include <BESDapResponseBuilder.h>

#include <BESError.h>
#include <BESDapError.h>
#include <BESForbiddenError.h>
#include <BESInternalFatalError.h>
#include <DapFunctionUtils.h>

#include <BESDebug.h>

#include "RequestServiceTimer.h"

#include "BESAsciiTransmit.h"
#include "get_ascii.h"
#include "get_ascii_dap4.h"

using namespace dap_asciival;
using namespace libdap;

#define MODULE "ascii"
#define prolog string("BESAsciiTransmit::").append(__func__).append("() - ")

BESAsciiTransmit::BESAsciiTransmit() :
        BESTransmitter()
{
    add_method(DATA_SERVICE, BESAsciiTransmit::send_basic_ascii);
    add_method(DAP4DATA_SERVICE, BESAsciiTransmit::send_dap4_csv);
}

void BESAsciiTransmit::send_basic_ascii(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG("ascii", "BESAsciiTransmit::send_basic_ascii() - BEGIN" << endl);

    try { // Expanded try block so all DAP errors are caught. ndp 12/23/2015
        BESDapResponseBuilder responseBuilder;

        // Use the DDS from the ResponseObject along with the parameters
        // from the DataHandlerInterface to load the DDS with values.
        // Note that the BESResponseObject will manage the loaded_dds object's
        // memory. Make this a shared_ptr<>. jhrg 9/6/16
        DDS *loaded_dds = responseBuilder.intern_dap2_data(obj, dhi);

        // Send data values as CSV/ASCII
        unique_ptr<DDS> ascii_dds(datadds_to_ascii_datadds(loaded_dds));  // unique_ptr<> jhrg 9/6/16

        // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed.
        RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog + " Ready to start streaming", __FILE__, __LINE__);
        BESUtil::conditional_timeout_cancel();

        get_data_values_as_ascii(ascii_dds.get(), dhi.get_output_stream());
        dhi.get_output_stream() << flush;
    }
    catch (Error &e) {
        throw BESDapError("Failed to get values as ascii: " + e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (BESError &e) {
        throw;
    }
    catch (std::exception &e) {
        throw BESInternalError("Failed to read data: STL Error: " + string(e.what()), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalError("Failed to get values as ascii: Unknown exception caught", __FILE__, __LINE__);
    }

    BESDEBUG("ascii", "Done BESAsciiTransmit::send_basic_ascii()" << endl);
}

/**
 * Handle parsing the CE and writing the ASCII/CSV representation.
 * This method can be called using a function result or the original
 * DMR.
 */
void BESAsciiTransmit::send_dap4_csv_helper(ostream &out, DMR *dmr, const string &dap4Constraint) {
    if (!dap4Constraint.empty()) {
        D4ConstraintEvaluator d4ce(dmr);
        bool parse_ok = d4ce.parse(dap4Constraint);
        if (!parse_ok) throw Error(malformed_expr, "Constraint Expression (" + dap4Constraint + ") failed to parse.");
    }
    else {
        dmr->root()->set_send_p(true);
    }

    if (dmr->response_limit() != 0 && (dmr->request_size(true) > dmr->response_limit())) {
        string msg = "The Request for " + long_to_string(dmr->request_size(true))
                     + "KB is too large; requests for this server are limited to " +
                     long_to_string(dmr->response_limit())
                     + "KB.";
        throw Error(msg);
    }

    // Verify the request hasn't exceeded bes_timeout.
    RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog + " bes-timeout: ready to start streaming", __FILE__, __LINE__);

    print_values_as_ascii(dmr, out);
    out << flush;
}

/**
 * Transmits DAP4 Data as Comma Separated Values
 */
void BESAsciiTransmit::send_dap4_csv(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG("ascii", "BESAsciiTransmit::send_dap4_csv" << endl);

    BESDMRResponse *bdmr = dynamic_cast<BESDMRResponse *>(obj);
    if (!bdmr) throw BESInternalFatalError("Expected a BESDMRResponse instance.", __FILE__, __LINE__);

    DMR *dmr = bdmr->get_dmr();

    string dap4Constraint = www2id(dhi.data[DAP4_CONSTRAINT], "%", "%20%26");
    string dap4Function = www2id(dhi.data[DAP4_FUNCTION], "%", "%20%26");

    // Not sure we need this...
    dhi.first_container();

    try {
        // @TODO Handle *functional* constraint expressions specially
        // Use the D4FunctionDriver class and evaluate the functions, building
        // a new DMR, then evaluate the D4CE in the context of that DMR.
        // This might be coded as "if (there's a function) do this else process the CE".
        // Or it might be coded as "if (there's a function) build the new DMR, then fall
        // through and process the CE but on the new DMR". jhrg 9/3/14
        if (!dap4Function.empty()) {
            D4BaseTypeFactory d4_factory;
            DMR function_result(&d4_factory, "function_results");

            // Function modules load their functions onto this list. The list is
            // part of libdap, not the BES.
            if (!ServerFunctionsList::TheList())
                throw Error(
                        "The function expression could not be evaluated because there are no server functions defined on this server");

            D4FunctionEvaluator parser(dmr, ServerFunctionsList::TheList());
            bool parse_ok = parser.parse(dap4Function);
            if (!parse_ok) throw Error("Function Expression (" + dap4Function + ") failed to parse.");

            parser.eval(&function_result);

            // Now use the results of running the functions for the remainder of the
            // send_data operation.
            send_dap4_csv_helper(dhi.get_output_stream(), &function_result, dap4Constraint);
        }
        else {
            send_dap4_csv_helper(dhi.get_output_stream(), dmr, dap4Constraint);
        }

#if 0
        // ORIG HERE

        if (!dap4Constraint.empty()) {
            D4ConstraintEvaluator d4ce(dmr);
            bool parse_ok = d4ce.parse(dap4Constraint);
            if (!parse_ok) throw Error(malformed_expr, "Constraint Expression (" + dap4Constraint + ") failed to parse.");
        }
        else {
            dmr->root()->set_send_p(true);
        }

        if (dmr->response_limit() != 0 && (dmr->request_size(true) > dmr->response_limit())) {
            string msg = "The Request for " + long_to_string(dmr->request_size(true))
                + "KB is too large; requests for this server are limited to " + long_to_string(dmr->response_limit())
                + "KB.";
            throw Error(msg);
        }

        // Now that we are ready to start building the response data we
        // cancel any pending timeout alarm according to the configuration.
        BESUtil::conditional_timeout_cancel();

        print_values_as_ascii(dmr, dhi.get_output_stream());
        dhi.get_output_stream() << flush;
#endif
    }
    catch (Error &e) {
        throw BESDapError("Failed to return values as ascii: " + e.get_error_message(), false, e.get_error_code(),__FILE__, __LINE__);
    }
    catch (BESError &e){
        throw;
    }
    catch (...) {
        throw BESInternalError("Failed to return values as ascii: Unknown exception caught", __FILE__, __LINE__);
    }

    BESDEBUG("ascii", "Done BESAsciiTransmit::send_dap4_csv" << endl);
}

