/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999, University of Rhode Island
// ALL RIGHTS RESERVED. 
//
// Please read the full copyright notice in the file COPYRIGHT in
// DODS_Root/etc

// Author: James Gallagher
//
// $Log: HDFInt16.h,v $
// Revision 1.2  1999/05/06 00:27:22  jimg
// Jakes String --> string changes
//
// Revision 1.1  1999/03/27 00:20:16  jimg
// Added
//

#ifndef _HDFINT16_H
#define _HDFINT16_H

#include <string>

// DODS includes
#include "Int16.h"

class HDFInt16: public Int16 {
public:
    HDFInt16(const string &n = (char *)0);
    virtual ~HDFInt16();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &, int &); 
};

Int16 *NewInt16(const string &n);

typedef HDFInt16 * HDFInt16Ptr;

#endif // _HDFINT16_H

