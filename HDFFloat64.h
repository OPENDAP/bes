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
// $RCSfile: HDFFloat64.h,v $ - HDFFloat64 class declaration
//
// $Log: HDFFloat64.h,v $
// Revision 1.2  1997/02/10 02:01:20  jimg
// Update from Todd.
//
// Revision 1.3  1996/09/24 20:53:26  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFFLOAT64_H
#define _HDFFLOAT64_H

// STL/libg++ includes
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif

// DODS includes
#include "Float64.h"

class HDFFloat64: public Float64 {
public:
    HDFFloat64(const String &n = (char *)0);
    virtual ~HDFFloat64();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const String &, int &);
};

Float64 *NewFloat64(const String &n);

typedef HDFFloat64 * HDFFloat64Ptr;

#endif // _HDFFLOAT64_H

