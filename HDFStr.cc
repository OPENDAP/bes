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
/////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include "InternalErr.h"
#include "HDFStr.h"

HDFStr::HDFStr(const string &n) : Str(n) {}
HDFStr::~HDFStr() {}
BaseType *HDFStr::ptr_duplicate() { return new HDFStr(*this); }  
bool HDFStr::read(const string &) { 
  throw InternalErr(__FILE__, __LINE__, "Unimplemented read method called.");
}

Str *NewStr(const string &n) { return new HDFStr(n); }

// $Log: HDFStr.cc,v $
// Revision 1.6  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
// Revision 1.5.4.1  2002/03/14 19:15:07  jimg
// Fixed use of int err in read() so that it's always initialized to zero.
// This is a fix for bug 135.
//
// Revision 1.5  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.4  1999/05/06 03:23:35  jimg
// Merged changes from no-gnu branch
//
// Revision 1.3.20.1  1999/05/06 00:27:22  jimg
// Jakes String --> string changes
//
// Revision 1.3  1997/03/10 22:45:37  jimg
// Update for 2.12
//
// Revision 1.5  1996/11/21 23:20:27  todd
// Added error return value to read() mfunc.
//
// Revision 1.4  1996/09/24 20:57:34  todd
// Added copyright and header.
