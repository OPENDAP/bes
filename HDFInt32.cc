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
// $RCSfile: HDFInt32.cc,v $ - HDFInt32 class implementation
//
// $Log: HDFInt32.cc,v $
// Revision 1.1  1996/10/31 18:43:37  jimg
// Added.
//
// Revision 1.4  1996/09/24 20:53:26  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#include "HDFInt32.h"

HDFInt32::HDFInt32(const String &n = (char *)0) : Int32(n) {}
HDFInt32::~HDFInt32() {}
BaseType *HDFInt32::ptr_duplicate() { return new HDFInt32(*this); }
bool HDFInt32::read(const String &, int &) { set_read_p(true); return true; }

Int32 *NewInt32(const String &n) { return new HDFInt32(n); }
