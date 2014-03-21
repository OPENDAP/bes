/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the missing "third-dimension" values of the HDF-EOS2 Grid.
// Some third-dimension coordinate variable values are not provided. 
// What we do here is to provide natural number series(1,2,3, ...) for
// these missing values. It doesn't make sense to visualize or analyze 
// with vertical cross-section. One can check the data level by level.
//  Authors:   MuQun Yang <myang6@hdfgroup.org> 
// Copyright (c) 2009 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifdef USE_HDFEOS2_LIB
#ifndef HDFEOS2ARRAY_MISSFIELD_H
#define HDFEOS2ARRAY_MISSFIELD_H

#include "Array.h"
using namespace libdap;

class HDFEOS2ArrayMissGeoField:public Array
{
    public:
        HDFEOS2ArrayMissGeoField (int rank, int tnumelm, const std::string & n = "", BaseType * v = 0):
            Array (n, v), rank (rank), tnumelm (tnumelm) {
            }
        virtual ~ HDFEOS2ArrayMissGeoField ()
        {
        }

        // Standard way to pass the coordinates of the subsetted region from the client to the handlers
        int format_constraint (int *cor, int *step, int *edg);

        BaseType *ptr_duplicate ()
        {
            return new HDFEOS2ArrayMissGeoField (*this);
        }

        virtual bool read ();

    private:

        // Field array rank
        int rank;

        // Total number of elements
        int tnumelm;
};


#endif
#endif
