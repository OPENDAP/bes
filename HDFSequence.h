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
// $RCSfile: HDFSequence.h,v $ - HDFSequence class declaration
//
// $Log: HDFSequence.h,v $
// Revision 1.5  1998/04/06 16:08:19  jimg
// Patch from Jake Hamby; change from switch to Mixin class for read_ref()
//
// Revision 1.4  1998/04/03 18:34:24  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.3  1997/03/10 22:45:36  jimg
// Update for 2.12
//
// Revision 1.3  1996/09/24 20:57:34  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFSEQUENCE_H
#define _HDFSEQUENCE_H

// STL/libg++ includes
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif

// DODS includes
#include "Sequence.h"
#include "ReadTagRef.h"

class HDFSequence: public Sequence, public ReadTagRef {
public:
    HDFSequence(const String &n = (char *)0);
    virtual ~HDFSequence();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const String &, int &);
    virtual bool read_tagref(const String &dataset, int32 tag, int32 ref, int &error);
protected:
    int row;         // current row
    hdf_vdata vd;    // holds Vdata
};

Sequence *NewSequence(const String &n);

typedef HDFSequence * HDFSequencePtr;



#endif // _HDFSEQUENCE_H

