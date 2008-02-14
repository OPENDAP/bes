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
// $RCSfile: HDFStructure.h,v $ - HDFStructure class declarations
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFSTRUCTURE_H
#define _HDFSTRUCTURE_H

// STL includes
#include <string>

// DODS includes
#include <Structure.h>

#include "ReadTagRef.h"

using namespace libdap ;

class HDFStructure: public Structure, public ReadTagRef {
public:
    HDFStructure(const string &n = "");
    virtual ~HDFStructure();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &);
    virtual bool read_tagref(const string &dataset, int32 tag, int32 ref, int &error);
    virtual void set_read_p(bool state);
//    virtual int nvars(void) { return _vars.length(); }
};

Structure *NewStructure(const string &n);

typedef HDFStructure * HDFStructurePtr;

// $Log: HDFStructure.h,v $
// Revision 1.7.8.1  2003/05/21 16:26:52  edavis
// Updated/corrected copyright statements.
//
// Revision 1.7  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.6  1999/05/06 03:23:35  jimg
// Merged changes from no-gnu branch
//
// Revision 1.5.10.1  1999/05/06 00:27:23  jimg
// Jakes String --> string changes
//
// Revision 1.5  1998/04/06 16:08:20  jimg
// Patch from Jake Hamby; change from switch to Mixin class for read_ref()
//
// Revision 1.4  1998/04/03 18:34:24  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.3  1997/03/10 22:45:39  jimg
// Update for 2.12
//
//

#endif // _HDFSTRUCTURE_H

