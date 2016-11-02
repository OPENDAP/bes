// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5GMCFSpecialCVArray.h
/// \brief This class specifies the retrieval of the missing lat/lon values for general HDF5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2016 The HDF Group
///
/// All rights reserved.

#ifndef _HDF5GMCFSpecialCVARRAY_H
#define _HDF5GMCFSpecialCVARRAY_H

// STL includes
#include <string>
#include <vector>

// DODS includes
#include "HDF5CF.h"
//#include <Array.h>
#include "HDF5BaseArray.h"

using namespace libdap;

class HDF5GMCFSpecialCVArray:public HDF5BaseArray {
    public:
        HDF5GMCFSpecialCVArray(H5DataType h5_dtype, int h5_tnumelm, const string &varfullpath, H5GCFProduct h5_product_type, const string & n="",  BaseType * v = 0):
        HDF5BaseArray(n,v),
        dtype(h5_dtype),
        tnumelm(h5_tnumelm),
        varname(varfullpath),
        product_type(h5_product_type),
        cvartype(CV_UNSUPPORTED)
    {
    }
        
    virtual ~ HDF5GMCFSpecialCVArray() {
    }
    virtual BaseType *ptr_duplicate();
    virtual bool read();
    //int format_constraint (int *cor, int *step, int *edg);
    

    private:
        H5DataType dtype;
        int tnumelm;
        string varname;
        H5GCFProduct product_type;
        CVType cvartype;    
        
        // GPM version 3.0 nlayer values are from the document https://storm.pps.eosdis.nasa.gov/storm/filespec.GPM.V1.pdf
        void obtain_gpm_l3_layer(int, vector<int>&,vector<int>&,vector<int>&);

        // GPM version 4.0 nlayer values are from the document 
        // http://www.eorc.jaxa.jp/GPM/doc/product/format/en/03.%20GPM_DPR_L2_L3%20Product%20Format%20Documentation_E.pdf
        void obtain_gpm_l3_layer2(int, vector<int>&,vector<int>&,vector<int>&);
        
        void obtain_gpm_l3_hgt(int, vector<int>&,vector<int>&,vector<int>&);
        void obtain_gpm_l3_nalt(int, vector<int>&,vector<int>&,vector<int>&);
        virtual void read_data_NOT_from_mem_cache(bool add_cache,void*buf);

};

#endif                          // _HDF5GMCFSpecialCVARRAY_H

