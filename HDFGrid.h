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
// $RCSfile: HDFGrid.h,v $ - HDFGrid class declaration
//
// $Log: HDFGrid.h,v $
// Revision 1.3  1997/03/10 22:45:27  jimg
// Update for 2.12
//
// Revision 1.3  1996/09/24 20:53:26  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFGRID_H
#define _HDFGRID_H

// STL/libg++ includes
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif

// DODS includes
#include "Grid.h"

class HDFGrid: public Grid {
public:
    HDFGrid(const String &n = (char *)0);
    virtual ~HDFGrid();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const String &dataset, int &);
};

Grid *NewGrid(const String &n);

#endif // _HDFGRID_H

