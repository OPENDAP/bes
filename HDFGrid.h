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
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFGRID_H
#define _HDFGRID_H

// STL includes
#include <string>

// DODS includes
#include "Grid.h"
#include "ReadTagRef.h"

class HDFGrid: public Grid, public ReadTagRef {
public:
    HDFGrid(const string &n = "");
    virtual ~HDFGrid();
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &dataset);
    virtual vector<array_ce> HDFGrid::get_map_constraints();
    virtual bool read_tagref(const string &dataset, int32 tag, int32 ref, int &error);
};

Grid *NewGrid(const string &n);

// $Log: HDFGrid.h,v $
// Revision 1.8  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
// Revision 1.7.4.1  2002/02/05 17:46:56  jimg
// Added the get_map_constraint() method.
//
// Revision 1.7  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.6  1999/05/06 03:23:34  jimg
// Merged changes from no-gnu branch
//
// Revision 1.5.10.1  1999/05/06 00:27:22  jimg
// Jakes String --> string changes
//
// Revision 1.5  1998/04/06 16:08:18  jimg
// Patch from Jake Hamby; change from switch to Mixin class for read_ref()
//
// Revision 1.4  1998/04/03 18:34:23  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.3  1997/03/10 22:45:27  jimg
// Update for 2.12
//
// Revision 1.3  1996/09/24 20:53:26  todd
// Added copyright and header.
//

#endif // _HDFGRID_H

