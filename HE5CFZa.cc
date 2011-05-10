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
#include "HE5CFZa.h"


HE5CFZa::HE5CFZa()
{
    _za = false;
    _za_time_dimensions = "";
    _za_lat_dimensions = "";
    _za_lev_dimensions = "";
    _za_time_variable = "";
    _za_lat_variable = "";
    _za_lev_variable = "";

    za_lat = 0;
    za_lon = 0;
    za_time = 0;
    za_lev = 0;
}

HE5CFZa::~HE5CFZa()
{

}

bool
HE5CFZa::get_za() const
{
    return _za;
}



// lat,lev,time: return 1
// lat,lev: return 2
// else return 0
int
HE5CFZa::get_za_coordinate_dimension_match(string varname)
{
    bool match_time = false;
    bool match_lat = false;
    bool match_lev = false;

    string str_dim = _za_variable_dimensions[varname];
    if(str_dim.find(_za_time_dimensions) != string::npos){
        match_time = true;
    }
    if(str_dim.find(_za_lat_dimensions) != string::npos){
        match_lat = true;
    }
    if(str_dim.find(_za_lev_dimensions) != string::npos){
        match_lev = true;
    }
    
    if( match_lat && match_lev){
      if(match_time) return 1;
      else return 2;
    }
    return 0;

}

void  
HE5CFZa::get_za_dimension_list(vector < string > &tokens)
{
    int j;
    for (j = 0; j < (int) _za_dimension_list.size(); j++) {
        string dim_name = _za_dimension_list.at(j);
        tokens.push_back(dim_name);
        DBG(cerr << "=get_all_dimensions():Dim name = " << dim_name <<
            std::endl);
    }
}

int 
HE5CFZa::get_za_dimension_size(string name)
{
    return _za_dimension_size[name];
}


bool HE5CFZa::get_za_variable(string varname)
{
    int i;
    for (i = 0; i < (int) _za_variable_list.size(); i++)
        {
            std::string str = _za_variable_list.at(i);
            if (str == varname)
                {
                    return true;
                }
        }
    return false;
}

void HE5CFZa::get_za_variable_dimensions(string name, 
                                              vector < string > &tokens)
{
    string str = _za_variable_dimensions[name];
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
HE5CFZa::set()
{

    za_lat = 0;
    za_time = 0;
    za_lev = 0;

    _za = false;
    _za_time_dimensions = "";
    _za_lat_dimensions = "";
    _za_lev_dimensions = "";
    _za_time_variable = "";
    _za_lat_variable = "";
    _za_lev_variable = "";

    if (!_za_dimension_size.empty())
        {
            _za_dimension_size.clear();
        }

    if (!_za_dimension_list.empty())
        {
            _za_dimension_list.clear();
        }
    if (!_za_variable_list.empty())
        {
            _za_variable_list.clear();
        }
    if(!_za_variable_dimensions.empty()){
       _za_variable_dimensions.clear();
    }
}

void
HE5CFZa::set_za(bool flag)
{
    _za = flag;
}



void
HE5CFZa::set_za_dimension_size(string dimension_name, int dimension)
{

    bool has_dimension = false;
    int i;
    for (i = 0; i < (int) _za_dimension_list.size(); i++)
        {
            std::string str = _za_dimension_list.at(i);
            if (str == dimension_name)
                {
                    has_dimension = true;
                    if (_za_dimension_size[dimension_name] != dimension)
                        {
                            DBG(cerr
                                << "Inconsistent dimension size " << dimension
                                << " for dimension " << dimension_name);

                              string msg =
                              "Inconsistent dimension size in EOS Za file";
                              throw InternalErr(__FILE__, __LINE__, msg);
                        }
                    break;
                }
        }

    if (!has_dimension)
        {
            _za_dimension_list.push_back(dimension_name);
            _za_dimension_size[dimension_name] = dimension;

        }
}

void
HE5CFZa::set_za_variable_list(string full_path)
{
    DBG(cerr << "Full path is:" << full_path << endl);
    _za_variable_list.push_back(full_path);
}

void
HE5CFZa::set_za_variable_dimensions(string full_path, 
                                          string dimension_list)
{

    _za_variable_dimensions[full_path] = dimension_list;
    
    // The number -5,-9 is the size of string "/Time"
    // Need to be changed later KY 2011-5-4
    if(full_path.find("/Time", ((int)full_path.size() - 5)) != 
       string::npos){
        _za_time_variable = full_path; 
        _za_time_dimensions = dimension_list;

        DBG(cerr << "Time dim list: " << _za_time_dimensions << endl);
    }

    if (full_path.find("/Latitude", ((int)full_path.size() - 9)) != 
        string::npos){
        _za_lat_variable = full_path; 
        _za_lat_dimensions = dimension_list;

        DBG(cerr << "Lat dim list: " << _za_lat_dimensions << endl);
    }

    if (full_path.find("/Pressure", ((int)full_path.size() - 9)) != 
        string::npos){
        _za_lev_variable = full_path; 
        _za_lev_dimensions = dimension_list;

        DBG(cerr << "Lev dim list: " << _za_lev_dimensions << endl);
    }

    DBG(cerr << "Dimension List is:" <<
       _za_variable_dimensions[full_path] << endl);
}

