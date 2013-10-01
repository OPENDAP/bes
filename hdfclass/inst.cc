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

#ifdef __GNUG__                 // force instantiation due to G++ bug
template class vector < hdf_sds >;
template class vector < hdf_attr >;
template class vector < hdf_dim >;
template class vector < hdf_genvec >;
template class vector < hdf_field >;
template class vector < hdf_vdata >;
template class vector < hdf_palette >;
template class vector < hdf_gri >;
#endif

// This dummy function is just so the presence of the HDFClass library 
// can be detected using GNU autoconf's AC_CHECK_LIB
extern "C" {
    void _HAVE_HDFCLASS(void) {
}}
// Changes:// $Log: inst.cc,v $// Revision 1.2.8.1.2.1  2004/02/23 02:08:03  rmorris// There is some incompatibility between the use of isascii() in the hdf library// and its use on OS X.  Here we force in the #undef of isascii in the osx case.//// Revision 1.2.8.1  2003/05/21 16:26:58  edavis// Updated/corrected copyright statements.//// Revision 1.2  2000/10/09 19:46:19  jimg// Moved the CVS Log entries to the end of each file.// Added code to catch Error objects thrown by the dap library.// Changed the read() method's definition to match the dap library.//// Revision 1.1  1996/10/31 18:43:05  jimg// Added.//
