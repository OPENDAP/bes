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
// $RCSfile: HDFByte.cc,v $ - HDFByte class implementation
//
// $Log: HDFByte.cc,v $
// Revision 1.1  1996/10/31 18:43:19  jimg
// Added.
//
// Revision 1.4  1996/09/24 20:53:26  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#include "HDFByte.h"

HDFByte::HDFByte(const String &n = (char *)0) : Byte(n) {}
HDFByte::~HDFByte() {}
BaseType *HDFByte::ptr_duplicate() {return new HDFByte(*this); }
bool HDFByte::read(const String &, int &) { set_read_p(true); return true; }

Byte *NewByte(const String &n) { return new HDFByte(n); }
