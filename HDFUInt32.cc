/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1996, California Institute of Technology.  
// ALL RIGHTS RESERVED.   U.S. Government Sponsorship acknowledged. 
//
// Please read the full copyright notice in the file COPYRIGH 
// in this directory.
//
// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: HDFUInt32.cc,v $ - HDFUInt32 class implementation
//
/////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#ifndef DONT_HAVE_UINT

#include "InternalErr.h"
#include "HDFUInt32.h"

HDFUInt32::HDFUInt32(const string &n) : UInt32(n) {}
HDFUInt32::~HDFUInt32() {}
BaseType *HDFUInt32::ptr_duplicate() { return new HDFUInt32(*this); }
bool HDFUInt32::read(const string &) { 
  throw InternalErr(__FILE__, __LINE__, "Unimplemented read method called.");
}

UInt32 *NewUInt32(const string &n) { return new HDFUInt32(n); }

// $Log: HDFUInt32.cc,v $
// Revision 1.5  2000/10/09 19:46:20  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.4  1999/05/06 03:23:35  jimg
// Merged changes from no-gnu branch
//
// Revision 1.3.20.1  1999/05/06 00:27:23  jimg
// Jakes String --> string changes
//
// Revision 1.3  1997/03/10 22:45:41  jimg
// Update for 2.12
//
// Revision 1.4  1996/11/21 23:20:27  todd
// Added error return value to read() mfunc.
//
// Revision 1.3  1996/10/14 18:19:14  todd
// Added compile option DONT_HAVE_UINT to allow compilation until DODS has
// unsigned integer types.
//
// Revision 1.2  1996/09/24 20:57:34  todd
// Added copyright and header.
//
//
#endif // DONT_HAVE_UINT
