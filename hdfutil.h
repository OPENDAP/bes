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
// $RCSfile: hdfutil.h,v $ - Miscellaneous classes and routines for DODS HDF server
//
// $Log: hdfutil.h,v $
// Revision 1.3  1997/03/10 22:45:58  jimg
// Update for 2.12
//
// Revision 1.1  1996/09/24 22:38:16  todd
// Initial revision
//
//
/////////////////////////////////////////////////////////////////////////////

void *ExportDataForDODS(const hdf_genvec& v);
void *ExportDataForDODS(const hdf_genvec& v, int i);
