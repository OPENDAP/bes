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
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFURL_H
#define _HDFURL_H

// STL includes
#include <string>

// DODS includes
#include "Url.h"

class HDFUrl: public Url {
public:
    HDFUrl(const string &n = "");
    virtual ~HDFUrl();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &);
};

Url *NewUrl(const string &n);

typedef HDFUrl * HDFUrlPtr;

// $Log: HDFUrl.h,v $
// Revision 1.5  2000/10/09 19:46:20  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.4  1999/05/06 03:23:35  jimg
// Merged changes from no-gnu branch
//
// Revision 1.3.20.1  1999/05/06 00:27:23  jimg
// Jakes String --> string changes
//
// Revision 1.3  1997/03/10 22:45:44  jimg
// Update for 2.12
//
// Revision 1.3  1996/09/24 20:57:34  todd
// Added copyright and header.
//
//
#endif // _HDFURL_H

