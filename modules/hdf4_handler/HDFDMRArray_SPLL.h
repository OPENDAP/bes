/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves HDF4 SDS values for a direct DMR-mapping DAP4 response.
// Each SDS will be mapped to a DAP variable.

//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifndef HDFDMRARRAY_SPLL_H
#define HDFDMRARRAY_SPLL_H

#include "mfhdf.h"
#include "hdf.h"

#include <libdap/Array.h>

class HDFDMRArray_SPLL:public libdap::Array
{
    public:
        HDFDMRArray_SPLL (const std::string& filename, float ll_start, float ll_res, const std::string & n = "", libdap::BaseType * v = nullptr):
            Array (n, v),
            filename(filename),
            ll_start(ll_start),
            ll_res(ll_res)
             {
        }
        ~ HDFDMRArray_SPLL () override = default;
       
        // Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
        // Return the number of elements to read.  
        int format_constraint (int *cor, int *step, int *edg);

        libdap::BaseType *ptr_duplicate () override
        {
            return new HDFDMRArray_SPLL (*this);
        }

        bool read () override;

    private:

        std::string filename;
        int32 sp_type = 0;
        float32 ll_start;
        float32 ll_res;
        int32 sds_rank = 1;

};


#endif
