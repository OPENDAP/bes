/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1996, California Institute of Technology.  
// ALL RIGHTS RESERVED.   U.S. Government Sponsorship acknowledged. 
//
// Please read the full copyright notice in the file COPYRIGH 
// in this directory.
//
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
