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
// Revision 1.6  1999/05/06 03:23:34  jimg
// Merged changes from no-gnu branch
//
// Revision 1.5.10.1  1999/05/06 00:27:21  jimg
// Jakes String --> string changes
//
// Revision 1.5  1998/04/06 16:08:17  jimg
// Patch from Jake Hamby; change from switch to Mixin class for read_ref()
//
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

// STL includes
#include <string>
#include <vector>

// DODS includes
#include "Array.h"
#include "ReadTagRef.h"

class HDFArray: public Array, public ReadTagRef {
public:
    HDFArray(const string &n = "", BaseType *v = 0);
    virtual ~HDFArray();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &dataset, int &error);
    virtual bool read_tagref(const string &dataset, int32 tag, int32 ref, int &error);
    bool GetSlabConstraint(vector<int>& start_array, vector<int>& edge_array, 
			   vector<int>& stride_array);
};

Array *NewArray(const string &n, BaseType *v);

#endif // _HDFARRAY_H
