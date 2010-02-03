/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf5 data handler for the OPeNDAP data server.
//
// Author:   Hyo-Kyung Lee <hyoklee@hdfgroup.org>

// Copyright (c) 2007, 2009 The HDF Group, Inc. and OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1901 South First Street,
// Suite C-2, Champaign, IL 61820  

#include "HE5Parser.h"
#include <InternalErr.h>

using namespace std;



struct yy_buffer_state;

int              he5ddslex();
int              he5ddsparse(void *arg);
yy_buffer_state *he5dds_scan_string(const char *str);


HE5Parser::HE5Parser()
{
    parser_state = 0; // parser state 
    valid_projection = false;
    grid_structure_found = false; 
    swath_structure_found = false; 
    _valid = false;
    bmetadata_Struct = false;
    bmetadata_Archived = false;
    bmetadata_Core = false;
    bmetadata_core = false;
    bmetadata_product = false;
    bmetadata_subset = false;


}

HE5Parser::~HE5Parser()
{
}


bool HE5Parser::has_group(hid_t id, const char *name)
{
    hid_t hid;
    H5E_BEGIN_TRY {
        hid = H5Gopen(id, name);
   } H5E_END_TRY;
    if (hid < 0) {
        return false;
    } else {
        return true;
    }

}

bool HE5Parser::has_dataset(hid_t id, const char *name)
{
    hid_t hid;
    H5E_BEGIN_TRY {
        hid = H5Dopen(id, name);
    } H5E_END_TRY;              
    if (hid < 0) {
        return false;
    } else {
        return true;
    }
}

bool HE5Parser::check_eos(hid_t id)
{

    reset();
  
    // Check if this file has the group called "HDFEOS INFORMATION".
    if (has_group(id, "HDFEOS INFORMATION")) {

        if (set_metadata(id, "StructMetadata", metadata_Struct)) {
            _valid = true;
        } else {
            _valid = false;
        }

        if (_valid) {

            he5dds_scan_string(metadata_Struct);
            he5ddsparse(this);

            set_metadata(id, "coremetadata", metadata_core);
            set_metadata(id, "CoreMetadata", metadata_Core);
            set_metadata(id, "ArchivedMetadata", metadata_Archived);
            set_metadata(id, "subsetmetadata", metadata_subset);
            set_metadata(id, "productmetadata", metadata_product);
            // Check if this file is TES.
            if(string(metadata_core).find("\"TES\"") != string::npos){
                set_grid_TES(true);
            }
            set_swath_2D();
            set_swath_missing_variable();
        }

        return _valid;
    }
    return false;
}



bool HE5Parser::is_valid()
{
    return _valid;
}

bool HE5Parser::set_metadata(hid_t id, char *metadata_name, char *chr_all)
{
    bool valid = false;
    int i = -1;
    
    // Assume that 30 is reasonable count of StructMetadata files
    for (i = -1; i < 30; i++) { 
        // Check if this file has the dataset called "StructMetadata".
        // Open the dataset.
        char dname[255];

        if (i == -1) {
            snprintf(dname, 255, "/HDFEOS INFORMATION/%s", metadata_name);
        } else {
            snprintf(dname, 255, "/HDFEOS INFORMATION/%s.%d",
                     metadata_name, i);
        }

        DBG(cerr << "Checking Dataset " << dname << endl);

        if (has_dataset(id, dname)) {
            hid_t dset = H5Dopen(id, dname);
            hid_t datatype, dataspace;

	    if (dset < 0){
		throw InternalErr(__FILE__, __LINE__, 
                                  "cannot open the existing dataset");
		break;
	    }
            if ((datatype = H5Dget_type(dset)) < 0) {
                cerr << "HE5Parser.cc failed to obtain datatype from  dataset "
                     << dset << endl;
                break;
            }
            if ((dataspace = H5Dget_space(dset)) < 0) {
                cerr << "HE5Parser.cc failed to obtain dataspace from dataset "
                     << dset << endl;
                break;
            }
            size_t size = H5Tget_size(datatype);
	    if (size == 0){
    	       throw InternalErr(__FILE__, __LINE__, 
                                 "cannot return the size of datatype");
            }

	    //char *chr = new char[size + 1];
            vector<char> chr(size+1);
            if (H5Dread(dset, datatype, dataspace, dataspace, H5P_DEFAULT, (void *)&chr[0])<0) {
            	throw InternalErr(__FILE__, __LINE__, "Unable to read the data.");
            }
            strncat(chr_all, &chr[0], size);
            valid = true;
            //delete[] chr;
        } else {
            // The sequence can skip <metdata>.0.
            // For example, "coremetadata" and then "coremetadata.1".
            if (i > 2)
                break;
        }
    }

    return valid;

}

void HE5Parser::reset()
{
    int j;

    HE5CF::set();
    
    grid_structure_found = false;
    valid_projection = false;
    swath_structure_found = false;
    parser_state = 0; 
    _valid = false;

    bmetadata_Struct = false;
    bmetadata_Archived = false;
    bmetadata_Core = false;
    bmetadata_core = false;
    bmetadata_product = false;
    bmetadata_subset = false;
    
    memset(metadata_Struct, 0, sizeof(metadata_Struct));
    memset(metadata_Archived,0,sizeof(metadata_Archived));
    memset(metadata_Core, 0,sizeof(metadata_Core));
    memset(metadata_core, 0,sizeof(metadata_core));
    memset(metadata_product, 0,sizeof(metadata_product));
    memset(metadata_subset, 0,sizeof(metadata_subset));

}

