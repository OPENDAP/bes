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
// $RCSfile: HDFUrl.cc,v $ - HDFUrl class implementation
//
// $Log: HDFUrl.cc,v $
// Revision 1.1  1996/10/31 18:43:57  jimg
// Added.
//
// Revision 1.4  1996/09/24 20:57:34  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#include "HDFUrl.h"

HDFUrl::HDFUrl(const String &n = (char *)0) : Url(n) {}
HDFUrl::~HDFUrl() {}
BaseType *HDFUrl::ptr_duplicate() { return new HDFUrl(*this); }  
bool HDFUrl::read(const String &, int &) { set_read_p(true); return true; }

Url *NewUrl(const String &n) { return new HDFUrl(n); }
