// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009 The HDF Group, Inc. and OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1901 South First Street,
// Suite C-2, Champaign, IL 61820  

#include "config_hdf5.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <algorithm>
#include <ctype.h>

#include <debug.h>
#include <Error.h>
#include <InternalErr.h>

#include "HDF5Array.h"
#include "HDF5Structure.h"
#include "HDF5Str.h"

using namespace std;

BaseType *HDF5Array::ptr_duplicate() {
    return new HDF5Array(*this);
}

HDF5Array::HDF5Array(const string & n, const string &d, BaseType * v) :
    Array(n, d, v) {
}

HDF5Array::~HDF5Array() {
}

int HDF5Array::format_constraint(int *offset, int *step, int *count) {
    long nels = 1;
    int id = 0;

    Dim_iter p = dim_begin();

    while (p != dim_end()) {

        int start = dimension_start(p, true);
        int stride = dimension_stride(p, true);
        int stop = dimension_stop(p, true);

        // Check for empty constraint
        if (stride <= 0 || start < 0 || stop < 0 || start > stop) {
            ostringstream oss;

            oss << "Array/Grid hyperslab indices are bad: [" << start <<
                ":" << stride << ":" << stop << "]";
            throw Error(malformed_expr, oss.str());
        }

        offset[id] = start;
        step[id] = stride;
        count[id] = ((stop - start) / stride) + 1; // count of elements
        nels *= count[id]; // total number of values for variable

        DBG(cerr
            << "=format_constraint():"
            << "id=" << id << " offset=" << offset[id]
            << " step=" << step[id]
            << " count=" << count[id]
            << endl);

        id++;
        p++;
    }

    return nels;
}

bool HDF5Array::m_array_of_structure() {
    DBG(cerr << "=read() Array of Structure length=" << length() << endl);

        vector<int> offset(d_num_dim);
        vector<int> count(d_num_dim);
        vector<int> step(d_num_dim);
        int nelms = format_constraint(&offset[0], &step[0], &count[0]);

        // Honor constraint evaluation here.
        vector<int> picks(nelms);
        int total_elems =
            linearize_multi_dimensions(&offset[0], &step[0], &count[0], &picks[0]);

        HDF5Structure *p = dynamic_cast < HDF5Structure * >(var());
        if (!p) {
            throw InternalErr(__FILE__, __LINE__, "Not a HDF5Structure");
        }
        
        p->set_array_size(nelms);
        p->set_entire_array_size(total_elems);

        // Set the vector.
        for (int i = 0; i < p->get_array_size(); i++) {
            p->set_array_index(picks[i]);
            set_vec(i, p);
        }

        set_read_p(true);

        return false;
}

// This private method makes a simple array and inserts it into the
// H5T_COMPOUND variable/object that was made at the start of
// m_array_in_structure(). This method is run for side effect only.
void HDF5Array::m_insert_simple_array(hid_t s1_tid, hsize_t *size2) {
    int size = d_memneed / length();
    hid_t s1_array2 = -1;
    if (d_type == H5T_INTEGER) {
        if (size == 1) {
            s1_array2
                = H5Tarray_create(H5T_NATIVE_CHAR, d_num_dim,
                                  size2, NULL);
        }
        if (size == 2) {
            s1_array2 = H5Tarray_create(H5T_NATIVE_SHORT,
                                        d_num_dim, size2, NULL);
        }
        if (size == 4) {
            s1_array2
                = H5Tarray_create(H5T_NATIVE_INT, d_num_dim,
                                  size2, NULL);
        }
        if(s1_array2 < 0){
            throw InternalErr(__FILE__, __LINE__,
                              "H5Tarray_create failed for H5T_INTEGER.");
        }
    }

    if (d_type == H5T_FLOAT) {
        if (size == 4) {
            s1_array2 = H5Tarray_create(H5T_NATIVE_FLOAT,
                                        d_num_dim, size2, NULL);
        }
        if (size == 8) {
            s1_array2 = H5Tarray_create(H5T_NATIVE_DOUBLE,
                                        d_num_dim, size2, NULL);
        }
        if(s1_array2 < 0){
            throw InternalErr(__FILE__, __LINE__,
                              "H5Tarray_create failed for H5T_FLOAT.");
        }
        
    }

    if (d_type == H5T_STRING) {
        DBG(cerr << "string array is detected" << endl);
        hid_t str_type = mkstr(size, H5T_STR_SPACEPAD);
        s1_array2 = H5Tarray_create(str_type, d_num_dim, size2, 
                                    NULL);
        if(s1_array2 < 0){
            throw InternalErr(__FILE__, __LINE__,
                              "H5Tarray_create failed for H5T_STRING.");
        }
    }

    if(H5Tinsert(s1_tid, name().c_str(), 0, s1_array2) < 0){
        throw InternalErr(__FILE__, __LINE__,
                          "H5Tinsert failed for " + name());        
    }
    if(H5Tclose(s1_array2) < 0){
        throw InternalErr(__FILE__, __LINE__,
                          "H5Tclose failed for " + name());        
    }
}

