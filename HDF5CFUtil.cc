// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2013 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5CFUtil.cc
/// \brief The implementation of several helper functions for translating HDF5 to CF-compliant
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2013 The HDF Group
///
/// All rights reserved.

#include "HDF5CFUtil.h"
#include <set>
#include <sstream>
#include <stdlib.h>
#include<InternalErr.h>
using namespace libdap;


H5DataType
HDF5CFUtil:: H5type_to_H5DAPtype(hid_t h5_type_id)
{
    size_t size = 0;
    int sign    = -2;

    switch (H5Tget_class(h5_type_id)) {

        case H5T_INTEGER:
            size = H5Tget_size(h5_type_id);
            sign = H5Tget_sign(h5_type_id);

	    if (size == 1) { // Either signed char or unsigned char
                if (sign == H5T_SGN_2) 
                    return H5CHAR; 
                else 
                    return H5UCHAR;
            }
            else if (size == 2) {
                if (sign == H5T_SGN_2) 
                    return H5INT16;
                else 
                    return H5UINT16;
            }
            else if (size == 4) {
                if (sign == H5T_SGN_2) 
                    return H5INT32;
                else 
                    return H5UINT32;
            }
            else if (size == 8) {
                if (sign == H5T_SGN_2) 
                    return H5INT64;
                else 
                    return H5UINT64;
            }
            else return H5UNSUPTYPE;

        case H5T_FLOAT:
            size = H5Tget_size(h5_type_id);

            if (size == 4) return H5FLOAT32;
            else if (size == 8) return H5FLOAT64;
            else return H5UNSUPTYPE;

        case H5T_STRING:
            if (H5Tis_variable_str(h5_type_id)) 
                return H5VSTRING;
            else return H5FSTRING;
    
        case H5T_REFERENCE:
            return H5REFERENCE;
            

        case H5T_COMPOUND:
            return H5COMPOUND;

        case H5T_ARRAY:
            return H5ARRAY;

        default:
            return H5UNSUPTYPE;
            
    }
}

bool HDF5CFUtil::cf_strict_support_type(H5DataType dtype) {
    if ((H5UNSUPTYPE == dtype)||(H5REFERENCE == dtype)
        || (H5COMPOUND == dtype) || (H5ARRAY == dtype)
        || (H5INT64 == dtype)    ||(H5UINT64 == dtype))
        return false;
    else 
        return true;
}

string HDF5CFUtil::obtain_string_after_lastslash(const string s) {

    string ret_str="";
    size_t last_fslash_pos = s.find_last_of("/");
    if (string::npos != last_fslash_pos && 
        last_fslash_pos != (s.size()-1)) 
        ret_str=s.substr(last_fslash_pos+1);
    return ret_str;
}

string HDF5CFUtil::trim_string(hid_t ty_id,const string s, int num_sect, size_t sect_size, vector<size_t>& sect_newsize) {

    string temp_sect_str = "";
    string temp_sect_newstr = "";
    string final_str="";
    
    for (int i = 0; i < num_sect; i++) {
        
        if (i != (num_sect-1)) 
            temp_sect_str = s.substr(i*sect_size,sect_size); 
        else 
            temp_sect_str = s.substr((num_sect-1)*sect_size,s.size()-(num_sect-1)*sect_size);
         
        size_t temp_pos = 0;

        if (H5T_STR_NULLTERM == H5Tget_strpad(ty_id)) 
            temp_pos = temp_sect_str.find_first_of('\0');
        else if (H5T_STR_SPACEPAD == H5Tget_strpad(ty_id))
            temp_pos = temp_sect_str.find_last_not_of(' ')+1;
        else temp_pos = temp_sect_str.find_last_not_of('0')+1;// NULL PAD
        
        if (temp_pos != string::npos) {

            // Here I only add a space at the end of each section for the SPACEPAD case, 
            // but not for NULL TERM
            // or NULL PAD. Two reasons: C++ string doesn't need NULL TERM.
            // We don't know and don't see any NULL PAD applications for NASA data.
            if (H5T_STR_SPACEPAD == H5Tget_strpad(ty_id)) {

                if (temp_pos == temp_sect_str.size()) 
                    temp_sect_newstr = temp_sect_str +" ";
                else 
                    temp_sect_newstr = temp_sect_str.substr(0,temp_pos+1);
            
                sect_newsize[i] = temp_pos +1; 
            }
            else {
                temp_sect_newstr = temp_sect_str.substr(0,temp_pos);
                sect_newsize[i] = temp_pos ;
            }

        }

        else {// NULL is not found, adding a NULL at the end of this string.

            temp_sect_newstr = temp_sect_str;

            // Here I only add a space for the SPACEPAD, but not for NULL TERM
            // or NULL PAD. Two reasons: C++ string doesn't need NULL TERM.
            // We don't know and don't see any NULL PAD applications for NASA data.
            if (H5T_STR_SPACEPAD == H5Tget_strpad(ty_id)) {
                temp_sect_newstr.resize(temp_sect_str.size()+1);
                temp_sect_newstr.append(1,' ');
                sect_newsize[i] = sect_size + 1;
            } 
            else 
                sect_newsize[i] = sect_size;
        }
        final_str+=temp_sect_newstr;
   }

   return final_str;
}

