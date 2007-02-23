#include<vector>
#include<iostream>

#include"CSV_Data.h"

CSV_Data::CSV_Data() {
  type = "";
  initialized = false;
}

CSV_Data::~CSV_Data() {
  if(initialized)
    delete data;
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
