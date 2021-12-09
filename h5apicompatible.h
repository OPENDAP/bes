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


#if ((H5_VERS_MAJOR == 1) && (H5_VERS_MINOR == 8))

#define H5RDEREFERENCE(obj_id,ref_type,ref) H5Rdereference(obj_id,ref_type,ref)
#define H5OGET_INFO_BY_IDX(loc_id, group_name, idx_type, order, n, oinfo, lapl_id) H5Oget_info_by_idx(loc_id, group_name, idx_type, order, n, oinfo, lapl_id)
#define H5OGET_INFO(loc_id, oinfo) H5Oget_info(loc_id,oinfo)
#define H5OVISIT(obj_id,idx_type,order,op,op_data) H5Ovisit(obj_id,idx_type,order,op,op_data)

#elif ((H5_VERS_MAJOR == 1) && (H5_VERS_MINOR == 10))

/*
  #if (defined H5_USE_18_API)
  #define H5RDEREFERENCE(obj_id,ref_type,ref) H5Rdereference1(obj_id,ref_type,ref)
  #define H5OGET_INFO(loc_id, oinfo) H5Oget_info1(loc_id,oinfo)
  #define H5OGET_INFO_BY_IDX(loc_id, group_name, idx_type, order, n, oinfo, lapl_id) H5Oget_info_by_idx(loc_id, group_name, idx_type, order, n, oinfo, lapl_id)
  #define H5OVISIT(obj_id,idx_type,order,op,op_data) H5Ovisit(obj_id,idx_type,order,op,op_data)
  #else                                        
*/
  #define H5RDEREFERENCE(obj_id,ref_type,ref) H5Rdereference2(obj_id,H5P_DEFAULT,ref_type,ref)
  #define H5OGET_INFO(loc_id, oinfo) H5Oget_info2(loc_id,oinfo,H5O_INFO_BASIC|H5O_INFO_NUM_ATTRS)
  #define H5OGET_INFO_BY_IDX(loc_id, group_name, idx_type, order, n, oinfo, lapl_id) H5Oget_info_by_idx2(loc_id, group_name, idx_type, order, n, oinfo, H5O_INFO_BASIC| H5O_INFO_NUM_ATTRS, lapl_id)
  #define H5OVISIT(obj_id,idx_type,order,op,op_data) H5Ovisit2(obj_id,idx_type,order,op,op_data,H5O_INFO_BASIC)
  //#endif

#else
#define H5RDEREFERENCE(obj_id,ref_type,ref) H5Rdereference2(obj_id,H5P_DEFAULT,ref_type,ref)
#define H5OGET_INFO_BY_IDX(loc_id, group_name, idx_type, order, n, oinfo, lapl_id) H5Oget_info_by_idx3(loc_id, group_name, idx_type, order, n, oinfo, H5O_INFO_BASIC | H5O_INFO_NUM_ATTRS, lapl_id)
#define H5OGET_INFO(loc_id, oinfo) H5Oget_info3(loc_id,oinfo,H5O_INFO_BASIC|H5O_INFO_NUM_ATTRS)
#define H5OVISIT(obj_id,idx_type,order,op,op_data) H5Ovisit3(obj_id,idx_type,order,op,op_data,H5O_INFO_BASIC)

#endif
#endif

