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
// $Log: HDFUInt32.cc,v $
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
/////////////////////////////////////////////////////////////////////////////

#ifndef DONT_HAVE_UINT

#include "HDFUInt32.h"

HDFUInt32::HDFUInt32(const String &n = (char *)0) : UInt32(n) {}
HDFUInt32::~HDFUInt32() {}
BaseType *HDFUInt32::ptr_duplicate() { return new HDFUInt32(*this); }
bool HDFUInt32::read(const String &, int &err) { 
  set_read_p(true); err = -1; return true; 
}

UInt32 *NewUInt32(const String &n) { return new HDFUInt32(n); }

#endif // DONT_HAVE_UINT
