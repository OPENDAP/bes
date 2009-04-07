/// A class for keeping track of short names.
///
/// This class contains functions that generate a shortened variable names.
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// Copyright (c) 2009 The HDF Group
///
/// All rights reserved.
#ifndef _H5SHORTNAME_H
#define _H5SHORTNAME_H

#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

class H5ShortName {
  
public:
  int index;
  map < string, string > long_to_short;
  map < string, string > short_to_long;	
  
  H5ShortName();
  
  string cut_long_name(string a_name);  
  string generate_short_name(string a_name);
  string get_long_name(string a_short_name);  
  string get_short_name(string a_long_name);
  void reset();
};

#endif
