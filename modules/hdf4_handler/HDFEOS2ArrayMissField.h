/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the missing "third-dimension" values of the HDF-EOS2 Grid.
// Some third-dimension coordinate variable values are not provided. 
// What we do here is to provide natural number series(1,2,3, ...) for
// these missing values. It doesn't make sense to visualize or analyze 
// with vertical cross-section. One can check the data level by level.
//  Authors:   Kent Yang <myang6@hdfgroup.org> 
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifdef USE_HDFEOS2_LIB
#ifndef HDFEOS2ARRAY_MISSFIELD_H
#define HDFEOS2ARRAY_MISSFIELD_H

#include <libdap/Array.h>

class HDFEOS2ArrayMissGeoField:public libdap::Array
{
    public:
        HDFEOS2ArrayMissGeoField (int rank, int tnumelm, const std::string & n = "", libdap::BaseType * v = nullptr):
            libdap::Array (n, v), rank (rank), tnumelm (tnumelm) {
            }
        ~ HDFEOS2ArrayMissGeoField () override = default;

        // Standard way to pass the coordinates of the subsetted region from the client to the handlers
        int format_constraint (int *cor, int *step, int *edg);

        libdap::BaseType *ptr_duplicate () override
        {
            return new HDFEOS2ArrayMissGeoField (*this);
        }

        bool read () override;

    private:

        // Field array rank
        int rank;

        // Total number of elements
        int tnumelm;
};


#endif
#endif