// This method proceeds in two steps. First it builds a simple array and
// links it into a chain of H5 Compund objects, recording information about
// the size of that simple array. Then it reads the data for that array and
// loads it into this instance of HDF5Array.
bool HDF5Array::m_array_in_structure() {
    DBG(cerr << "=read() Array in Structure of length=" << length() << endl);

    int array_index = 0, array_size = 0, entire_array_size = 0;
    hid_t s1_tid = H5Tcreate(H5T_COMPOUND, d_memneed);
    if(s1_tid < 0){
        throw InternalErr(__FILE__, __LINE__,
                          "H5Tcreate failed.");
    }
    // Build the simple array and record its size.

        // Construct an array read from the structure.
        vector<hsize_t> size2(d_num_dim);
        vector<int> perm(d_num_dim); // perm is not used
        if(H5Tget_array_dims(d_ty_id, &size2[0], &perm[0]) < 0){
            throw InternalErr(__FILE__, __LINE__,
                              "H5Tget_array_ndims failed.");
        }

        // Grab the BaseType to this object's parent. If it's a constructor
        // type, then look at the type of this array and insert a H5 array of
        // the matching type in the H5 Compound object made above. Record
        // information about the size of the array and its index, then go up
        // to q's parent.
        string parent_name;
        BaseType *q = get_parent();
        if (q && q->is_constructor_type()) { // Grid, structure or sequence

            m_insert_simple_array(s1_tid, &size2[0]);

            // Remember the last parent name.
            parent_name = q->name();

            HDF5Structure &p = dynamic_cast < HDF5Structure & >(*q);
            // Remember the index of array from the parent.
            array_index = p.get_array_index();
            array_size = p.get_array_size();
            entire_array_size = p.get_entire_array_size();

            q = q->get_parent();
        }

        // Now iterate up the hierarchy of BaseTypes to the top (when we hit
        // the top of the dataset get_parent() returns NULL). This code builds
        // a chain of H5 Compound objects which ultimately link the stuff we
        // just built to the top of the dataset.
        while (q && q->is_constructor_type()) {
            DBG(cerr  << ": parent_name=" << parent_name << endl);

            hid_t stemp_tid = H5Tcreate(H5T_COMPOUND, d_memneed);
            if(stemp_tid < 0){
                throw InternalErr(__FILE__, __LINE__,
                                  "H5Tcreate failed.");
            }            
            if(H5Tinsert(stemp_tid, parent_name.c_str(), 0, s1_tid) < 0){
                throw InternalErr(__FILE__, __LINE__,
                                  "H5Tinsert failed.");                
            }
            s1_tid = stemp_tid;

            // Remember the last parent name.
            parent_name = q->name();

            HDF5Structure &p = dynamic_cast < HDF5Structure & >(*q);
            // Remember the index of array from the parent.
            array_index = p.get_array_index();
            array_size = p.get_array_size();
            entire_array_size = p.get_entire_array_size();

            q = q->get_parent();
        }

        DBG(cerr << "=read() parent's element count=" << array_size << endl);
    DBG(cerr << "=read() parent's entire element count=" << entire_array_size << endl);
    DBG(cerr << "=read() parent's index=" << array_index << endl);


    // Read data from the HDF5 file and load those values into this instance.
#if 0
    char *convbuf = 0;
    char *buf = 0;
    string *v_str = 0;
    char *strbuf = 0;
    try {
#endif
	// Allocate enough buffer for entire array to be read.
        if (!entire_array_size)
            throw InternalErr(__FILE__, __LINE__,
                              "entire_array_size is zero.");
        vector<char> buf(entire_array_size * d_memneed);

        if(H5Dread(d_dset_id, s1_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, 
                   static_cast<void*>(&buf[0])) < 0)
            throw InternalErr(__FILE__, __LINE__,
                              "H5Dread failed.");
        
        if(H5Tclose(s1_tid) < 0)
            throw InternalErr(__FILE__, __LINE__,
                              "H5Tclose failed.");;

        // Originally, this code used entire_array_size, but it looks like
        // array_size will do. jhrg 4/15/08
        if (!array_size) {
            throw InternalErr(__FILE__, __LINE__, "array_size is zero");
        }
        vector<char> convbuf(array_size * d_memneed);
        
        for (int l = 0; l < array_size; l++) {
            for (int i = 0; i < (int) d_memneed; i++) {
                convbuf[l * d_memneed + i] = buf[array_index * d_memneed + i];
            }
        }

        // Treat string differently with vector of strings.
        if (d_type == H5T_STRING) {
            vector<string> v_str(d_num_elm);
            int size = d_memneed / length();
            vector<char> strbuf(size + 1);
            for (int strindex = 0; strindex < d_num_elm; strindex++) {
                get_strdata(strindex, &convbuf[0], &strbuf[0], size);
                DBG(cerr << "=read()<get_strdata() strbuf=" << &strbuf[0] << endl);
                v_str[strindex] = &strbuf[0];
            }
            set_read_p(true);
            val2buf((void *)&v_str[0]);
        } 
        else {
            set_read_p(true);
            val2buf((void *)&convbuf[0]);
        }
#if 0
    }
    catch(...) {
        // memory allocation exceptions could have been thrown in
        // creating these ptrs, so could still be null. Check if
        // exists before deleting. pwest Mar 18, 2009
        if( buf ) delete[] buf;
        if( strbuf ) delete[] strbuf;
        if( v_str ) delete[] v_str;
        if( convbuf ) delete[] convbuf;
        throw;
    }
#endif
    return false;
}

