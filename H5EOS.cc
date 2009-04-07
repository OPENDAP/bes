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

#include "H5EOS.h"

using namespace std;

int hdfeoslex();
int hdfeosparse(void *arg);
struct yy_buffer_state;
yy_buffer_state *hdfeos_scan_string(const char *str);
extern bool valid_projection;
extern bool grid_structure_found;
H5EOS::H5EOS()
{
  valid = false;
  point_lower = 0.0;
  point_upper = 0.0;
  point_left = 0.0;
  point_right = 0.0;
  gradient_x = 0.0;
  gradient_y = 0.0;
  dimension_data = NULL;
  bmetadata_Struct = false;
#ifdef NASA_EOS_META
  bmetadata_Archived = false;
  bmetadata_Core = false;
  bmetadata_core = false;
  bmetadata_product = false;
  bmetadata_subset = false;
#endif

}

H5EOS::~H5EOS()
{
}


bool H5EOS::has_group(hid_t id, const char *name)
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

bool H5EOS::has_dataset(hid_t id, const char *name)
{
  hid_t hid;
  H5E_BEGIN_TRY {
    hid = H5Dopen(id, name);
  } H5E_END_TRY;              // <hyokyung 2007.06.20. 10:25:25>
  if (hid < 0) {
    return false;
  } else {
    return true;
  }
}

void H5EOS::add_data_path(string full_path)
{
#ifdef CF
  string short_path = generate_short_name(full_path);
  DBG(cerr << "Short path is:" << short_path << endl);  
  long_to_short[full_path] = short_path;
#endif

  DBG(cerr << "Full path is:" << full_path << endl);
  full_data_paths.push_back(full_path);
}


void H5EOS::add_dimension_list(string full_path, string dimension_list)
{
  full_data_path_to_dimension_list_map[full_path] = dimension_list;
  DBG(cerr << "Dimension List is:" <<
      full_data_path_to_dimension_list_map[full_path] << endl);
}

void H5EOS::add_dimension_map(string dimension_name, int dimension)
{
  bool has_dimension = false;
#ifdef CF
  dimension_name = cut_long_name(dimension_name);
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

bool H5EOS::check_eos(hid_t id)
{

  reset();
  
  // Check if this file has the group called "HDFEOS INFORMATION".
  if (has_group(id, "HDFEOS INFORMATION")) {

    if (set_metadata(id, "StructMetadata", metadata_Struct)) {
      valid = true;
    } else {
      valid = false;
    }

    if (valid) {
      hdfeos_scan_string(metadata_Struct);
      hdfeosparse(this);
#ifdef NASA_EOS_META
      set_metadata(id, "coremetadata", metadata_core);
      set_metadata(id, "CoreMetadata", metadata_Core);
      set_metadata(id, "ArchivedMetadata", metadata_Archived);
      set_metadata(id, "subsetmetadata", metadata_subset);
      set_metadata(id, "productmetadata", metadata_product);
#endif
    }

    return valid;
  }
  return false;
}

void H5EOS::get_dimensions(string name, vector < string > &tokens)
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
int H5EOS::get_dimension_size(string name)
{
  return dimension_map[name];
}

bool H5EOS::is_grid(string name)
{
  int i;
  for (i = 0; i < (int) full_data_paths.size(); i++) {
    std::string str = full_data_paths.at(i);
    if (str == name) {
      return true;
    }
  }
  return false;
}

bool H5EOS::is_valid()
{
  return valid;
}

void H5EOS::print()
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

bool H5EOS::set_dimension_array()
{
  int i = 0;
  int j = 0;
  int size = dimensions.size();

  dods_float32 *convbuf = NULL;
    
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
	  convbuf[i] = (dods_float32)
	    (point_lower + (float) i * gradient_y + (gradient_y / 2.0)) / 1000000.0;
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

string H5EOS::get_grid_name(string full_path)
{
  int end = full_path.find("/", 14);
  return full_path.substr(0, end + 1);        // Include the last "/".
}

int H5EOS::get_dimension_data_location(string dimension_name)
{
  int j;
  for (j = 0; j < (int) dimensions.size(); j++) {
    string dim_name = dimensions.at(j);
    if (dim_name == dimension_name)
      return j;
  }
  return -1;
}

bool H5EOS::set_metadata(hid_t id, char *metadata_name, char *chr_all)
{
  bool valid = false;
  int i = -1;

  for (i = -1; i < 10; i++) { // Assume that 10 is reasonable count of StructMetadata files
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

      if ((datatype = H5Dget_type(dset)) < 0) {
	cerr << "H5EOS.cc failed to obtain datatype from  dataset "
	     << dset << endl;
	break;
      }
      if ((dataspace = H5Dget_space(dset)) < 0) {
	cerr <<
	  "H5EOS.cc failed to obtain dataspace from  dataset " <<
	  dset << endl;
	break;
      }
      size_t size = H5Tget_size(datatype);
      char *chr = new char[size + 1];
      H5Dread(dset, datatype, dataspace, dataspace, H5P_DEFAULT,
	      (void *) chr);
      strcat(chr_all, chr);
      valid = true;
      delete[] chr;
    } else {
      // The sequence can skip <metdata>.0.
      // Forexample, "coremetadata" and then "coremetadata.1".
      if (i > 2)
	break;
    }
  }

  return valid;

}

void H5EOS::reset()
{
  int j;
  grid_structure_found = false;
  valid_projection = false;
  valid = false;
  point_lower = 0.0;
  point_upper = 0.0;
  point_left = 0.0;
  point_right = 0.0;
  gradient_x = 0.0;
  gradient_y = 0.0;
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
  
  if(!dimensions.empty()){
    dimensions.clear();
  }
  if(!full_data_paths.empty()){
    full_data_paths.clear();
  }
  // Clear the contents of the metadata string buffer.
  strcpy(metadata_Struct, "");
  strcpy(metadata_Archived, "");
  strcpy(metadata_Core, "");
  strcpy(metadata_core, "");
  strcpy(metadata_product, "");
  strcpy(metadata_subset, "");
  H5CF::reset();
}

void H5EOS::get_all_dimensions(vector < string > &tokens)
{
  int j;
  for (j = 0; j < (int) dimensions.size(); j++) {
    string dim_name = dimensions.at(j);
    tokens.push_back(dim_name);
    DBG(cerr << "=get_all_dimensions():Dim name = " << dim_name <<
	std::endl);
  }
}

