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
// $RCSfile: HDFByte.h,v $ - HDFByte class declaration
//
// $Log: HDFByte.h,v $
// Revision 1.4  1999/05/06 03:23:34  jimg
// Merged changes from no-gnu branch
//
// Revision 1.3.20.1  1999/05/06 00:27:21  jimg
// Jakes String --> string changes
//
// Revision 1.3  1997/03/10 22:45:18  jimg
// Update for 2.12
//
// Revision 1.3  1996/09/24 20:53:26  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFBYTE_H
#define _HDFBYTE_H

// STL includes
#include <string>

// DODS includes
#include "Byte.h"

class HDFByte: public Byte {
public:
    HDFByte(const string &n = "");
    virtual ~HDFByte();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &, int &);
};

Byte *NewByte(const string &n);

typedef HDFByte * HDFBytePtr;

#endif // _HDFBYTE_H

