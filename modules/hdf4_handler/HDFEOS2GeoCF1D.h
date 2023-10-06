/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
//  Authors:   Kent Yang <myang6@hdfgroup.org> 
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifdef USE_HDFEOS2_LIB
#ifndef HDFEOS2GEO_CF1D_H
#define HDFEOS2GEO_CF1D_H

#include <libdap/Array.h>

class HDFEOS2GeoCF1D:public libdap::Array
{
    public:
        HDFEOS2GeoCF1D (int eos2_proj_code, double eos2_svalue, double eos2_evalue, int eos2_dim_size, const std::string & n = "", libdap::BaseType * v = nullptr):
            libdap::Array (n, v), proj_code(eos2_proj_code), svalue (eos2_svalue),evalue(eos2_evalue),tnumelm(eos2_dim_size) {
            }
        ~ HDFEOS2GeoCF1D () override = default;

        // Standard way to pass the coordinates of the subsetted region from the client to the handlers
        int format_constraint (int *cor, int *step, int *edg);

        libdap::BaseType *ptr_duplicate () override
        {
            return new HDFEOS2GeoCF1D (*this);
        }

        bool read () override;

    private:

        // Projection code, currently not used.
        int proj_code;
 
        // Start value
        double svalue;

        // End value
        double evalue;

        // Total number of elements
        int tnumelm;
};


#endif
#endif
