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
// $RCSfile: HDFUInt32.h,v $ - HDFUInt32 class declaration
//
// $Log: HDFUInt32.h,v $
// Revision 1.4  1999/05/06 03:23:35  jimg
// Merged changes from no-gnu branch
//
// Revision 1.3.20.1  1999/05/06 00:27:23  jimg
// Jakes String --> string changes
//
// Revision 1.3  1997/03/10 22:45:42  jimg
// Update for 2.12
//
// Revision 1.2  1996/09/24 20:57:34  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFUINT32_H
#define _HDFUINT32_H

// STL includes
#include <string>

// DODS includes
#include "UInt32.h"

class HDFUInt32: public UInt32 {
public:
    HDFUInt32(const string &n = "");
    virtual ~HDFUInt32();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &, int &); 
};

UInt32 *NewUInt32(const string &n);

typedef HDFUInt32 * HDFUInt32Ptr;

#endif // _HDFUINT32_H

