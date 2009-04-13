/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf5 data handler for the OPeNDAP data server.
//
// Author:   Hyo-Kyung Lee <hyoklee@hdfgroup.org>
// Copyright (c) 2007 The HDF Group
//
/////////////////////////////////////////////////////////////////////////////
// #define DODS_DEBUG
#include <iostream>

#include <debug.h>
#include <util.h>

#include "HDFEOS.h"

using namespace std;

int hdfeos2lex();
int hdfeos2parse(void *arg);
struct yy_buffer_state;
yy_buffer_state *hdfeos2_scan_string(const char *str);


HDFEOS::HDFEOS()
{
  _parsed = false;
  valid = false;
  borigin_upper = false;
  point_lower = 0.0;
  point_upper = 0.0;
  point_left = 0.0;
  point_right = 0.0;
  gradient_x = 0.0;
  gradient_y = 0.0;
  dimension_data = NULL;
#ifdef CF
  shared_dimension = false;
#endif
  bmetadata_Struct = false;
#ifdef NASA_EOS_META
  bmetadata_Archived = false;
  bmetadata_Core = false;
  bmetadata_core = false;
  bmetadata_product = false;
  bmetadata_subset = false;
#endif

}

HDFEOS::~HDFEOS()
{
}



void HDFEOS::add_data_path(string full_path)
{
#ifdef SHORT_PATH
  full_path = get_short_name(full_path);
#endif

  DBG(cerr << "Full path is:" << full_path << endl);
  full_data_paths.push_back(full_path);
}


void HDFEOS::add_dimension_list(string full_path, string dimension_list)
{

#ifdef SHORT_PATH
  full_path = get_short_name(full_path);
  dimension_list = get_short_name(dimension_list);
#endif

  full_data_path_to_dimension_list_map[full_path] = dimension_list;
  DBG(cerr << "Dimension List is:" <<
      full_data_path_to_dimension_list_map[full_path] << endl);
}

void HDFEOS::add_dimension_map(string dimension_name, int dimension)
{
  bool has_dimension = false;
#ifdef SHORT_PATH
  dimension_name = get_short_name(dimension_name);
#endif

  int i;
  for (i = 0; i < (int) dimensions.size(); i++) {
    std::string str = dimensions.at(i);
    if (str == dimension_name) {
      has_dimension = true;
      break;
    }
  }

  if (!has_dimension) {
    dimensions.push_back(dimension_name);
    dimension_map[dimension_name] = dimension;
  }
}


