#include "HDFStructure.h"

HDFStructure::HDFStructure(const String &n = (char *)0) : Structure(n) {}
HDFStructure::~HDFStructure() {}
BaseType *HDFStructure::ptr_duplicate() { return new HDFStructure(*this); }  
bool HDFStructure::read(const String &, int &err) { 
  set_read_p(true); err = -1; return true; 
}

Structure *NewStructure(const String &n) { return new HDFStructure(n); }
