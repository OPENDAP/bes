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
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
 
/////////////////////////////////////////////////////////////////////////////
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

// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: dhdferr.cc,v $ - HDF server error class implementations
//
/////////////////////////////////////////////////////////////////////////////

#include <iostream>

using std::ostream ;
using std::cerr ;
using std::endl ;

#include "config_hdf.h"

#include <mfhdf.h>
#include <hcerr.h>
#include "dhdferr.h"

// print out value of exception
void dhdferr::_print(ostream &out) const {
    out << "Exception:    " << errmsg() << endl 
	<< "Location: \"" << file() << "\", line " << line() << endl;
    return;
}

void dhdferr_hcerr::_print(ostream &out) const {
    dhdferr::_print(out);
    for (int i=0; i<5; ++i)
	out << i << ") " << HEstring((hdf_err_code_t)HEvalue(i)) << endl;;
    return;
}

// stream the value of the exception to out
ostream& operator<<(ostream& out, const dhdferr& dhe) {
    dhe._print(out);
    return out;
}

//#ifdef NO_EXCEPTIONS
void fakethrow(const dhdferr& e) {
    cerr << "A fatal exception has been thrown:"  << endl;
    cerr << e;
    exit(1);
}
//#endif
    
// $Log: dhdferr.cc,v $
// Revision 1.5.4.1  2003/05/21 16:26:55  edavis
// Updated/corrected copyright statements.
//
// Revision 1.5  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
// Revision 1.4.4.1  2002/12/18 23:32:50  pwest
// gcc3.2 compile corrections, mainly regarding the using statement. Also,
// missing semicolon in .y file
//
// Revision 1.4  2000/10/09 19:46:20  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.3  1997/03/10 22:45:46  jimg
// Update for 2.12
//
// Revision 1.1  1996/09/27 18:19:48  todd
// Initial revision
//
//