void HDFEOS::get_dimensions(string name, vector < string > &tokens)
{
  string str = full_data_path_to_dimension_list_map[name];
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

///////////////////////////////////////////////////////////////////////////////
/// Retrieve the dimension list from the argument "name" grid and tokenize the list into the string vector.
///////////////////////////////////////////////////////////////////////////////
int HDFEOS::get_dimension_size(string name)
{
  return dimension_map[name];
}

bool HDFEOS::is_grid(string name)
{
  int i;
  // cout << ">HDFEOS::is_grid() Name:" << name << endl;
  for (i = 0; i < (int) full_data_paths.size(); i++) {
    std::string str = full_data_paths.at(i);
    // cout << "=HDFEOS::is_grid() Name:" << name << "Datapath: " << str << endl; // <hyokyung 2008.11.11. 14:39:12>
    if (str == name) {
      return true;
    }
  }
  return false;
}

bool HDFEOS::is_valid()
{
  return valid;
}

void HDFEOS::print()
{

  cout << "Left = " << point_left << endl;
  cout << "Right = " << point_right << endl;
  cout << "Lower = " << point_lower << endl;
  cout << "Upper = " << point_upper << endl;

  cout << "Total number of paths = " << full_data_paths.size() << endl;
  /*
  for (int i = 0; i < (int) full_data_paths.size(); i++) {
      cout << "Element " << full_data_paths.at(i) << endl;
  }
  */
}

bool HDFEOS::set_dimension_array()
{
  int i = 0;
  int j = 0;
  int size = dimensions.size();

  dods_float32 *convbuf = NULL;
    
  //#ifdef CF
  // Resetting this flag is necessary for Hyrax. <hyokyung 2008.07.18. 14:50:05>
  // This has been replaced by reset() member function. <hyokyung 2008.10.20. 14:08:15>
  //    shared_dimension = false;
  //#endif
  
  if (libdap::size_ok(sizeof(dods_float32), size))
    dimension_data = new dods_float32*[size];
  else
    throw InternalErr(__FILE__, __LINE__,
		      "Unable to allocate memory.");
  DBG(cerr << ">set_dimension_array():Dimensions size = " << size <<
      endl);
  for (j = 0; j < (int) dimensions.size(); j++) {
    string dim_name = dimensions.at(j);
    int dim_size = dimension_map[dim_name];

    DBG(cerr << "=set_dimension_array():Dim name = " << dim_name <<
	std::endl);
    DBG(cerr << "=set_dimension_array():Dim size = " << dim_size <<
	std::endl);

    if (dim_size > 0) {
      if (libdap::size_ok(sizeof(dods_float32), dim_size))
	convbuf = new dods_float32[dim_size];
      else
	throw InternalErr(__FILE__, __LINE__,
			  "Unable to allocate memory.");

      if ((dim_name.find("XDim", (int) dim_name.size() - 4)) !=
	  string::npos) {
	gradient_x =
	  (point_right - point_left) / (float) (dim_size);
	for (i = 0; i < dim_size; i++) {
	  convbuf[i] = (dods_float32)
	    (point_left + (float) i * gradient_x + (gradient_x / 2.0)) / 1000000.0;
	}
      } else if ((dim_name.find("YDim", (int) dim_name.size() - 4))
		 != string::npos) {
	gradient_y =
	  (point_upper - point_lower) / (float) (dim_size);	
	for (i = 0; i < dim_size; i++) {
	  float sign = -1;
	  /* Doesn't matter
	  if(borigin_upper){
	    cerr << "sign is reversed." << endl;
	    sign = -1;
	  }
	  */
	  convbuf[i] = (dods_float32)
	    (sign) * (point_lower + (float) i * gradient_y + (gradient_y / 2.0)) / 1000000.0;
	}
      } else {
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
  DBG(cerr << "<set_dimension_array()" << endl);
  return true;
}

string HDFEOS::get_grid_name(string full_path)
{
  int end = full_path.find("/", 14);
  return full_path.substr(0, end + 1);        // Include the last "/".
}

int HDFEOS::get_dimension_data_location(string dimension_name)
{
  int j;
  for (j = 0; j < (int) dimensions.size(); j++) {
    string dim_name = dimensions.at(j);
    if (dim_name == dimension_name)
      return j;
  }
  return -1;
}

#ifdef SHORT_PATH
string HDFEOS::get_short_name(string varname)
{
  int pos = varname.find_last_of('/', varname.length() - 1);
  return varname.substr(pos + 1);
}
#endif

#ifdef CF
bool HDFEOS::is_shared_dimension_set()
{
  return shared_dimension;
}

void HDFEOS::set_shared_dimension()
{
  shared_dimension = true;
}

const char *HDFEOS::get_CF_name(char *eos_name)
{
  string str(eos_name);

  DBG(cerr << ">get_CF_name:" << str << endl);
  // <hyokyung 2007.08. 2. 14:25:58>  
  eos_to_cf_map["MissingValue"] = "missing_value";
  eos_to_cf_map["Units"] = "units";
  eos_to_cf_map["XDim"] = "lon";
  eos_to_cf_map["YDim"] = "lat";
  eos_to_cf_map["nCandidate"] = "lev";

  // eos_to_grads_map["Offset"] = "add_offset";
  // eos_to_grads_map["ScaleFactor"] = "scale_factor";
  // eos_to_grads_map["ValidRange"] = "valid_range";

  DBG(cerr << eos_to_cf_map[str] << endl);
  if (eos_to_cf_map[str].size() > 0) {
    return eos_to_cf_map[str].c_str();
  } else {
    return str.c_str();
  }
}

string HDFEOS::get_EOS_name(string str)
{
  DBG(cerr << ">get_EOS_name:" << str << endl);
  cf_to_eos_map["lon"] = "XDim";
  cf_to_eos_map["lat"] = "YDim";
  cf_to_eos_map["lev"] = "nCandidate";

  DBG(cerr << cf_to_eos_map[str] << endl);
  if (cf_to_eos_map[str].size() > 0) {
    return cf_to_eos_map[str];
  } else {
    return str;
  }
}


#endif



void HDFEOS::reset()
{
  int j;
  _parsed = false;
  valid = false;
  point_lower = 0.0;
  point_upper = 0.0;
  point_left = 0.0;
  point_right = 0.0;
  gradient_x = 0.0;
  gradient_y = 0.0;
#ifdef CF
  shared_dimension = false;
#endif
  bmetadata_Struct = false;
#ifdef NASA_EOS_META
  bmetadata_Archived = false;
  bmetadata_Core = false;
  bmetadata_core = false;
  bmetadata_product = false;
  bmetadata_subset = false;
#endif
    
  if(dimension_data != NULL){
    for (j = 0; j < (int) dimensions.size(); j++) {
      if(dimension_data[j] != NULL)
	delete dimension_data[j];
    }
    delete dimension_data;
    dimension_data = NULL;
  }

  if(!dimension_map.empty()){
    dimension_map.clear();
  }
  if(!full_data_path_to_dimension_list_map.empty()){
    full_data_path_to_dimension_list_map.clear();
  }
#ifdef CF
  if(!eos_to_cf_map.empty()){
    eos_to_cf_map.clear();
  }
  if(!cf_to_eos_map.empty()){
    cf_to_eos_map.clear();
  }
#endif
  
  if(!dimensions.empty()){
    dimensions.clear();
  }
  if(!full_data_paths.empty()){
    full_data_paths.clear();
  }
  // Clear the contents of the metadata string buffer.
  strcpy(metadata_Struct, "");
}
#ifdef CF
void HDFEOS::get_all_dimensions(vector < string > &tokens)
{
  int j;
  for (j = 0; j < (int) dimensions.size(); j++) {
    string dim_name = dimensions.at(j);
    tokens.push_back(dim_name);
    DBG(cerr << "=get_all_dimensions():Dim name = " << dim_name <<
	std::endl);
  }
}
#endif

bool HDFEOS::parse_struct_metadata(const char* str_metadata)
{
  if(!_parsed){
    hdfeos2_scan_string(str_metadata);
    hdfeos2parse(this);
    _parsed = true;
  }
}
