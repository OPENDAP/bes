// This file is part of the hdf4_handler implementing for the CF-compliant
// Copyright (c) The HDF Group, Inc. and OPeNDAP, Inc.
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820  

////////////////////////////////////////////////////////////////////////////////
/// \file HDFEOS2CFStrField.h
/// \brief This class provides a way to map HDFEOS2 character >1D array to DAP Str array for the CF option
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#ifdef USE_HDFEOS2_LIB
#ifndef _HDFEOS2CFSTRFIELD_H
#define _HDFEOS2CFSTRFIELD_H

// STL includes
#include <string>

// DODS includes
#include <libdap/dods-limits.h>
#include <libdap/Array.h>
#include "mfhdf.h"
#include "hdf.h"
#include "HdfEosDef.h"


class HDFEOS2CFStrField:public libdap::Array {
  public:
    HDFEOS2CFStrField(
                        const int rank,
                        const int gsfd, 
                        const std::string &filename,
                        const std::string &objname,
                        const std::string &varname, 
                        int grid_or_swath,
                        const std::string &n="",
                        libdap::BaseType*v=nullptr):
        libdap::Array (n, v),
        rank(rank),
        gsfd(gsfd),
        filename(filename),
        objname(objname),
        varname(varname),
        grid_or_swath(grid_or_swath)
    {
    }

    ~ HDFEOS2CFStrField() override = default;
    libdap::BaseType *ptr_duplicate() override{
        return new HDFEOS2CFStrField(*this);
    }

    // Standard way to pass the coordinates of the subsetted region from the client to the handlers
    int format_constraint (int *cor, int *step, int *edg);

    bool read() override;
  private:
    int   rank;
    int32 gsfd;
    std::string filename;
    std::string objname;
    std::string varname;
    int grid_or_swath;
   
};

#endif                          // _HDFEOS2CFSTRFIELD_H
#endif

