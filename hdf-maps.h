/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1998, California Institute of Technology.  
// ALL RIGHTS RESERVED.   U.S. Government Sponsorship acknowledged. 
//
// Please read the full copyright notice in the file COPYRIGH 
// in this directory.
//
// Author: Jake Hamby, NASA/Jet Propulsion Laboratory
//         Jake.Hamby@jpl.nasa.gov
//
//
// $RCSfile: hdf-maps.h,v $ - Definition of mapping types for vgroup ordering
// 
// $Log: hdf-maps.h,v $
// Revision 1.1  1998/04/06 16:11:43  jimg
// Added by Jake Hamby (via patch)
//
// Revision 1.1  1998/03/31  15:48:50  jehamby
// Initial revision
//
//////////////////////////////////////////////////////////////////////////////

// STL map include
#include <map>

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

typedef map<int32, sds_info> sds_map;
typedef map<int32, vd_info> vd_map;
typedef map<int32, gr_info> gr_map;
typedef map<int32, vg_info> vg_map;

typedef map<int32, sds_info>::const_iterator SDSI;
typedef map<int32, vd_info>::const_iterator VDI;
typedef map<int32, gr_info>::const_iterator GRI;
typedef map<int32, vg_info>::const_iterator VGI;

/* Function prototypes */
HDFGrid *NewGridFromSDS(const hdf_sds& sds);
HDFArray *NewArrayFromSDS(const hdf_sds& sds);
HDFArray *NewArrayFromGR(const hdf_gri& gr);
HDFSequence *NewSequenceFromVdata(const hdf_vdata& vd);
HDFStructure *NewStructureFromVgroup(const hdf_vgroup& vg,
                   vg_map& vgmap, sds_map& map, vd_map& vdmap, gr_map& grmap);
BaseType *NewDAPVar(int32 hdf_type);
String DAPTypeName(int32 hdf_type);