bool HDF5Array::m_array_of_reference() {
#if 1
    //int *offset = 0;
    //int *count = 0;
    //int *step = 0;
    hdset_reg_ref_t *rbuf = 0;
    //string *v_str = 0;
    try {
#endif
	vector<int> offset(d_num_dim);
        vector<int> count(d_num_dim);
        vector<int> step(d_num_dim);
        hdset_reg_ref_t *rbuf = new hdset_reg_ref_t[d_num_elm];

        int nelms = format_constraint(&offset[0], &step[0], &count[0]); // Throws Error.
        vector<string> v_str(nelms);

        DBG(cerr << "=read() URL type is detected. "
            << "nelms=" << nelms << " full_size=" << d_num_elm << endl);

        // Handle regional reference.
	if (H5Tequal(d_ty_id, H5T_STD_REF_DSETREG) < 0){
	    throw InternalErr(__FILE__, __LINE__, "H5Tequal() failed");
	}
        if (H5Tequal(d_ty_id, H5T_STD_REF_DSETREG) > 0) {
            DBG(cerr << "=read() Got regional reference. " << endl);
            if (H5Dread(d_dset_id, H5T_STD_REF_DSETREG, H5S_ALL, H5S_ALL, 
                        H5P_DEFAULT, &rbuf[0]) < 0) {
                throw InternalErr(__FILE__, __LINE__, "H5Dread failed()");
            }

            for (int i = 0; i < nelms; i++) {
                // Let's assume that URL array is always 1 dimension.
                DBG(cerr << "=read() rbuf[" << i << "]" <<
                    rbuf[offset[0] + i * step[0]] << endl);

                if (rbuf[offset[0] + i * step[0]][0] != '\0') {
                    char name[DODS_NAMELEN];

                    hid_t did_r = H5Rdereference(d_dset_id,
                                                 H5R_DATASET_REGION,
                                                 rbuf[offset[0]+i*step[0]]
                                                 );
                    if(did_r < 0) {
			throw InternalErr(__FILE__, __LINE__,
                                          "H5Rdereference() failed.");
                        
                    }
                    
                    if( H5Iget_name(did_r, (char *) name, DODS_NAMELEN) < 0){
			throw InternalErr(__FILE__, __LINE__,
                                          "H5Iget_name() failed.");
                    }
                    DBG(cerr << "=read() dereferenced name is " << name 
                        << endl);
                    
                    string varname(name);
                    hid_t space_id = H5Rget_region(
                                                   did_r, H5R_DATASET_REGION,
                                                   rbuf[offset[0]+i* step[0]]
                                                   );
                    if(space_id < 0) {
                        throw InternalErr(__FILE__, __LINE__,
                                          "H5Rget_region() failed.");

                    }

                    int ndim = H5Sget_simple_extent_ndims(space_id);
		    if(ndim < 0) {
                        throw
                            InternalErr(__FILE__, __LINE__,
                                        "H5Sget_simple_extent_ndims() \
failed.");
                    }

                    DBG(cerr << "=read() dim is " << ndim << endl);

                    string expression;
                    switch (H5Sget_select_type(space_id)) {

                    case H5S_SEL_NONE:
                        DBG(cerr << "=read() None selected." << endl);
                        break;

                    case H5S_SEL_POINTS: {
#if 0
                	hsize_t *buf = 0;
                        try {
#endif
                            DBG(cerr << "=read() Points selected." << endl);
                            hsize_t npoints = H5Sget_select_npoints(space_id);
			    if(npoints < 0) {
                                throw InternalErr(__FILE__, __LINE__,
                                                  "Cannot determine number of \
elements in the dataspace selection");
                    	    }

                            DBG(cerr << "=read() npoints are " << npoints 
                                << endl);
                            vector<hsize_t> buf(npoints * ndim);
                            if(H5Sget_select_elem_pointlist(space_id,
                                                            0, 
                                                            npoints,
                                                            &buf[0]) < 0){
                                throw InternalErr(__FILE__, __LINE__,
                                                  "H5Sget_select_elem_pointlist() failed.");                                
                                
                            };

#ifdef DODS_DEBUG
                            for (int j = 0; j < npoints * ndim; j++) {
                                cerr << "=read() npoints buf[0] =" << buf[j] 
                                     <<endl;
                            }
#endif

                            for (int j = 0; j < (int) npoints; j++) {
                                // Name of the dataset.
                                expression.append(varname); 
                                for (int k = 0; k < ndim; k++) {
                                    ostringstream oss; 
                                    oss << "["
                                        << (int) buf[j * ndim + k]
                                        << "]";
                                    expression.append(oss.str());
                                }
                                if (j != (int) (npoints - 1)) {
                                    expression.append(",");
                                }
                            }
                            v_str[i].append(expression);
#if 0
                            delete[] buf;
                        }
                        catch (...) {
                            if( buf ) delete[] buf;
                            throw;
                        }
#endif
                        break;
                    }
                    case H5S_SEL_HYPERSLABS: {
#if 0
                	hsize_t *start = 0;
                        hsize_t *end = 0;
                        try {
#endif
                            vector<hsize_t> start(ndim);
                            vector<hsize_t> end(ndim);

                            DBG(cerr << "=read() Slabs selected." << endl);
                            DBG(cerr << "=read() nblock is " <<
                                H5Sget_select_hyper_nblocks(space_id) << endl);

                            if(H5Sget_select_bounds(space_id, &start[0], &end[0]) < 0){
                                throw InternalErr(__FILE__, __LINE__,
                                                  "H5Sget_select_bounds() \
failed.");                                
                            }
                            
                            for (int j = 0; j < ndim; j++) {
                                ostringstream oss; 
                                DBG(cerr << "=read() start is " << start[j] 
                                    << "=read() end is " << end[j] << endl);
                                oss << "[" << (int) start[j] << ":" 
                                    << (int) end[j] << "]";
                                expression.append(oss.str());
                                DBG(cerr << "=read() expression is " 
                                    << expression << endl);
                            }
                            v_str[i] = varname;
                            if (!expression.empty()) {
                                v_str[i].append(expression);
                            }
                            // Constraint expression. [start:1:end]
#if 0
                            delete[] start;
                            delete[] end;
                        }
                        catch (...) {
                            if( start ) delete[] start;
                            if( end ) delete[] end;
                            throw;
                        }
#endif
                        break;
                    }
                    case H5S_SEL_ALL:
                        DBG(cerr << "=read() All selected." << endl);
                        break;

                    default:
                        DBG(cerr << "Unknown space type." << endl);
                        break;
                    }

                } 
                else {
                    v_str[i] = "";
                }
            }
        }

        // Handle object reference.
	if (H5Tequal(d_ty_id, H5T_STD_REF_OBJ) < 0) {
            throw InternalErr(__FILE__, __LINE__, "H5Tequal() failed.");
	}
        if (H5Tequal(d_ty_id, H5T_STD_REF_OBJ) > 0) {
            DBG(cerr << "=read() Got object reference. " << endl);
            if (H5Dread(d_dset_id, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL, 
                        H5P_DEFAULT, &rbuf[0]) < 0) {
                throw InternalErr(__FILE__, __LINE__, "H5Dread failed()");
            }

            for (int i = 0; i < nelms; i++) {
                // Let's assume that URL array is always 1 dimension.
                hid_t did_r = H5Rdereference(d_dset_id, H5R_OBJECT,
                                             rbuf[offset[0] + i * step[0]]);
		if (did_r < 0) {
		    throw InternalErr(__FILE__, __LINE__,
                                      "H5Rdereference() failed.");
		}
                char name[DODS_NAMELEN];
                if(H5Iget_name(did_r, (char *) name, DODS_NAMELEN) < 0) {
                    throw InternalErr(__FILE__, __LINE__,
                                      "H5Iget_name() failed.");
                }

                
                // Shorten the dataset name
                string varname(name);
                
                DBG(cerr << "=read() dereferenced name is " << name <<endl);
                v_str[i] = varname;
            }
        }

        set_value(&v_str[0], nelms);
#if 1
        //delete[] offset;
        //delete[] count;
        //delete[] step;
        delete[] rbuf;
        //delete[] v_str;
#endif
        return false;
#if 1
    }
    catch(...) {
        // memory allocation exceptions could have been thrown in
        // creating these ptrs so check if exist before deleting.
        // pwest Mar 18, 2009
        //if( offset ) delete[] offset;
        //if( count ) delete[] count;
        //if( step ) delete[] step;
        if( rbuf ) delete[] rbuf;
        //if( v_str ) delete[] v_str;

        throw;
    }
#endif
}

