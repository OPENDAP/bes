/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 *  This example outputs the string dataset values in an HDF5 file.
 *  These datasets should be under the root group. 
 *  The other cases are simply ignored.
 *  The program is mostly written in C except the string handling since
 *  this is for quick illustration purpose and some code is adapted from the HDF5 handler.
 *  Error checks are minimal. Coding style, comments etc are not enforced. 
 *  WARNING: NOT for copying and pasteing for operational use but readers can fine tune
 *  the code and use them. Should include the HDF Group's copyright. 
 *  Author: Kent Yang <myang6@hdfgroup.org>
 */

#include "hdf5.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#define MAX_PATHLEN 256

using namespace std;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

#define HR3  "# ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ... ..."
#define HR2  "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
#define HR1  "# -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==-"
#define HR0 "###################################################################"


bool debug = false;

bool is_str_type(hid_t);
int obtain_str_info_data(hid_t,hid_t, int);
int read_str_data(hid_t,vector<string>&strdata);
int read_vlen_str(hid_t dset, int nelms, hid_t dtype, hid_t dspace, hid_t mspace, bool is_scalar);
int read_fixed_str(hid_t dset_id, int nelems, hid_t dtype, hid_t dspace, hid_t mspace, bool is_scalar);
string get_object_name(hid_t root_id, int dset_index);
int read_fixed_length_str(hid_t dset_id, const vector<unsigned long long> &shape, hid_t dtype, hid_t dspace, hid_t mspace, bool is_scalar);


