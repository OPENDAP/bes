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
// $RCSfile: HDFStr.h,v $ - HDFStr class declaration
//
// $Log: HDFStr.h,v $
// Revision 1.3  1997/03/10 22:45:38  jimg
// Update for 2.12
//
// Revision 1.3  1996/09/24 20:57:34  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFSTR_H
#define _HDFSTR_H

// STL/libg++ includes
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif

// DODS includes
#include "dods-limits.h"
#include "Str.h"

class HDFStr: public Str {
public:
    HDFStr(const String &n = (char *)0);
    virtual ~HDFStr();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const String &, int &);
};

Str *NewStr(const String &n);

typedef HDFStr * HDFStrPtr;



#endif // _HDFSTR_H

