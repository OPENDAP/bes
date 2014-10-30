// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2007-2013 The HDF Group, Inc. and OPeNDAP, Inc.
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

///////////////////////////////////////////////////////////////////////////////
/// \file h5cfdap.cc
/// \brief DAS/DDS/DATA request processing holder for the CF option
///
/// This file is the main holder of the HDF5 for the CF option, a C++ implementation.
///
///
/// \author Muqun Yang    <myang6@hdfgroup.org>
///

#include <InternalErr.h>
#include <debug.h>
#include <mime_util.h>
#include "config_hdf5.h"
#include "h5cfdap.h"
#include <BESDebug.h>

void read_cfdds(DDS&,const string&,hid_t);
void read_cfdas(DAS&, const string&,hid_t);

void read_cfdds(DDS & dds, const string &filename,hid_t myfile_id) {

    BESDEBUG("h5","Coming to CF DDS read function read_cfdds "<<endl);
    // Set the DDS name be the file name
    dds.set_dataset_name(name_path(filename));

    hid_t fileid = myfile_id;
    H5CFModule moduletype;

#if 0
    fileid = H5Fopen(filename.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
    if (fileid < 0) {
        string msg =
            "h5_cf_dds handler: Cannot open the HDF5 file ";
        msg += filename;
        throw InternalErr(__FILE__, __LINE__, msg);
    }
#endif

    moduletype = check_module(fileid);
    if (moduletype == HDF_EOS5) {
        // cerr<<"coming to HDF-EOS5 "<<endl;
        map_eos5_cfdds(dds,fileid, filename);
    }

    else if(moduletype == HDF5_JPSS) {
        // handle HDF5 JPSS product, will handle this later.
        //read_cfjpss(fileid,filename);
        ;
    }

    else { // handle HDF5 general product 
        // cerr<<"coming to general HDF5"<<endl;
        map_gmh5_cfdds(dds,fileid, filename);

    }

}

void read_cfdas(DAS & das, const string &filename,hid_t myfile_id) {

    BESDEBUG("h5","Coming to CF DAS read function read_cfdas "<<endl);
    hid_t fileid = myfile_id;
    H5CFModule moduletype;

#if 0
    fileid = H5Fopen(filename.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
    if (fileid < 0) {
        string msg =
            "h5_cf_das handler: Cannot open the HDF5 file ";
        msg += filename;
        throw InternalErr(__FILE__, __LINE__, msg);
    }
#endif

    moduletype = check_module(fileid);
    if (moduletype == HDF_EOS5) {
        //cerr<<"coming to HDF-EOS5 "<<endl;
        map_eos5_cfdas(das,fileid, filename);
    }

    else if(moduletype == HDF5_JPSS) {
        // handle HDF5 JPSS product, will handle this later.
        //read_cfjpss(fileid,filename);
        ;
    }

    else { // handle HDF5 general product 
        // cerr<<"coming to general HDF5"<<endl;
        map_gmh5_cfdas(das,fileid, filename);
        // cerr<<"end of general HDF5 DAS"<<endl;
     
    }

}
