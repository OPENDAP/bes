/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999, University of Rhode Island
// ALL RIGHTS RESERVED. 
//
// Please read the full copyright notice in the file COPYRIGHT in
// DODS_Root/etc

// Author: James Gallagher
//
// $Log: HDFUInt16.cc,v $
// Revision 1.1  1999/03/27 00:20:16  jimg
// Added
//

#ifndef DONT_HAVE_UINT

#include "HDFUInt16.h"

HDFUInt16::HDFUInt16(const String &n = (char *)0) : UInt16(n) {}
HDFUInt16::~HDFUInt16() {}
BaseType *HDFUInt16::ptr_duplicate() { return new HDFUInt16(*this); }
bool HDFUInt16::read(const String &, int &err) { 
  set_read_p(true); err = 1; return false; 
}

UInt16 *NewUInt16(const String &n) { return new HDFUInt16(n); }

#endif // DONT_HAVE_UINT
