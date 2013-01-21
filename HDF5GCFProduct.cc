// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2012 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

////////////////////////////////////////////////////////////////////////////////
/// \file HDF5GCFProduct.cc
/// \brief The implementation of functions to identify different NASA HDF5 products.
/// Current supported products include MEaSUREs SeaWiFS, OZone, Aquarius level 3
/// Decadal survey SMAP level 2 and ACOS level 2S.
///
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////


#include <InternalErr.h>
#include <assert.h>
#include "HDF5GCFProduct.h"
using namespace libdap;


// The knowledge on how to distinguish each HDF5 product is all buried here
// All product attribute names and values are defined at the header files. 
H5GCFProduct check_product(hid_t file_id) {

    hid_t root_id = -1;
    H5GCFProduct product_type = General_Product;

    // Open the root group.
    if ((root_id = H5Gopen(file_id,ROOT_NAME,H5P_DEFAULT))<0){
        string msg = "cannot open the HDF5 root group  ";
        msg += string(ROOT_NAME);
        H5Fclose(file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // First check if the product is MEaSUREs SeaWiFS
    int s_level = -1;

    // Also set Aquarius level, although we only support
    // Aquarius level 3, in the future we may support Aquarius
    // level 2.
    int a_level = -1;

    if (true == check_measure_seawifs(root_id,s_level)) {
        // cerr <<"after check seawifs " <<endl << " level is "<< s_level <<endl;
        if (2 == s_level) product_type =  Mea_SeaWiFS_L2;
        if (3 == s_level) product_type =  Mea_SeaWiFS_L3;
    }

    else if (true == check_aquarius(root_id,a_level)){
        // cerr <<"after check aquarius" <<endl << " level is "<< a_level <<endl;
        if (3 == a_level) product_type =  Aqu_L3;
    }

    else if (true == check_measure_ozone(root_id)) {
        // cerr<<"after check ozone level 3 zonal average" <<endl;
        product_type = Mea_Ozone;
    }
    else { 
        int smap_flag = 1; // This is SMAP 
        if (true == check_smap_acosl2s(root_id,smap_flag)) 
            product_type =  SMAP;
        // cerr <<"After checking smap, product type is " << product_type << endl;

        if (General_Product == product_type) {

            int acosl2s_flag = 2; // This is ACOSL2S
            if (true == check_smap_acosl2s(root_id,acosl2s_flag)) 
                product_type =  ACOS_L2S;
            else if (true == check_netcdf4_general(root_id)) 
                product_type = NETCDF4_GENERAL;
                
            // cerr <<" After checking acos, product type is " << product_type <<endl;
        }

       
    }

    // netCDF-4 products
    

    H5Gclose(root_id);
    return product_type;
}

// Function to check if the product is MeaSure seaWiFS
bool check_measure_seawifs(hid_t s_root_id,int & s_lflag) {

    htri_t has_seawifs_attr1 = -1;
    bool   ret_flag = false;
    // cerr <<"coming to measure seawifs "<<endl;

    // Here we check the existence of attribute 1 first since
    // attribute 1 will tell if the product is SeaWIFS or not. 
    // attribute 2 and 3 will distinguish if it is level 2 or level 3.
    has_seawifs_attr1 = H5Aexists(s_root_id,SeaWiFS_ATTR1_NAME);

    if (has_seawifs_attr1 >0) {

        string attr1_value="";
        obtain_gm_attr_value(s_root_id, SeaWiFS_ATTR1_NAME, attr1_value);
        // cerr<<"seawifs attr1 value size " << attr1_value.length() <<endl;
        // cerr <<"seawifs ATTR1_VALUE size "<< SeaWiFS_ATTR1_VALUE.length()  <<endl;
        if (0 == attr1_value.compare(SeaWiFS_ATTR1_VALUE)) {
            // cerr<<"coming after comparing "<<endl;
            htri_t has_seawifs_attr2 = -1;
            htri_t has_seawifs_attr3 = -1;
            has_seawifs_attr2 = H5Aexists(s_root_id,SeaWiFS_ATTR2_NAME);
            has_seawifs_attr3 = H5Aexists(s_root_id,SeaWiFS_ATTR3_NAME);

            if ((has_seawifs_attr2 >0) && (has_seawifs_attr3 > 0)){
                // cerr <<"coming to seawifs attr2 and 3" <<endl;
                string attr2_value ="";
                string  attr3_value ="";
                obtain_gm_attr_value(s_root_id,SeaWiFS_ATTR2_NAME, attr2_value);
                obtain_gm_attr_value(s_root_id,SeaWiFS_ATTR3_NAME, attr3_value);

                // The first part of the <long_name> should be "SeaWiFS"
                // "Level 2" or "Level 3" should also be inside <long_name>.
                // OR the short_name should start with "SWDB_L2" and "SWDB_L3".
                 
                if (((0 == attr2_value.find(SeaWiFS_ATTR2_FPVALUE)) &&
                     (attr2_value.find(SeaWiFS_ATTR2_L2PVALUE)!=string::npos))
                   ||(0 == attr3_value.find(SeaWiFS_ATTR3_L2FPVALUE))) { 
                    // cerr <<"coming to seaWiFS level 2" <<endl;
                    s_lflag = 2;
                    ret_flag = true; 
                }
                else if (((0 == attr2_value.find(SeaWiFS_ATTR2_FPVALUE)) &&
                        (attr2_value.find(SeaWiFS_ATTR2_L3PVALUE)!=string::npos))
                        ||(0 == attr3_value.find(SeaWiFS_ATTR3_L3FPVALUE))) { 
                    s_lflag = 3;
                    ret_flag = true; 
                }
            }
            // else if long_name or short_name don't exist, not what we supported.
            else if ((0 == has_seawifs_attr2 ) || (0 == has_seawifs_attr3))
                ; // no op
            else {
                string msg = "Fail to determine if the HDF5 attribute  ";
                msg += string(SeaWiFS_ATTR2_NAME);
                msg += "or the HDF5 attribute ";
                msg += string(SeaWiFS_ATTR3_NAME);
                msg +=" exists ";
                H5Gclose(s_root_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }
        }
    }
    else if (0 == has_seawifs_attr1) 
            ;//no op
    else {
        string msg = "Fail to determine if the HDF5 attribute  ";
        msg += string(SeaWiFS_ATTR1_NAME);
        msg +=" exists ";
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    return ret_flag;
}

 // Function to check if the product is MeaSure seaWiFS
bool check_measure_ozone(hid_t s_root_id) {

    htri_t has_ozone_attr1 = -1;
    bool  ret_flag = false;
    // cerr <<"coming to measure ozone "<<endl;

    // Here we check the existence of attribute 1 first since
    // attribute 1 will tell if the product is SeaWIFS or not. 
    // attribute 2 and 3 will distinguish if it is level 2 or level 3.
    has_ozone_attr1 = H5Aexists(s_root_id,Ozone_ATTR1_NAME);

// STOP HERE 2012-3-06
    if (has_ozone_attr1 >0) {
        string attr1_value = "";
        obtain_gm_attr_value(s_root_id, Ozone_ATTR1_NAME, attr1_value);
        // cerr<<"ozone attr1 value size " << attr1_value.length() <<endl;
        // cerr <<"ozone ATTR1_VALUE size "<< Ozone_ATTR1_VALUE.length()  <<endl;
        if ((0 == attr1_value.compare(Ozone_ATTR1_VALUE1)) ||
            (0 == attr1_value.compare(Ozone_ATTR1_VALUE2))) {
            htri_t has_ozone_attr2 = -1;
            has_ozone_attr2 = H5Aexists(s_root_id,Ozone_ATTR2_NAME);
            if (has_ozone_attr2 >0) {
                string attr2_value = "";
                obtain_gm_attr_value(s_root_id,Ozone_ATTR2_NAME, attr2_value);
                if(0 == attr2_value.compare(Ozone_ATTR2_VALUE)) 
                    ret_flag = true;
            }
            // else if "ParameterName" attributes don't exist, not what we supported.
            else if (0 == has_ozone_attr2)
                     ;// no op
            else {
                string msg = "Fail to determine if the HDF5 attribute  ";
                msg += string(Ozone_ATTR2_NAME);
                msg +=" exists ";
                H5Gclose(s_root_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }
        }
    }
    else if (0 == has_ozone_attr1 )
        ; // no op 
    else {
        string msg = "Fail to determine if the HDF5 attribute  ";
        msg += string(Ozone_ATTR1_NAME);
        msg +=" exists ";
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    return ret_flag;
}       
// Function to check if the product is Aquarius
// We still leave a flag to indicate the level although
// we just support the special arrangement of level 3 now. 
// Possibly level 2 can be added 
// in the future.
bool check_aquarius(hid_t s_root_id,int & s_level) {

    htri_t has_aquarius_attr1 = -1;
    bool ret_flag = false;
    // cerr <<"coming to aquarius "<<endl;

    // Here we check the existence of attribute 1 first since
    // attribute 1 will tell if the product is Aquarius or not. 
    // attribute 2 will tell its level.
    has_aquarius_attr1 = H5Aexists(s_root_id,Aquarius_ATTR1_NAME);
    if (has_aquarius_attr1 >0) {
        string attr1_value = "";
        obtain_gm_attr_value(s_root_id, Aquarius_ATTR1_NAME, attr1_value);
        if (0 == attr1_value.compare(Aquarius_ATTR1_VALUE)) {
            htri_t has_aquarius_attr2 = -1;
            has_aquarius_attr2 = H5Aexists(s_root_id,Aquarius_ATTR2_NAME);
            if (has_aquarius_attr2 >0) {
                string attr2_value ="";
                obtain_gm_attr_value(s_root_id,Aquarius_ATTR2_NAME, attr2_value);

                // The "Title" of Aquarius should include "Level-3".
                if (attr2_value.find(Aquarius_ATTR2_PVALUE)!=string::npos){
                    // Set it to level 3
                    s_level = 3;
                    ret_flag = true; 
                }
            }
            // else if long_name or short_name don't exist, not what we supported.
            else if (0 == has_aquarius_attr2)
                     ; // no op
            else {
                string msg = "Fail to determine if the HDF5 attribute  ";
                msg += string(Aquarius_ATTR2_NAME);
                msg +=" exists ";
                H5Gclose(s_root_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }
        }
    }
    else if (0 == has_aquarius_attr1) 
        ;// no op
    else {
        string msg = "Fail to determine if the HDF5 attribute  ";
        msg += string(Aquarius_ATTR1_NAME);
        msg +=" exists ";
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    return ret_flag;
}       

// Function to check if the product is ACOS Level 2 or SMAP.
bool check_smap_acosl2s(hid_t s_root_id, int which_pro) {

    htri_t has_smac_group;
    // cerr <<"coming to smap acos "<<endl;
    bool return_flag = false;
    has_smac_group = H5Lexists(s_root_id,SMAC2S_META_GROUP_NAME,H5P_DEFAULT);

    if (has_smac_group >0){
        hid_t s_group_id = -1;

        // Open the group
        if ((s_group_id = H5Gopen(s_root_id, SMAC2S_META_GROUP_NAME,H5P_DEFAULT))<0) {
           string msg = "Cannot open the HDF5 Group  ";
           msg += string(SMAC2S_META_GROUP_NAME);
           H5Gclose(s_root_id);
           throw InternalErr(__FILE__, __LINE__, msg);
        }

        // SMAP 
        if (1 == which_pro) {
        
            htri_t has_smap_attr = -1;
            // SMAP will have an attribute called ProjectID
            has_smap_attr = H5Aexists(s_group_id,SMAP_ATTR_NAME);
            if (has_smap_attr >0) {
                string attr_value = "";
                obtain_gm_attr_value(s_group_id, SMAP_ATTR_NAME, attr_value);
                if (attr_value.compare(SMAP_ATTR_VALUE) == 0) 
                    return_flag = true;
                else 
                    return_flag = false;
                H5Gclose(s_group_id);
            }
            else if (0 == has_smap_attr) {
                H5Gclose(s_group_id);
                return_flag = false;
            }
            else {
                string msg = "Fail to determine if the HDF5 link  ";
                msg += string(SMAP_ATTR_NAME);
                msg +="  exists ";
                H5Gclose(s_group_id);
                H5Gclose(s_root_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }
        }
        else if (2 == which_pro) {
            // cerr <<"coming to acos l2s "<<endl;

            htri_t has_acos_dset = -1;

            // ACOSL2S will have a dataset called ProjectID
            has_acos_dset = H5Lexists(s_group_id,ACOS_L2S_DSET_NAME,H5P_DEFAULT);
            if (has_acos_dset > 0) {
                // cerr <<"coming to acos l2s dataset "<<endl;
                // Obtain the dataset ID
                hid_t s_dset_id = -1;
                if ((s_dset_id = H5Dopen(s_group_id, ACOS_L2S_DSET_NAME,H5P_DEFAULT)) < 0) {
                    string msg = "cannot open the HDF5 dataset  ";
                    msg += string(ACOS_L2S_DSET_NAME);
                    H5Gclose(s_group_id);
                    H5Gclose(s_root_id);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                // Obtain the datatype ID
                hid_t dtype = -1;
                if ((dtype = H5Dget_type(s_dset_id)) < 0) {
                    H5Dclose(s_dset_id);
                    H5Gclose(s_group_id);
                    H5Gclose(s_root_id);
                    string msg = "cannot get the datatype of HDF5 dataset  ";
                    msg += string(ACOS_L2S_DSET_NAME);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                // Obtain the datatype class 
                H5T_class_t ty_class = H5Tget_class(dtype);
                if (ty_class < 0) {
                    H5Tclose(dtype);
                    H5Dclose(s_dset_id);
                    H5Gclose(s_group_id);
                    H5Gclose(s_root_id);
                    string msg = "cannot get the datatype class of HDF5 dataset  ";
                    msg += string(ACOS_L2S_DSET_NAME);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                if (ty_class != H5T_STRING) {
                    H5Tclose(dtype);
		    H5Dclose(s_dset_id);
                    H5Gclose(s_group_id);
                    H5Gclose(s_root_id);
                    string msg = "This dataset must be a H5T_STRING class  ";
                    msg += string(ACOS_L2S_DSET_NAME);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }


                hid_t dspace = -1;
                if ((dspace = H5Dget_space(s_dset_id)) < 0) {
                    H5Tclose(dtype);
                    H5Dclose(s_dset_id);
                    H5Gclose(s_group_id);
                    H5Gclose(s_root_id);
                    string msg = "cannot get the the dataspace of HDF5 dataset  ";
                    msg += string(ACOS_L2S_DSET_NAME);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                hssize_t num_elem = 0;
                if ((num_elem = H5Sget_simple_extent_npoints(dspace))<=0) {
                    H5Tclose(dtype);
                    H5Sclose(dspace);
        	    H5Dclose(s_dset_id);
                    H5Gclose(s_group_id);
                    H5Gclose(s_root_id);
                    string msg = "cannot get the the number of points of HDF5 dataset  ";
                    msg += string(ACOS_L2S_DSET_NAME);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                size_t dtype_size = H5Tget_size(dtype);
                if (dtype_size <= 0) {
                    H5Tclose(dtype);
                    H5Dclose(s_dset_id);
                    H5Sclose(dspace);
                    H5Gclose(s_group_id);
                    H5Gclose(s_root_id);
                    string msg = "cannot get the the dataspace of HDF5 dataset  ";
                    msg += string(ACOS_L2S_DSET_NAME);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }
 
                size_t total_data_size = num_elem * H5Tget_size(dtype);

                if (H5Tis_variable_str(dtype)) {
                    // cerr <<"coming to variable length string "<<endl;
                    char *temp_buf = NULL;
                    try {
                    	// TODO replace with vector<char>
                        temp_buf = new char[total_data_size];
                        if (H5Dread(s_dset_id,dtype,H5S_ALL,H5S_ALL,H5P_DEFAULT, temp_buf)<0){
                           H5Tclose(dtype);
                           H5Dclose(s_dset_id);
                           H5Sclose(dspace);
                           H5Gclose(s_group_id);
                           H5Gclose(s_root_id);
               	           string msg = "cannot get the the dataspace of HDF5 dataset  ";
                           msg += string(ACOS_L2S_DSET_NAME);
                           throw InternalErr(__FILE__, __LINE__, msg);
                        }

                        char *temp_bp = temp_buf;
                        char *onestring = NULL;
                        string total_string="";
                        
                        for (int temp_i = 0; temp_i <num_elem; temp_i++) {

                            // This line will assure that we get the real variable length string value.
                            onestring =*(char **)temp_bp;
                            // Change the C-style string to C++ STD string just for easy handling.
                            if (onestring !=NULL) {
                                string tempstring(onestring);
                                total_string+=tempstring;
                                // cerr <<"temp_string attr "<<tempstring <<endl;
                            }
                            // going to the next value.
                            temp_bp += dtype_size;
                        }
                        
                        if (temp_buf != NULL) {
                            // Reclaim any VL memory if necessary.
                            H5Dvlen_reclaim(dtype,dspace,H5P_DEFAULT,temp_buf);
                            delete []temp_buf;
                        }

                        H5Sclose(dspace);
                        H5Tclose(dtype);
                        H5Dclose(s_dset_id);
                        H5Gclose(s_group_id);
                        // cerr<<"total_string "<<total_string <<endl;
 
                        if (total_string.compare(ACOS_L2S_ATTR_VALUE) ==0) return_flag = true;
                    }
                    catch (...) {
                        if (temp_buf != NULL)
                            delete[] temp_buf;
                        throw;
                    }
                }
                else {
                    vector<char> temp_buf(total_data_size+1);
                    if (H5Dread(s_dset_id,dtype,H5S_ALL,H5S_ALL,H5P_DEFAULT, &temp_buf[0])<0){
                           H5Tclose(dtype);
                           H5Dclose(s_dset_id);
                           H5Sclose(dspace);
                           H5Gclose(s_group_id);
                           H5Gclose(s_root_id);
               	           string msg = "cannot data of HDF5 dataset  ";
                           msg += string(ACOS_L2S_DSET_NAME);
                           throw InternalErr(__FILE__, __LINE__, msg);
                    }

                    string total_string(temp_buf.begin(),temp_buf.end()-1);
                    H5Sclose(dspace);
                    H5Tclose(dtype);
                    H5Dclose(s_dset_id);
                    H5Gclose(s_group_id);
                    // cerr<<"total_string "<<total_string <<endl;

                    if (0 == total_string.compare(ACOS_L2S_ATTR_VALUE)) 
                        return_flag = true;
                    else 
                        return_flag = false;
                } 
            }
            else if (0 == has_acos_dset) {
                H5Gclose(s_group_id);
                return_flag = false;
            }
            else {
                string msg = "Fail to determine if the HDF5 link  "; 
                msg += string(ACOS_L2S_DSET_NAME);
                msg +="  exists ";
                H5Gclose(s_group_id);
                H5Gclose(s_root_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }
         }
         else ;// Other product, don't do anything.
    }
    else if (0 == has_smac_group) return_flag = false;
    else {
        string msg = "Fail to determine if the link  ";
        msg += string(SMAC2S_META_GROUP_NAME);
        msg +=" exists or not ";
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    return return_flag;
}

// Function to check if the product is NETCDF4_GENERAL
bool check_netcdf4_general(hid_t root_id) {


}
        
void obtain_gm_attr_value(hid_t s_root_id, const char* s_attr_name, string & s_attr_value) {

    hid_t s_attr_id = -1;
    if ((s_attr_id = H5Aopen_by_name(s_root_id,".",s_attr_name,
                     H5P_DEFAULT, H5P_DEFAULT)) <0) {
        string msg = "Cannot open the HDF5 attribute  ";
        msg += string(s_attr_name);
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    hid_t attr_type = -1;
    if ((attr_type = H5Aget_type(s_attr_id)) < 0) {
        string msg = "cannot get the attribute datatype for the attribute  ";
        msg += string(s_attr_name);
        H5Aclose(s_attr_id);
        H5Gclose(s_root_id);
    }
               
    hid_t attr_space = -1;
    if ((attr_space = H5Aget_space(s_attr_id)) < 0) {
        string msg = "cannot get the hdf5 dataspace id for the attribute ";
        msg += string(s_attr_name);
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    int num_elm = 0;
    if (((num_elm = H5Sget_simple_extent_npoints(attr_space)) == 0)) {
        string msg = "cannot get the number for the attribute ";
        msg += string(s_attr_name);
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    size_t atype_size = H5Tget_size(attr_type);
    if (atype_size <= 0) {
        string msg = "cannot obtain the datatype size of the attribute ";
        msg += string(s_attr_name);
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    vector<char> temp_buf(atype_size*num_elm+1);
    // cerr <<"attribute size "<<atype_size*num_elm <<endl;
    if (H5Aread(s_attr_id,attr_type, &temp_buf[0])<0){
        string msg = "cannot retrieve the value of  the attribute ";
        msg += string(s_attr_name);
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);

    }

    string temp_attr_value(temp_buf.begin(),temp_buf.end());
    // cerr <<"size of temp_attr_value "<<temp_attr_value.size() <<endl;
    size_t temp_null_pos = temp_attr_value.find_first_of('\0');
    s_attr_value = temp_attr_value.substr(0,temp_null_pos);
    //s_attr_value(temp_buf.begin(),temp_buf.end()-1);
    H5Tclose(attr_type);
    H5Sclose(attr_space);
    H5Aclose(s_attr_id);
}

