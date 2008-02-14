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
 
/////////////////////////////////////////////////////////////////////////////
// (c) COPYRIGHT URI/MIT 1994-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.

// Author: James Gallagher
//

#ifndef _HDFFLOAT32_H
#define _HDFFLOAT32_H

#include <string>

// DODS includes
#include <Float32.h>

using namespace libdap ;

class HDFFloat32: public Float32 {
public:
    HDFFloat32(const string &n = "");
    virtual ~HDFFloat32();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &);
};

Float32 *NewFloat32(const string &n);

typedef HDFFloat32 * HDFFloat32Ptr;

// $Log: HDFFloat32.h,v $
// Revision 1.4.4.1  2003/05/21 16:26:51  edavis
// Updated/corrected copyright statements.
//
// Revision 1.4  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
// Revision 1.3.4.1  2002/04/12 00:03:14  jimg
// Fixed casts that appear throughout the code. I changed most/all of the
// casts to the new-style syntax. I also removed casts that we're not needed.
//
// Revision 1.3  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.2  1999/05/06 00:27:21  jimg
// Jakes String --> string changes
//
// Revision 1.1  1999/03/27 00:20:16  jimg
// Added

#endif // _HDFFLOAT32_H

