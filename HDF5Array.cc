////////////////////////////////////////////////////////////////////////////////
/// \file HDF5Array.cc
/// \brief A HDF5Array class
///
/// This class converts HDF5 array type into DAP array.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Muqun Yang <ymuqun@hdfgroup.org>
///
/// Copyright (c) 2007 HDF Group
///
/// Copyright (c) 1999 National Center for Supercomputing Applications.
/// 
/// All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation
#endif

// #define DODS_DEBUG

#include <iostream>
#include <memory>
#include <sstream>
#include <ctype.h>

#include "config_hdf5.h"
#include "debug.h"
#include "Error.h"
#include "InternalErr.h"
#include "HDF5Array.h"
#include "HDF5Structure.h"
#include "HDF5Str.h"
#include "h5dds.h"

using namespace std;

BaseType *
HDF5Array::ptr_duplicate()
{
  return new HDF5Array(*this);
}

HDF5Array::HDF5Array(const string & n, BaseType * v) : Array(n, v)
{
}

HDF5Array::~HDF5Array()
{
}

// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDF5Array::format_constraint(int *offset, int *step, int *count)
{
  long nels = 1;
  int id = 0;

  Dim_iter p = dim_begin();
  
  while (p != dim_end()) {
      
    int start = dimension_start(p, true);
    int stride = dimension_stride(p, true);
    int stop = dimension_stop(p, true);

    // Check for empty constraint
    if (stride <= 0 || start < 0 || stop < 0 || start > stop){
	
      ostringstream oss;

      oss << "Array/Grid hyperslab indices are bad: [" << start <<
	":" << stride << ":" << stop << "]";
      throw Error(malformed_expr, oss.str());
    }

    offset[id] = start;
    step[id] = stride;
    count[id] = ((stop - start) / stride) + 1;	// count of elements
    nels *= count[id];	// total number of values for variable
	
    DBG(cerr
	<< "=format_constraint():"
	<< "id="      << id 
	<< " offset=" << offset[id]
	<< " step="   << step[id]
	<< " count="  << count[id]
	<< endl);
	
    id++;
    ++p;
  }
    
  return nels;
}

