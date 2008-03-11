////////////////////////////////////////////////////////////////////////////////
/// This file is part of h5_dap_handler, A C++ implementation of the DAP handler
/// for HDF5 data.
///
/// This file contains functions that remember the paths within a HDF5 file.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// Copyright (c) 2007 HDF Group
///
/// All rights reserved.
//////////////////////////////////////////////////////////////////////////////////
// #define DODS_DEBUG
#include "debug.h"
#include "H5PathFinder.h"

using namespace std;

H5PathFinder::H5PathFinder()
{
}

H5PathFinder::~H5PathFinder()
{
  
}


bool H5PathFinder::add(string id, const string name)
{
  DBG(cerr
      << ">add(): id is:" << id
      << "   name is:" << name
      << endl);
  if(!visited(id)){
    
    id_to_name_map[id] = name;
    return true;
  }
  else{
    DBG(cerr
	<< "=add(): already added."
	<< endl);    
    return false;
  }
}


bool H5PathFinder::visited(string id)
{
  string str =  id_to_name_map[id];
  if(!str.empty()){
    return true;
  }
  else{
    return false;
  }
}

string H5PathFinder::get_name(string id)
{
  return id_to_name_map[id];
}