void HDF5Array::m_intern_plain_array_data(char *convbuf) {
    char Msga[255];

    if (check_h5str(d_ty_id)) {
#if 0
	string *v_str = 0;
        char *strbuf = 0;
        try {
#endif
            vector<string> v_str(d_num_elm);
            size_t elesize = H5Tget_size(d_ty_id);
	    if (elesize == 0){
		throw InternalErr(__FILE__, __LINE__, "H5Tget_size() failed.");
	    }
            vector<char> strbuf(elesize + 1);
            DBG(cerr << "=read()<check_h5str()  element size=" << elesize
                << " d_num_elm=" << d_num_elm << endl);

            for (int strindex = 0; strindex < d_num_elm; strindex++) {
                get_strdata(strindex, &convbuf[0], &strbuf[0], elesize);
                DBG(cerr << "=read()<get_strdata() strbuf=" << &strbuf[0] << endl);
                v_str[strindex] = &strbuf[0];
            }

            if(H5Dclose(d_dset_id) < 0){
                throw InternalErr(__FILE__, __LINE__, "H5Dclose() failed.");
            };
            set_read_p(true);
            val2buf((void *)&v_str[0]);
#if 0
            delete[] strbuf;
            delete[] v_str;
        }
        catch (...) {
            // memory allocation exceptions could have been thrown
            // in creating these so check if exist before deleting.
            // pwest Mar 18, 2009
            if( strbuf ) delete[] strbuf;
            if( v_str ) delete[] v_str;
            throw;
        }
#endif
    } 
    else {
        set_read_p(true);
        val2buf((void *) convbuf);
    }
}

