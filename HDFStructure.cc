#include "HDFStructure.h"

HDFStructure::HDFStructure(const String &n = (char *)0) : Structure(n) {}
HDFStructure::~HDFStructure() {}
BaseType *HDFStructure::ptr_duplicate() { return new HDFStructure(*this); }  
bool HDFStructure::read(const String &, int &) 
{ set_read_p(true); return false; }

Structure *NewStructure(const String &n) { return new HDFStructure(n); }
