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
// Copyright 1998, by the California Institute of Technology.
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

// Author: Jake Hamby, NASA/Jet Propulsion Laboratory
//         Jake.Hamby@jpl.nasa.gov
//
// $RCSfile: ReadTagRef.h,v $ - Declaration of abstract read_tagref() method
//
// $Log: ReadTagRef.h,v $
// Revision 1.2.18.1  2003/05/21 16:26:55  edavis
// Updated/corrected copyright statements.
//
// Revision 1.2  1999/05/06 03:23:35  jimg
// Merged changes from no-gnu branch
//
// Revision 1.1.10.1  1999/05/06 00:27:23  jimg
// Jakes String --> string changes
//
// Revision 1.1  1998/04/06 16:11:43  jimg
// Added by Jake Hamby (via patch)
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _READTAGREF_H
#define _READTAGREF_H

// STL includes
#include <string>
// HDF includes (int32 type)
#include <hdf.h>

class ReadTagRef {
public:
  virtual bool read_tagref(const string &dataset, int32 tag, int32 ref, int &error) = 0;
};

#endif // _READTAGREF_H
