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
// Revision 1.4  1999/05/06 03:23:35  jimg
// Merged changes from no-gnu branch
//
// Revision 1.3.20.1  1999/05/06 00:27:23  jimg
// Jakes String --> string changes
//
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

// STL includes
#include <string>

// DODS includes
#include "dods-limits.h"
#include "Str.h"

class HDFStr: public Str {
public:
    HDFStr(const string &n = "");
    virtual ~HDFStr();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &, int &);
};

Str *NewStr(const string &n);

typedef HDFStr * HDFStrPtr;



#endif // _HDFSTR_H

