// This file is part of hdf5_handler - an HDF5 file handler for the OPeNDAP
// data server.

// Authors: 
// Hyo-Kyung Lee <hyoklee@hdfgroup.org>
// MuQun Yang <myang6@hdfgroup.org> 

// Copyright (c) 2009 The HDF Group, Inc. and OPeNDAP, Inc.
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

#include "HE5CFGrid.h"

#include "BESInternalError.h"

using namespace std;

HE5CFGrid::HE5CFGrid()
{

    _grid_TES = false;
    _grid_lat = 0;
    _grid_lon = 0;
    _grid_lev = 0;
    _grid_time = 0;
                
    point_lower = 0.0f;
    point_upper = 0.0f;
    point_left = 0.0f;
    point_right = 0.0f;
	pixelregistration = HE5CFGrid::HE5_HDFE_CENTER;
	gridorigin = HE5CFGrid::HE5_HDFE_GD_UL;
	bRead_point_lower=false;
	bRead_point_upper=false;
	bRead_point_left=false;
	bRead_point_right=false;
	bRead_pixelregistration=false;
	bRead_gridorigin=false;

    gradient_x = 0.0f;
    gradient_y = 0.0f;
    dimension_data = NULL;
}

HE5CFGrid::~HE5CFGrid()
{
}

bool
HE5CFGrid::get_grid()
{
    return _grid;
}


bool
HE5CFGrid::get_grid_TES()
{
    return _grid_TES;
}

int
HE5CFGrid::get_grid_lat()
{
    return _grid_lat;
}
int
HE5CFGrid::get_grid_lon()
{
    return _grid_lon;
}
int
HE5CFGrid::get_grid_lev()
{
    return _grid_lev;
}
int
HE5CFGrid::get_grid_time()
{
    return _grid_time;
}


int
HE5CFGrid::get_grid_dimension_data_location(string dimension_name)
{
    int j;
    for (j = 0; j < (int) _grid_dimension_list.size(); j++) {
        string dim_name = _grid_dimension_list.at(j);
        if (dim_name == dimension_name)
            return j;
    }
    return -1;
}


void
HE5CFGrid::get_grid_dimension_list(vector < string > &tokens)
{
    int j;

    for (j = 0; j < (int) _grid_dimension_list.size(); j++) {
        string dim_name = _grid_dimension_list.at(j);
        tokens.push_back(dim_name);
        DBG(cerr << "=get_grid_dimension_list():Dim name = " << dim_name <<
            std::endl);
    }
}

int
HE5CFGrid::get_grid_dimension_size(string name)
{
    return _grid_dimension_size[name];
}



string
HE5CFGrid::get_grid_name(string full_path)
{
    int end = full_path.find("/", 14); // 14 is the length of "/HDFEOS/GRIDS/".
    return full_path.substr(0, end + 1);        // Include the last "/".
}

bool
HE5CFGrid::get_grid_variable(string name)
{
    int i;
    for (i = 0; i < (int) _grid_variable_list.size(); i++) {
        std::string str = _grid_variable_list.at(i);
        if (str == name) {
            return true;
        }
    }
    return false;
}

