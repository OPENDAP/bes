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
/////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include "InternalErr.h"
#include "HDFUrl.h"

HDFUrl::HDFUrl(const string &n) : Url(n) {}
HDFUrl::~HDFUrl() {}
BaseType *HDFUrl::ptr_duplicate() { return new HDFUrl(*this); }  
bool HDFUrl::read(const string &) { 
#if 0
  set_read_p(true); err = -1; return true; 
#endif
  throw InternalErr(__FILE__, __LINE__, "Unimplemented read method called.");
}

Url *NewUrl(const string &n) { return new HDFUrl(n); }

// $Log: HDFUrl.cc,v $
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
