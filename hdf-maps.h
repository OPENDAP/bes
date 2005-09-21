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

// STL map include
#include <map>

using std::map ;
using std::less ;

struct sds_info {
  hdf_sds sds;
  bool in_vgroup;
};

struct vd_info {
  hdf_vdata vdata;
  bool in_vgroup;
};

struct gr_info {
  hdf_gri gri;
  bool in_vgroup;
};

struct vg_info {
  hdf_vgroup vgroup;
  bool toplevel;
};

typedef map<int32, sds_info, less<int32> > sds_map;
typedef map<int32, vd_info, less<int32> > vd_map;
typedef map<int32, gr_info, less<int32> > gr_map;
typedef map<int32, vg_info, less<int32> > vg_map;

typedef map<int32, sds_info, less<int32> >::const_iterator SDSI;
typedef map<int32, vd_info, less<int32> >::const_iterator VDI;
typedef map<int32, gr_info, less<int32> >::const_iterator GRI;
typedef map<int32, vg_info, less<int32> >::const_iterator VGI;

/* Function prototypes */
HDFGrid *NewGridFromSDS(const hdf_sds& sds);
HDFArray *NewArrayFromSDS(const hdf_sds& sds);
HDFArray *NewArrayFromGR(const hdf_gri& gr);
HDFSequence *NewSequenceFromVdata(const hdf_vdata& vd);
HDFStructure *NewStructureFromVgroup(const hdf_vgroup& vg,
                   vg_map& vgmap, sds_map& map, vd_map& vdmap, gr_map& grmap);
BaseType *NewDAPVar(int32 hdf_type);
string DAPTypeName(int32 hdf_type);

// $Log: hdf-maps.h,v $
// Revision 1.3.4.1  2003/05/21 16:26:55  edavis
// Updated/corrected copyright statements.
//
// Revision 1.3  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
// Revision 1.2.14.1  2002/12/18 23:32:50  pwest
// gcc3.2 compile corrections, mainly regarding the using statement. Also,
// missing semicolon in .y file
//
// Revision 1.2  1999/05/06 03:23:36  jimg
// Merged changes from no-gnu branch
//
// Revision 1.1.10.1  1999/05/06 00:27:24  jimg
// Jakes String --> string changes
//
// Revision 1.1  1998/04/06 16:11:43  jimg
// Added by Jake Hamby (via patch)
//
// Revision 1.1  1998/03/31  15:48:50  jehamby
// Initial revision

