/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the real field values for some special NASA HDF data.
//  Authors:   MuQun Yang <myang6@hdfgroup.org>  
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifndef HDFSPARRAY_REALFIELD_H
#define HDFSPARRAY_REALFIELD_H

#include "HDFSP.h"
#include "Array.h"
using namespace libdap;

class HDFSPArray_RealField:public Array
{
  public:
  HDFSPArray_RealField (int32 rank, const std::string & filename, int32 sdsref, int32 dtype, SPType & sptype, const std::string & fieldname, const string & n = "", BaseType * v = 0):
	Array (n, v),
		rank (rank),
		filename (filename),
		sdsref (sdsref), dtype (dtype), sptype (sptype), name (fieldname) {
	}
	virtual ~ HDFSPArray_RealField ()
	{
	}
	int format_constraint (int *cor, int *step, int *edg);

	BaseType *ptr_duplicate ()
	{
		return new HDFSPArray_RealField (*this);
	}

	virtual bool read ();

  private:
	std::string filename, name;
	int32 rank;
	int32 sdsref;
	int32 dtype;
	SPType sptype;
};


#endif