bool
HDF5Array::read(const string & dataset)
{

  int *offset = new int[d_num_dim];
  int *count = new int[d_num_dim];
  int *step = new int[d_num_dim];
  int i;
  int j;
  int k;
  int l;
  int m;
  int n;
  
  char *convbuf = 0;
  char Msga[255];

  size_t data_size = 0;
  
  DBG(cerr
      << ">read() dataset=" << dataset
      << " data_type_id=" << d_ty_id
      << " name=" << name()
      << " return_type=" << return_type(d_ty_id)
      << " dimension=" << d_num_dim
      << " data_size=" << d_memneed
      << " length=" << length()
      << endl);

  if(return_type(d_ty_id) == "Structure"){
    
    DBG(cerr << "=read() Array of Structure length=" << length() <<  endl);

    int nelms = format_constraint(offset, step, count);
    int picks[nelms];

    // Honor constraint evaluation here.
    int total_elems = linearize_multi_dimensions(offset, step, count, picks);

    HDF5Structure *p = dynamic_cast<HDF5Structure*>(var());
    p->set_array_size(nelms);
    p->set_entire_array_size(total_elems);
    
    // Set the vector.
    for(i=0; i < p->get_array_size() ; i++){
      p->set_array_index(picks[i]);
      set_vec(i, p);      
    }

    set_read_p(true);
    return false;
  } // if (Structure)
  
  if(return_type(d_ty_id) == "Array"){
    
    DBG(cerr << "=read() Array (in Structure) of length=" << length() <<  endl);
    // Construct an array read from the structure.
    data_size = d_memneed;


    hsize_t size2[d_num_dim];
    int perm[d_num_dim];
    H5Tget_array_dims(d_ty_id, size2, perm);
    
    hid_t s1_tid = H5Tcreate(H5T_COMPOUND, data_size);
    hid_t s1_array2;
    hid_t stemp_tid;
    
    void* buf_array = NULL;
    n = 0;
    
    BaseType *q = get_parent();
    string parent_name;
    int size = data_size / length();
    
    while(q != NULL){
      if(q->is_constructor_type()){ // Grid, structure or sequence
	if(n == 0){ // Array at the Bottom level 
    	  if(d_type == H5T_INTEGER){
	    if(size == 1){
	      s1_array2 = H5Tarray_create(H5T_NATIVE_CHAR, d_num_dim, size2, NULL);	
	    }
	    if(size == 2){
	      s1_array2 = H5Tarray_create(H5T_NATIVE_SHORT, d_num_dim, size2, NULL);	
	    }
	    if(size == 4){
	      s1_array2 = H5Tarray_create(H5T_NATIVE_INT, d_num_dim, size2, NULL);
	    }
	  }
    
	  if(d_type == H5T_FLOAT){
	    if(size == 4) {
	      s1_array2 = H5Tarray_create(H5T_NATIVE_FLOAT, d_num_dim, size2, NULL);
	    }
	    if(size == 8) {
	      s1_array2 = H5Tarray_create(H5T_NATIVE_DOUBLE, d_num_dim, size2, NULL);
	    }
	  }
	  
	  if(d_type == H5T_STRING){
	    DBG(cerr << "string array is detected" << endl);
	    hid_t str_type = mkstr(size, H5T_STR_SPACEPAD);
	    s1_array2 = H5Tarray_create(str_type, d_num_dim, size2, NULL);
	  }

	  H5Tinsert(s1_tid, name().c_str(), 0, s1_array2);
	  H5Tclose(s1_array2);
	} // if (n == 0)
	else{
	  DBG(cerr << n << ": parent_name=" <<  parent_name << endl);
	  stemp_tid = H5Tcreate(H5T_COMPOUND, data_size);
	  H5Tinsert(stemp_tid, parent_name.c_str(), 0, s1_tid);
	  s1_tid = stemp_tid;
	}
	// Remember the last parent name.
	parent_name = q->name();
	HDF5Structure *p = dynamic_cast<HDF5Structure*>(q);
	// Remember the index of array from the parent.
	j = p->get_array_index();
	k = p->get_array_size();
	m = p->get_entire_array_size();
	q = q->get_parent();
	
      } // if (q->is_constructor_type())
      else{
	q = NULL;
      }
      n++;
    } // while()

    
    DBG(cerr << "=read() parent's element count="  << k << endl);
    DBG(cerr << "=read() parent's entire element count="  << m << endl);
    DBG(cerr << "=read() parent's index="  << j << endl);
    DBG(cerr << "=read() element size=" << size << endl);
    
    // For HDF5, we need to read in bulk.
    // Thus, the entire array size(m) is used instead of the size
    // selected by constraint expression(k) here.    
    convbuf = new char[data_size * m];
    
    // Allocate enough buffer for entire array to be read.
    buf_array = malloc(m * data_size);    
    H5Dread(d_dset_id, s1_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf_array);
    H5Tclose(s1_tid);


    for(l=0; l < k; l++){
      for(i=0; i < data_size; i++){
	// Cast the array buffer into character buffer.
	char* a = (char*)buf_array;
	convbuf[l*data_size + i] = a[j*data_size+i];
      }
    }

    // Treat string differently with vector of strings.
    if(d_type == H5T_STRING){
      
      string v_str[d_num_elm];
      
      for (int strindex = 0; strindex < d_num_elm; strindex++) {
	char *strbuf = new char[size + 1];
	if (get_strdata(strindex, convbuf, strbuf, size, Msga) < 0) {
	  throw InternalErr(__FILE__, __LINE__,
			    string("hdf5_dods server failed on getting data.\n")
			    + Msga);
	}
	DBG(cerr << "=read()<get_strdata() strbuf=" << strbuf << endl);
	v_str[strindex] = strbuf;
	delete[]strbuf;
      }      
      set_read_p(true);
      val2buf((void*)&v_str);      
    }
    else{
      set_read_p(true);
      val2buf((void *) convbuf);    
    }
    free(buf_array);
    return false;

  } // if (Array)

  if(return_type(d_ty_id) == "Url"){
    hsize_t size2[d_num_dim];
    int perm[d_num_dim];
    int size = length();
    int nelms = format_constraint(offset, step, count); // Throws Error.
    string v_str[nelms];

    // H5Tget_array_dims(d_ty_id, size2, perm);
    
    DBG(cerr << "=read() URL type is detected. "
	<< "nelms=" << nelms
	<< " full_size=" << d_num_elm
	<< endl);


    // Handle regional Reference.
    if(H5Tequal(d_ty_id, H5T_STD_REF_DSETREG) > 0){

      hdset_reg_ref_t *rbuf = (hdset_reg_ref_t *)malloc(sizeof(hdset_reg_ref_t)*d_num_elm);
      
      DBG(cerr << "=read() Got regional reference. " << endl);
      herr_t status = H5Dread(d_dset_id, H5T_STD_REF_DSETREG, H5S_ALL, H5S_ALL,
			      H5P_DEFAULT, rbuf);
      hid_t did_r;
      
      for(i=0; i < nelms; i++){
	// Let's assume that URL array is always 1 dimension.
	DBG(cerr << "=read() rbuf[" << i << "]" << rbuf[offset[0] + i*step[0]] << endl);
	if(rbuf[offset[0] + i*step[0]][0] != '\0'){
	  char buf1[DODS_NAMELEN];
	  char buf2[DODS_NAMELEN];
	  
	  did_r =  H5Rdereference(d_dset_id, H5R_DATASET_REGION, &rbuf[offset[0] + i*step[0]]);
	  int name_size = H5Iget_name(did_r, (char*)buf2, DODS_NAMELEN);
	  DBG(cerr << "=read() dereferenced name is " << buf2 << endl);

	
	  H5Rget_name(d_dset_id, H5R_DATASET_REGION, &rbuf[0], (char*)buf1, DODS_NAMELEN);
	  DBG(cerr << "=read() dereferenced region points to " << buf1 << endl);
	  hid_t space_id = H5Rget_region(did_r, H5R_DATASET_REGION, &rbuf[offset[0] + i*step[0]]);
	  int ndim = H5Sget_simple_extent_ndims(space_id);
	  DBG(cerr << "=read() dim is " << ndim << endl);
	  hsize_t start[ndim];
	  hsize_t end[ndim];

	  string expression;
	  char strbuf2[128];
	  char strbuf3[128];
	  
	  hsize_t npoints;
	  hsize_t *buf;
	  switch(H5Sget_select_type(space_id)){
	    
	  case H5S_SEL_NONE:
	    DBG(cerr << "=read() None selected." << endl);
	    break;
	    
	  case H5S_SEL_POINTS:
	    DBG(cerr << "=read() Points selected." << endl);
	    npoints = H5Sget_select_npoints(space_id);
	    DBG(cerr << "=read() npoints are " << npoints << endl);

	    buf = (hsize_t *)malloc(sizeof(hsize_t)*npoints*ndim);
	    H5Sget_select_elem_pointlist(space_id, 0, npoints, buf);
	    
#ifdef DODS_DEBUG	    
	    for(j=0; j < npoints * ndim ;j++){
	      cerr << "=read() npoints buf[0] =" <<  buf[j] << endl;
	    }
#endif
	    sprintf(strbuf3, ",");
	    for(j=0; j < npoints; j++){
      	      expression.append(buf2); // Name of the dataset.
	      for(k=0; k < ndim; k++){
		sprintf(strbuf2, "[%d]", buf[j*ndim+k]); // Point indexes.
		expression.append(strbuf2);
	      }
	      if(j != npoints - 1){
		expression.append(strbuf3);
	      }
	    }
	    v_str[i].append(expression);

	    break;
	    
	  case H5S_SEL_HYPERSLABS:
	    DBG(cerr << "=read() Slabs selected." << endl);
	    DBG(cerr << "=read() nblock is " << H5Sget_select_hyper_nblocks(space_id) << endl);
	  
	    H5Sget_select_bounds(space_id, start, end);

	    for(j=0; j < ndim; j++){
	      DBG(cerr << "=read() start is " << start[j] << endl);
	      DBG(cerr << "=read() end is " << end[j] << endl);

	      sprintf(strbuf2, "[%d:", start[j]);
	      sprintf(strbuf3, "%d]", end[j]);
	      expression.append(strbuf2);
	      expression.append(strbuf3);
	      DBG(cerr << "=read() expression is " << expression << endl);
	    }
	    v_str[i] = buf2;
	    if(!expression.empty()){
	      v_str[i].append(expression);
	    }
	    // Constraint expression. [start:1:end]
	    
	    break;
	    
	  case H5S_SEL_ALL:
	    DBG(cerr << "=read() All selected." << endl);
	    break;
	  
	  default:
	    DBG(cerr << "Unknown space type." << endl);
	    break;
	  }

	
	}
	else{
	  v_str[i] = "";
	}
      }
    }
    
    // Handle object reference.
    if(H5Tequal(d_ty_id, H5T_STD_REF_OBJ) > 0){
      hobj_ref_t *rbuf = (hobj_ref_t *)malloc(sizeof(hobj_ref_t)*d_num_elm);
      DBG(cerr << "=read() Got object reference. " << endl);
      herr_t status = H5Dread(d_dset_id, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL,
			      H5P_DEFAULT, rbuf);
      for(i=0; i < nelms; i++){
	// Let's assume that URL array is always 1 dimension.
	hid_t did_r =  H5Rdereference(d_dset_id, H5R_OBJECT, &rbuf[offset[0] + i*step[0]]);
	char buf2[DODS_NAMELEN];
	int name_size = H5Iget_name(did_r, (char*)buf2, DODS_NAMELEN);
	DBG(cerr << "=read() dereferenced name is " << buf2 << endl);
	v_str[i] = buf2;
      }
    }
    
    set_read_p(true);
    val2buf((void*)&v_str);
    return false;
	
  }
  
  
  try {
    int nelms = format_constraint(offset, step, count); // Throws Error.

    if (H5Tis_variable_str(d_ty_id) && H5Tget_class(d_ty_id) == H5T_STRING) {
      return read_vlen_string(d_dset_id, d_ty_id, nelms, offset, step, count);
    }
    
    if (nelms == d_num_elm) {

      data_size = d_memneed;      
      convbuf = new char[data_size];

      if (get_data(d_dset_id, (void *) convbuf, Msga) < 0) {
	throw InternalErr(__FILE__, __LINE__,
			  string("hdf5_dods server failed on getting data.\n")
			  + Msga);
      }

      
      if (check_h5str(d_ty_id)) {
	string v_str[d_num_elm];
	size_t elesize = H5Tget_size(d_ty_id);
	DBG(cerr
	    << "=read()<check_h5str()  element size=" << elesize 
	    << " d_num_elm=" << d_num_elm
	    << endl);

	for (int strindex = 0; strindex < d_num_elm; strindex++) {
	  char *strbuf = new char[elesize + 1];
	  if (get_strdata(strindex, convbuf, strbuf, elesize, Msga) < 0) {
	    throw InternalErr(__FILE__, __LINE__,
			      string("hdf5_dods server failed on getting data.\n")
			      + Msga);
	  }
	  DBG(cerr << "=read()<get_strdata() strbuf=" << strbuf << endl);
	  v_str[strindex] = strbuf;
	  delete[]strbuf;
	}

	H5Dclose(d_dset_id);
	set_read_p(true);
	val2buf((void*)&v_str);

      } else {
	set_read_p(true);
	val2buf((void *) convbuf);
      }

    }  // if (nelms == d_num_elm)

    else {
      if ((data_size = nelms * H5Tget_size(d_ty_id)) < 0) {
	throw InternalErr(__FILE__, __LINE__,
			  string("hdf5_dods server failed on getting data size."));
      }

      char *convbuf = new char[data_size];

      if (!get_slabdata(d_dset_id, offset, step, count, d_num_dim, 
			data_size, (void *)convbuf, Msga)) {
	throw InternalErr(__FILE__, __LINE__,
			  string("hdf5_dods server failed on getting hyperslab data.\n")
			  + Msga);
      }
      // Check URL
      
      if (check_h5str(d_ty_id)) {
	string v_str[nelms];
	size_t elesize = H5Tget_size(d_ty_id);
	DBG(cerr
	    << "element size = " << elesize 
	    << " nelms = " << nelms
	    << endl);
	for (int strindex = 0; strindex < nelms; strindex++) {
	  char *strbuf = new char[elesize + 1];
	  if (get_strdata(strindex, convbuf, strbuf, elesize, Msga) < 0) {
	    throw InternalErr(__FILE__, __LINE__,
			      string("hdf5_dods server failed on getting data.\n")
			      + Msga);
	  }
	  DBG(cerr << "strbuf=" << strbuf << endl);
	  v_str[strindex] = strbuf;
	  delete[]strbuf;	
	}
	set_read_p(true);
	val2buf((void*)&v_str);
      }
      else{
	set_read_p(true);
	val2buf((void *) convbuf);
      }

    }
  }
  catch (...) {
    delete[]offset;
    delete[]step;
    delete[]count;
    delete[]convbuf;
    throw;
  }

  delete[]offset;
  delete[]step;
  delete[]count;
  delete[]convbuf;

  return false;
}

