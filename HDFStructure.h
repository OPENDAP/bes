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
// Revision 1.6  1999/05/06 03:23:35  jimg
// Merged changes from no-gnu branch
//
// Revision 1.5.10.1  1999/05/06 00:27:23  jimg
// Jakes String --> string changes
//
// Revision 1.5  1998/04/06 16:08:20  jimg
// Patch from Jake Hamby; change from switch to Mixin class for read_ref()
//
// Revision 1.4  1998/04/03 18:34:24  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.3  1997/03/10 22:45:39  jimg
// Update for 2.12
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFSTRUCTURE_H
#define _HDFSTRUCTURE_H

// STL includes
#include <string>

// DODS includes
#include "Structure.h"
#include "ReadTagRef.h"

class HDFStructure: public Structure, public ReadTagRef {
public:
    HDFStructure(const string &n = "");
    virtual ~HDFStructure();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &, int &);
    virtual bool read_tagref(const string &dataset, int32 tag, int32 ref, int &error);
    virtual void set_read_p(bool state);
//    virtual int nvars(void) { return _vars.length(); }
};

Structure *NewStructure(const string &n);

typedef HDFStructure * HDFStructurePtr;



#endif // _HDFSTRUCTURE_H

