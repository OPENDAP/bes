/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999, University of Rhode Island
// ALL RIGHTS RESERVED. 
//
// Please read the full copyright notice in the file COPYRIGHT in
// DODS_Root/etc

// Author: James Gallagher

#ifndef _HDFINT16_H
#define _HDFINT16_H

#include <string>

// DODS includes
#include "Int16.h"

class HDFInt16: public Int16 {
public:
    HDFInt16(const string &n = "");
    virtual ~HDFInt16();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &); 
};

Int16 *NewInt16(const string &n);

typedef HDFInt16 * HDFInt16Ptr;

// $Log: HDFInt16.h,v $
// Revision 1.4  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
// Revision 1.3.4.1  2002/04/12 00:03:14  jimg
// Fixed casts that appear throughout the code. I changed most/all of the
// casts to the new-style syntax. I also removed casts that we're not needed.
//
// Revision 1.3  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.2  1999/05/06 00:27:22  jimg
// Jakes String --> string changes
//
// Revision 1.1  1999/03/27 00:20:16  jimg
// Added
//

#endif // _HDFINT16_H