/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main (int argc, char *argv[])
{
    hid_t file;    // file handle
    hid_t root_id; // dataset handle

    cout << HR0 << endl;
    cout << "# " << argv[0] << endl;
    cout << "#" << endl;

    if(argc !=2) {
        cerr << "Please provide the HDF5 file name and the HDF5 dataset name as the following:" << endl;
        cerr << argv[0] << " h5_file_name" << endl ;
        exit(1);
    }
    string hdf_file(argv[1]);
    cout << "#  FILE: " << hdf_file << endl;

    file = H5Fopen(hdf_file.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

    if(file < 0) {
        cerr << "ERROR: HDF5 file " << hdf_file << " cannot be opened successfully,check the file name and try again." << endl;
        exit(1);
    }
    cout << "#  Opened file: " << hdf_file << endl;
    cout << "#" << endl;

    root_id = H5Gopen(file, "/", H5P_DEFAULT);

     // Iterate through the file to see the members from the root.
    H5G_info_t g_info;
    if(H5Gget_info(root_id,&g_info) <0) {
        cerr << "ERROR: H5Gget_info failed." << endl;
        exit(1);
    }


    hsize_t nelems = g_info.nlinks;
    for (hsize_t grmndx = 0; grmndx < nelems; grmndx++) {
        cout << HR1 << endl;
        cout << "# Processing HDF5 object. grmndx: " << grmndx << endl;

        string object_name = get_object_name(root_id,grmndx);
        cout << "# object_name: " << object_name << endl;

        // Obtain the object type, such as group or dataset. 
        H5O_info_t oinfo;

        if (H5Oget_info_by_idx2(root_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                                grmndx, &oinfo, H5O_INFO_BASIC, H5P_DEFAULT)<0) {
            cerr <<"ERROR: problem obtaining the info for the object with H5Oget_info_by_idx2()" <<endl;
            exit(1);
        }
        DBG(cout << "# H5Oget_info_by_idx2() succeeded." << endl);

        if(oinfo.type  == H5O_TYPE_DATASET) {
            cout << "# Found H5O_TYPE_DATASET" << endl;
            hid_t dset_id = H5Oopen_by_idx(root_id,
                                           "/",
                                           H5_INDEX_NAME,
                                           H5_ITER_NATIVE,
                                           grmndx,
                                           H5P_DEFAULT);
            if(dset_id < 0) {
                cerr <<"ERROR: Problem obtaining the dataset ID" <<endl;
                exit(1);
            }

            DBG(cout << "# H5Oopen_by_idx() succeeded." << endl);

            bool str_type = is_str_type(dset_id);
            if (str_type) {
                DBG(cout << "# The object '" << object_name << "' is a String type." << endl);
                if (obtain_str_info_data(root_id, dset_id, grmndx) < 0) {
                    cerr << "ERROR: Fail to obtain string info and data " <<endl;
                    exit(1);
                }
            }
            else {
                cout << "# The object '" << object_name << "' is not a string type." << endl;
            }
            H5Dclose(dset_id);
        }
    } // for i is 0 ... nelems

    exit(0);
}
// #####################################################################################################################



/**
 *
 * @param dset_id
 * @return
 */
bool is_str_type(hid_t dset_id) {
    bool ret_value = false;
    hid_t dtype_id = H5Dget_type(dset_id);

    if(H5Tget_class(dtype_id) == H5T_STRING)
        ret_value = true;
    H5Tclose(dtype_id);
    return ret_value;
}

/**
 *
 * @param root_id
 * @param dset_index
 * @return
 */
string get_object_name(hid_t root_id, int dset_index)
{
    ssize_t oname_size;
    vector <char>oname;

    // Query the length of object name.
    oname_size = H5Lget_name_by_idx(root_id,
                                    ".",
                                    H5_INDEX_NAME,
                                    H5_ITER_NATIVE,
                                    dset_index,
                                    NULL,
                                    (size_t)MAX_PATHLEN,
                                    H5P_DEFAULT);

    if (oname_size <= 0) {
        cerr<<" ERROR: Failed H5Lget_name_by_idx() call " << endl;
        exit(1);
    }
    // Obtain the name of the object
    oname.resize((size_t) oname_size + 1);

    if (H5Lget_name_by_idx(root_id,
                           ".",
                           H5_INDEX_NAME,
                           H5_ITER_NATIVE,
                           dset_index,
                           oname.data(),
                           (size_t)(oname_size+1),
                           H5P_DEFAULT) < 0){
        cerr << "ERROR: Failed to retrieve the hdf5 object name using H5Lget_name_by_idx()" << endl;
        exit(1);
    }
    string object_name(oname.begin(),oname.end()-1);
    return object_name;
}


/**
 *
 * @param root_id
 * @param dset_id
 * @param dset_index
 * @return
 */
int obtain_str_info_data(hid_t root_id, hid_t dset_id, int dset_index) {

    cout << HR2 << endl << "# BEGIN: obtain_str_info_data()" << endl << "#" << endl;
    string object_name = get_object_name(root_id,dset_index);
    cout <<"# object_name:  " << object_name << endl;

    // ************* Note: ******************** 
    // So far, the code above is mainly the set_up of this program. The DMRPP should
    // obtain all the information above somewhere before retrieving the data.

    // Obtain the data space. To demostrate the hyperslab(expression constraint) feature.
    // I use offset, count and step. The illustration is trivial here. All data will be returned.
    // offset = 0; count = dimension length in each dimension, step =1 
    // Scalar will be handled differently.

    hid_t dspace = H5Dget_space(dset_id);
    hid_t mspace;

    bool is_scalar = H5Sget_simple_extent_type(dspace) == H5S_SCALAR;

    vector<hsize_t> offset;
    vector<hsize_t> shape;
    vector<hsize_t> step;
    unsigned long long total_num_elems = 1;
 
    if (!is_scalar) {

        int ndims = H5Sget_simple_extent_ndims(dspace);
        cout <<"# The Array " << object_name << " has " << ndims << " dimensions." << endl;

        offset.resize(ndims);
        shape.resize(ndims);
        step.resize(ndims);

        if (H5Sget_simple_extent_dims(dspace,shape.data(),NULL) < 0) {
            cerr<<"ERROR: Cannot obtain dataspace dimension info. "<<endl;
            return -1;
        }

        for (int i = 0; i < ndims; i++) {
            offset[i] = 0;
            step[i] = 1;
            total_num_elems *= shape[i];
            cout <<"# shape[" << i << "]: " << shape[i] << endl;
        }



        // By passing the shape array into H5Sselect_hyperslab() for the count array:
        // "The count array determines how many positions to select from the dataspace in each dimension."
        // ask for all of the data in the array.
        if (H5Sselect_hyperslab(dspace, H5S_SELECT_SET, 
                               offset.data(), step.data(),
                                shape.data(), NULL) < 0) {
            cerr << "ERROR: Cannot select hyperslab  " << endl;
            return -1;
        } 
   
        if ((mspace = H5Screate_simple(ndims, shape.data(),NULL)) < 0) {
            cerr <<"ERROR: Cannot create the memory space " << endl;
            return -1;
        }
    }   

    hid_t dtype_id = H5Dget_type(dset_id);
    if (H5Tis_variable_str(dtype_id)) {
        cout <<"# Object '" << object_name << "' is a variable length String." << endl;
        if (read_vlen_str(dset_id, total_num_elems, dtype_id, dspace, mspace, is_scalar) < 0) {
            cerr << "ERROR: read_vlen_str failed " << endl;
            return -1;
        } 
    }
    else {
        cout <<"# Object '" << object_name << "' is a fixed length String" << endl;
       // if (read_fixed_str(dset_id, total_num_elems, dtype_id, dspace, mspace, is_scalar) < 0) {
       //     cerr << "ERROR: read_fixed_str failed " << endl;
       //     return -1;
       // }
        if (read_fixed_length_str(dset_id, shape, dtype_id, dspace, mspace, is_scalar) < 0) {
            cerr << "ERROR: read_fixed_length_str failed." << endl;
            return -1;
        }
    }

    if (!is_scalar)
        H5Sclose(mspace);

    H5Sclose(dspace);
    H5Tclose(dtype_id);

    cout << "# END: obtain_str_info_data()" << endl << HR2 << endl << "#" << endl;
    return 0;
}

/**
 *
 * @param dset
 * @param nelms
 * @param dtype
 * @param dspace
 * @param mspace
 * @param is_scalar
 * @return
 */
int read_vlen_str(hid_t dset, int nelms, hid_t dtype, hid_t dspace, hid_t mspace, bool is_scalar) {

    vector<string> finstrval;
    finstrval.resize(nelms);

    size_t ty_size = H5Tget_size(dtype);
    vector <char> strval;
    strval.resize(nelms*ty_size);
    hid_t read_ret = -1;
    if (true == is_scalar) 
        read_ret = H5Dread(dset,dtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,(void*)strval.data());
    else 
        read_ret = H5Dread(dset,dtype,mspace,dspace,H5P_DEFAULT,(void*)strval.data());

    if (read_ret < 0) {
        cerr<<"H5Dread failed "<<endl;
        return -1;
    }

    // For scalar, nelms is 1.
    char*temp_bp = strval.data();
    char*onestring = NULL;
    for (int i =0;i<nelms;i++) {
        onestring = *(char**)temp_bp;
        if(onestring!=NULL ) 
            finstrval[i] =string(onestring);
        else // We will add a NULL if onestring is NULL.
            finstrval[i]="";
        temp_bp +=ty_size;
    }

    if (false == strval.empty()) {
        herr_t ret_vlen_claim;
        if (true == is_scalar) 
            ret_vlen_claim = H5Dvlen_reclaim(dtype,dspace,H5P_DEFAULT,(void*)strval.data());
        else 
            ret_vlen_claim = H5Dvlen_reclaim(dtype,mspace,H5P_DEFAULT,(void*)strval.data());
        if (ret_vlen_claim < 0){
            cerr<<" H5Dvlen_reclaim failed " <<endl;
            return -1;
        }
    }

    // String data 
    cout << "Variable-length string data: "<<endl;
    if(read_str_data(dtype, finstrval) <0) {
        cerr << "Cannot read the string data successfully "<<endl;
        return -1;
    }
 
    return 0;
}


int read_fixed_length_str(hid_t dset_id, const vector<unsigned long long> &shape, hid_t dtype, hid_t dspace, hid_t mspace, bool is_scalar)
{
    cout << HR3 << endl << "# BEGIN: read_fixed_length_str()" <<  endl;
    cout << "# shape.size():  " << shape.size() << endl;

    size_t type_size = H5Tget_size(dtype);
    cout << "# type_size:  " << type_size << endl;

    // size_t fixed_str_length = shape.back();
    size_t fixed_str_length = shape.back();
    cout << "# fixed_str_length: " << fixed_str_length  << endl;

    unsigned long long total_str_count=1;
    for(unsigned int index=1; index < (shape.size()) ;index++){
        total_str_count *= shape[index];
    }
    cout << "# total_str_count: " << total_str_count  << endl;

    unsigned long long total_bytes = total_str_count * fixed_str_length * type_size;
    cout << "# total_bytes: " << total_bytes  << endl;

    vector <char> data;
    data.resize(total_bytes);
    hid_t read_ret;
    if (is_scalar) {
        cout << "# The object is a scalar."  << endl;
        read_ret = H5Dread(dset_id, dtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void *) data.data());
        string scalar_value(data.begin(),data.end());
        cout << "# scalar_value: '" << scalar_value << "'" << endl;
        return 0;
    }
    else {
        cout << "# The object is an array."  << endl;
        read_ret = H5Dread(dset_id, dtype, mspace, dspace, H5P_DEFAULT, (void *) data.data());
    }

    if (read_ret < 0) {
        cerr << "ERROR: H5Dread failed " << endl;
        return -1;
    }

    switch( shape.size()) {
        case 2:
        {
            cout << "# 2D array" << endl;
            vector<string> result_set;
            result_set.resize(shape[0]);
            for (unsigned long long i = 0; i < total_str_count; i++) {
                string foo = string(&(data[fixed_str_length * i]), fixed_str_length);
                result_set[i] = foo;
                cout << "## result_set[" << i << "/" << total_str_count << "):  '" << foo << "'" << endl;
            }
        }
        break;

        case 3:
        {
            cout << "# 3D array" << endl;
            vector<string> result_set;
            result_set.resize(shape[0]);
            for (unsigned long long i = 0; i < shape[0]; i++) {
                result_set[i] = string(&(data[fixed_str_length * i]), fixed_str_length);
                cout << "## result_set[" << i << "/" << result_set[i].size() << "):  '" << result_set[i] << "'" << endl;
            }
        }
        break;

        case 1:
        default:
        {
            string result_set(data.begin(),data.end());
            cout << "## result_set(size:"<<result_set.size() << "): '" << result_set << "'" << endl;
        }
        break;

    }
    data.clear();

    cout << "# END: read_fixed_length_str()" << endl << HR3 << endl << "#" << endl;

    return 0;
}

