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
// $RCSfile: HDFStructure.h,v $ - HDFStructure class declarations
//
// $Log: HDFStructure.h,v $
// Revision 1.3  1997/03/10 22:45:39  jimg
// Update for 2.12
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFSTRUCTURE_H
#define _HDFSTRUCTURE_H

// STL/libg++ includes
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif

// DODS includes
#include "Structure.h"

class HDFStructure: public Structure {
public:
    HDFStructure(const String &n = (char *)0);
    virtual ~HDFStructure();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const String &, int &);
//    virtual int nvars(void) { return _vars.length(); }
};

Structure *NewStructure(const String &n);

typedef HDFStructure * HDFStructurePtr;



#endif // _HDFSTRUCTURE_H

