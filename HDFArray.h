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
// $RCSfile: HDFArray.h,v $ - HDFArray class declaration
//
// $Log: HDFArray.h,v $
// Revision 1.4  1998/04/03 18:34:21  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.3  1997/03/10 22:45:16  jimg
// Update for 2.12
//
// Revision 1.4  1996/09/24 20:25:18  todd
// Added copyright and header
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFARRAY_H
#define _HDFARRAY_H

// STL/libg++ includes
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif
#include <vector.h>

// DODS includes
#include "Array.h"
#include <hdf.h>

class HDFArray: public Array {
public:
    HDFArray(const String &n = (char *)0, BaseType *v = 0);
    virtual ~HDFArray();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const String &dataset, int &error);
    bool GetSlabConstraint(vector<int>& start_array, vector<int>& edge_array, 
			   vector<int>& stride_array);
    bool read_tagref(const String &datset, int32 tag, int32 ref, int &error);
};

Array *NewArray(const String &n, BaseType *v);

#endif // _HDFARRAY_H
