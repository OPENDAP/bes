// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2023 The HDF Group, Inc. and OPeNDAP, Inc.
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

/////////////////////////////////////////////////////////////////////////////
/// \file HDFEOS5CFMissNonLLCVArray.cc
/// \brief The implementation of the retrieval of the missing lat/lon values for HDFEOS5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2023 The HDF Group
///
/// All rights reserved.

#include <iostream>
#include <sstream>
#include <memory>
#include <BESDebug.h>

#include "HDFEOS5CFMissNonLLCVArray.h"
#include "HDF5RequestHandler.h"

using namespace std;
using namespace libdap;

BaseType *HDFEOS5CFMissNonLLCVArray::ptr_duplicate()
{
    auto HDFEOS5CFMissNonLLCVArray_unique = make_unique<HDFEOS5CFMissNonLLCVArray>(*this);
    return HDFEOS5CFMissNonLLCVArray_unique.release();
}

bool HDFEOS5CFMissNonLLCVArray::read()
{
    BESDEBUG("h5","Coming to HDFEOS5CFMissNonLLCVArray read "<<endl);

    // Don't use cache since the calculation is fairly small.
    read_data_NOT_from_mem_cache(false,nullptr);
#if 0
    if(nullptr == HDF5RequestHandler::get_srdata_mem_cache())                                          
        read_data_NOT_from_mem_cache(false,nullptr);                                                   
    else {                                                                                          
        // Notice for this special fake variable case, we just use the info. from DAP.
        // dataset() is the file name and name() is the HDF5 unique var name.
        string cache_key = dataset() + name();
        handle_data_with_mem_cache(H5INT32,tnumelm,1,cache_key);             
    }
#endif
 
    return true;
}

void HDFEOS5CFMissNonLLCVArray::read_data_NOT_from_mem_cache(bool /*add_cache*/, void* /*buf*/)
{
    BESDEBUG("h5","Coming to HDFEOS5CFMissNonLLCVArray: read_data_NOT_from_mem_cache "<<endl);
    write_nature_number_buffer(rank,tnumelm);
 
    return;
}
