#ifndef HDFEOS2HandleType_H
#define HDFEOS2HandleType_H
#include <vector>
#include <string>


// Don't like to handle this way. Seem this is the only doable solution for now.
// KY 2012-07-31
// A global field name vector is set to 0. This vector includes all field names
// of which the datatypes need to be changed. This is mainly due to the applying
// of scale and offset(MODIS). The typical example is changing the datatype from 
// 16-bit integer to 32-bit float.
// The current handling is not ideal. It is because of added HDF-EOS2 attributes by HDF4 APIs
// To change the current handling may be a huge task.  KY 2012-8-1

std::vector <std::string> ctype_field_namelist;

#endif
