//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1996, California Institute of Technology.
//                     U.S. Government Sponsorship under NASA Contract
//		       NAS7-1260 is acknowledged.
// 
// Author: Todd.K.Karakashian@jpl.nasa.gov
//
// $rcsfile$ - force g++ to instantiate templates.  Does nothing if g++
//             is not being used.
//                  
//////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <mfhdf.h>
#ifdef __POWERPC__
#undef isascii
#endif

#include <hdfclass.h>

#ifdef __GNUG__			// force instantiation due to G++ bug
template class vector<hdf_sds>;
template class vector<hdf_attr>;
template class vector<hdf_dim>;
template class vector<hdf_genvec>;
template class vector<hdf_field>;
template class vector<hdf_vdata>;
template class vector<hdf_palette>;
template class vector<hdf_gri>;
#endif

// This dummy function is just so the presence of the HDFClass library 
// can be detected using GNU autoconf's AC_CHECK_LIB
extern "C" {
  void _HAVE_HDFCLASS(void) {}
}

// Changes:
// $Log: inst.cc,v $
// Revision 1.3  2004/07/09 18:08:50  jimg
// Merged with release-3-4-3FCS.
//
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
// Revision 1.1  1996/10/31 18:43:05  jimg
// Added.
//
