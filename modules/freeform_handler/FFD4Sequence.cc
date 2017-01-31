// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of freeform_handler; a FreeForm API handler for the OPeNDAP
// data server.

// Copyright (c) 2014 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config_ff.h"

#include <sstream>

using std::endl;
using std::ostringstream;

// #define DODS_DEBUG

#include "Error.h"
#include "debug.h"

#include "FFStr.h"
#include "FFD4Sequence.h"
#include "util_ff.h"

extern long BufPtr;
extern char *BufVal;
extern long BufSiz;

#if 0
static long Records(const string &filename)
{
    int error = 0;
    DATA_BIN_PTR dbin = NULL;
    FF_STD_ARGS_PTR SetUps = NULL;
    PROCESS_INFO_LIST pinfo_list = NULL;
    PROCESS_INFO_PTR pinfo = NULL;
    static char Msgt[255];

    SetUps = ff_create_std_args();
    if (!SetUps) {
        return -1;
    }

    /** set the structure values to create the FreeForm DB**/
    SetUps->user.is_stdin_redirected = 0;
    SetUps->input_file = const_cast<char*>(filename.c_str());

    SetUps->output_file = NULL;

    error = SetDodsDB(SetUps, &dbin, Msgt);
    if (error && error < ERR_WARNING_ONLY) {
        db_destroy(dbin);
        return -1;
    }

    ff_destroy_std_args(SetUps);

    error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_DATA,
            &pinfo_list);
    if (error)
        return (-1);

    pinfo_list = dll_first(pinfo_list);

    pinfo = ((PROCESS_INFO_PTR) (pinfo_list)->data.u.pi);

    long num_records = PINFO_SUPER_ARRAY_ELS(pinfo);

    ff_destroy_process_info_list(pinfo_list);
    db_destroy(dbin);

    return num_records;
}
#endif

/** Read a row from the Sequence.

 @note Does not use either the \e in_selection or \e send_p properties. If
 this method is called and the \e read_p property is not true, the values
 are read.

 @exception Error if the size of the returned data is zero.
 @return Always returns false. */
bool FFD4Sequence::read()
{
	DBG(cerr << "Entering FFD4Sequence::read..." << endl);

    if (read_p()) // Nothing to do
        return true;

    if ((BufPtr >= BufSiz) && (BufSiz != 0))
        return true; // End of sequence

    if (!BufVal) { // Make the cache (BufVal is global)
        // Create the output Sequence format
        ostringstream o_fmt;
        int endbyte = 0;
        int stbyte = 1;

        o_fmt << "binary_output_data \"DODS binary output data\"" << endl;
        for (Vars_iter p = var_begin(); p != var_end(); ++p) {
            if ((*p)->synthesized_p())
                continue;
            if ((*p)->type() == dods_str_c)
                endbyte += static_cast<FFStr&>(**p).length();
            else
                endbyte += (*p)->width();

            o_fmt << (*p)->name() << " " << stbyte << " " << endbyte << " " << ff_types((*p)->type()) << " "
                    << ff_prec((*p)->type()) << endl;
            stbyte = endbyte + 1;
        }

        DBG(cerr << o_fmt.str());

        // num_rec could come from DDS if sequence length was known...
        long num_rec = Records(dataset());
        if (num_rec == -1) {
            return true;
        }

        BufSiz = num_rec * (stbyte - 1);
        BufVal = new char[BufSiz];

        long bytes = read_ff(dataset().c_str(), d_input_format_file.c_str(), o_fmt.str().c_str(), BufVal, BufSiz);

        if (bytes == -1)
            throw Error("Could not read requested data from the dataset.");
    }

    for (Vars_iter p = var_begin(); p != var_end(); ++p)
        (*p)->read();

    set_read_p(false);

    return false;
}

void FFD4Sequence::transfer_attributes(AttrTable *at)
{
    if (at) {
        Vars_iter var = var_begin();
        while (var != var_end()) {
            (*var)->transfer_attributes(at);
            var++;
        }
    }
}

