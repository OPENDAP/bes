/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999, University of Rhode Island
// ALL RIGHTS RESERVED. 
//
// Please read the full copyright notice in the file COPYRIGHT in
// DODS_Root/etc

// Author: James Gallagher
//
// $Log: HDFUInt16.h,v $
// Revision 1.1  1999/03/27 00:20:16  jimg
// Added
//

#ifndef _HDFUINT16_H
#define _HDFUINT16_H

// STL/libg++ includes
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif

// DODS includes
#include "UInt16.h"

class HDFUInt16: public UInt16 {
public:
    HDFUInt16(const String &n = (char *)0);
    virtual ~HDFUInt16();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const String &, int &); 
};

UInt16 *NewUInt16(const String &n);

typedef HDFUInt16 * HDFUInt16Ptr;

#endif // _HDFUINT16_H

