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
// Revision 1.1  1996/10/31 18:43:24  jimg
// Added.
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
bool HDFFloat64::read(const String &, int &)
{ set_read_p(true); return true; }

Float64 *NewFloat64(const String &n) { return new HDFFloat64(n); }