bool HDF5Array::read() {
    DBG(cerr
        << ">read() dataset=" << dataset()
        << " data_type_id=" << d_ty_id << " name=" << name()
        << " get_dap_type=" << get_dap_type(d_ty_id)
        << " dimension=" << d_num_dim
        << " data_size=" << d_memneed << " length=" << length()
        << endl);

    if (get_dap_type(d_ty_id) == "Structure") {
        return m_array_of_structure();
    }

    // This "Array" datatype is the HDF5 "Array" datatype. It should not be confused 
    // as the normal DAP way to handle a data array although essentially it is doing the same thing.
    // If the "Array" datatype is never defined, this part of the code will never be executed.
    // HDF5 "Array" datatype is  defined inside an HDF5 compound datatype for very
    // rare cases. It means that this part of code  may never be used by most (99.9%) applications. 
    //  Kyang 2009/11/23
    if (get_dap_type(d_ty_id) == "Array") { 
        return m_array_in_structure();
    }

    if (get_dap_type(d_ty_id) == "Url") {
        return m_array_of_reference();
    }
#if 0
    int *offset = 0;
    int *count = 0;
    int *step = 0;
    char *convbuf = 0;
    try {
#endif
	vector<int> offset(d_num_dim);
        vector<int> count(d_num_dim);
        vector<int> step(d_num_dim);
        int nelms = format_constraint(&offset[0], &step[0], &count[0]); // Throws Error.

        if (H5Tis_variable_str(d_ty_id) &&
            H5Tget_class(d_ty_id) == H5T_STRING){
            
            bool status = read_vlen_string(d_dset_id, d_ty_id, nelms, &offset[0],
                                           &step[0], &count[0]);
#if 0
            delete[] offset;
            delete[] count;
            delete[] step;
#endif
            return status;
        }

	if (H5Tis_variable_str(d_ty_id) < 0){
	    throw InternalErr(__FILE__, __LINE__, "H5Tis_variable_str() failed.");
	}
	if (H5Tget_class(d_ty_id) < 0){
	    throw InternalErr(__FILE__, __LINE__, "H5Tget_class() failed.");
	}
        char Msga[255];
        if (nelms == d_num_elm) {
            vector<char> convbuf(d_memneed);
            get_data(d_dset_id, (void *)&convbuf[0]);
            
            // Check if a Signed Byte to Int16 conversion is necessary.
            
            if(get_dap_type(d_ty_id) == "Int8"){
                vector<short> convbuf2(nelms);
                for(int i=0; i < nelms ; i++){
                    convbuf2[i] = (signed char)(convbuf[i]);
                    DBG(cerr << "convbuf[" << i << "]="
                        << (signed char)convbuf[i] << endl);
                    DBG(cerr << "convbuf2[" << i << "]="
                        << convbuf2[i] << endl);
                }
                // Libdap will generate the wrong output. 
                m_intern_plain_array_data((char*)&convbuf2[0]);
                //delete[] convbuf2;
            }       
            m_intern_plain_array_data(&convbuf[0]);
            //delete[] convbuf;
        } // if (nelms == d_num_elm)
        else {
            size_t data_size = nelms * H5Tget_size(d_ty_id);
            if (data_size < 0)
                throw InternalErr(__FILE__, __LINE__, "get_size failed");
            vector<char> convbuf(data_size);
            get_slabdata(d_dset_id, &offset[0], &step[0], &count[0], d_num_dim, &convbuf[0]);

            // Check if a Signed Byte to Int16 conversion is necessary.
            if(get_dap_type(d_ty_id) == "Int8"){
                vector<short> convbuf2(data_size);
                for(int i=0; i < data_size; i++){
                    convbuf2[i] = static_cast<signed char>(convbuf[i]);
                }
                m_intern_plain_array_data((char*)&convbuf2[0]);
                //delete[] convbuf2;
            }
            else{
                m_intern_plain_array_data(&convbuf[0]);
            }
            
            //delete[] convbuf;
        }
#if 0
        delete[]offset;
        delete[]step;
        delete[]count;
    }
    catch(...) {
        // memory allocation exception could have been thrown in
        // creating these ptrs so check if exists before deleting.
        // pwest Mar 18, 2009
        if( offset ) delete[]offset;
        if( step ) delete[]step;
        if( count ) delete[]count;
        if( convbuf ) delete[]convbuf;
        throw;
    }
#endif
    return false;
}

