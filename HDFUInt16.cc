/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999, University of Rhode Island
// ALL RIGHTS RESERVED. 
//
// Please read the full copyright notice in the file COPYRIGHT in
// DODS_Root/etc

// Author: James Gallagher
//

#include "config_hdf.h"

#ifndef DONT_HAVE_UINT

#include "InternalErr.h"
#include "HDFUInt16.h"

HDFUInt16::HDFUInt16(const string &n = (char *)0) : UInt16(n) {}
HDFUInt16::~HDFUInt16() {}
BaseType *HDFUInt16::ptr_duplicate() { return new HDFUInt16(*this); }
bool HDFUInt16::read(const string &) { 
  throw InternalErr(__FILE__, __LINE__, "Unimplemented read method called.");
}

UInt16 *NewUInt16(const string &n) { return new HDFUInt16(n); }

// $Log: HDFUInt16.cc,v $
// Revision 1.3  2000/10/09 19:46:20  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.2  1999/05/06 00:27:23  jimg
// Jakes String --> string changes
//
// Revision 1.1  1999/03/27 00:20:16  jimg
// Added
//
#endif // DONT_HAVE_UINT
