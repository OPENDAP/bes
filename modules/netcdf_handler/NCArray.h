
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

#ifndef _ncarray_h
#define _ncarray_h 1

#include <sys/types.h>

#include <sstream>

#include <Array.h>

#include <NCStructure.h>

using namespace libdap ;
using namespace std ;

class NCArray: public Array {
private:
    long format_constraint(size_t *cor, ptrdiff_t *step, size_t *edg, bool *has_stride);

    void do_cardinal_array_read(int ncid, int varid, nc_type datatype,
            vector<char> &values, bool has_values, int values_offset,
            int nels, size_t cor[], size_t edg[], ptrdiff_t step[], bool has_stride);

    void do_array_read(int ncid, int varid, nc_type datatype,
            vector<char> &values, bool has_values, int values_offset,
            int nels, size_t cor[], size_t edg[], ptrdiff_t step[], bool has_stride);

    friend class NCStructure;

public:
    NCArray(const string &n, const string &d, BaseType *v);
    NCArray(const NCArray &nc_array);
    NCArray &operator=(const NCArray &rhs);
    virtual ~NCArray();

    virtual BaseType *ptr_duplicate();

    virtual bool read();
};

/*
 * $Log: NCArray.h,v $
 * Revision 1.16  2005/04/19 23:16:18  jimg
 * Removed client side parts; the client library is now in libnc-dap.
 *
 * Revision 1.15  2005/04/11 18:38:20  jimg
 * Fixed a problem with NCSequence where nested sequences were not flagged
 * but instead were translated. The extract_values software cannot process a
 * nested sequence yet. Now the code inserts an attribute that notes that a
 * nested sequence has been elided.
 *
 * Revision 1.14  2005/04/08 17:08:47  jimg
 * Removed old 'virtual ctor' functions which have now been replaced by the
 * factory class code in libdap++.
 *
 * Revision 1.13  2005/03/31 00:04:51  jimg
 * Modified to use the factory class in libdap++ 3.5.
 *
 * Revision 1.12  2005/02/26 00:43:20  jimg
 * Check point: This version of the CL can now translate strings from the
 * server into char arrays. This is controlled by two things: First a
 * compile-time directive STRING_AS_ARRAY can be used to remove/include
 * this feature. When included in the code, only Strings associated with
 * variables created by the translation process will be turned into char
 * arrays. Other String variables are assumed to be single character strings
 * (although there may be a bug with the way these are handled, see
 * NCAccess::extract_values()).
 *
 * Revision 1.11  2005/02/17 23:44:13  jimg
 * Modifications for processing of command line projections combined
 * with the limit stuff and projection info passed in from the API. I also
 * consolodated some of the code by moving d_source from various
 * classes to NCAccess. This may it so that DODvario() could be simplified
 * as could build_constraint() and store_projection() in NCArray.
 *
 * Revision 1.10  2005/01/26 23:25:51  jimg
 * Implemented a fix for Sequence access by row number when talking to a
 * 3.4 or earlier server (which contains a bug in is_end_of_rows()).
 *
 * Revision 1.9  2004/11/30 22:11:35  jimg
 * I replaced the flatten_*() functions with a flatten() method in
 * NCAccess. The default version of this method is in NCAccess and works
 * for the atomic types; constructors must provide a specialization.
 * Then I removed the code that copied the variables from vectors to
 * lists. The translation code in NCConnect was modified to use the
 * new method.
 *
 * Revision 1.8  2004/10/28 16:38:19  jimg
 * Added support for error handling to ClientParams. Added use of
 * ClientParams to NCConnect, although that's not complete yet. NCConnect
 * now has an instance of ClientParams. The instance is first built and
 * then passed into NCConnect's ctor which stores a const reference to the CP
 * object.
 *
 * Revision 1.7  2004/10/22 21:51:34  jimg
 * More massive changes: Translation of Sequences now works so long as the
 * Sequence contains only atomic types.
 *
 * Revision 1.6  2004/09/08 22:08:21  jimg
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
 * Revision 1.3.4.2  2003/06/24 11:36:32  rmorris
 * Removed #pragma interface directives for the OS X.
 *
 * Revision 1.3.4.1  2003/06/07 22:02:32  reza
 * Fixed char vs. byte and long vs. nclong error checks.
 *
 * Revision 1.3  2002/05/03 00:01:52  jimg
 * Merged with release-3-2-7.
 *
 * Revision 1.2.4.1  2001/12/26 03:32:24  rmorris
 * Removed redundant default args.  VC++ only allows them to be specified
 * a single time.
 *
 * Revision 1.2  2000/10/06 01:22:02  jimg
 * Moved the CVS Log entries to the ends of files.
 * Modified the read() methods to match the new definition in the dap library.
 * Added exception handlers in various places to catch exceptions thrown
 * by the dap library.
 *
 * Revision 1.1  1999/07/28 00:22:42  jimg
 * Added
 *
 * Revision 1.5  1999/05/07 23:45:31  jimg
 * String --> string fixes
 *
 * Revision 1.4  1996/09/17 17:06:17  jimg
 * Merge the release-2-0 tagged files (which were off on a branch) back into
 * the trunk revision.
 *
 * Revision 1.3.4.2  1996/07/10 21:43:55  jimg
 * Changes for version 2.06. These fixed lingering problems from the migration
 * from version 1.x to version 2.x.
 * Removed some (but not all) warning generated with gcc's -Wall option.
 *
 * Revision 1.3.4.1  1996/06/25 22:04:19  jimg
 * Version 2.0 from Reza.
 *
 * Revision 1.3  1995/03/21  20:58:16  jimg
 * Resolved conflicts between my (jhrg) files and those checked in by Reza.
 *
 * Revision 1.2  1995/03/16  16:56:26  reza
 * Updated for the new DAP. All the read_val mfunc. and their memory management
 * are now moved to their parent class.
 * Data transfers are now in binary while DAS and DDS are still sent in ASCII.
 *
 * Revision 1.1  1995/02/10  04:57:15  reza
 * Added read and read_val functions.
 */

#endif