// public functions to set all parameters needed in read function.

void HDF5Array::set_did(hid_t dset) {
    d_dset_id = dset;
}

void HDF5Array::set_tid(hid_t type) {
    d_ty_id = type;
}

void HDF5Array::set_memneed(size_t need) {
    d_memneed = need;
}

void HDF5Array::set_numdim(int ndims) {
    d_num_dim = ndims;
}

void HDF5Array::set_numelm(int nelms) {
    d_num_elm = nelms;
}

hid_t HDF5Array::get_did() {
    return d_dset_id;
}

hid_t HDF5Array::get_tid() {
    return d_ty_id;
}

int HDF5Array::linearize_multi_dimensions(int *start, int *stride, int *count,
                                          int *picks) {
    DBG(cerr << ">linearize_multi_dimensions()" << endl);
    int total = 1;
#if 0
    int *dim = 0;
    int *temp_count = 0;
    try {
#endif
	int id = 0;
        vector<int> dim(d_num_dim);
        Dim_iter p2 = dim_begin();

        while (p2 != dim_end()) {
            int a_size = dimension_size(p2, false); // unconstrained size
            DBG(cerr << "dimension[" << id << "] = " << a_size << endl);
            dim[id] = a_size;
            total = total * a_size;
            ++id;
            ++p2;
        }

        vector<int> temp_count(d_num_dim);
        int temp_index;
        int i;
        int array_index = 0;
        int temp_count_dim = 0; // This variable changes when dim. is added. 
        int temp_dim = 1;

        for (i = 0; i < d_num_dim; i++)
            temp_count[i] = 1;

        int num_ele_so_far = 0;
        int total_ele = 1;
        for (i = 0; i < d_num_dim; i++)
            total_ele = total_ele * count[i];

        while (num_ele_so_far < total_ele) {
            // loop through the index 

            while (temp_count_dim < d_num_dim) {
                temp_index = (start[d_num_dim - 1 - temp_count_dim]
                              + (temp_count[d_num_dim - 1 - temp_count_dim]-1)
                              * stride[d_num_dim - 1 -
                                       temp_count_dim]) * temp_dim;
                array_index = array_index + temp_index;
                temp_dim = temp_dim * dim[d_num_dim - 1 - temp_count_dim];
                temp_count_dim++;
            }

            picks[num_ele_so_far] = array_index;

            num_ele_so_far++;
            // index can be added 
            DBG(cerr << "number of element looped so far = " <<
                num_ele_so_far << endl);
            for (i = 0; i < d_num_dim; i++) {
                DBG(cerr << "temp_count[" << i << "]=" << temp_count[i] <<
                    endl);
            }
            DBG(cerr << "index so far " << array_index << endl);

            temp_dim = 1;
            temp_count_dim = 0;
            array_index = 0;

            for (i = 0; i < d_num_dim; i++) {
                if (temp_count[i] < count[i]) {
                    temp_count[i]++;
                    break;
                } 
                else { // We reach the end of the dimension, set it to 1 and
                    // increase the next level dimension.  
                    temp_count[i] = 1;
                }
            }
        }
#if 0
        delete[] dim;
        delete[] temp_count;
    }
    catch(...) {
        // memory allocation exception could have been thrown in
        // creating these ptrs so check if exists before deleting.
        // pwest Mar 18, 2009
        if( dim ) delete[] dim;
        if( temp_count ) delete[] temp_count;
        throw;
    }
#endif
    DBG(cerr << "<linearize_multi_dimensions()" << endl);
    return total;
}

