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
#include <string>
#include <vector>
#include <iostream>
#define MAX_PATHLEN 256

using namespace std;

bool is_str_type(hid_t);
int obtain_str_info_data(hid_t,hid_t, int);
int read_str_data(hid_t,vector<string>&strdata);
int read_vlen_str(hid_t dset_id, int num_elems, hid_t dtype,hid_t dspace, hid_t mspace,bool is_scalar);
int read_fixed_str(hid_t dset_id, int num_elems, hid_t dtype,hid_t dspace, hid_t mspace,bool is_scalar);

int
main (int argc, char **argv)
{
    hid_t       file, root_id;         /* file and dataset handles */

    file = H5Fopen(argv[1], H5F_ACC_RDONLY, H5P_DEFAULT);
    if(file < 0) {
        printf("HDF5 file %s cannot be opened successfully,check the file name and try again.\n",argv[1]);
        return -1;
    }

    root_id = H5Gopen(file, "/", H5P_DEFAULT);

    // Iterate through the file to see the members from the root.
    H5G_info_t g_info;
    hsize_t nelems = 0;
    if(H5Gget_info(root_id,&g_info) <0) {
        cerr<<"H5Gget_info failed "<<endl;
        return -1;
    }

    nelems = g_info.nlinks;

    for (hsize_t i = 0; i < nelems; i++) {

        // Obtain the object type, such as group or dataset.
        H5O_info_t oinfo;

        if (H5Oget_info_by_idx2(root_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                                i, &oinfo, H5O_INFO_BASIC, H5P_DEFAULT)<0) {
            cerr <<"Error obtaining the info for the object" <<endl;
            return -1;
        }

        if(oinfo.type  == H5O_TYPE_DATASET) {

            hid_t dset_id = H5Oopen_by_idx(root_id,"/",H5_INDEX_NAME,H5_ITER_NATIVE,i,H5P_DEFAULT);
            if(dset_id < 0) {
                cerr <<"Error obtaining the dataset ID" <<endl;
                return -1;
            }

            bool str_type = is_str_type(dset_id);
            if (str_type) {
                if (obtain_str_info_data(root_id, dset_id, i) < 0) {
                    cerr << "Fail to obtain string info and data " <<endl;
                    return -1;
                }
            }
            H5Dclose(dset_id);
        }
    } // for i is 0 ... nelems

    return 0;
}

bool is_str_type(hid_t dset_id) {
    bool ret_value = false;
    hid_t dtype_id = H5Dget_type(dset_id);

    if(H5Tget_class(dtype_id) == H5T_STRING)
        ret_value = true;
    H5Tclose(dtype_id);
    return ret_value;
}

int obtain_str_info_data(hid_t root_id, hid_t dset_id, int dset_index) {

    ssize_t oname_size;
    vector <char>oname;

    // Query the length of object name.
    oname_size =
            H5Lget_name_by_idx(root_id,".",H5_INDEX_NAME,H5_ITER_NATIVE,dset_index,NULL,
                               (size_t)MAX_PATHLEN, H5P_DEFAULT);
    if (oname_size <= 0) {
        cerr<<" Error getting the size of the hdf5 object from the group: " << endl;
        return -1;
    }

    // Obtain the name of the object
    oname.resize((size_t) oname_size + 1);

    if (H5Lget_name_by_idx(root_id,".",H5_INDEX_NAME,H5_ITER_NATIVE,dset_index,&oname[0],
                           (size_t)(oname_size+1), H5P_DEFAULT) < 0){
        cerr << "Error getting the hdf5 object name from the group: " << endl;
        return -1;
    }

    string oname_str(oname.begin(),oname.end()-1);
    cout <<"var name is "<<oname_str<<endl;

    // ************* Note: ********************
    // So far, the code above is mainly the set_up of this program. The DMRPP should
    // obtain all the information above somewhere before retrieving the data.

    // Obtain the data space. To demostrate the hyperslab(expression constraint) feature.
    // I use offset, count and step. The illustration is trivial here. All data will be returned.
    // offset = 0; count = dimension length in each dimension, step =1
    // Scalar will be handled differently.

    hid_t dspace = H5Dget_space(dset_id);
    bool is_scalar = false;
    hid_t mspace;

    if (H5S_SCALAR == H5Sget_simple_extent_type(dspace))
        is_scalar = true;

    vector<hsize_t> offset;
    vector<hsize_t> count;
    vector<hsize_t> step;
    int total_num_elems = 1;

    if (false == is_scalar) {

        int ndims = H5Sget_simple_extent_ndims(dspace);
        offset.resize(ndims);
        count.resize(ndims);
        step.resize(ndims);

        if (H5Sget_simple_extent_dims(dspace,&count[0],NULL) < 0) {
            cerr<<"Cannot obtain dataspace dimension info. "<<endl;
            return -1;
        }

        for (int i = 0; i < ndims; i++) {
            offset[i] = 0;
            step[i] = 1;
            total_num_elems *=count[i];
        }

        if (H5Sselect_hyperslab(dspace, H5S_SELECT_SET,
                                &offset[0], &step[0],
                                &count[0], NULL) < 0) {
            cerr<<"Cannot select hyperslab  "<<endl;
            return -1;
        }

        if ((mspace = H5Screate_simple(ndims, &count[0],NULL)) < 0) {
            cerr <<" Cannot create the memory space " << endl;
            return -1;
        }
    }

    hid_t dtype_id = H5Dget_type(dset_id);
    if (H5Tis_variable_str(dtype_id) > 0) {
        if (read_vlen_str(dset_id,total_num_elems,dtype_id,dspace,mspace,is_scalar) < 0) {
            cerr << "read_vlen_str failed " << endl;
            return -1;
        }
    }
    else {
        if (read_fixed_str(dset_id,total_num_elems,dtype_id,dspace,mspace,is_scalar) < 0) {
            cerr << "read_fixed_str failed " << endl;
            return -1;
        }
    }
    if (false == is_scalar)
        H5Sclose(mspace);
    H5Sclose(dspace);
    H5Tclose(dtype_id);

    return 0;
}