/**
 *
 * @param dset_id
 * @param nelems
 * @param dtype
 * @param dspace
 * @param mspace
 * @param is_scalar
 * @return
 */
int read_fixed_str(hid_t dset_id, int nelems, hid_t dtype, hid_t dspace, hid_t mspace, bool is_scalar)
{
    cout << HR3 << endl << "# BEGIN: read_fixed_str()" <<  endl;

    size_t type_size = H5Tget_size(dtype);
    cout << "# type_size:  " << type_size << endl;

    vector <char> strval;
    strval.resize(nelems*type_size);
    hid_t read_ret;
    if (is_scalar) {
        cout << "# The object is a scalar."  << endl;
        read_ret = H5Dread(dset_id, dtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void *) strval.data());
    }
    else {
        cout << "# The object is an array."  << endl;
        read_ret = H5Dread(dset_id, dtype, mspace, dspace, H5P_DEFAULT, (void *) strval.data());
    }

    if (read_ret < 0) {
        cerr<<"H5Dread failed "<<endl;
        return -1;
    }

    string total_string(strval.begin(),strval.end());
    strval.clear();
//    cout << "# total_string(" << total_string.length() << " chars): '" << total_string << "'" << endl;

    vector <string> strdata; 
    strdata.resize(nelems);
    string sanity_check;
    for (int i = 0; i<nelems; i++) {
        strdata[i] = total_string.substr(i * type_size, type_size);
        sanity_check.append(strdata[i]);
    }
    cout << "# strdata.size(): " << strdata.size() <<endl;
    int i=0;
