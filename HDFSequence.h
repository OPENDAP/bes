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
// Revision 1.1  1996/10/31 18:43:46  jimg
// Added.
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

class HDFSequence: public Sequence {
public:
    HDFSequence(const String &n = (char *)0);
    virtual ~HDFSequence();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const String &, int &);
};

Sequence *NewSequence(const String &n);

typedef HDFSequence * HDFSequencePtr;



#endif // _HDFSEQUENCE_H

