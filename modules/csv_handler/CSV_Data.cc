// CSV_Data.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Stephan Zednik <zednik@ucar.edu> and Patrick West <pwest@ucar.edu>
// and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//	zednik      Stephan Zednik <zednik@ucar.edu>
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include<vector>
#include<iostream>
#include<cstdlib>

#include"CSV_Data.h"

CSV_Data::CSV_Data() : data(0), type(""), initialized(false) {
}

CSV_Data::~CSV_Data() {
  if(initialized) {
    if(type.compare(string(STRING)) == 0) {
      delete (vector<string> *)data;
      initialized = false;
    } else if(type.compare(string(FLOAT32)) == 0) {
      delete (vector<float> *)data;
      initialized = false;
    } else if(type.compare(string(FLOAT64)) == 0) {
      delete (vector<double> *)data;
      initialized = false;
    } else if(type.compare(string(INT16)) == 0) {
      delete (vector<short> *)data;
      initialized = false;
    } else if(type.compare(string(INT32)) == 0) {
      delete (vector<int> *)data;
      initialized = false;
    }
  }
}

void CSV_Data::insert(CSV_Field* field, void* value) {

  if(type.compare("") == 0)
    type = field->getType();

  if(!initialized) {
    if(type.compare(string(STRING)) == 0) {
      data = new vector<string>();
      initialized = true;
    } else if(type.compare(string(FLOAT32)) == 0) {
      data = new vector<float>();
      initialized = true;
    } else if(type.compare(string(FLOAT64)) == 0) {
      data = new vector<double>();
      initialized = true;
    } else if(type.compare(string(INT16)) == 0) {
      data = new vector<short>();
      initialized = true;
    } else if(type.compare(string(INT32)) == 0) {
      data = new vector<int>();
      initialized = true;
    }
  }
  
  if(type.compare(string(STRING)) == 0) {
    string str = *reinterpret_cast<string*>(value);
    ((vector<string>*)data)->push_back(str);
  } else if(type.compare(string(FLOAT32)) == 0) {
    float flt = atof((reinterpret_cast<string*>(value))->c_str());
    ((vector<float>*)data)->push_back(flt);
  } else if(type.compare(string(FLOAT64)) == 0) {
    double dbl = atof((reinterpret_cast<string*>(value))->c_str());
    ((vector<double>*)data)->push_back(dbl);
  } else if(type.compare(string(INT16)) == 0) {
    short shrt = atoi((reinterpret_cast<string*>(value))->c_str());
    ((vector<short>*)data)->push_back(shrt);
  } else if(type.compare(string(INT32)) == 0) {
    int integer = atoi((reinterpret_cast<string*>(value))->c_str());
    ((vector<int>*)data)->push_back(integer);
  }
}

void* CSV_Data::getData() {
  return data;
}

string CSV_Data::getType() {
  return type;
}
