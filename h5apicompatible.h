 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by  The HDF Group                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of hdf5_handler, an HDF5 file handler for the OPeNDAP   *
 * data server.                                                              *
 * including terms governing use, modification, and redistribution, is       *
 * contained in the COPYING file, which can be found at the root of the      *
 * source code distribution tree.                                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
 
#ifndef H5APICOMPITABLE_H
#define H5APICOMPITABLE_H


#if (H5_VERS_MAJOR == 1 && H5_VERS_MINOR == 8)


#define H5RDEREFERENCE(obj_id,ref_type,ref) H5Rdereference(obj_id,ref_type,ref)
#else

#if (defined H5_USE_18_API)
#define H5RDEREFERENCE(obj_id,ref_type,ref) H5Rdereference1(obj_id,ref_type,ref)
#else                                        
#define H5RDEREFERENCE(obj_id,ref_type,ref) H5Rdereference2(obj_id,H5P_DEFAULT,ref_type,ref)
#endif

#endif


#endif

