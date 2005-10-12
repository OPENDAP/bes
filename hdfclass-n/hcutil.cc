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
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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

using std::vector ;
using std::string ;

#include <mfhdf.h>

vector<string> split(const string& str, const string& delim) {
    vector<string> rv;

    string::size_type len = str.length();
    string::size_type dlen = delim.length();
    for (string::size_type i=0, previ=-dlen; ;previ = i) {
	i = str.find(delim, previ+dlen);
	if (i == 0)
	    continue;
	if (i < 0) {
	    if (previ+dlen < len)
	        rv.push_back(str.substr(previ+dlen,(len-previ-dlen)));
	    break;
	}
	rv.push_back(str.substr(previ+dlen,(i-previ-dlen)));
    }

    return rv;
}

string join(const vector<string>& sv, const string& delim) {
    string str;
    if (sv.size() > 0) {
	str = sv[0];
	for (int i=1; i<(int)sv.size(); ++i)
	    str += (delim + sv[i]);
    }
    return str;
}

bool SDSExists(const char *filename, const char *sdsname) {

    int32 sd_id, index;
    if ( (sd_id = SDstart(filename, DFACC_RDONLY)) < 0)
	return false;
    
    index = SDnametoindex(sd_id, (char *)sdsname);
    SDend(sd_id);

    return (index >= 0);
}

bool GRExists(const char *filename, const char *grname) {

    int32 file_id, gr_id, index;
    if ( (file_id = Hopen(filename, DFACC_RDONLY, 0)) < 0)
	return false;
    if ( (gr_id = GRstart(file_id)) < 0)
	return false;
    
    index = GRnametoindex(gr_id, (char *)grname);
    GRend(gr_id);
    Hclose(file_id);

    return (index >= 0);
}

bool VdataExists(const char *filename, const char *vdname) {

    int32 file_id, ref;
    if ( (file_id = Hopen(filename, DFACC_RDONLY, 0)) < 0)
	return false;
    Vstart(file_id);
    ref = VSfind(file_id, vdname);
    Vend(file_id);
    Hclose(file_id);
    
    return (ref > 0);
}

// $Log: hcutil.cc,v $
// Revision 1.6.4.1  2003/05/21 16:26:58  edavis
// Updated/corrected copyright statements.
//
// Revision 1.6  2003/01/31 02:08:37  jimg
// Merged with release-3-2-7.
//
// Revision 1.5.4.1  2002/12/18 23:32:50  pwest
// gcc3.2 compile corrections, mainly regarding the using statement. Also,
// missing semicolon in .y file
//
// Revision 1.5  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.4  2000/03/09 01:44:33  jimg
// merge with 3.1.3
//
// Revision 1.3.8.1  2000/03/09 00:24:59  jimg
// Replaced int and uint32 with string::size_type
//
// Revision 1.3  1999/05/06 03:23:33  jimg
// Merged changes from no-gnu branch
//
// Revision 1.2  1999/05/05 23:33:43  jimg
// String --> string conversion
//
// Revision 1.1.20.1  1999/05/06 00:35:45  jimg
// Jakes String --> string changes
//
// Revision 1.1  1996/10/31 18:43:02  jimg
// Added.
//
//