int read_vlen_str(hid_t dset, int nelms,hid_t dtype,hid_t dspace, hid_t mspace,bool is_scalar) {

    vector<string> finstrval;
    finstrval.resize(nelms);

    size_t ty_size = H5Tget_size(dtype);
    vector <char> strval;
    strval.resize(nelms*ty_size);
    hid_t read_ret = -1;
    if (true == is_scalar)
        read_ret = H5Dread(dset,dtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,(void*)&strval[0]);
    else
        read_ret = H5Dread(dset,dtype,mspace,dspace,H5P_DEFAULT,(void*)&strval[0]);

    if (read_ret < 0) {
        cerr<<"H5Dread failed "<<endl;
        return -1;
    }

    // For scalar, nelms is 1.
    char*temp_bp = &strval[0];
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
            ret_vlen_claim = H5Dvlen_reclaim(dtype,dspace,H5P_DEFAULT,(void*)&strval[0]);
        else
            ret_vlen_claim = H5Dvlen_reclaim(dtype,mspace,H5P_DEFAULT,(void*)&strval[0]);
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

int read_fixed_str(hid_t dset, int nelms, hid_t dtype,hid_t dspace, hid_t mspace,bool is_scalar)
{
    size_t ty_size = H5Tget_size(dtype);
    vector <char> strval;
    strval.resize(nelms*ty_size);
    hid_t read_ret = -1;
    if (true == is_scalar)
        read_ret = H5Dread(dset,dtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,(void*)&strval[0]);
    else
        read_ret = H5Dread(dset,dtype,mspace,dspace,H5P_DEFAULT,(void*)&strval[0]);

    if (read_ret < 0) {
        cerr<<"H5Dread failed "<<endl;
        return -1;
    }

    string total_string(strval.begin(),strval.end());
    strval.clear();
    vector <string> strdata;
    strdata.resize(nelms);
    for (int i = 0; i<nelms; i++)
        strdata[i] = total_string.substr(i*ty_size,ty_size);

    // String data
    cout << "Fixed-size string data: "<<endl;
    if (read_str_data(dtype, strdata) <0)  {
        cerr << "Cannot read the string data successfully "<<endl;
        return -1;
    }

    return 0;
}

int read_str_data(hid_t dtype, vector<string> & strdata) {

    // PAD will be removed in the trimmed output.
    //vector <string>trimmed_strdata;
    //trimmed_strdata.resize(strdata.size());
    // pad information is represented as an enum.
    // H5T_STR_ERROR: -1
    // H5T_STR_NULLTERM: 0
    // H5T_STR_NULPAD: 1
    // H5T_STR_SPACEPAD: 2

    H5T_str_t str_pad = H5Tget_strpad(dtype);
    char pad_char = '\0';
    if (str_pad == H5T_STR_SPACEPAD)
        pad_char = ' ';
    else if (str_pad != H5T_STR_NULLTERM && str_pad != H5T_STR_NULLPAD) {
        cerr<< "h5tget_strpad failed" <<endl;
        return -1;
    }

    for (int i = 0; i < strdata.size(); i++) {

        // For space pad, we need to find the first non-space character
        // backward.
        size_t pad_pos = string::npos;
        if (pad_char == ' ') {
            size_t last_not_pad_pos = strdata[i].find_last_not_of(pad_char);
            if(last_not_pad_pos==string::npos)
                pad_pos = 0;
            else
                pad_pos = last_not_pad_pos + 1;
        }
        else
            pad_pos = strdata[i].find(pad_char);

        if (pad_pos == string::npos) {
            cerr<<" Error: string pad is not found. \n";
            return -1;
        }

        if (pad_pos != 0)
            strdata[i] = strdata[i].substr(0,pad_pos);
        cout << strdata[i] << endl;
    }

    return 0;

}