//    for(auto &s:strdata)
//        cout << "#     strdata[" << i++ << "]: '" << s << "'" <<endl;

//    cout << "# sanity_check(" << sanity_check.length() << " chars): '" << sanity_check << "'" << endl;

    if(sanity_check == total_string){
        cout << "## The sanity_check matched the total_string." << endl;
    }
    else {
        cout << "## The sanity_check did NOT match the total_string." << endl;
    }

    // String data 
    cout << "# Fixed-size string data: "<<endl;
    if (read_str_data(dtype, strdata) <0)  {
        cerr << "Cannot read the string data successfully "<<endl;
        return -1;
    }

    cout << "# strdata.size(): " << strdata.size() <<endl;
    i=0;
    //for(auto &s:strdata)
    //    cout << "#     strdata[" << i++ << "]: '" << std::hex << s.c_str() << std::dec << "'" <<endl;

    cout << "# END: read_fixed_str()" << endl << HR3 << endl << "#" << endl;
    return 0;
}


/**
 * @brief Trim all of the trailing pad_char values from the string sdata.
 * @param sdata
 * @param pad_char
 * @return
 */
int trim_string_padding(string &sdata, const char pad_char){
    // For space pad, we need to find the first non-space character
    // backward.
    size_t pad_pos = string::npos;
    if (pad_char == ' ') {
        size_t last_not_pad_pos = sdata.find_last_not_of(pad_char);
        if(last_not_pad_pos==string::npos) {
            pad_pos = 0;
        }
        else {
            pad_pos = last_not_pad_pos + 1;
        }
    }
    else {
        pad_pos = sdata.find(pad_char);
    }

    if (pad_pos == string::npos) {
        // I elided this error state return because I think it's a bug
        // Basically if the fixed length string is full of valid string chars then
        // there are no pad chars, and that's legit. - ndp
        // cerr << "#  WaRNing:  String pad is not found. \n";
        // return -1;
    }

    if (pad_pos != 0) {
        sdata = sdata.substr(0, pad_pos);
    }

    return 0;
}

/**
 *
 * @param dtype
 * @param strdata
 * @return
 */
int read_str_data(hid_t dtype, vector<string> & strdata) {

    // PAD will be removed in the trimmed output.
    // vector <string>trimmed_strdata;
    // trimmed_strdata.resize(strdata.size());
    // pad information is represented as an enum.
    // H5T_STR_ERROR: -1
    // H5T_STR_NULLTERM: 0
    // H5T_STR_NULPAD: 1
    // H5T_STR_SPACEPAD: 2
   
    H5T_str_t str_pad = H5Tget_strpad(dtype);
    char pad_char = '\0'; // I think this can just be 0 instead of '\0' - ndp
    if (str_pad == H5T_STR_SPACEPAD) 
        pad_char = ' ';
    else if (str_pad != H5T_STR_NULLTERM && str_pad != H5T_STR_NULLPAD) {
        cerr<< "h5tget_strpad failed" <<endl;
        return -1;
    }

    int retval;
    for(string &sdata:strdata){
        retval = trim_string_padding(sdata,pad_char);
        if(retval!=0)
            return retval;
    }
    return 0;
}
