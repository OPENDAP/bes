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
// Revision 1.4  1999/05/06 03:23:35  jimg
// Merged changes from no-gnu branch
//
// Revision 1.3.20.1  1999/05/06 00:27:23  jimg
// Jakes String --> string changes
//
// Revision 1.3  1997/03/10 22:45:43  jimg
// Update for 2.12
//
// Revision 1.5  1996/11/21 23:20:27  todd
// Added error return value to read() mfunc.
//
// Revision 1.4  1996/09/24 20:57:34  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#include "HDFUrl.h"

HDFUrl::HDFUrl(const string &n) : Url(n) {}
HDFUrl::~HDFUrl() {}
BaseType *HDFUrl::ptr_duplicate() { return new HDFUrl(*this); }  
bool HDFUrl::read(const string &, int &err) { 
  set_read_p(true); err = -1; return true; 
}

Url *NewUrl(const string &n) { return new HDFUrl(n); }
