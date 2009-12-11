// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author:   Hyo-Kyung Lee <hyoklee@hdfgroup.org>

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

#include "H5CF.h"


H5CF::H5CF()
{
    OMI = false;
    _swath = false;
    shared_dimension = false;

    // CF-convention requires strict attribute and variable names.
    eos_to_cf_map["MissingValue"] = "missing_value";
    eos_to_cf_map["Units"] = "units";
    eos_to_cf_map["XDim"] = "lon";
    eos_to_cf_map["YDim"] = "lat";
    eos_to_cf_map["nCandidate"] = "lev";

    eos_to_cf_map["Offset"] = "add_offset";
    eos_to_cf_map["ScaleFactor"] = "scale_factor";
    eos_to_cf_map["ValidRange"] = "valid_range";
    eos_to_cf_map["Title"] = "title";

    // Reverse look up for Grid case.
    cf_to_eos_map["lon"] = "XDim";
    cf_to_eos_map["lat"] = "YDim";
    cf_to_eos_map["lev"] = "nCandidate";
    
    //  The following translation rules apply to OMI L2 only.
    //  At this point, only OMI swath is 2-D swath, which IDV
    //  OPeNDAP visualization tool can support.
    eos_to_cf_map["nTime"] = "lat";
    eos_to_cf_map["nXtrack"] = "lon";
    eos_to_cf_map["Longitude"] = "lon";
    eos_to_cf_map["Latitude"] = "lat";



}

H5CF::~H5CF()
{

}

void
H5CF::add_data_path_swath(string full_path)
{

#ifdef CF
    // Generating short name is useful if --enable-cf configuration
    // is enabled since this is mainly for GrADS.
    //  For the CF option, please refer to
    //  http://hdfgroup.org/projects/opendap/publications/cf.html
    string short_path = generate_short_name(full_path);
    DBG(cerr << "Short path is:" << short_path << endl);
    long_to_short[full_path] = short_path;
#endif

    DBG(cerr << "Full path is:" << full_path << endl);
    _full_data_paths_swath.push_back(full_path);


}

void
H5CF::add_dimension_map_swath(string dimension_name, int dimension)
{

    bool has_dimension = false;
#ifdef CF
    dimension_name = cut_long_name(dimension_name);
#endif
    DBG(cerr << "add_dimension_map_swath " << dimension_name << " = " <<
        dimension << endl);

    int i;
    for (i = 0; i < (int) _dimensions_swath.size(); i++)
        {
            std::string str = _dimensions_swath.at(i);
            if (str == dimension_name)
                {
                    has_dimension = true;
                    if (_dimension_map_swath[dimension_name] != dimension)
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
            _dimensions_swath.push_back(dimension_name);
            _dimension_map_swath[dimension_name] = dimension;

        }
}


const char *
H5CF::get_CF_name(char *eos_name)
{
    string str(eos_name);

    DBG(cerr << ">get_CF_name:" << str << endl);
    DBG(cerr << eos_to_cf_map[str] << endl);
    if (is_swath())
        {
            if (str.find("/Longitude") != string::npos)
                {
                    return "lon";
                }
            if (str.find("/Latitude") != string::npos)
                {
                    return "lat";
                }
        }
    if (eos_to_cf_map[str].size() > 0)
        {
            return eos_to_cf_map[str].c_str();
        }
    else
        {
            // HACK: The following appends weird character under Hyrax.
            // return str.c_str(); 
            return (const char *) eos_name;
        }
}

string
H5CF::get_EOS_name(string str)
{
    DBG(cerr << cf_to_eos_map[str] << endl);
    if (cf_to_eos_map[str].size() > 0)
        {
            return cf_to_eos_map[str];
        }
    else
        {
            return str;
        }
}


bool
H5CF::is_shared_dimension_set()
{
    return shared_dimension;
}


bool
H5CF::is_swath()
{
    if (OMI)
        return _swath;
    else
        return false;
}

// See also  is_grid(string varname).
bool H5CF::is_swath(string varname)
{
    int i;
    for (i = 0; i < (int) _full_data_paths_swath.size(); i++)
        {
            std::string str = _full_data_paths_swath.at(i);
            if (str == varname)
                {
                    return true;
                }
        }
    return false;

}

void
H5CF::reset()
{
    HE5ShortName::reset();

    OMI = false;
    shared_dimension = false;
    _swath = false;

    if (!_dimension_map_swath.empty())
        {
            _dimension_map_swath.clear();
        }

    if (!_dimensions_swath.empty())
        {
            _dimensions_swath.clear();
        }
    if (!_full_data_paths_swath.empty())
        {
            _full_data_paths_swath.clear();
        }



}


void
H5CF::set_shared_dimension()
{
    shared_dimension = true;
}

void
H5CF::set_swath(bool flag)
{
    _swath = flag;
}
