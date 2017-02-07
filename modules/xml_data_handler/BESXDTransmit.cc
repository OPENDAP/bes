// BESXDTransmit.cc

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

// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <BaseType.h>
#include <Sequence.h>
#include <ConstraintEvaluator.h>
#include <DataDDS.h>

#include <escaping.h>
#include <InternalErr.h>
#include <util.h>
#include <mime_util.h>
#include <XMLWriter.h>

#include <BESDapTransmit.h>
#include <BESContainer.h>
#include <BESDataNames.h>
#include <BESDataDDSResponse.h>
#include <BESDapError.h>
#include <BESInternalFatalError.h>
#include <BESDebug.h>
#include <DapFunctionUtils.h>

#include "BESXDTransmit.h"
#include "get_xml_data.h"

using namespace xml_data;
using namespace libdap;

void BESXDTransmit::send_basic_ascii(BESResponseObject * obj, BESDataHandlerInterface & dhi)
{
    BESDEBUG("xd", "BESXDTransmit::send_base_ascii() - BEGIN" << endl);
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(obj);
    if (!bdds)
        throw BESInternalFatalError("Expected a BESDataDDSResponse instance.", __FILE__, __LINE__);

    DDS *dds = bdds->get_dds();
    ConstraintEvaluator & ce = bdds->get_ce();

    dhi.first_container();

    string constraint = www2id(dhi.data[POST_CONSTRAINT], "%", "%20%26");

    try {
        BESDEBUG("xd", "BESXDTransmit::send_base_ascii() - " "parsing constraint: " << constraint << endl);
        ce.parse_constraint(constraint, *dds);
    }
    catch (InternalErr &e) {
        string err = "Failed to parse the constraint expression: " + e.get_error_message();
        throw BESDapError(err, true, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (Error &e) {
        string err = "Failed to parse the constraint expression: " + e.get_error_message();
        throw BESDapError(err, false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (...) {
        string err = (string) "Failed to parse the constraint expression: " + "Unknown exception caught";
        throw BESInternalFatalError(err, __FILE__, __LINE__);
    }

    BESDEBUG("xd", "BESXDTransmit::send_base_ascii() - " "tagging sequences" << endl);
    dds->tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

    BESDEBUG("xd", "BESXDTransmit::send_base_ascii() - " "accessing container" << endl);
    string dataset_name = dhi.container->access();

    BESDEBUG("xd", "BESXDTransmit::send_base_ascii() - dataset_name = " << dataset_name << endl);

    bool functional_constraint = false;
    try {
        // Handle *functional* constraint expressions specially
        if (ce.function_clauses()) {
            BESDEBUG("xd", "BESXDTransmit::send_base_ascii() Processing functional constraint clause(s)." << endl);
            DDS *tmp_dds = ce.eval_function_clauses(*dds);
            delete dds;
            dds = tmp_dds;
            bdds->set_dds(dds);
            // This next step utilizes a well known function, promote_function_output_structures()
            // to look for one or more top level Structures whose name indicates (by way of ending
            // with "_uwrap") that their contents should be promoted (aka moved) to the top level.
            // This is in support of a hack around the current API where server side functions
            // may only return a single DAP object and not a collection of objects. The name suffix
            // "_unwrap" is used as a signal from the function to the the various response
            // builders and transmitters that the representation needs to be altered before
            // transmission, and that in fact is what happens in our friend
            // promote_function_output_structures()
            promote_function_output_structures(dds);
        }
        else {
            // Iterate through the variables in the DataDDS and read
            // in the data if the variable has the send flag set.
            for (DDS::Vars_iter i = dds->var_begin(); i != dds->var_end(); i++) {
                if ((*i)->send_p()) {
                    (*i)->intern_data(ce, *dds);
                }
            }
        }
    }
    catch (InternalErr &e) {
        if (functional_constraint)
            delete dds;
        string err = "Failed to read data: " + e.get_error_message();
        throw BESDapError(err, true, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (Error & e) {
        if (functional_constraint)
            delete dds;
        string err = "Failed to read data: " + e.get_error_message();
        throw BESDapError(err, false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (BESError &e) {
        throw;
    }
    catch (...) {
        if (functional_constraint)
            delete dds;
        string err = "Failed to read data: Unknown exception caught";
        throw BESInternalFatalError(err, __FILE__, __LINE__);
    }

    try {
        // Now that we have constrained the DataDDS and read in the data,
        // send it as ascii
        BESDEBUG("xd", "converting to xd datadds" << endl);
        DDS *xd_dds = dds_to_xd_dds(dds);

        BESDEBUG("xd", "getting xd values" << endl);
        XMLWriter writer;
        get_data_values_as_xml(xd_dds, &writer);
        dhi.get_output_stream() << writer.get_doc();

        BESDEBUG("xd", "got the ascii values" << endl);
        dhi.get_output_stream() << flush;
        delete xd_dds;

        BESDEBUG("xd", "done transmitting ascii" << endl);
    }
    catch (InternalErr &e) {
        if (functional_constraint)
            delete dds;
        string err = "Failed to get values as ascii: " + e.get_error_message();
        throw BESDapError(err, true, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (Error &e) {
        if (functional_constraint)
            delete dds;
        string err = "Failed to get values as ascii: " + e.get_error_message();
        throw BESDapError(err, false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (BESError &e) {
        throw;
    }
    catch (...) {
        if (functional_constraint)
            delete dds;
        string err = "Failed to get values as ascii: Unknown exception caught";
        throw BESInternalFatalError(err, __FILE__, __LINE__);
    }

    if (functional_constraint)
        delete dds;
}


