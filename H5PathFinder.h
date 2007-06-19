////////////////////////////////////////////////////////////////////////////////
/// This file is part of h5_dap_handler, A C++ implementation of the DAP handler
/// for HDF5 data.
///
/// This file contains functions that remembers the paths within a HDF5
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// Copyright (c) 2007 HDF Group
///
/// All rights reserved.
//////////////////////////////////////////////////////////////////////////////////

#include <map>
#include <string>

using namespace std;

class H5PathFinder {
  
private:
  map<int, string>     id_to_name_map;

  
public:
  H5PathFinder();
  virtual ~H5PathFinder();
  
  bool add(int id, const string name);
  bool visited(int id);
  string get_name(int id);
  
};
