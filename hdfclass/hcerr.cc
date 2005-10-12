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
// $RCSfile: hcerr.cc,v $ - implementation of hcerr class
// 
//////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <hdf.h>
#ifdef __POWERPC__
#undef isascii
#endif

#include <iostream>

#include <hdf.h>
#include <hcerr.h>

using namespace std;
 
//#ifdef NO_EXCEPTIONS
void fakethrow(const hcerr& e) {
    cerr << "A fatal exception has been thrown:"  << endl;
    cerr << e;
    exit(1);
}
//#endif

// print out value of exception
void hcerr::_print(ostream &out) const {
    out << "Exception:    " << _errmsg << endl 
	<< "Location: \"" << _file << "\", line " << _line << endl;
    out << "HDF Error stack:" << endl;
    for (int i=0; i<5; ++i)
	out << i << ") " << HEstring((hdf_err_code_t)HEvalue(i)) << endl;;
    return;
}

// stream the value of the exception to out
ostream& operator<<(ostream& out, const hcerr& x) {
    x._print(out);
    return out;
}

// $Log: hcerr.cc,v $
// Revision 1.2.8.1.2.1  2004/02/23 02:08:03  rmorris
// There is some incompatibility between the use of isascii() in the hdf library
// and its use on OS X.  Here we force in the #undef of isascii in the osx case.
//
// Revision 1.2.8.1  2003/05/21 16:26:58  edavis
// Updated/corrected copyright statements.
//
// Revision 1.2  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.1  1996/10/31 18:42:59  jimg
// Added.
//
// Revision 1.3  1996/05/23  18:15:58  todd
// Added copyright statement.
//
// Revision 1.3  1996/05/23  18:15:58  todd
// Added copyright statement.
//
// Revision 1.2  1996/04/22  17:42:42  todd
// Corrected a minor bug in hcerr::_print(ostream &) const.
//
// Revision 1.1  1996/04/02  20:47:50  todd
// Initial revision
//
