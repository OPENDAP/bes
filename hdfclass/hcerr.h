#ifndef _HCERR_H
#define _HCERR_H

//////////////////////////////////////////////////////////////////////////////
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

// 
// U.S. Government Sponsorship under NASA Contract
// NAS7-1260 is acknowledged.
// 
// Author: Todd.K.Karakashian@jpl.nasa.gov
//
//////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>

class hcerr;

using std::string;
using std::ostream;

#include <Error.h>

using namespace libdap;

#define THROW(x) throw x(__FILE__,__LINE__)

// HDFClass exceptions class
class hcerr: public Error {
public:
	hcerr(const char *msg, const char *file, int line);
	virtual ~ hcerr(void) {
	}
};

// Define valid HDFClass exceptions
class hcerr_invfile: public hcerr {
public:
	hcerr_invfile(const char *file, int line) :
		hcerr("Invalid file", file, line) {
	}
}; // invalid file given

class hcerr_invsize: public hcerr {
public:
	hcerr_invsize(const char *file, int line) :
		hcerr("Invalid size", file, line) {
	}
}; // invalid size parameter given

class hcerr_invnt: public hcerr {
public:
	hcerr_invnt(const char *file, int line) :
		hcerr("Invalid HDF number type", file, line) {
	}
}; // invalid HDF number type parameter given

class hcerr_invarr: public hcerr {
public:
	hcerr_invarr(const char *file, int line) :
		hcerr("Invalid array given", file, line) {
	}
}; // invalid array given as an argument to a function

class hcerr_nomemory: public hcerr {
public:
	hcerr_nomemory(const char *file, int line) :
		hcerr("Memory allocation failed", file, line) {
	}
}; // out of memory (new failed)

class hcerr_range: public hcerr {
public:
	hcerr_range(const char *file, int line) :
		hcerr("Subscript out of range", file, line) {
	}
};

class hcerr_invstream: public hcerr {
public:
	hcerr_invstream(const char *file, int line) :
		hcerr("Invalid hdfstream", file, line) {
	}
}; // hdfstream class uninitialized

class hcerr_copystream: public hcerr {
public:
	hcerr_copystream(const char *file, int line) :
		hcerr("Streams must be passed by reference", file, line) {
	}
}; // copying of streams disallowed

class hcerr_openfile: public hcerr {
public:
	hcerr_openfile(const char *file, int line) :
		hcerr("Could not open file", file, line) {
	}
}; // open file call failed

class hcerr_fileinfo: public hcerr {
public:
	hcerr_fileinfo(const char *file, int line) :
		hcerr("Could not retrieve information about a file", file, line) {
	}
}; // a file query function failed

class hcerr_anninit: public hcerr {
public:
	hcerr_anninit(const char *file, int line) :
		hcerr("Could not initialize annotation interface", file, line) {
	}
}; // ANstart() failed

class hcerr_anninfo: public hcerr {
public:
	hcerr_anninfo(const char *file, int line) :
		hcerr("Could not retrieve annotation info", file, line) {
	}
}; // ANfileinfo() or ANnumann() failed

class hcerr_annlist: public hcerr {
public:
	hcerr_annlist(const char *file, int line) :
		hcerr("Could not retrieve list of annotations", file, line) {
	}
}; // ANannlist() failed

class hcerr_annread: public hcerr {
public:
	hcerr_annread(const char *file, int line) :
		hcerr("Could not read an annotation", file, line) {
	}
}; // ANreadann() failed

class hcerr_dftype: public hcerr {
public:
	hcerr_dftype(const char *file, int line) :
		hcerr("Invalid HDF data type specified", file, line) {
	}
};

class hcerr_dataexport: public hcerr {
public:
	hcerr_dataexport(const char *file, int line) :
		hcerr("Could not export data from generic vector", file, line) {
	}
}; // if trying to export data into wrong type of STL vector

class hcerr_sdsinit: public hcerr {
public:
	hcerr_sdsinit(const char *file, int line) :
		hcerr("Could not select an SDS in SDS input stream", file, line) {
	}
}; // if SDfileinfo() fails when opening an hdfistream_sds

class hcerr_nosds: public hcerr {
public:
	hcerr_nosds(const char *file, int line) :
		hcerr("There are no SDS's in the stream", file, line) {
	}
}; // if SDfileinfo() indicates no SDS's in the file

