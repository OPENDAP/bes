/// A class for generating CF compliant output.
///
/// This class contains functions that generate CF compliant output
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// Copyright (c) 2009 The HDF Group
///
/// All rights reserved.

#ifndef _H5CF_H
#define _H5CF_H

#include "H5ShortName.h"
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <debug.h>

using namespace std;

class H5CF: public H5ShortName {

public:
  bool shared_dimension;
  map < string, string > eos_to_cf_map;
  map < string, string > cf_to_eos_map;

  H5CF();
  virtual ~H5CF();

  const char* get_CF_name(char *eos_name);
  string      get_EOS_name(string cf_name);

  bool        is_shared_dimension_set();  
  void        reset();
  void        set_shared_dimension();
  
};

#endif
