/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999, University of Rhode Island
// ALL RIGHTS RESERVED. 
//
// Please read the full copyright notice in the file COPYRIGHT in
// DODS_Root/etc

// Author: James Gallagher
//

#ifndef _HDFUINT16_H
#define _HDFUINT16_H

#include <string>

// DODS includes
#include "UInt16.h"

class HDFUInt16: public UInt16 {
public:
    HDFUInt16(const string &n = (char *)0);
    virtual ~HDFUInt16();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &); 
};

UInt16 *NewUInt16(const string &n);

typedef HDFUInt16 * HDFUInt16Ptr;

// $Log: HDFUInt16.h,v $
// Revision 1.3  2000/10/09 19:46:20  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.2  1999/05/06 00:27:23  jimg
// Jakes String --> string changes
//
// Revision 1.1  1999/03/27 00:20:16  jimg
// Added

#endif // _HDFUINT16_H

