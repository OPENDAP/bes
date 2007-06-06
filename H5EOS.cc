//
//
// #define DODS_DEBUG
#include "debug.h"
#include "H5EOS.h"
#include <iostream>
using namespace std;
int hdfeoslex();
int hdfeosparse(void *arg);
struct yy_buffer_state;
yy_buffer_state *hdfeos_scan_string(const char *str);

H5EOS::H5EOS()
{
  valid = false;
  point_lower = 0.0;
  point_upper = 0.0;
  point_left = 0.0;
  point_right = 0.0;
  dimension_data = NULL;  
}

H5EOS::~H5EOS()
{
  
}

bool H5EOS::has_group(hid_t id, const char* name)
{
  hid_t hid;
  H5E_BEGIN_TRY {
    hid = H5Gopen(id, name);
  } H5E_END_TRY;    
  if(hid < 0){
    return false;
  }
  else{
    return true;
  }

}

bool H5EOS::has_dataset(hid_t id, const char* name)
{

  if(H5Dopen(id, name) <0){
    return false;
  }
  else{
    return true;
  }
}

void H5EOS::add_data_path(const string full_path)
{
  full_data_paths.push_back(full_path);
  DBG(cerr << "Full path is:" << full_path << endl);
}


void H5EOS::add_dimension_list(const string full_path, const string dimension_list)
{
  full_data_path_to_dimension_list_map[full_path] =  dimension_list;
  DBG(cerr << "Dimension List is:" << full_data_path_to_dimension_list_map[full_path] << endl);
}

void H5EOS::add_dimension_map(const string dimension_name, int dimension)
{
  bool has_dimension = false;
  int i;
  for(i=0; i < dimensions.size(); i++){
    std::string str = dimensions.at(i);
    if(str == dimension_name){
      has_dimension = true;
      break;
    }
  }
  
  if(!has_dimension){
    dimensions.push_back(dimension_name);  
    dimension_map[dimension_name] =  dimension;
  }
}


// Check if this file is EOS file by examining metadata
bool H5EOS::check_eos(hid_t id)
{
  // Check if this file has the group called "HDFEOS INFORMATION".
  if(has_group(id, "HDFEOS INFORMATION")){
#ifdef FULL_TEST    
    // Check if this file has the dataset called "ArchivedMetadata".
    if(has_dataset(id, "/HDFEOS INFORMATION/ArchivedMetadata")){
      // Check if this file has the dataset called "CoreMetadata".
      if(has_dataset(id, "/HDFEOS INFORMATION/CoreMetadata")){
#endif	
	// Check if this file has the dataset called "StructMetadata".
	if(has_dataset(id, "/HDFEOS INFORMATION/StructMetadata.0"))
	  {
	    // Open the dataset.
	    hid_t dset = H5Dopen(id, "/HDFEOS INFORMATION/StructMetadata.0");
	    hid_t datatype, dataspace, memtype;
	    
	    if ((datatype = H5Dget_type(dset)) < 0) {
              cout << "H5EOS.cc failed to obtain datatype from  dataset " << dset << endl;
	      return false;
	    }
	    if ((dataspace = H5Dget_space(dset)) < 0) {
	      cout << "H5EOS.cc failed to obtain dataspace from  dataset " <<dset << endl;
	      return false;
	    }
	    size_t size = H5Tget_size(datatype);
	    char *chr = new char[size + 1];	    
	    H5Dread(dset, datatype, dataspace, dataspace, H5P_DEFAULT, (void*)chr);
	    hdfeos_scan_string(chr);
	    hdfeosparse(this);
	    valid = true;
	    return valid;
	  }
#ifdef FULL_test	
      }
    }
#endif    
  }
  return false; 
}


// Check if this class has parsed the argument "name" as grid.

// Retrieve the dimension list from the argument "name" grid and tokenize the list into the string vector.
void H5EOS::get_dimensions(const string name, vector<string>& tokens)
{
  string str = full_data_path_to_dimension_list_map[name];
  // Skip delimiters at beginning.
  string::size_type lastPos = str.find_first_not_of(',', 0);
  // Find first "non-delimiter".
  string::size_type pos     = str.find_first_of(',', lastPos);

  while (string::npos != pos || string::npos != lastPos)
    {
      // Found a token, add it to the vector.
      tokens.push_back(str.substr(lastPos, pos - lastPos));
      // Skip delimiters.  Note the "not_of"
      lastPos = str.find_first_not_of(',', pos);
      // Find next "non-delimiter"
      pos = str.find_first_of(',', lastPos);
    } 
}
// Retrieve the dimension list from the argument "name" grid and tokenize the list into the string vector.
int H5EOS::get_dimension_size(const string name)
{
  return dimension_map[name];
}

// Check if this class has parsed the argument "name" as grid.
bool H5EOS::is_grid(const string name)
{
  int i;
  for(i=0; i < full_data_paths.size(); i++){
    std::string str = full_data_paths.at(i);
    if(str == name){
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
  for(int i=0; i < full_data_paths.size(); i++){
    cout << "Element " << full_data_paths.at(i) << endl;
  }
}
  
bool H5EOS::set_dimension_array()
{
  int i = 0;
  int j = 0;
  int size = dimensions.size();
  
  dods_float32* convbuf = NULL;
  dimension_data = new dods_float32*[size];
  DBG(cerr << "Dimensions size = " << size  << endl);
  for(j=0; j < dimensions.size(); j++){
    string dim_name = dimensions.at(j);
    int dim_size = dimension_map[dim_name];
    
    DBG(cerr << "Dim name = " << dim_name << std::endl);
    DBG(cerr << "Dim size = " << dim_size << std::endl);
    
    if(dim_size > 0){
      
      convbuf = new dods_float32[dim_size];

      if((dim_name.find("XDim", (int)dim_name.size()-4)) != string::npos){
	float gradient_x = (point_right - point_left) / (float)(dim_size - 1);
	for(i=0; i < dim_size; i++){
	  convbuf[i] = (dods_float32)(point_left + (float)i * gradient_x);
	}
      }
      else if((dim_name.find("YDim", (int)dim_name.size()-4)) != string::npos){    
	float gradient_y = (point_upper - point_lower) / (float)(dim_size - 1);      
	for(i=0; i< dim_size; i++){
	  convbuf[i] = (dods_float32)(point_upper - (float)i * gradient_y);
	}      
      }
      else{
	for(i=0; i< dim_size; i++){
	  convbuf[i] = (dods_float32)i; // meaningless number.
	}
      }
    } // if dim_size > 0
    else{
      DBG(cerr << "Negative dimension " << endl);
    }
    dimension_data[j] = convbuf;        
  }
  return true;
}

string H5EOS::get_grid_name(string full_path)
{
  int end =  full_path.find("/", 14);
  return full_path.substr(0, end+1); // Include the last "/".
}

int H5EOS::get_dimension_data_location(string dimension_name)
{
  int j;
  for(j=0; j < dimensions.size(); j++){
    string dim_name = dimensions.at(j);
    if(dim_name == dimension_name)
      return j;
  }
  return -1;
}

