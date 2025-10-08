// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Kent Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009-2023 The HDF Group, Inc. and OPeNDAP, Inc.
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820  

#ifndef _HDF5ARRAY_H
#define _HDF5ARRAY_H 

#include <H5Ipublic.h>
#include <H5Rpublic.h>

#include <libdap/Array.h>

#include "h5get.h"


///////////////////////////////////////////////////////////////////////////////
/// \file HDF5Array.h
/// \brief A class for handling all types of array in HDF5 for the default option.
///
/// This class converts HDF5 array type into DAP array.
/// 
/// \author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// \author Kent Yang       (myang6@hdfgroup.org)
/// \author James Gallagher (jgallagher@opendap.org)
///
////////////////////////////////////////////////////////////////////////////////
class HDF5Array:public libdap::Array {

  private:

    int d_num_dim = 0;
    hsize_t d_num_elm = 0;
    
    hsize_t d_memneed = 0;
    string var_path;
    
    bool m_array_of_structure(hid_t dsetid, std::vector<char>&values,bool has_values, size_t values_offset,
                              int64_t nelms,const int64_t* offset,const int64_t* count,const int64_t *step);

    void m_array_of_structure_member(libdap::BaseType *field, hid_t memtype, unsigned int u,hid_t dsetid,
                                     std::vector<char>&values,bool has_values, size_t struct_elem_offset ) const;
    void m_array_of_structure_close_hdf5_ids(std::vector<char> &values, bool has_values, hid_t mspace,
                                             hid_t dtypeid, hid_t memtype) const;
    void m_array_of_structure_catch_close_hdf5_ids(hid_t memb_id, char * memb_name, std::vector<char> &values,
                                                   bool has_values, hid_t mspace, hid_t dtypeid, hid_t memtype) const;

    bool m_array_of_reference(hid_t dset_id,hid_t dtype_id);
    void m_array_of_region_reference(hid_t d_dset_id,std::vector<std::string>& v_str, int64_t nelms,
                                     const std::vector<int64_t>& offset, const std::vector<int64_t> &step);
    void m_array_of_object_reference(hid_t d_dset_id, std::vector<std::string>& v_str, int64_t nelms,
                                     const std::vector<int64_t>& offset, const std::vector<int64_t> &step) const;

    bool m_array_of_reference_new_h5_apis(hid_t dset_id,hid_t dtype_id) const;

    void m_intern_plain_array_data(char *convbuf,hid_t memtype);
    void m_array_of_atomic(hid_t, hid_t,int64_t ,const int64_t *,const int64_t *,const int64_t *);
    void handle_vlen_string(hid_t dset_id, hid_t memtype, int64_t nelms, const std::vector<hsize_t>& hoffset,
                                   const std::vector<hsize_t>& hcount, const std::vector<hsize_t>& hstep);
    void handle_array_read_whole(hid_t dset_id, hid_t memtype, int64_t nelms);
    void handle_array_read_slab(hid_t dset_id, hid_t memtype, int64_t nelms,
                                const int64_t *offset, const int64_t *step, const int64_t *count);

    void do_array_read(hid_t dset_id,hid_t dtype_id,std::vector<char>&values,
                       int64_t nelms,const int64_t* offset,const int64_t* count, const int64_t* step);
    bool do_h5_array_type_read(hid_t dsetid, hid_t memb_id,std::vector<char>&values,bool has_values,size_t values_offset,
                               int64_t at_nelms,int64_t* at_offset,int64_t*at_count,int64_t* at_step);
    void do_h5_array_type_read_base_compound_member(hid_t dsetid, libdap::BaseType *field, hid_t child_memb_id,
                                                    H5T_class_t child_memb_cls, std::vector<char>&values, bool has_values,
                                                    size_t values_offset, int64_t at_nelms, int64_t at_total_nelms,
                                                    size_t at_base_type_size, int64_t array_index,
                                                    int64_t at_orig_index, size_t child_memb_offset) const;
    void do_h5_array_type_read_base_compound_member_string(libdap::BaseType *field, hid_t child_memb_id,
                                                            const std::vector<char> &values, size_t data_offset) const;

    void do_h5_array_type_read_base_atomic(H5T_class_t array_cls, hid_t at_base_type, size_t at_base_type_size,
                                           std::vector<char>&values, size_t values_offset, int64_t at_nelms,
                                           int64_t at_total_nelms, int at_ndims, std::vector<int64_t> &at_dims,
                                           int64_t* at_offset, int64_t* at_step, int64_t *at_count);
    void do_h5_array_type_read_base_atomic_whole_data(H5T_class_t array_cls, hid_t at_base_type,int64_t at_nelms,
                                                      std::vector<char> &values, size_t values_offset);
    inline int64_t INDEX_nD_TO_1D (const std::vector < int64_t > &dims, const std::vector < int64_t > &pos) const;
    bool obtain_next_pos(std::vector<int64_t>& pos, std::vector<int64_t>&start,std::vector<int64_t>&end,
                         std::vector<int64_t>&step,int rank_change);

    template<typename T>  int subset(
            const T input[],
            int rank,
            std::vector<int64_t> & dim,
            int64_t start[],
            int64_t stride[],
            int64_t edge[],
            std::vector<T> *poutput,
            std::vector<int64_t>& pos,
            int index);

    bool handle_one_dim(libdap::Array::Dim_iter d, libdap::D4Group *temp_grp, libdap::D4Dimension * &d4_dim,
                        const vector<string> &dimpath, int k, unsigned long long dim_size) const;
    void m_array_of_region_reference_point_selection(hid_t space_id, int ndim, const std::string &varname,
                                                             std::vector<std::string> &v_str,int64_t i) const;
    void m_array_of_region_reference_hyperslab_selection(hid_t space_id, int ndim, const std::string &varname,
                                                             std::vector<std::string> &v_str,int64_t i) const;
    friend class HDF5Structure;

  protected:
    
    // Parse constraint expression and make HDF5 coordinate point location.
    // return number of elements to read. 
   int64_t format_constraint(int64_t *cor, int64_t *step, int64_t *edg);


  public:

    /// Constructor
    HDF5Array(const std::string & n, const std::string &d, libdap::BaseType * v);
    ~ HDF5Array() override = default;

    /// Clone this instance.
    /// 
    /// Allocate a new instance and copy *this into it. This method must 
    /// perform a deep copy.
    /// \return A newly allocated copy of this class
    libdap::BaseType *ptr_duplicate() override;

    /// Reads HDF5 array data into local buffer
    bool read() override;

    string get_VarPath() const { return var_path;}
    /// remembers memory size needed.    
    void set_memneed(size_t need);

    /// remembers number of dimensions of this array.
    void set_numdim(int ndims);

    /// obtain the number of dimensions of this array.
    int get_numdim() const { return d_num_dim;}

    /// remembers number of elements in this array.  
    void set_numelm(hsize_t nelms);

    void set_varpath(const std::string& vpath) { var_path = vpath;}
    libdap::BaseType *h5dims_transform_to_dap4(libdap::D4Group *root,const std::vector<std::string> &dimpath);

};

#endif
