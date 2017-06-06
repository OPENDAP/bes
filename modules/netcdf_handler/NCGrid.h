
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of nc_handler, a data handler for the OPeNDAP data
// server. 

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
 

// (c) COPYRIGHT URI/MIT 1994-1996
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors:
//      reza            Reza Nekovei (reza@intcomm.net)

// netCDF sub-class implementation for NCByte,...NCGrid.
// The files are patterned after the subcalssing examples 
// Test<type>.c,h files.
//
// ReZa 1/12/95

#ifndef _ncgrid_h
#define _ncgrid_h 1

#include "Grid.h"

using namespace libdap ;

class NCGrid: public Grid {

public:
    NCGrid(const string &n, const string &d);
    NCGrid(const NCGrid &rhs);
    virtual ~NCGrid();

    NCGrid &operator=(const NCGrid &rhs);
    virtual BaseType *ptr_duplicate();

    virtual bool read();

    virtual void transfer_attributes(AttrTable *at);
};

/* 
 * $Log: NCGrid.h,v $
 * Revision 1.9  2005/04/19 23:16:18  jimg
 * Removed client side parts; the client library is now in libnc-dap.
 *
 * Revision 1.8  2005/02/26 00:43:20  jimg
 * Check point: This version of the CL can now translate strings from the
 * server into char arrays. This is controlled by two things: First a
 * compile-time directive STRING_AS_ARRAY can be used to remove/include
 * this feature. When included in the code, only Strings associated with
 * variables created by the translation process will be turned into char
 * arrays. Other String variables are assumed to be single character strings
 * (although there may be a bug with the way these are handled, see
 * NCAccess::extract_values()).
 *
 * Revision 1.7  2005/01/26 23:25:51  jimg
 * Implemented a fix for Sequence access by row number when talking to a
 * 3.4 or earlier server (which contains a bug in is_end_of_rows()).
 *
 * Revision 1.6  2004/09/08 22:08:22  jimg
 * More Massive changes: Code moved from the files that clone the netCDF
 * function calls into NCConnect, NCAccess or nc_util.cc. Much of the
 * translation functions are now methods. The netCDF type classes now
 * inherit from NCAccess in addition to the DAP type classes.
 *
 * Revision 1.5  2003/12/08 18:06:37  edavis
 * Merge release-3-4 into trunk
 *
 * Revision 1.4  2003/09/25 23:09:36  jimg
 * Meerged from 3.4.1.
 *
 * Revision 1.3.4.1  2003/06/24 11:36:32  rmorris
 * Removed #pragma interface directives for the OS X.
 *
 * Revision 1.3  2003/01/28 07:08:24  jimg
 * Merged with release-3-2-8.
 *
 * Revision 1.2.4.1  2002/12/18 23:40:33  pwest
 * gcc3.2 compile corrections, mainly regarding using statements. Also,
 * problems with multi line string literatls.
 *
 * Revision 1.2  2000/10/06 01:22:02  jimg
 * Moved the CVS Log entries to the ends of files.
 * Modified the read() methods to match the new definition in the dap library.
 * Added exception handlers in various places to catch exceptions thrown
 * by the dap library.
 *
 * Revision 1.1  1999/07/28 00:22:43  jimg
 * Added
 *
 * Revision 1.4  1999/05/07 23:45:32  jimg
 * String --> string fixes
 *
 * Revision 1.3  1996/09/17 17:06:31  jimg
 * Merge the release-2-0 tagged files (which were off on a branch) back into
 * the trunk revision.
 *
 * Revision 1.2.4.2  1996/07/10 21:44:10  jimg
 * Changes for version 2.06. These fixed lingering problems from the migration
 * from version 1.x to version 2.x.
 * Removed some (but not all) warning generated with gcc's -Wall option.
 *
 * Revision 1.2.4.1  1996/06/25 22:04:32  jimg
 * Version 2.0 from Reza.
 *
 * Revision 1.2  1995/03/16  16:56:35  reza
 * Updated for the new DAP. All the read_val mfunc. and their memory management
 * are now moved to their parent class.
 * Data transfers are now in binary while DAS and DDS are still sent in ASCII.
 *
 * Revision 1.1  1995/02/10  04:57:32  reza
 * Added read and read_val functions.
 */

#endif





