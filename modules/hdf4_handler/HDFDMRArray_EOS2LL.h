/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves HDF4 EOS2LL values for a direct DMR-mapping DAP4 response.
// Each EOS2LL will be mapped to a DAP variable.

//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifndef HDFDMRARRAY_EOS2LL_H
#define HDFDMRARRAY_EOS2LL_H

#include "mfhdf.h"
#include "hdf.h"
#include "HdfEosDef.h"

#include <libdap/Array.h>

class HDFDMRArray_EOS2LL:public libdap::Array
{
    public:
        HDFDMRArray_EOS2LL (const std::string& filename,  const std::string& gridname, bool is_lat, const std::string & n = "", libdap::BaseType * v = nullptr):
            Array (n, v),
            filename(filename),
            gridname(gridname),
            is_lat(is_lat) {
        }
        ~ HDFDMRArray_EOS2LL () override = default;
       
        // Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
        // Return the number of elements to read.  
        int format_constraint (int *cor, int *step, int *edg);

        libdap::BaseType *ptr_duplicate () override
        {
            return new HDFDMRArray_EOS2LL (*this);
        }

        void set_rank(int32 eos2_rank) {ll_rank = eos2_rank;}
        bool read () override;

    private:

        std::string filename;

        std::string gridname;

        int32 ll_rank = 2;

        bool is_lat;

        // Subsetting the latitude and longitude.
        template <class T> void LatLon2DSubset (T* outlatlon, int xdim, T* latlon, const int * offset, const int * count, const int * step) const; 

};


#endif