// Obtain the unique name for the clashed names and save it to set namelist.

void HDF5CFUtil::gen_unique_name(string &str,set<string>& namelist, int&clash_index) {

    pair<set<string>::iterator,bool> ret;
    string newstr = "";
    stringstream sclash_index;
    sclash_index << clash_index;
    newstr = str + sclash_index.str();

    ret = namelist.insert(newstr);
    if (false == ret.second) {
        clash_index++;
        gen_unique_name(str,namelist,clash_index);
    }
    else 
       str = newstr;
}

           
// From a string separated by a separator to a list of string,
// for example, split "ab,c" to {"ab","c"}
void
HDF5CFUtil::Split(const char *s, int len, char sep, std::vector<std::string> &names)
{
    names.clear();
    for (int i = 0, j = 0; j <= len; ++j) {
        if ((j == len && len) || s[j] == sep) {
            string elem(s + i, j - i);
            names.push_back(elem);
            i = j + 1;
            continue;
        }
    }
}
    

// Assume sz is Null terminated string.
void
HDF5CFUtil::Split(const char *sz, char sep, std::vector<std::string> &names)
{
    Split(sz, (int)strlen(sz), sep, names);
}

void HDF5CFUtil::parser_gpm_l3_gridheader(const vector<char>& value, 
                                          int& latsize, int&lonsize, 
                                          float& lat_start, float& lon_start,
                                          float& lat_res, float& lon_res,
                                          bool check_reg_orig ){

    //bool cr_reg = false;
    //bool sw_origin = true;
    //float lat_res = 1.;
    //float lon_res = 1.;
    float lat_north = 0.;
    float lat_south = 0.;
    float lon_east = 0.;
    float lon_west = 0.;
     
    vector<string> ind_elems;
    char sep='\n';
    HDF5CFUtil::Split(&value[0],sep,ind_elems);
     
    // The number of elements in the GridHeader is 9. The string vector will add a leftover. So the size should be 10.
    if(ind_elems.size()!=10)
        throw InternalErr(__FILE__,__LINE__,"The number of elements in the TRMM level 3 GridHeader is not right.");

    if(false == check_reg_orig) {
        if (0 != ind_elems[1].find("Registration=CENTER"))        
            throw InternalErr(__FILE__,__LINE__,"The TRMM grid registration is not center.");
    }
        
    if (0 == ind_elems[2].find("LatitudeResolution")){ 

        size_t equal_pos = ind_elems[2].find_first_of('=');
        if(string::npos == equal_pos)
            throw InternalErr(__FILE__,__LINE__,"Cannot find latitude resolution for TRMM level 3 products");
           
        size_t scolon_pos = ind_elems[2].find_first_of(';');
        if(string::npos == scolon_pos)
            throw InternalErr(__FILE__,__LINE__,"Cannot find latitude resolution for TRMM level 3 products");
        if (equal_pos < scolon_pos){

            string latres_str = ind_elems[2].substr(equal_pos+1,scolon_pos-equal_pos-1);
            lat_res = strtof(latres_str.c_str(),NULL);
        }
        else 
            throw InternalErr(__FILE__,__LINE__,"latitude resolution is not right for TRMM level 3 products");
    }
    else
        throw InternalErr(__FILE__,__LINE__,"The TRMM grid LatitudeResolution doesn't exist.");

    if (0 == ind_elems[3].find("LongitudeResolution")){ 

        size_t equal_pos = ind_elems[3].find_first_of('=');
        if(string::npos == equal_pos)
            throw InternalErr(__FILE__,__LINE__,"Cannot find longitude resolution for TRMM level 3 products");
           
        size_t scolon_pos = ind_elems[3].find_first_of(';');
        if(string::npos == scolon_pos)
            throw InternalErr(__FILE__,__LINE__,"Cannot find longitude resolution for TRMM level 3 products");
        if (equal_pos < scolon_pos){
            string lonres_str = ind_elems[3].substr(equal_pos+1,scolon_pos-equal_pos-1);
            lon_res = strtof(lonres_str.c_str(),NULL);
        }
        else 
            throw InternalErr(__FILE__,__LINE__,"longitude resolution is not right for TRMM level 3 products");
    }
    else
        throw InternalErr(__FILE__,__LINE__,"The TRMM grid LongitudeResolution doesn't exist.");

    if (0 == ind_elems[4].find("NorthBoundingCoordinate")){ 

        size_t equal_pos = ind_elems[4].find_first_of('=');
        if(string::npos == equal_pos)
            throw InternalErr(__FILE__,__LINE__,"Cannot find latitude resolution for TRMM level 3 products");
           
        size_t scolon_pos = ind_elems[4].find_first_of(';');
        if(string::npos == scolon_pos)
            throw InternalErr(__FILE__,__LINE__,"Cannot find latitude resolution for TRMM level 3 products");
        if (equal_pos < scolon_pos){
            string north_bounding_str = ind_elems[4].substr(equal_pos+1,scolon_pos-equal_pos-1);
            lat_north = strtof(north_bounding_str.c_str(),NULL);
        }
        else 
            throw InternalErr(__FILE__,__LINE__,"NorthBoundingCoordinate is not right for TRMM level 3 products");
 
    }
     else
        throw InternalErr(__FILE__,__LINE__,"The TRMM grid NorthBoundingCoordinate doesn't exist.");

    if (0 == ind_elems[5].find("SouthBoundingCoordinate")){ 

            size_t equal_pos = ind_elems[5].find_first_of('=');
            if(string::npos == equal_pos)
                throw InternalErr(__FILE__,__LINE__,"Cannot find south bound coordinate for TRMM level 3 products");
           
            size_t scolon_pos = ind_elems[5].find_first_of(';');
            if(string::npos == scolon_pos)
                throw InternalErr(__FILE__,__LINE__,"Cannot find south bound coordinate for TRMM level 3 products");
            if (equal_pos < scolon_pos){
                string lat_south_str = ind_elems[5].substr(equal_pos+1,scolon_pos-equal_pos-1);
                lat_south = strtof(lat_south_str.c_str(),NULL);
            }
            else 
                throw InternalErr(__FILE__,__LINE__,"south bound coordinate is not right for TRMM level 3 products");
 
    }
    else
        throw InternalErr(__FILE__,__LINE__,"The TRMM grid SouthBoundingCoordinate doesn't exist.");

    if (0 == ind_elems[6].find("EastBoundingCoordinate")){ 

        size_t equal_pos = ind_elems[6].find_first_of('=');
        if(string::npos == equal_pos)
            throw InternalErr(__FILE__,__LINE__,"Cannot find south bound coordinate for TRMM level 3 products");
           
        size_t scolon_pos = ind_elems[6].find_first_of(';');
        if(string::npos == scolon_pos)
            throw InternalErr(__FILE__,__LINE__,"Cannot find south bound coordinate for TRMM level 3 products");
        if (equal_pos < scolon_pos){
            string lon_east_str = ind_elems[6].substr(equal_pos+1,scolon_pos-equal_pos-1);
            lon_east = strtof(lon_east_str.c_str(),NULL);
        }
        else 
            throw InternalErr(__FILE__,__LINE__,"south bound coordinate is not right for TRMM level 3 products");
 
    }
    else
        throw InternalErr(__FILE__,__LINE__,"The TRMM grid EastBoundingCoordinate doesn't exist.");

    if (0 == ind_elems[7].find("WestBoundingCoordinate")){ 

        size_t equal_pos = ind_elems[7].find_first_of('=');
        if(string::npos == equal_pos)
            throw InternalErr(__FILE__,__LINE__,"Cannot find south bound coordinate for TRMM level 3 products");
           
        size_t scolon_pos = ind_elems[7].find_first_of(';');
        if(string::npos == scolon_pos)
            throw InternalErr(__FILE__,__LINE__,"Cannot find south bound coordinate for TRMM level 3 products");
        if (equal_pos < scolon_pos){
            string lon_west_str = ind_elems[7].substr(equal_pos+1,scolon_pos-equal_pos-1);
            lon_west = strtof(lon_west_str.c_str(),NULL);
//cerr<<"latres str is "<<lon_west_str <<endl;
//cerr<<"latres is "<<lon_west <<endl;
        }
        else 
            throw InternalErr(__FILE__,__LINE__,"south bound coordinate is not right for TRMM level 3 products");
 
    }
    else
        throw InternalErr(__FILE__,__LINE__,"The TRMM grid WestBoundingCoordinate doesn't exist.");

    if (false == check_reg_orig) {
        if (0 != ind_elems[8].find("Origin=SOUTHWEST")) 
            throw InternalErr(__FILE__,__LINE__,"The TRMM grid origin is not SOUTHWEST.");
    }

    // Since we only treat the case when the Registration is center, so the size should be the 
    // regular number size - 1.
    latsize =(int)((lat_north-lat_south)/lat_res);
    lonsize =(int)((lon_east-lon_west)/lon_res);
    lat_start = lat_south;
    lon_start = lon_west;
}

void HDF5CFUtil::close_fileid(hid_t file_id,bool pass_fileid) {
    if(false == pass_fileid) {
        if(file_id != -1)
            H5Fclose(file_id);
    }

}


