/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf5 data handler for the OPeNDAP data server.
/////////////////////////////////////////////////////////////////////////////

#ifndef HDF5CFPROJ1D_H
#define HDF5CFPROJ1D_H

#include <libdap/Array.h>
#include "h5dmr.h"

class HDF5CFProj1D:public libdap::Array
{
    public:
        HDF5CFProj1D (double eos5_svalue, double eos5_evalue, int eos5_dim_size,  const std::string & n = "", libdap::BaseType * v = nullptr):
            libdap::Array(n, v), svalue (eos5_svalue),evalue(eos5_evalue),tnumelm(eos5_dim_size) {
            }
        ~ HDF5CFProj1D () override = default;

        bool read() override;

        libdap::BaseType *ptr_duplicate () override
        {
            return new HDF5CFProj1D (*this);
        }

    private:

        // Start value
        double svalue;

        // End value
        double evalue;

        // Total number of elements
        int tnumelm;

        // Standard way to pass the coordinates of the subsetted region from the client to the handlers
        int64_t format_constraint (int64_t *cor, int64_t *step, int64_t *edg);
 
};


#endif
