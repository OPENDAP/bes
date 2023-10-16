/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf5 data handler for the OPeNDAP data server.
//  Authors:   Kent Yang <myang6@hdfgroup.org> 
// Copyright (c) 2017 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifndef HDF5CFGEO_CF1D_H
#define HDF5CFGEO_CF1D_H

#include "HDF5BaseArray.h"

class HDF5CFGeoCF1D:public HDF5BaseArray
{
    public:
        HDF5CFGeoCF1D (EOS5GridPCType eos5_proj_code, double eos2_svalue, double eos2_evalue, int eos2_dim_size, const std::string & n = "", libdap::BaseType * v = nullptr):
            HDF5BaseArray (n, v), proj_code(eos5_proj_code), svalue (eos2_svalue),evalue(eos2_evalue),tnumelm(eos2_dim_size) {
            }
        ~ HDF5CFGeoCF1D () override = default;

#if 0
        // Standard way to pass the coordinates of the subsetted region from the client to the handlers
        //int format_constraint (int *cor, int *step, int *edg);
#endif
        void read_data_NOT_from_mem_cache(bool add_cache,void*buf) override;

        libdap::BaseType *ptr_duplicate () override
        {
            return new HDF5CFGeoCF1D (*this);
        }

        bool read () override;

    private:

        // Projection code, this is not currently used. It may be used as other projections are supported.
        EOS5GridPCType proj_code;
 
        // Start value
        double svalue;

        // End value
        double evalue;

        // Total number of elements
        int tnumelm;
};


#endif
