// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

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

#include "HE5ShortName.h"

HE5ShortName::HE5ShortName()
{
  index = 0;
}

string HE5ShortName::get_unique_name(string varname)
{
#ifdef SHORT_PATH  
  ostringstream oss;  
  int pos = varname.find_last_of('/', varname.length() - 1);
  oss << "A";
  oss << index;
  oss << varname.substr(pos + 1);
  // Prepend
  string temp_varname = oss.str();
  // Increment the index
  ++index;
  return temp_varname.substr(0, 15);
#else
  return varname;
#endif  
}

string HE5ShortName::get_dataset_name(string varname)
{
  int pos = varname.find_last_of('/', varname.length() - 1);
  return varname.substr(pos + 1);
}

string HE5ShortName::get_group_name(string varname)
{
  int pos = varname.find_last_of('/', varname.length() - 1);
  return varname.substr(0, pos);
}


string HE5ShortName::get_long_name(string short_varname)
{
  return short_to_long[short_varname];
}

string HE5ShortName::get_short_name(string long_varname)
{
  if(long_to_short[long_varname].empty()){
      string sstr = get_unique_name(long_varname);
      long_to_short[long_varname] = sstr;
      short_to_long[sstr] = long_varname;
  }
  return long_to_short[long_varname];  
}

void HE5ShortName::reset()
{
  index = 0;
  
  if(!long_to_short.empty()){
    long_to_short.clear();
  }
  if(!short_to_long.empty()){
    short_to_long.clear();
  }

}
