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
// Revision 1.1  1996/10/31 18:43:56  jimg
// Added.
//
// Revision 1.2  1996/09/24 20:57:34  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFUINT32_H
#define _HDFUINT32_H

// STL/libg++ includes
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif

// DODS includes
#include "UInt32.h"

class HDFUInt32: public UInt32 {
public:
    HDFUInt32(const String &n = (char *)0);
    virtual ~HDFUInt32();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const String &, int &); 
};

UInt32 *NewUInt32(const String &n);

typedef HDFUInt32 * HDFUInt32Ptr;

#endif // _HDFUINT32_H

