/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999, University of Rhode Island
// ALL RIGHTS RESERVED. 
//
// Please read the full copyright notice in the file COPYRIGHT in
// DODS_Root/etc

// Author: James Gallagher
//
// $Log: HDFFloat32.h,v $
// Revision 1.1  1999/03/27 00:20:16  jimg
// Added
//

#ifndef _HDFFLOAT32_H
#define _HDFFLOAT32_H

// STL/libg++ includes
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif

// DODS includes
#include "Float32.h"

class HDFFloat32: public Float32 {
public:
    HDFFloat32(const String &n = (char *)0);
    virtual ~HDFFloat32();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const String &, int &);
};

Float32 *NewFloat32(const String &n);

typedef HDFFloat32 * HDFFloat32Ptr;

#endif // _HDFFLOAT32_H

