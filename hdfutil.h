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

void *ExportDataForDODS(const hdf_genvec& v);
void *ExportDataForDODS(const hdf_genvec& v, int i);
void *AccessDataForDODS(const hdf_genvec& v, int i);

// $Log: hdfutil.h,v $
// Revision 1.4  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
// Revision 1.3.36.1  2002/04/11 03:04:03  jimg
// Added AccessDataForDODS.
//
// Revision 1.3  1997/03/10 22:45:58  jimg
// Update for 2.12
//
// Revision 1.1  1996/09/24 22:38:16  todd
// Initial revision
