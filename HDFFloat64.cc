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
/////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include "InternalErr.h"
#include "HDFFloat64.h"

HDFFloat64::HDFFloat64(const string &n) : Float64(n) {}
HDFFloat64::~HDFFloat64() {}
BaseType *HDFFloat64::ptr_duplicate() { return new HDFFloat64(*this); }
bool HDFFloat64::read(const string &) { 
  throw InternalErr(__FILE__, __LINE__, "Unimplemented read method called.");
}

Float64 *NewFloat64(const string &n) { return new HDFFloat64(n); }

// $Log: HDFFloat64.cc,v $
// Revision 1.5  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.4  1999/05/06 03:23:34  jimg
// Merged changes from no-gnu branch
//
// Revision 1.3.20.1  1999/05/06 00:27:21  jimg
// Jakes String --> string changes
//
// Revision 1.3  1997/03/10 22:45:21  jimg
// Update for 2.12
//
// Revision 1.5  1996/11/21 23:20:27  todd
// Added error return value to read() mfunc.
//
// Revision 1.4  1996/09/24 20:53:26  todd
// Added copyright and header.
