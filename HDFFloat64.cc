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
// $RCSfile: HDFFloat64.cc,v $ - HDFFloat64 class implementation
//
// $Log: HDFFloat64.cc,v $
// Revision 1.2  1997/02/10 02:01:19  jimg
// Update from Todd.
//
// Revision 1.5  1996/11/21 23:20:27  todd
// Added error return value to read() mfunc.
//
// Revision 1.4  1996/09/24 20:53:26  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#include "HDFFloat64.h"

HDFFloat64::HDFFloat64(const String &n = (char *)0) : Float64(n) {}
HDFFloat64::~HDFFloat64() {}
BaseType *HDFFloat64::ptr_duplicate() { return new HDFFloat64(*this); }
bool HDFFloat64::read(const String &, int &err) { 
  set_read_p(true); err = -1; return true; 
}

Float64 *NewFloat64(const String &n) { return new HDFFloat64(n); }
