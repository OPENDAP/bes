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
// Revision 1.1  1998/04/06 16:11:43  jimg
// Added by Jake Hamby (via patch)
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _READTAGREF_H
#define _READTAGREF_H

// STL/libg++ includes
#include <String.h>
// HDF includes (int32 type)
#include <hdf.h>

class ReadTagRef {
public:
  virtual bool read_tagref(const String &dataset, int32 tag, int32 ref, int &error) = 0;
};

#endif // _READTAGREF_H
