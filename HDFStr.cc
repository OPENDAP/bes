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
// $RCSfile: HDFStr.cc,v $ - HDFStr class implementation
//
// $Log: HDFStr.cc,v $
// Revision 1.1  1996/10/31 18:43:48  jimg
// Added.
//
// Revision 1.4  1996/09/24 20:57:34  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#include "HDFStr.h"

HDFStr::HDFStr(const String &n = (char *)0) : Str(n) {}
HDFStr::~HDFStr() {}
BaseType *HDFStr::ptr_duplicate() { return new HDFStr(*this); }  
bool HDFStr::read(const String &, int &) { set_read_p(true); return true; }

Str *NewStr(const String &n) { return new HDFStr(n); }
