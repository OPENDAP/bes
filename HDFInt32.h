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
// Revision 1.4  1999/05/06 03:23:34  jimg
// Merged changes from no-gnu branch
//
// Revision 1.3.20.1  1999/05/06 00:27:22  jimg
// Jakes String --> string changes
//
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

// STL includes
#include <string>

// DODS includes
#include "Int32.h"

class HDFInt32: public Int32 {
public:
    HDFInt32(const string &n = "");
    virtual ~HDFInt32();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &, int &); 
};

Int32 *NewInt32(const string &n);

typedef HDFInt32 * HDFInt32Ptr;

#endif // _HDFINT32_H