hid_t HDF5Array::mkstr(int size, H5T_str_t pad) {
    
    hid_t type;

    if ((type = H5Tcopy(H5T_C_S1)) < 0)
        return -1;
    if (H5Tset_size(type, (size_t) size) < 0)
        return -1;
    if (H5Tset_strpad(type, pad) < 0)
        return -1;

    return type;
}

bool HDF5Array::read_vlen_string(hid_t d_dset_id, hid_t d_ty_id, int nelms,
                                 int *offset, int *step, int *count) {
    DBG(cerr <<
        "=read_vlen_string(): variable string is detected with nelms = "
        << nelms << endl);
#if 0
    char **convbuf2 = 0;        // an array of strings
    string *v_str = 0;
    char *strbuf = 0;

    try {
#endif
	vector<char*> convbuf2(d_num_elm);
        if(H5Dread(d_dset_id, d_ty_id,
                   H5S_ALL, H5S_ALL, H5P_DEFAULT, &convbuf2[0]) < 0){
            throw InternalErr(__FILE__, __LINE__, "H5Dread failed()");
        }

        // Find the maximum size of the strings in convbuf2.
        int size_max = 0;
        for (int strindex = 0; strindex < d_num_elm; strindex++) {
            if (convbuf2[strindex] != '\0') {
                size_max = max(size_max, (int)strlen(convbuf2[strindex]));
            }
        }

        vector<char> strbuf(size_max + 1);
        vector<string> v_str(d_num_elm);

        for (int strindex = 0; strindex < nelms; strindex++) {
            memset(&strbuf[0], 0, size_max + 1);
            // Let's assume that variable length array is 1 dimension.
            int real_index = offset[0] + strindex * step[0];
            if (convbuf2[real_index] != NULL) {
                strncpy(&strbuf[0], convbuf2[real_index], size_max);
                strbuf[size_max] = '\0';
                v_str[strindex] = &strbuf[0];
                DBG(cerr << "v_str" << v_str[strindex] << endl);
            } 
            else {
                v_str[strindex] = &strbuf[0];
            }
        }

        if(H5Dclose(d_dset_id) < 0){
            throw InternalErr(__FILE__, __LINE__, "H5Dclose() failed.");
        }
        set_read_p(true);
        set_value(v_str, d_num_elm);
#if 0
        delete[] strbuf;
        delete[] convbuf2;
        delete[] v_str;
    }
    catch (...) {
        if( strbuf ) delete[] strbuf;
        if( convbuf2 ) delete[] convbuf2;
        if( v_str ) delete[] v_str;
        throw;
    }
#endif
    return false;
}