class hcerr_sdsinfo: public hcerr {
public:
	hcerr_sdsinfo(const char *file, int line) :
		hcerr("Could not retrieve information about an SDS", file, line) {
	}
}; // if SDgetinfo() or SDfileinfo() fails

class hcerr_sdsfind: public hcerr {
public:
	hcerr_sdsfind(const char *file, int line) :
				hcerr("Could not find an SDS with the specified parameters",
						file, line) {
	}
}; // if SDnametoindex() fails

class hcerr_sdsopen: public hcerr {
public:
	hcerr_sdsopen(const char *file, int line) :
		hcerr("Could not open an SDS", file, line) {
	}
}; // if SDstart() fails

class hcerr_maxdim: public hcerr {
public:
	hcerr_maxdim(const char *file, int line) :
		hcerr("SDS rank exceeds the maximum supported", file, line) {
	}
}; //

class hcerr_iscoord: public hcerr {
public:
	hcerr_iscoord(const char *file, int line) :
				hcerr(
						"SDS cannot be opened because it is a coordinate variable",
						file, line) {
	}
}; //

class hcerr_sdsread: public hcerr {
public:
	hcerr_sdsread(const char *file, int line) :
		hcerr("Problem reading SDS", file, line) {
	}
}; // if SDreaddata() fails

class hcerr_sdsscale: public hcerr {
public:
	hcerr_sdsscale(const char *file, int line) :
		hcerr("Cannot determine dim scale; SDS is in a bad state.", file, line) {
	}
}; // if hdf_sds::has_scale() fails

class hcerr_vdataopen: public hcerr {
public:
	hcerr_vdataopen(const char *file, int line) :
		hcerr("Could not open a Vdata.", file, line) {
	}
}; // if VSattach() fails

class hcerr_vdatainfo: public hcerr {
public:
	hcerr_vdatainfo(const char *file, int line) :
		hcerr("Could not obtain information about a Vdata.", file, line) {
	}
}; // if VSinquire() fails

class hcerr_vdatafind: public hcerr {
public:
	hcerr_vdatafind(const char *file, int line) :
		hcerr("Could not locate Vdata in the HDF file.", file, line) {
	}
}; // if VSfind() fails

class hcerr_vdataread: public hcerr {
public:
	hcerr_vdataread(const char *file, int line) :
		hcerr("Could not read Vdata records.", file, line) {
	}
}; // if VSread() or VSsetfields() fails

class hcerr_vdataseek: public hcerr {
public:
	hcerr_vdataseek(const char *file, int line) :
		hcerr("Could not seek to first Vdata record.", file, line) {
	}
}; // if VSseek() fails

class hcerr_vgroupopen: public hcerr {
public:
	hcerr_vgroupopen(const char *file, int line) :
		hcerr("Could not open a Vgroup.", file, line) {
	}
}; // if Vattach() fails

class hcerr_vgroupinfo: public hcerr {
public:
	hcerr_vgroupinfo(const char *file, int line) :
		hcerr("Could not obtain information about a Vgroup.", file, line) {
	}
}; // if Vinquire() fails

class hcerr_vgroupfind: public hcerr {
public:
	hcerr_vgroupfind(const char *file, int line) :
		hcerr("Could not locate Vgroup in the HDF file.", file, line) {
	}
}; // if Vfind() fails

class hcerr_vgroupread: public hcerr {
public:
	hcerr_vgroupread(const char *file, int line) :
		hcerr("Could not read Vgroup records.", file, line) {
	}
}; // if Vgettagref() fails

class hcerr_griinfo: public hcerr {
public:
	hcerr_griinfo(const char *file, int line) :
		hcerr("Could not retrieve information about an GRI", file, line) {
	}
};

class hcerr_griread: public hcerr {
public:
	hcerr_griread(const char *file, int line) :
		hcerr("Problem reading GRI", file, line) {
	}
}; // if GRreadiamge() fails

class hcerr_invslab: public hcerr {
public:
	hcerr_invslab(const char *file, int line) :
		hcerr("Invalid slab parameters for SDS or GR", file, line) {
	}
}; // if slab parameters do not make sense

class hcerr_interlace: public hcerr {
public:
	hcerr_interlace(const char *file, int line) :
		hcerr("Unknown interlace type.", file, line) {
	}
}; // if bad interlace type is passed to setinterlace

#endif                          // ifndef _HCERR_H
