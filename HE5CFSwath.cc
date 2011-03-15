// This file is part of hdf5_handler - na HDF5 file handler for the OPeNDAP
// data server.

// Authors: 
// Hyo-Kyung Lee <hyoklee@hdfgroup.org>
// Muqun Yang <myang6@hdfgroup.org> 

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

using namespace std;
#include "HE5CFSwath.h"


HE5CFSwath::HE5CFSwath()
{
    _swath = false;
    _swath_2D = false;
    _swath_lon_dimensions = "";
    _swath_lat_dimensions = "";
    _swath_lev_dimensions = "";
    _swath_lon_variable = "";
    _swath_lat_variable = "";
    _swath_lev_variable = "";

    sw_lat = 0;
    sw_lon = 0;
    sw_time = 0;
    sw_lev = 0;
}

HE5CFSwath::~HE5CFSwath()
{

}

bool
HE5CFSwath::get_swath()
{
    return _swath;
}

bool
HE5CFSwath::get_swath_2D()
{
    return _swath_2D;
}


string  
HE5CFSwath::get_swath_coordinate_attribute()
{
    if(get_swath_2D()){
        return "lat lon";
    }
    else{
        return "lat lev";
    }
}

bool 
HE5CFSwath::get_swath_coordinate_dimension_match(string varname)
{
    bool match_lon = false;
    bool match_lat = false;
    bool match_lev = false;

    string str_dim = _swath_variable_dimensions[varname];
    if(str_dim.find(_swath_lon_dimensions) != string::npos){
        match_lon = true;
    }
    if(str_dim.find(_swath_lat_dimensions) != string::npos){
        match_lat = true;
    }
    if(str_dim.find(_swath_lev_dimensions) != string::npos){
        match_lev = true;
    }
    
    if(match_lon && match_lat){
        if(get_swath_2D()){
            return true;
        }
        else{
            if(match_lev && match_lat){
                return true;
            }
            else{
                return false;
            }
        }
    }
    else{
        return false;
    }
    
    
}

void  
HE5CFSwath::get_swath_dimension_list(vector < string > &tokens)
{
    int j;
    for (j = 0; j < (int) _swath_dimension_list.size(); j++) {
        string dim_name = _swath_dimension_list.at(j);
        tokens.push_back(dim_name);
        DBG(cerr << "=get_all_dimensions():Dim name = " << dim_name <<
            std::endl);
    }
}

int 
HE5CFSwath::get_swath_dimension_size(string name)
{
    return _swath_dimension_size[name];
}


bool HE5CFSwath::get_swath_variable(string varname)
{
    int i;
    for (i = 0; i < (int) _swath_variable_list.size(); i++)
        {
            std::string str = _swath_variable_list.at(i);
            if (str == varname)
                {
                    return true;
                }
        }
    return false;
}

void HE5CFSwath::get_swath_variable_dimensions(string name, 
                                              vector < string > &tokens)
{
    string str = _swath_variable_dimensions[name];
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
HE5CFSwath::set()
{

    sw_lat = 0;
    sw_lon = 0;
    sw_time = 0;
    sw_lev = 0;

    _swath = false;
    _swath_2D = false;
    _swath_lon_dimensions = "";
    _swath_lat_dimensions = "";
    _swath_lev_dimensions = "";
    _swath_lon_variable = "";
    _swath_lat_variable = "";
    _swath_lev_variable = "";

    if (!_swath_dimension_size.empty())
        {
            _swath_dimension_size.clear();
        }

    if (!_swath_dimension_list.empty())
        {
            _swath_dimension_list.clear();
        }
    if (!_swath_variable_list.empty())
        {
            _swath_variable_list.clear();
        }
    if(!_swath_variable_dimensions.empty()){
       _swath_variable_dimensions.clear();
    }
}


void
HE5CFSwath::set_swath(bool flag)
{
    _swath = flag;
}

bool
HE5CFSwath::set_swath_2D()
{
    int lon_ndims = 0;
    int lat_ndims = 0;
    vector < string > lon_dimension_names;
    vector < string > lat_dimension_names;


    get_swath_variable_dimensions(_swath_lon_variable, lon_dimension_names);
    lon_ndims = lon_dimension_names.size();
    get_swath_variable_dimensions(_swath_lat_variable, lat_dimension_names);
    lat_ndims = lat_dimension_names.size();

    if((lat_ndims == lon_ndims)){
        if(lat_ndims == 2){
            _swath_2D = true;
        }
        else if(lat_ndims == 1){
            _swath_2D = false;
        }
        else {
            return false;
        }
    }
    else{
        return false;
    }
    return true;
}


void
HE5CFSwath::set_swath_dimension_size(string dimension_name, int dimension)
{

    bool has_dimension = false;
    int i;
    for (i = 0; i < (int) _swath_dimension_list.size(); i++)
        {
            std::string str = _swath_dimension_list.at(i);
            if (str == dimension_name)
                {
                    has_dimension = true;
                    if (_swath_dimension_size[dimension_name] != dimension)
                        {
                            DBG(cerr
                                << "Inconsistent dimension size " << dimension
                                << " for dimension " << dimension_name);

                              string msg =
                              "Inconsistent dimension size in EOS Swath file";
                              throw InternalErr(__FILE__, __LINE__, msg);
                        }
                    break;
                }
        }

    if (!has_dimension)
        {
            _swath_dimension_list.push_back(dimension_name);
            _swath_dimension_size[dimension_name] = dimension;

        }
}

void
HE5CFSwath::set_swath_variable_list(string full_path)
{
    DBG(cerr << "Full path is:" << full_path << endl);
    _swath_variable_list.push_back(full_path);
}

void
HE5CFSwath::set_swath_variable_dimensions(string full_path, 
                                          string dimension_list)
{

    _swath_variable_dimensions[full_path] = dimension_list;
    
    if(full_path.find("/Longitude", ((int)full_path.size() - 10)) != 
       string::npos){
        _swath_lon_variable = full_path; 
        _swath_lon_dimensions = dimension_list;

        DBG(cerr << "Lon dim list: " << _swath_lon_dimensions << endl);
    }

    if (full_path.find("/Latitude", ((int)full_path.size() - 9)) != 
        string::npos){
        _swath_lat_variable = full_path; 
        _swath_lat_dimensions = dimension_list;

        DBG(cerr << "Lat dim list: " << _swath_lat_dimensions << endl);
    }

    if (full_path.find("/Pressure", ((int)full_path.size() - 9)) != 
        string::npos){
        _swath_lev_variable = full_path; 
        _swath_lev_dimensions = dimension_list;

        DBG(cerr << "Lev dim list: " << _swath_lev_dimensions << endl);
    }

    DBG(cerr << "Dimension List is:" <<
       _swath_variable_dimensions[full_path] << endl);
}

void
HE5CFSwath::set_swath_missing_variable()
{
    // New OMUVB doesn't have the missing entry in struct metadata.
    if(!get_swath_variable("/HDFEOS/SWATHS/UVB/Data Fields/OPIrradiance305")){
        
        set_swath_variable_list("/HDFEOS/SWATHS/UVB/Data Fields/OPIrradiance305");
        set_swath_variable_dimensions("/HDFEOS/SWATHS/UVB/Data Fields/OPIrradiance305","nTimes,nXtrack");
    }
}
