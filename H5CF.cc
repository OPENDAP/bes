/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf5 data handler for the OPeNDAP data server.
//
// Author:   Hyo-Kyung Lee <hyoklee@hdfgroup.org>
// Copyright (c) 2009 The HDF Group
//
/////////////////////////////////////////////////////////////////////////////

using namespace std;


#include "H5CF.h"

H5CF::H5CF()
{
  shared_dimension = false;
  // <hyokyung 2007.08. 2. 14:25:58>  
  eos_to_cf_map["MissingValue"] = "missing_value";
  eos_to_cf_map["Units"] = "units";
  eos_to_cf_map["XDim"] = "lon";
  eos_to_cf_map["YDim"] = "lat";
  eos_to_cf_map["nCandidate"] = "lev";

  // eos_to_grads_map["Offset"] = "add_offset";
  // eos_to_grads_map["ScaleFactor"] = "scale_factor";
  // eos_to_grads_map["ValidRange"] = "valid_range";

  cf_to_eos_map["lon"] = "XDim";
  cf_to_eos_map["lat"] = "YDim";
  cf_to_eos_map["lev"] = "nCandidate";

  
}

H5CF::~H5CF()
{
  
}



bool H5CF::is_shared_dimension_set()
{
  return shared_dimension;
}

void H5CF::set_shared_dimension()
{
  shared_dimension = true;
}

const char *H5CF::get_CF_name(char *eos_name)
{
  string str(eos_name);

  DBG(cerr << ">get_CF_name:" << str << endl);
  DBG(cerr << eos_to_cf_map[str] << endl);
  if (eos_to_cf_map[str].size() > 0) {
    return eos_to_cf_map[str].c_str();
  } else {
    return str.c_str();
  }
}

string H5CF::get_EOS_name(string str)
{
  DBG(cerr << cf_to_eos_map[str] << endl);
  if (cf_to_eos_map[str].size() > 0) {
    return cf_to_eos_map[str];
  } else {
    return str;
  }
}

void H5CF::reset()
{
  shared_dimension = false;
  H5ShortName::reset();
    /* Not needed
#ifdef CF
  if(!eos_to_cf_map.empty()){
    eos_to_cf_map.clear();
  }
  if(!cf_to_eos_map.empty()){
    cf_to_eos_map.clear();
  }
#endif
    */
    
}
