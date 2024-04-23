/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves HDF4 Vdata values for a direct DMR-mapping DAP4 response.
// Each Vdata will be mapped to a DAP variable.

//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifndef HDFDMRARRAY_VD_H
#define HDFDMRARRAY_VD_H

#include "hdf.h"

#include <libdap/Array.h>

class HDFDMRArray_VD:public libdap::Array
{
    public:
        HDFDMRArray_VD (const std::string& filename, int32 objref,  const std::string & n = "", libdap::BaseType * v = nullptr):
            Array (n, v),
            filename(filename),
            vdref (objref) {
        }
        ~ HDFDMRArray_VD () override = default;
       
        // Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
        // Return the number of elements to read.  
        int format_constraint (int *cor, int *step, int *edg);

        libdap::BaseType *ptr_duplicate () override
        {
            return new HDFDMRArray_VD (*this);
        }

        bool read () override;
        void set_rank(int32 vd_rank) { rank = vd_rank; } 

        friend class HDFStructure;

    private:

        // Field array rank
        int rank;

        std::string filename;

        // Vdata reference number
        int32 vdref;

        void read_one_field_vdata(int32 vdata_id,const vector<int>&offset, const vector<int>&count, const vector<int>&step, int nelms);
        void read_multi_fields_vdata(int32 vdata_id,const vector<int>&offset, const vector<int>&count, const vector<int>&step, int nelms);

};


#endif
