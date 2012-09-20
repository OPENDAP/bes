/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the latitude and longitude fields for some special HDF4 data products. 
// The products include TRMML2,TRMML3,CER_AVG,CER_ES4,CER_CDAY,CER_CGEO,CER_SRB,CER_SYN,CER_ZAVG,OBPGL2,OBPGL3
// To know more information about these products,check HDFSP.h.
// Each product stores lat/lon in different way, so we have to retrieve them differently. 
//  Authors:   MuQun Yang <myang6@hdfgroup.org>  
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifndef HDFSPARRAYGeoField_H
#define HDFSPARRAYGeoField_H

#include "Array.h"
#include "hdf.h"
#include "mfhdf.h"
#include "HDFSPEnumType.h"
using namespace libdap;

class HDFSPArrayGeoField:public Array
{
  public:
  HDFSPArrayGeoField (int32 rank, const std::string & filename, int32 sdsref, int32 dtype, SPType sptype, int fieldtype, const std::string & fieldname, const string & n = "", BaseType * v = 0):
	Array (n, v),
		rank (rank),
		filename (filename),
		sdsref (sdsref),
		dtype (dtype),
		sptype (sptype), fieldtype (fieldtype), name (fieldname) {
	}
	virtual ~ HDFSPArrayGeoField ()
	{
	}
	int format_constraint (int *cor, int *step, int *edg);

	BaseType *ptr_duplicate ()
	{
		return new HDFSPArrayGeoField (*this);
	}

	virtual bool read ();

	// Read TRMM level 2 lat/lon
	void readtrmml2 (int32 *, int32 *, int32 *, int);

	// Read OBPG level 2 lat/lon
	void readobpgl2 (int32 *, int32 *, int32 *, int);

	// Read OBPG level 3 lat/lon
	void readobpgl3 (int *, int *, int *, int);

	// Read TRMM level 3 lat/lon
	void readtrmml3 (int32 *, int32 *, int32 *, int);

	// Read CERES SAVG and CERES ICCP_DAYLIKE lat/lon
	void readcersavgid1 (int *, int *, int *, int);

	// Read CERES SAVG and ICCP_DAYLIKE lat/lon
	void readcersavgid2 (int *, int *, int *, int);

	// Read CERES ZAVG lat/lon
	void readcerzavg (int32 *, int32 *, int32 *, int);

	// Read CERES AVG and SYN lat/lon
	void readceravgsyn (int32 *, int32 *, int32 *, int);

	// Read CERES ES4 and ICCP_GEO lat/lon
	void readceres4ig (int32 *, int32 *, int32 *, int);

        template <typename T>  void LatLon2DSubset (T* outlatlon, int ydim, int xdim,
                                                 T* latlon, int32 * offset, int32 * count,
                                                 int32 * step);


  private:
	int32 rank;
        string filename;
	int32 sdsref;
	int32 dtype;
	SPType sptype;
	int fieldtype;
	string name;

};


#endif
