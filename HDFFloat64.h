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
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFFLOAT64_H
#define _HDFFLOAT64_H

// STL includes
#include <string>

// DODS includes
#include "Float64.h"

class HDFFloat64: public Float64 {
public:
    HDFFloat64(const string &n = "");
    virtual ~HDFFloat64();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &);
};

Float64 *NewFloat64(const string &n);

typedef HDFFloat64 * HDFFloat64Ptr;

// $Log: HDFFloat64.h,v $
// Revision 1.5  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.4  1999/05/06 03:23:34  jimg
// Merged changes from no-gnu branch
//
// Revision 1.3.20.1  1999/05/06 00:27:21  jimg
// Jakes String --> string changes
//
// Revision 1.3  1997/03/10 22:45:22  jimg
// Update for 2.12
//
// Revision 1.3  1996/09/24 20:53:26  todd
// Added copyright and header.
//

#endif // _HDFFLOAT64_H

