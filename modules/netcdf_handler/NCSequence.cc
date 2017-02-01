
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

#include "config_nc.h"

static char rcsid[] not_used ={"$Id$"};

#include <sstream>
#include <algorithm>

// #define DODS_DEBUG 1

#include <InternalErr.h>
#include <debug.h>

#include "NCSequence.h"

BaseType *
NCSequence::ptr_duplicate()
{
    return new NCSequence(*this);
}

NCSequence::NCSequence(const string &n, const string &d) : Sequence(n, d)
{
}

NCSequence::NCSequence(const NCSequence &rhs) : Sequence(rhs)
{
}

NCSequence::~NCSequence()
{
}

NCSequence &
NCSequence::operator=(const NCSequence &rhs)
{
    if (this == &rhs)
        return *this;

    dynamic_cast<Sequence &>(*this) = rhs; // run Sequence assignment

    return *this;
}

void NCSequence::transfer_attributes(AttrTable *at)
{
	if (at) {
		Vars_iter var = var_begin();
		while (var != var_end()) {
			(*var)->transfer_attributes(at);
			var++;
		}
	}
}


// $Log: NCSequence.cc,v $
// Revision 1.17  2005/04/19 23:16:18  jimg
// Removed client side parts; the client library is now in libnc-dap.
//
// Revision 1.16  2005/04/11 18:38:20  jimg
// Fixed a problem with NCSequence where nested sequences were not flagged
// but instead were translated. The extract_values software cannot process a
// nested sequence yet. Now the code inserts an attribute that notes that a
// nested sequence has been elided.
//
// Revision 1.15  2005/04/07 23:35:36  jimg
// Changed the value of the translation attribute from "translated" to "flatten".
//
// Revision 1.14  2005/03/31 00:04:51  jimg
// Modified to use the factory class in libdap++ 3.5.
//
// Revision 1.13  2005/03/19 00:33:03  jimg
// Checkpoint: All tests pass and only one memory leak remains (in
// NCConnect::flatten_attributes()).
//
// Revision 1.12  2005/03/05 00:16:58  jimg
// checkpoint: working on memory leaks found using unit tests
//
// Revision 1.11  2005/02/26 00:43:20  jimg
// Check point: This version of the CL can now translate strings from the
// server into char arrays. This is controlled by two things: First a
// compile-time directive STRING_AS_ARRAY can be used to remove/include
// this feature. When included in the code, only Strings associated with
// variables created by the translation process will be turned into char
// arrays. Other String variables are assumed to be single character strings
// (although there may be a bug with the way these are handled, see
// NCAccess::extract_values()).
//
// Revision 1.10  2005/02/17 23:44:13  jimg
// Modifications for processing of command line projections combined
// with the limit stuff and projection info passed in from the API. I also
// consolodated some of the code by moving d_source from various
// classes to NCAccess. This may it so that DODvario() could be simplified
// as could build_constraint() and store_projection() in NCArray.
//
// Revision 1.9  2005/01/29 00:20:29  jimg
// Checkpoint: CEs ont he command line/ncopen() almost work.
//
// Revision 1.8  2005/01/26 23:25:51  jimg
// Implemented a fix for Sequence access by row number when talking to a
// 3.4 or earlier server (which contains a bug in is_end_of_rows()).
//
// Revision 1.7  2004/11/30 22:11:35  jimg
// I replaced the flatten_*() functions with a flatten() method in
// NCAccess. The default version of this method is in NCAccess and works
// for the atomic types; constructors must provide a specialization.
// Then I removed the code that copied the variables from vectors to
// lists. The translation code in NCConnect was modified to use the
// new method.
//
// Revision 1.6  2004/11/05 17:13:57  jimg
// Added code to copy the BaseType pointers from the vector container into
// a list. This will enable more efficient translation software to be
// written.
//
// Revision 1.5  2004/10/22 21:51:34  jimg
// More massive changes: Translation of Sequences now works so long as the
// Sequence contains only atomic types.
//
// Revision 1.4  2004/09/08 22:08:22  jimg
// More Massive changes: Code moved from the files that clone the netCDF
// function calls into NCConnect, NCAccess or nc_util.cc. Much of the
// translation functions are now methods. The netCDF type classes now
// inherit from NCAccess in addition to the DAP type classes.
//
// Revision 1.3  2003/12/08 18:06:37  edavis
// Merge release-3-4 into trunk
//
// Revision 1.2  2000/10/06 01:22:02  jimg
// Moved the CVS Log entries to the ends of files.
// Modified the read() methods to match the new definition in the dap library.
// Added exception handlers in various places to catch exceptions thrown
// by the dap library.
//
// Revision 1.1  1999/07/28 00:22:44  jimg
// Added
//
// Revision 1.6  1999/05/07 23:45:32  jimg
// String --> string fixes
//
// Revision 1.5  1998/08/06 16:33:24  jimg
// Fixed misuse of the read(...) member function. Return true if more data
// is to be read, false is if not and error if an error is detected
//
// Revision 1.4  1996/09/17 17:06:37  jimg
// Merge the release-2-0 tagged files (which were off on a branch) back into
// the trunk revision.
//
// Revision 1.3.4.4  1996/09/17 00:26:29  jimg
// Merged changes from a side branch which contained various changes from
// Reza and Charles.
// Removed ncdump and netexec since ncdump is now in its own directory and
// netexec is no longer used.
//
// Revision 1.3.4.3  1996/08/13 21:19:01  jimg
// *** empty log message ***
//
// Revision 1.3.4.2  1996/07/10 21:44:18  jimg
// Changes for version 2.06. These fixed lingering problems from the migration
// from version 1.x to version 2.x.
// Removed some (but not all) warning generated with gcc's -Wall option.
//
// Revision 1.3.4.1  1996/06/25 22:04:40  jimg
// Version 2.0 from Reza.
//
// Revision 1.3  1995/07/09  21:33:49  jimg
// Added copyright notice.
//
// Revision 1.2  1995/03/16  16:56:41  reza
// Updated for the new DAP. All the read_val mfunc. and their memory management
// are now moved to their parent class.
// Data transfers are now in binary while DAS and DDS are still sent in ASCII.
//
// Revision 1.1  1995/02/10  04:57:43  reza
// Added read and read_val functions.
//
