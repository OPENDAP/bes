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

#include "HE5CF.h"


HE5CF::HE5CF()
{

    _shared_dimension = false;

    // CF-convention requires strict attribute/variable/dimension names.
    
    // Grid dimension names that are used for pseudo coordinate variables
    eos_to_cf_map["XDim"] = "lon";
    eos_to_cf_map["YDim"] = "lat";
    eos_to_cf_map["nLevels"] = "lev";    
    eos_to_cf_map["nCandidate"] = "lev";
    eos_to_cf_map["nWavel"] = "time";
    eos_to_cf_map["nLayers"] = "nlayers";
    eos_to_cf_map["Wavelength"] = "wavelength";
    eos_to_cf_map["Month"] = "month";
    
    // Swath coordinate variable names
    eos_to_cf_map["Time"] = "time";
    eos_to_cf_map["Pressure"] = "lev";
    eos_to_cf_map["Longitude"] = "lon";
    eos_to_cf_map["Latitude"] = "lat";

    // Attributes
    eos_to_cf_map["MissingValue"] = "missing_value";
    eos_to_cf_map["Units"] = "units";
    eos_to_cf_map["Offset"] = "add_offset";
    eos_to_cf_map["ScaleFactor"] = "scale_factor";
    eos_to_cf_map["ValidRange"] = "valid_range";
    eos_to_cf_map["Title"] = "title";
}

HE5CF::~HE5CF()
{

}


const char *
HE5CF::get_CF_name(char *eos_name)
{
    string str(eos_name);

    DBG(cerr << ">get_CF_name:" << str << endl);
    DBG(cerr << eos_to_cf_map[str] << endl);
    
    if (eos_to_cf_map[get_dataset_name(str)].size() > 0)
        {
            return eos_to_cf_map[get_dataset_name(str)].c_str();
        }
    else
        {
            // HACK: The following appends weird character under Hyrax.
            // return str.c_str();
            return (const char *) eos_name;
        }
}



bool
HE5CF::get_shared_dimension()
{
    return _shared_dimension;
}



void
HE5CF::set()
{
    HE5ShortName::reset();
    HE5CFSwath::set();
    HE5CFGrid::set();
    _shared_dimension = false;

}


void
HE5CF::set_shared_dimension()
{
    _shared_dimension = true;
}

