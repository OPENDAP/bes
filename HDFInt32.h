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
// $RCSfile: HDFInt32.h,v $ - HDFInt32 class declaration
//
// $Log: HDFInt32.h,v $
// Revision 1.3  1997/03/10 22:45:29  jimg
// Update for 2.12
//
// Revision 1.3  1996/09/24 20:53:26  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFINT32_H
#define _HDFINT32_H

// STL/libg++ includes
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif

// DODS includes
#include "Int32.h"

class HDFInt32: public Int32 {
public:
    HDFInt32(const String &n = (char *)0);
    virtual ~HDFInt32();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const String &, int &); 
};

Int32 *NewInt32(const String &n);

typedef HDFInt32 * HDFInt32Ptr;

#endif // _HDFINT32_H