void
HE5CFGrid::get_grid_variable_dimensions(string name, vector < string > &tokens)
{
    string str = _grid_variable_dimensions[name];
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(',', 0);
    // Find first "non-delimiter".
    string::size_type pos = str.find_first_of(',', lastPos);

    while (string::npos != pos || string::npos != lastPos) {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(',', pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(',', lastPos);
    }
}

void
HE5CFGrid::print()
{

    cout << "Left = " << point_left << endl;
    cout << "Right = " << point_right << endl;
    cout << "Lower = " << point_lower << endl;
    cout << "Upper = " << point_upper << endl;

    cout << "Total number of paths = " << _grid_variable_list.size() << endl;
    for (int i = 0; i < (int) _grid_variable_list.size(); i++) {
        cout << "Element " << _grid_variable_list.at(i) << endl;
    }
}

void
HE5CFGrid::set()
{
    int j;

    _grid = false;
    _grid_TES = false;
    _grid_lat = 0;
    _grid_lon = 0;
    _grid_lev = 0;
    _grid_time = 0;

    point_lower = 0.0;
    point_upper = 0.0;
    point_left = 0.0;
    point_right = 0.0;
    gradient_x = 0.0;
    gradient_y = 0.0;
    
    if(dimension_data != NULL){
        for (j = 0; j < (int) _grid_dimension_list.size(); j++) {
            if(dimension_data[j] != NULL)
                delete dimension_data[j];
        }
        delete dimension_data;
        dimension_data = NULL;
    }

    if(!_grid_dimension_size.empty()){
        _grid_dimension_size.clear();
    }
    if(!_grid_variable_dimensions.empty()){
        _grid_variable_dimensions.clear();
    }
  
    if(!_grid_dimension_list.empty()){
        _grid_dimension_list.clear();
    }
    if(!_grid_variable_list.empty()){
        _grid_variable_list.clear();
    }

    pixelregistration = HE5CFGrid::HE5_HDFE_CENTER;
    gridorigin = HE5CFGrid::HE5_HDFE_GD_UL;
    bRead_point_lower = false;
    bRead_point_upper = false;
    bRead_point_left = false;
    bRead_point_right = false;
    bRead_pixelregistration = false;
    bRead_gridorigin = false;

}

void
HE5CFGrid::set_grid(bool flag)
{
    _grid = flag;
}

void
HE5CFGrid::set_grid_TES(bool flag)
{
    _grid_TES = flag;
}


/**
 * This function calculates the values of lat/lon from the information
 * in StructMetadata. Basically, it uses PixelRegistration, GridOrigin,
 * UpperLeftPintMtrs and LowerRightMtrs.
 *
 * If the GridOrigin is UL or UR, lat progresses from UL_lat to LR_lat.
 * If the GridOrigin is LL or LR, lat progresses from LR_lat to UL_lat.
 * If the GridOrigin is UL or LL, lon progresses from UL_lon to LR_lon.
 * If the GridOrigin is UR or LR, lat progresses from LR_lon to UL_lon.
 *
 * -Eunsoo Seo <eseo2@hdfgroup.org>
 */
bool
HE5CFGrid::set_grid_dimension_data()
{
    int i = 0;
    int j = 0;
    int size = _grid_dimension_list.size();

    dods_float32 *convbuf = NULL;
    
    if (libdap::size_ok(sizeof(dods_float32), size))
        dimension_data = new dods_float32*[size];
    else
        throw InternalErr(__FILE__, __LINE__,
                          "Unable to allocate memory.");
    DBG(cerr << ">set_grid_dimension_data():Dimensions size = " << size <<
        endl);
    for (j = 0; j < (int) _grid_dimension_list.size(); j++) {
        string dim_name = _grid_dimension_list.at(j);
        int dim_size = _grid_dimension_size[dim_name];

        DBG(cerr << "=set_grid_dimension_data():Dim name = " << dim_name <<
            std::endl);
        DBG(cerr << "=set_grid_dimension_data():Dim size = " << dim_size <<
            std::endl);

        if (dim_size > 0) {
            if (libdap::size_ok(sizeof(dods_float32), dim_size))
                convbuf = new dods_float32[dim_size];
            else
                throw InternalErr(__FILE__, __LINE__,
                                  "Unable to allocate memory.");

			if ((dim_name.find("XDim", (int) dim_name.size() - 4)) != string::npos) {
				_grid_lon = dim_size;
				gradient_x =
					(point_right - point_left) / (float) (dim_size);
				if(_grid_TES){
					for (i = 0; i < dim_size; i++)
						convbuf[i] = (dods_float32)
							(point_left
							 + (float) i * gradient_x) / 1000000.0;	    
				}
				else
				{
					float start, end;
					if(gridorigin == HE5_HDFE_GD_UL || gridorigin == HE5_HDFE_GD_LL)
					{
						start = point_left;
						end = point_right;
					}
					else // (gridorigin == HE5_HDFE_GD_UR || gridorigin == HE5_HDFE_GD_LR)
					{
						end = point_left;
						start = point_right;
					}
					float intv = (end-start) / dim_size;
					if(pixelregistration == HE5_HDFE_CENTER)
					{
						for (i = 0; i < dim_size; i++)
							convbuf[i] = (dods_float32) (((float)i + 0.5f) * intv + start) / 1000000.0;
					}
					else	// HE5_HDFE_CORNER
					{
						for (i = 0; i < dim_size; i++)
							convbuf[i] = (dods_float32) (((float)i) * intv + start) / 1000000.0;
					}
				}
			} else if ((dim_name.find("YDim", (int) dim_name.size() - 4)) != string::npos) {
				_grid_lat = dim_size;
				if(_grid_TES){
					// swap the upper and lower points for TES products
					float temp = point_upper;
					point_upper = point_lower;
					point_lower = temp;
				} 
				gradient_y =
					(point_upper - point_lower) / (float) (dim_size);	
				if(_grid_TES){
					for (i = 0; i < dim_size; i++) {
						gradient_y = ceilf(gradient_y/1000000.0f) * 1000000.0f;
						convbuf[i] = (dods_float32)
							(point_lower
							 + (float) i * gradient_y) / 1000000.0;	    
					}
				}
				else
				{
					float start, end;
					if(gridorigin == HE5_HDFE_GD_UL || gridorigin == HE5_HDFE_GD_UR)
					{
						start = point_upper;
						end = point_lower;
					}
					else // (gridorigin == HE5_HDFE_GD_LL || gridorigin == HE5_HDFE_GD_LR)
					{
						start = point_lower;
						end = point_upper;
					}
					float intv = (end-start) / dim_size;
					if(pixelregistration == HE5_HDFE_CENTER)
					{
						for (i = 0; i < dim_size; i++)
							convbuf[i] = (dods_float32) (((float)i + 0.5f) * intv + start) / 1000000.0;
					}
					else	// HE5_HDFE_CORNER
					{
						for (i = 0; i < dim_size; i++)
							convbuf[i] = (dods_float32) (((float)i) * intv + start) / 1000000.0;
					}
				}




            } else if ((dim_name.find("nCandidate",
                                      (int) dim_name.size() - 10))
                        != string::npos) {
                _grid_lev = dim_size;
                for (i = 0; i < dim_size; i++) {
                    convbuf[i] = (dods_float32) i;      // meaningless number.
                }                
                
            } else if ((dim_name.find("nWavel", (int) dim_name.size() - 6))
                       != string::npos) {
                _grid_time = dim_size;
                for (i = 0; i < dim_size; i++) {
                    convbuf[i] = (dods_float32) i;      // meaningless number.
                }                
            }
            else {
                for (i = 0; i < dim_size; i++) {
                    convbuf[i] = (dods_float32) i;      // meaningless number.
                }
            }
        }                       // if dim_size > 0
        else {
            DBG(cerr << "Negative dimension " << endl);
            return false;
        }
        dimension_data[j] = convbuf;
    }                           // for
    DBG(cerr << "<set_grid_dimension_data()" << endl);
    return true;
}

void
HE5CFGrid::set_grid_dimension_size(string dimension_name, int dimension)
{
    bool has_dimension = false;

    int i;
    for (i = 0; i < (int) _grid_dimension_list.size(); i++) {
        std::string str = _grid_dimension_list.at(i);
        if (str == dimension_name) {
            has_dimension = true;
            break;
        }
    }

    if (!has_dimension) {
        _grid_dimension_list.push_back(dimension_name);
        _grid_dimension_size[dimension_name] = dimension;
    }
}


void
HE5CFGrid::set_grid_variable_list(string full_path)
{

    DBG(cerr << "Full path is:" << full_path << endl);
    _grid_variable_list.push_back(full_path);
}


void
HE5CFGrid::set_grid_variable_dimensions(string full_path,
                                        string dimension_list)
{
    _grid_variable_dimensions[full_path] = dimension_list;
    DBG(cerr << "Dimension List is:" <<
        _grid_variable_dimensions[full_path] << endl);
}
