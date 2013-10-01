// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

//////////////////////////////////////////////////////////////////////////////
// Copyright 1996, by the California Institute of Technology.
// ALL RIGHTS RESERVED. United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the
// Office of Technology Transfer at the California Institute of
// Technology. This software may be subject to U.S. export control
// laws and regulations. By accepting this software, the user
// agrees to comply with all applicable U.S. export laws and
// regulations. User has the responsibility to obtain export
// licenses, or other export authority as may be required before
// exporting such information to foreign countries or providing
// access to foreign persons.

// U.S. Government Sponsorship under NASA Contract
// NAS7-1260 is acknowledged.
//
// Author: Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: hcutil.cc,v $ - misc utility routines for HDFClass
//
//////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <string>
#include <vector>

using std::vector;
using std::string;

#include <mfhdf.h>

#include <BESDebug.h>

#if 0

// This function is not used and is broken. The loop depends on i being less
// than zero for termination, but i is an unsigned type.
vector < string > split(const string & str, const string & delim)
{
    vector < string > rv;

    string::size_type len = str.length();
    string::size_type dlen = delim.length();
    for (string::size_type i = 0, previ = -dlen;; previ = i) {
        i = str.find(delim, previ + dlen);
        if (i == 0)
            continue;
        if (i < 0) {
            if (previ + dlen < len)
                rv.push_back(str.
                             substr(previ + dlen, (len - previ - dlen)));
            break;
        }
        rv.push_back(str.substr(previ + dlen, (i - previ - dlen)));
    }

    return rv;
}
#endif

string join(const vector < string > &sv, const string & delim)
{
    string str;
    if (sv.size() > 0) {
        str = sv[0];
        for (int i = 1; i < (int) sv.size(); ++i)
            str += (delim + sv[i]);
    }
    return str;
}

bool SDSExists(const char *filename, const char *sdsname)
{

    int32 sd_id, index;
    if ((sd_id = SDstart(filename, DFACC_RDONLY)) < 0) {
        BESDEBUG("h4", "hcutil:96 SDstart for " << filename << " error" << endl);
        return false;
    }

    index = SDnametoindex(sd_id, (char *) sdsname);
    if (SDend(sd_id) < 0)
        BESDEBUG("h4", "hcutil: SDend error: " << HEstring((hdf_err_code_t)HEvalue(1)) << endl);

    return (index >= 0);
}

bool GRExists(const char *filename, const char *grname)
{

    int32 file_id, gr_id, index;
    if ((file_id = Hopen(filename, DFACC_RDONLY, 0)) < 0)
        return false;
    if ((gr_id = GRstart(file_id)) < 0)
        return false;

    index = GRnametoindex(gr_id, (char *) grname);
    GRend(gr_id);
    Hclose(file_id);

    return (index >= 0);
}

bool VdataExists(const char *filename, const char *vdname)
{

    int32 file_id, ref;
    if ((file_id = Hopen(filename, DFACC_RDONLY, 0)) < 0)
        return false;
    Vstart(file_id);
    ref = VSfind(file_id, vdname);
    Vend(file_id);
    Hclose(file_id);

    return (ref > 0);
}
