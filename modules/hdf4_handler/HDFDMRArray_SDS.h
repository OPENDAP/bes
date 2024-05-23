/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves HDF4 SDS values for a direct DMR-mapping DAP4 response.
// Each SDS will be mapped to a DAP variable.

//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifndef HDFDMRARRAY_SDS_H
#define HDFDMRARRAY_SDS_H

#include "mfhdf.h"
#include "hdf.h"

#include <libdap/Array.h>

class HDFDMRArray_SDS:public libdap::Array
{
    public:
        HDFDMRArray_SDS (const std::string& filename, int32 objref,  int32 rank, unsigned int dtype_size, const std::string & n = "", libdap::BaseType * v = nullptr):
            Array (n, v),
            filename(filename),
            sds_ref (objref),
            sds_rank (rank), 
            sds_typesize(dtype_size) {
        }
        ~ HDFDMRArray_SDS () override = default;
       
        // Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
        // Return the number of elements to read.  
        int format_constraint (int *cor, int *step, int *edg);

        libdap::BaseType *ptr_duplicate () override
        {
            return new HDFDMRArray_SDS (*this);
        }

        bool read () override;

    private:

        std::string filename;
        int32 sds_ref;

        // SDS array rank
        int32 sds_rank ;

        unsigned int sds_typesize ;


};


#endif
