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
// $RCSfile: HDFUrl.h,v $ - HDFUrl class declaration
//
// $Log: HDFUrl.h,v $
// Revision 1.3  1997/03/10 22:45:44  jimg
// Update for 2.12
//
// Revision 1.3  1996/09/24 20:57:34  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFURL_H
#define _HDFURL_H

// STL/libg++ includes
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif

// DODS includes
#include "Url.h"

class HDFUrl: public Url {
public:
    HDFUrl(const String &n = (char *)0);
    virtual ~HDFUrl();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const String &, int &);
};

Url *NewUrl(const String &n);

typedef HDFUrl * HDFUrlPtr;



#endif // _HDFURL_H

