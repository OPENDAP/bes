/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999, University of Rhode Island
// ALL RIGHTS RESERVED. 
//
// Please read the full copyright notice in the file COPYRIGHT in
// DODS_Root/etc

// Author: James Gallagher
//
// $Log: HDFFloat32.cc,v $
// Revision 1.1  1999/03/27 00:20:16  jimg
// Added
//

#include "HDFFloat32.h"

HDFFloat32::HDFFloat32(const String &n = (char *)0) : Float32(n) {}
HDFFloat32::~HDFFloat32() {}
BaseType *HDFFloat32::ptr_duplicate() { return new HDFFloat32(*this); }
bool HDFFloat32::read(const String &, int &err) { 
  set_read_p(true); err = 1; return false; 
}

Float32 *NewFloat32(const String &n) { return new HDFFloat32(n); }
