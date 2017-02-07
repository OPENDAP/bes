/*
 * test_send_data.cc
 *
 *  Created on: Feb 3, 2013
 *      Author: jimg
 */

#include <iostream>
#include <fstream>

#include <DataDDS.h>
#include <ConstraintEvaluator.h>

#include <BESDapResponseBuilder.h>

using namespace libdap;

/**
 * Given a DataDDS and a file name, write the DAP2 DAS (aka .das)
 * response to that file. Do not write the MIME headers.
 */
void build_das_response(DDS **dds, const string &file_name)
{
    BESDapResponseBuilder rb;
    ofstream dods_strm(file_name.c_str(), ios::out | ios::trunc);
    ConstraintEvaluator eval_dods;
    rb.send_das(dods_strm, dds, eval_dods, false, false);
}

/**
 * Given a DataDDS and a file name, write the DAP2 Data (aka .dods)
 * response to that file. Do not write the MIME headers.
 */
void build_dods_response(DDS **dds, const string &file_name)
{
    for (DDS::Vars_citer i = (*dds)->var_begin(), e = (*dds)->var_end(); i != e; ++i) {
        cerr << (*i)->name() << " read_p: " << (*i)->read_p() << endl;
        // already done in set_value(). jhrg 11/27/15 (*i)->set_read_p(true);
        (*i)->set_send_p(false);
    }
    BESDapResponseBuilder rb;
    ofstream dods_strm(file_name.c_str(), ios::out | ios::trunc);
    ConstraintEvaluator eval_dods;
    rb.send_dap2_data(dods_strm, dds, eval_dods, false);
}
