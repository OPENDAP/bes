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
// Revision 1.2  1997/02/10 02:01:16  jimg
// Update from Todd.
//
// Revision 1.3  1996/09/24 20:53:26  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFBYTE_H
#define _HDFBYTE_H

// STL/libg++ includes
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif

// DODS includes
#include "Byte.h"

class HDFByte: public Byte {
public:
    HDFByte(const String &n = (char *)0);
    virtual ~HDFByte();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const String &, int &);
};

Byte *NewByte(const String &n);

typedef HDFByte * HDFBytePtr;

#endif // _HDFBYTE_H

