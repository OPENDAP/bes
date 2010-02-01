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

void HE5Parser::print()
{

    cout << "Left = " << point_left << endl;
    cout << "Right = " << point_right << endl;
    cout << "Lower = " << point_lower << endl;
    cout << "Upper = " << point_upper << endl;

    cout << "Total number of paths = " << full_data_paths.size() << endl;
    for (int i = 0; i < (int) full_data_paths.size(); i++) {
        cout << "Element " << full_data_paths.at(i) << endl;
    }
}

bool HE5Parser::set_dimension_array()
{
    int i = 0;
    int j = 0;
    int size = dimensions.size();
#if 0
    dods_float32 *convbuf = NULL;
#endif
    if (!libdap::size_ok(sizeof(dods_float32), size))
	throw InternalErr(__FILE__, __LINE__, "Unable to allocate memory.");

    vector <dods_float32 *> dimension_data(size);

    DBG(cerr << ">set_dimension_array():Dimensions size = " << size <<
	    endl);
    for (j = 0; j < (int) dimensions.size(); j++) {
	string dim_name = dimensions.at(j);
	int dim_size = dimension_map[dim_name];

	DBG(cerr << "=set_dimension_array():Dim name = " << dim_name <<
		std::endl); DBG(cerr << "=set_dimension_array():Dim size = " << dim_size <<
		std::endl);

	if (dim_size > 0) {
	    if (!libdap::size_ok(sizeof(dods_float32), dim_size))
		throw InternalErr(__FILE__, __LINE__,
			"Unable to allocate memory.");

	    vector<dods_float32> convbuf(dim_size);

	    if ((dim_name.find("XDim", (int) dim_name.size() - 4))
		    != string::npos) {
		gradient_x = (point_right - point_left) / (float) (dim_size);
		for (i = 0; i < dim_size; i++) {
		    if (!TES) {
			convbuf[i] = (dods_float32) (point_left + (float) i
				* gradient_x + (gradient_x / 2.0)) / 1000000.0;
		    }
		    else {
			convbuf[i] = (dods_float32) (point_left + (float) i
				* gradient_x) / 1000000.0;
		    }
		}
	    }
	    else if ((dim_name.find("YDim", (int) dim_name.size() - 4))
		    != string::npos) {

		if (TES) {
		    // swap the upper and lower points for TES products
		    float temp = point_upper;
		    point_upper = point_lower;
		    point_lower = temp;
		}

		gradient_y = (point_upper - point_lower) / (float) (dim_size);
		for (i = 0; i < dim_size; i++) {
		    if (!TES) {
			convbuf[i] = (dods_float32) (point_lower + (float) i
				* gradient_y + (gradient_y / 2.0)) / 1000000.0;
		    }
		    else {
			gradient_y = ceilf(gradient_y / 1000000.0f)
				* 1000000.0f;
			convbuf[i] = (dods_float32) (point_lower + (float) i
				* gradient_y) / 1000000.0;
		    }
		}
	    }
	    else {
		for (i = 0; i < dim_size; i++) {
		    convbuf[i] = (dods_float32) i; // meaningless number.
		}
	    }
	    // Moved here from below jhrg 1/14/10
	    dimension_data[j] = &convbuf[0];
	} // if dim_size > 0
	else {
	    DBG(cerr << "Negative dimension " << endl);
	    return false;
	}
#if 0
	// What if convbuf was not allocated? jhrg 1/11/10
	dimension_data[j] = convbuf;
#endif
    } // for

    DBG(cerr << "<set_dimension_array()" << endl);
    return true;
}

string HE5Parser::get_grid_name(string full_path)
{
    int end = full_path.find("/", 14);
    return full_path.substr(0, end + 1);        // Include the last "/".
}

int HE5Parser::get_dimension_data_location(string dimension_name)
{
    int j;
    for (j = 0; j < (int) dimensions.size(); j++) {
        string dim_name = dimensions.at(j);
        if (dim_name == dimension_name)
            return j;
    }
    return -1;
}

bool HE5Parser::set_metadata(hid_t id, char *metadata_name, char *chr_all)
{
    bool valid = false;
    int i = -1;
    
    // Assume that 10 is reasonable count of StructMetadata files
    for (i = -1; i < 10; i++) { 
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

