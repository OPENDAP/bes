
// -*- C++ -*-

// (c) COPYRIGHT URI/MIT 1994-1996
// Please read the full copyright statement in the file COPYRIGH.  
//
// Authors:
//      reza            Reza Nekovei (reza@intcomm.net)

// netCDF sub-class implementation for HDF5Byte,...HDF5Grid.
// The files are patterned after the subcalssing examples 
// Test<type>.c,h files.
//
// ReZa 1/12/95

/* $Log: HDF5Int32.h,v $
/* Revision 1.1  2001/07/16 22:49:27  jimg
/* Initial revision
/*
/* Revision 1.1  1999/07/28 00:22:43  jimg
/* Added
/*
/* Revision 1.4  1999/05/07 23:45:32  jimg
/* String --> string fixes
/*
/* Revision 1.3  1996/09/17 17:06:34  jimg
/* Merge the release-2-0 tagged files (which were off on a branch) back into
/* the trunk revision.
/*
 * Revision 1.2.4.2  1996/07/10 21:44:13  jimg
 * Changes for version 2.06. These fixed lingering problems from the migration
 * from version 1.x to version 2.x.
 * Removed some (but not all) warning generated with gcc's -Wall option.
 *
 * Revision 1.2.4.1  1996/06/25 22:04:35  jimg
 * Version 2.0 from Reza.
 *
 * Revision 1.2  1995/03/16  16:56:37  reza
 * Updated for the new DAP. All the read_val mfunc. and their memory management
 * are now moved to their parent class.
 * Data transfers are now in binary while DAS and DDS are still sent in ASCII.
 *
 * Revision 1.1  1995/02/10  04:57:37  reza
 * Added read and read_val functions.
 *
*/

#ifndef _HDF5Int32_h
#define _HDF5Int32_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>
#include "Int32.h"

#
extern "C"{int get_data(hid_t dset,void *buf,char *);}

//extern {static string print_type(hid_t datatype));}
extern Int32 * NewInt32(const string &n = "");

class HDF5Int32: public Int32 {

private:
  hid_t dset_id;
  hid_t ty_id;

public:
friend string return_type(hid_t datatype);   
    HDF5Int32(const string &n = "");
    virtual ~HDF5Int32() {}

    virtual BaseType *ptr_duplicate();
    
    virtual bool read(const string &dataset, int &error);
    
  void set_did(hid_t dset);
  void set_tid(hid_t type);
  hid_t get_did();
  hid_t get_tid();
};



#endif