// public functions to set all parameters needed in read function.

void 
HDF5Array::set_did(hid_t dset) {d_dset_id = dset;}

void 
HDF5Array::set_tid(hid_t type) {d_ty_id = type;}

void
HDF5Array::set_memneed(size_t need) {d_memneed = need;}

void
HDF5Array::set_numdim(int ndims) {d_num_dim = ndims;}

void
HDF5Array::set_numelm(int nelms) {d_num_elm = nelms;}

hid_t 
HDF5Array::get_did() {return d_dset_id;}

hid_t
HDF5Array::get_tid() {return d_ty_id;}
  
int
HDF5Array::linearize_multi_dimensions(int* start, int* stride, int* count, int* picks)
{
  DBG(cerr << ">linearize_multi_dimensions()" <<endl;);
  int id = 0;
  int *dim = new int[d_num_dim];
  int total = 1;
  
  Dim_iter p2 = dim_begin();
  
  while (p2 != dim_end()) {
    int a_size = dimension_size(p2, false); // unconstrained size
    DBG(cerr << "dimension[" <<  id << "] = " << a_size << endl);
    dim[id] = a_size;
    total = total * a_size;
    ++id;
    ++p2;
  } // while()

  // Kent's contribution.
  int temp_count[d_num_dim];
  int temp_index;
  int i;
  int array_index = 0;
  int temp_count_dim = 0; /* this variable changes when dim. is added */
  int temp_dim = 1;

    
  for (i=0;i<d_num_dim;i++)
    temp_count[i] = 1;

  
  int num_ele_so_far = 0;
  int total_ele = 1;
  for (i=0;i<d_num_dim;i++)
    total_ele=total_ele*count[i];


  while(num_ele_so_far <total_ele) {
    // loop through the index 

    while(temp_count_dim < d_num_dim) {
      temp_index = (start[d_num_dim - 1 - temp_count_dim]+
		    (temp_count[d_num_dim - 1 - temp_count_dim]-1)*stride[d_num_dim - 1 - temp_count_dim])*temp_dim;	
      array_index =array_index+temp_index;
      temp_dim = temp_dim*dim[d_num_dim - 1 - temp_count_dim];
      temp_count_dim++;
    }
      
    picks[num_ele_so_far] = array_index;
      
    num_ele_so_far++;
    // index can be added 
    DBG(cerr << "number of element looped so far = " << num_ele_so_far << endl);
    for (i =0; i<d_num_dim;i++) {
      DBG(cerr << "temp_count[" << i << "]=" << temp_count[i] << endl);
    }
    DBG(cerr << "index so far " << array_index << endl);

 
    temp_dim = 1;
    temp_count_dim = 0;
    temp_index = 0;
    array_index = 0;

    for (i=0;i<d_num_dim;i++) {
      if(temp_count[i] < count[i]) {
	temp_count[i]++;
	break;
      }
      else {// We reach the end of the dimension, set it to 1 and increase the next level dimension. 
	temp_count[i] = 1;
      }
    }
  }
  DBG(cerr << "<linearize_multi_dimensions()" <<endl;);
  return total;
}

