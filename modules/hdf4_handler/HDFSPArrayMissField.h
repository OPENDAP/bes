/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the missing fields for some special NASA HDF4 data products.
// The products include TRMML2_V6,TRMML3B_V6,CER_AVG,CER_ES4,CER_CDAY,CER_CGEO,CER_SRB,CER_SYN,CER_ZAVG,OBPGL2,OBPGL3
// To know more information about these products,check HDFSP.h.
// Some third-dimension coordinate variable values are not provided.
// What we do here is to provide natural number series(1,2,3, ...) for
// these missing values. It doesn't make sense to visualize or analyze
// with vertical cross-section. One can check the data level by level.

//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifndef HDFSPARRAY_MISSFIELD_H
#define HDFSPARRAY_MISSFIELD_H

#include <libdap/Array.h>

class HDFSPArrayMissGeoField:public libdap::Array
{
    public:
        HDFSPArrayMissGeoField (int rank, int tnumelm, const std::string & n = "", libdap::BaseType * v = nullptr):
            libdap::Array (n, v), rank (rank), tnumelm (tnumelm) {
        }
        ~ HDFSPArrayMissGeoField () override = default;

        // Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
        // Return the number of elements to read. 
        int format_constraint (int *cor, int *step, int *edg);

        libdap::BaseType *ptr_duplicate () override
        {
            return new HDFSPArrayMissGeoField (*this);
        }

        bool read () override;

    private:

        int rank;
        int tnumelm;
};


#endif
