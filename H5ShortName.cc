#include "H5ShortName.h"

H5ShortName::H5ShortName()
{
  index = 0;
}

string H5ShortName::generate_short_name(string varname)
{
#ifdef SHORT_PATH  
  ostringstream oss;  
  int pos = varname.find_last_of('/', varname.length() - 1);
  oss << "A";
  oss << index;
  oss << varname.substr(pos + 1);
  // Prepend
  string temp_varname = oss.str();
  // Increment the index
  ++index;
  return temp_varname.substr(0, 15);
#else
  return varname;
#endif  
}

string H5ShortName::cut_long_name(string varname)
{
  int pos = varname.find_last_of('/', varname.length() - 1);
  return varname.substr(pos + 1);
}


string H5ShortName::get_long_name(string short_varname)
{
  return short_to_long[short_varname];
}

string H5ShortName::get_short_name(string long_varname)
{
  return long_to_short[long_varname];
}

void H5ShortName::reset()
{
  index = 0;
  
  if(!long_to_short.empty()){
    long_to_short.clear();
  }
  if(!short_to_long.empty()){
    short_to_long.clear();
  }

}