hid_t HDF5Array::mkstr(int size, H5T_str_t pad)
{
  hid_t type;

  if ((type=H5Tcopy(H5T_C_S1))<0) return -1;
  if (H5Tset_size(type, (size_t)size)<0) return -1;
  if (H5Tset_strpad(type, pad)<0) return -1;

  return type;
}

bool HDF5Array::read_vlen_string(hid_t d_dset_id, hid_t d_ty_id,
				 int nelms, int* offset, int* step, int* count)
{
  DBG(cerr << "=read_vlen_string(): variable string is detected with nelms = " << nelms << endl);
  hid_t sid = H5Dget_space(d_dset_id);
  char *convbuf2[d_num_elm];
  string v_str[d_num_elm];

  H5Dread(d_dset_id, d_ty_id,  H5S_ALL, H5S_ALL, H5P_DEFAULT, convbuf2);

  int size_max = 0;
  for (int strindex = 0; strindex < d_num_elm; strindex++) {
    cerr << "convbuf2[" << strindex << "]= " << convbuf2[strindex] << endl;
    if(convbuf2[strindex] !=NULL){
      if(strlen(convbuf2[strindex]) > size_max)
	size_max = strlen(convbuf2[strindex]);
    }
  }

  char *strbuf = new char[size_max + 1];

  for (int strindex = 0; strindex < nelms; strindex++) {
    bzero(strbuf, size_max);
    // Let's assume that variable length array is 1 dimension.
    int real_index = offset[0] + strindex * step[0];
    if(convbuf2[real_index] !=NULL){
      strncpy(strbuf, convbuf2[real_index], strlen(convbuf2[real_index]));
      strbuf[size_max] = '\0';
      v_str[strindex] = strbuf;
      DBG(cerr << "v_str" << v_str[strindex] <<endl);
    }
    else{
      strbuf[size_max] = '\0';
      v_str[strindex] = strbuf;
    }
  }
  H5Dclose(d_dset_id);

  set_read_p(true);
  val2buf((void*)&v_str);
  return false;
}
