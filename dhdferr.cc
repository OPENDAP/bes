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
// $Log: dhdferr.cc,v $
// Revision 1.2  1997/02/10 02:01:49  jimg
// Update from Todd.
//
// Revision 1.1  1996/09/27 18:19:48  todd
// Initial revision
//
//
/////////////////////////////////////////////////////////////////////////////

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
    
