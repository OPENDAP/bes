/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the real field values.
//  Authors:   MuQun Yang <myang6@hdfgroup.org>  Eunsoo Seo
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifdef USE_HDFEOS2_LIB
#ifndef HDFEOS2ARRAY_REALFIELD_H
#define HDFEOS2ARRAY_REALFIELD_H

#include "Array.h"

#include "HDFCFUtil.h"
#include "HdfEosDef.h"


#include "HDFEOS2EnumType.h"

using namespace libdap;

class HDFEOS2Array_RealField:public Array
{
  public:
  HDFEOS2Array_RealField (int rank, const std::string & filename, const std::string & gridname, const std::string & swathname, const std::string & fieldname, SOType sotype, const string & n = "", BaseType * v = 0):
	Array (n, v),
		rank (rank),
		filename (filename),
		gridname (gridname), swathname (swathname), fieldname (fieldname),sotype(sotype) {
	}
	virtual ~ HDFEOS2Array_RealField ()
	{
	}
	int format_constraint (int *cor, int *step, int *edg);

	BaseType *ptr_duplicate ()
	{
		return new HDFEOS2Array_RealField (*this);
	}

	virtual bool read ();

  private:
	int rank;
	std::string filename, gridname, swathname, fieldname;
        SOType sotype;
};


#endif
#endif
