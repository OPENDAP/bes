/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1998, California Institute of Technology.  
// ALL RIGHTS RESERVED.   U.S. Government Sponsorship acknowledged. 
//
// Please read the full copyright notice in the file COPYRIGH 
// in this directory.
//
// Author: Jake Hamby, NASA/Jet Propulsion Laboratory
//         Jake.Hamby@jpl.nasa.gov
//
// $RCSfile: ReadTagRef.h,v $ - Declaration of abstract read_tagref() method
//
// $Log: ReadTagRef.h,v $
// Revision 1.2  1999/05/06 03:23:35  jimg
// Merged changes from no-gnu branch
//
// Revision 1.1.10.1  1999/05/06 00:27:23  jimg
// Jakes String --> string changes
//
// Revision 1.1  1998/04/06 16:11:43  jimg
// Added by Jake Hamby (via patch)
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _READTAGREF_H
#define _READTAGREF_H

// STL includes
#include <string>
// HDF includes (int32 type)
#include <hdf.h>

class ReadTagRef {
public:
  virtual bool read_tagref(const string &dataset, int32 tag, int32 ref, int &error) = 0;
};

#endif // _READTAGREF_H
