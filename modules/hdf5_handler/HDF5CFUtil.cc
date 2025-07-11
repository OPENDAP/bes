// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2023 The HDF Group, Inc. and OPeNDAP, Inc.
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

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5CFUtil.cc
/// \brief The implementation of several helper functions for translating HDF5 to CF-compliant
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2023 The HDF Group
///
/// All rights reserved.

#include "HDF5CFUtil.h"
#include "HDF5RequestHandler.h"
#include <set>
#include <sstream>
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include<InternalErr.h>

using namespace std;
using namespace libdap;
// For using GCTP to calculate the lat/lon
extern "C" {
int hinv_init(int insys, int inzone, double *inparm, int indatum, char *fn27, char *fn83, int *iflg, int (*hinv_trans[])(double, double, double*, double*));

int hfor_init(int outsys, int outzone, double *outparm, int outdatum, char *fn27, char *fn83, int *iflg, int (*hfor_trans[])(double, double, double *, double *));
 
}

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

size_t HDF5CFUtil::H5_numeric_atomic_type_size(H5DataType h5type) {
    
    switch(h5type) {
        case H5CHAR:
        case H5UCHAR:
            return 1;
        case H5INT16:
        case H5UINT16:
            return 2;
        case H5INT32:
        case H5UINT32:
        case H5FLOAT32:
            return 4;
        case H5FLOAT64:
        case H5INT64:
        case H5UINT64:
            return 8;
        default: {
            string msg = "This routine doesn't support to return the size of this datatype";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
    }

}

#if 0
bool HDF5CFUtil::use_lrdata_mem_cache(H5DataType h5type, CVType cvtype, bool islatlon ) {
    if(h5type != H5CHAR && h5type !=H5UCHAR && h5type!=H5INT16 && h5type !=H5UINT16 &&
            h5type != H5INT32 && h5type !=H5UINT32 && h5type !=H5FLOAT32 && h5type!=H5FLOAT64 &&
            h5type != H5INT64 && h5type !=H5UINT64)
        return false;
    else {
         if(cvtype != CV_UNSUPPORTED) 
            return true;
         // TODO; if varpath is specified by the user, this should return true!
         else if(varpath == "")
             return false;
         else 
             return false;

    }

}
#endif

// Check if we cna use data memory cache
// TODO: This functio is not used, we will check it in the next release.
bool HDF5CFUtil::use_data_mem_cache(H5DataType h5type, CVType cvtype, const string &varpath) {
    if(h5type != H5CHAR && h5type !=H5UCHAR && h5type!=H5INT16 && h5type !=H5UINT16 &&
            h5type != H5INT32 && h5type !=H5UINT32 && h5type !=H5FLOAT32 && h5type!=H5FLOAT64 &&
            h5type != H5INT64 && h5type !=H5UINT64)
        return false;
    else {
         if(cvtype != CV_UNSUPPORTED) 
            return true;
         // TODO; if varpath is specified by the user, this should return true!
         else if(varpath == "")
             return false;
         else 
             return false;

    }

}

bool HDF5CFUtil::cf_strict_support_type(H5DataType dtype, bool is_dap4) {
    if ((H5UNSUPTYPE == dtype)||(H5REFERENCE == dtype)
        || (H5COMPOUND == dtype) || (H5ARRAY == dtype))
        // Important comments for the future work. 
        // Try to suport 64-bit integer for DAP4 CF, check the starting code at get_dmr_64bit_int()
        //"|| (H5INT64 == dtype)    ||(H5UINT64 == dtype))"
        return false;
    else if ((H5INT64 == dtype) || (H5UINT64 == dtype)) {
        if (true == is_dap4 || HDF5RequestHandler::get_dmr_long_int()==true)
            return true;
        else
            return false;
    }
    else 
        return true;
}

bool HDF5CFUtil::cf_dap2_support_numeric_type(H5DataType dtype,bool is_dap4) {
    if ((H5UNSUPTYPE == dtype)||(H5REFERENCE == dtype)
        || (H5COMPOUND == dtype) || (H5ARRAY == dtype)
        || (H5INT64 == dtype)    ||(H5UINT64 == dtype)
        || (H5FSTRING == dtype)  ||(H5VSTRING == dtype))
        return false;
    else if ((H5INT64 == dtype) ||(H5UINT64 == dtype)) {
        if(true == is_dap4 || true == HDF5RequestHandler::get_dmr_long_int())
            return true;
        else 
            return false;
    }
    else
        return true;
}

string HDF5CFUtil::obtain_string_after_lastslash(const string &s) {

    string ret_str="";
    size_t last_fslash_pos = s.find_last_of("/");
    if (string::npos != last_fslash_pos && 
        last_fslash_pos != (s.size()-1)) 
        ret_str=s.substr(last_fslash_pos+1);
    return ret_str;
}

string HDF5CFUtil::obtain_string_before_lastslash(const string & s) {

    string ret_str="";
    size_t last_fslash_pos = s.find_last_of("/");
    if (string::npos != last_fslash_pos)
        ret_str=s.substr(0,last_fslash_pos+1);
    return ret_str;

}

string HDF5CFUtil::trim_string(hid_t ty_id,const string & s, int num_sect, size_t sect_size, vector<size_t>& sect_newsize) {

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

string HDF5CFUtil::remove_substrings(string str,const string &substr) {

    string::size_type i = str.find(substr);
    while (i != std::string::npos) {
        str.erase(i, substr.size());
        i = str.find(substr, i);
    }
    return str;
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

void
HDF5CFUtil::Split_helper(vector<string> &tokens, const string &text, const char sep)
{
    string::size_type start = 0;
    string::size_type end = 0;

    while ((end = text.find(sep, start)) != string::npos) {
        tokens.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(text.substr(start));
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

    float lat_north = 0.;
    float lat_south = 0.;
    float lon_east = 0.;
    float lon_west = 0.;
     
    vector<string> ind_elems;
    char sep='\n';
    HDF5CFUtil::Split(value.data(),sep,ind_elems);
     
    // The number of elements in the GridHeader is 9. The string vector will add a leftover. So the size should be 10.
    // KY: on a CentOS 7 machine, the number of elements is wrongly generated by compiler to 11 instead of 10. 
#if 0
    //if(ind_elems.size()!=10)
#endif
    if (ind_elems.size()<9) {
        string msg = "The number of elements in the GPM level 3 GridHeader is not right.";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    if(false == check_reg_orig) {
        if (0 != ind_elems[1].find("Registration=CENTER")) {       
            string msg = "The GPM grid registration is not center.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
    }
        
    if (0 == ind_elems[2].find("LatitudeResolution")){ 

        size_t equal_pos = ind_elems[2].find_first_of('=');
        if (string::npos == equal_pos) {
            string msg = "Cannot find latitude resolution for GPM level 3 products.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
           
        size_t scolon_pos = ind_elems[2].find_first_of(';');
        if (string::npos == scolon_pos) {
            string msg = "Cannot find latitude resolution for GPM level 3 products. ";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
        if (equal_pos < scolon_pos){

            string latres_str = ind_elems[2].substr(equal_pos+1,scolon_pos-equal_pos-1);
            lat_res = strtof(latres_str.c_str(),nullptr);
        }
        else  {
            string msg = "Latitude resolution is not right for GPM level 3 products.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
    }
    else {
        string msg = "The GPM grid LatitudeResolution doesn't exist.";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    if (0 == ind_elems[3].find("LongitudeResolution")){ 

        size_t equal_pos = ind_elems[3].find_first_of('=');
        if(string::npos == equal_pos){
            string msg = "Cannot find longitude resolution for GPM level 3 products. ";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
           
        size_t scolon_pos = ind_elems[3].find_first_of(';');
        if(string::npos == scolon_pos) {
            string msg = "Cannot find longitude resolution for GPM level 3 products. ";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
        if (equal_pos < scolon_pos){
            string lonres_str = ind_elems[3].substr(equal_pos+1,scolon_pos-equal_pos-1);
            lon_res = strtof(lonres_str.c_str(),nullptr);
        }
        else {
            string msg = "Longitude resolution is not right for GPM level 3 products.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
    }
    else {
        string msg = "The GPM grid LongitudeResolution doesn't exist.";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    if (0 == ind_elems[4].find("NorthBoundingCoordinate")){ 

        size_t equal_pos = ind_elems[4].find_first_of('=');
        if(string::npos == equal_pos) {
            string msg = "Cannot find latitude resolution for GPM level 3 products.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
           
        size_t scolon_pos = ind_elems[4].find_first_of(';');
        if(string::npos == scolon_pos) {
            string msg = "Cannot find latitude resolution for GPM level 3 products.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
        if (equal_pos < scolon_pos){
            string north_bounding_str = ind_elems[4].substr(equal_pos+1,scolon_pos-equal_pos-1);
            lat_north = strtof(north_bounding_str.c_str(),nullptr);
        }
        else {
            string msg = "NorthBoundingCoordinate is not right for GPM level 3 products.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
    }
    else {
        string msg = "The GPM grid NorthBoundingCoordinate doesn't exist.";
        throw InternalErr(__FILE__,__LINE__, msg);
     
    }

    if (0 == ind_elems[5].find("SouthBoundingCoordinate")){ 

        size_t equal_pos = ind_elems[5].find_first_of('=');
        if(string::npos == equal_pos) {
            string msg = "Cannot find south bound coordinate for GPM level 3 products.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
       
        size_t scolon_pos = ind_elems[5].find_first_of(';');
        if(string::npos == scolon_pos) {
            string msg = "Cannot find south bound coordinate for GPM level 3 products.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
        if (equal_pos < scolon_pos){
            string lat_south_str = ind_elems[5].substr(equal_pos+1,scolon_pos-equal_pos-1);
            lat_south = strtof(lat_south_str.c_str(),nullptr);
        }
        else {
            string msg = "The south bound coordinate is not right for GPM level 3 products.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
 
    }
    else {
        string msg = "The GPM grid SouthBoundingCoordinate doesn't exist.";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    if (0 == ind_elems[6].find("EastBoundingCoordinate")){ 

        size_t equal_pos = ind_elems[6].find_first_of('=');
        if(string::npos == equal_pos) {
            string msg = "Cannot find the east bound coordinate for GPM level 3 products. ";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
           
        size_t scolon_pos = ind_elems[6].find_first_of(';');
        if(string::npos == scolon_pos) {
            string msg = "Cannot find the east bound coordinate for GPM level 3 products.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
        if (equal_pos < scolon_pos){
            string lon_east_str = ind_elems[6].substr(equal_pos+1,scolon_pos-equal_pos-1);
            lon_east = strtof(lon_east_str.c_str(),nullptr);
        }
        else {
            string msg = "The east bound coordinate is not right for GPM level 3 products.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
 
    }
    else {
        string msg = "The GPM grid EastBoundingCoordinate doesn't exist.";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    if (0 == ind_elems[7].find("WestBoundingCoordinate")){ 

        size_t equal_pos = ind_elems[7].find_first_of('=');
        if(string::npos == equal_pos) {
            string msg = "Cannot find the west bound coordinate for GPM level 3 products";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
           
        size_t scolon_pos = ind_elems[7].find_first_of(';');
        if(string::npos == scolon_pos) {
            string msg = "Cannot find the west bound coordinate for GPM level 3 products";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
        if (equal_pos < scolon_pos){
            string lon_west_str = ind_elems[7].substr(equal_pos+1,scolon_pos-equal_pos-1);
            lon_west = strtof(lon_west_str.c_str(),nullptr);
        }
        else { 
            string msg = "The west bound coordinate is not right for GPM level 3 products.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
    }
    else {
        string msg = "The GPM grid WestBoundingCoordinate doesn't exist.";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    if (false == check_reg_orig) {
        if (0 != ind_elems[8].find("Origin=SOUTHWEST")) {
            string msg = "The GPM grid origin is not SOUTHWEST.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
    }

    // Since we only treat the case when the Registration is center, so the size should be the 
    // regular number size - 1.
    latsize =(int)((lat_north-lat_south)/lat_res);
    lonsize =(int)((lon_east-lon_west)/lon_res);
    lat_start = lat_south;
    lon_start = lon_west;
}

void HDF5CFUtil::close_fileid(hid_t file_id,bool pass_fileid) {
    if((false == pass_fileid) && (file_id !=-1)) 
            H5Fclose(file_id);
}

// Somehow the conversion of double to c++ string with sprintf causes the memory error in
// the testing code. I used the following code to do the conversion. Most part of the code
// in reverse, int_to_str and dtoa are adapted from geeksforgeeks.org

// reverses a string 'str' of length 'len'
void HDF5CFUtil::rev_str(char *str, int len)
{
    int i=0;
    int j=len-1;
    int temp = 0;
    while (i<j)
    {
        temp = str[i];
        str[i] = str[j];
        str[j] = (char)temp;
        i++; 
        j--;
    }
}
 
// Converts a given integer x to string str[].  d is the number
// of digits required in output. If d is more than the number
// of digits in x, then 0s are added at the beginning.
int HDF5CFUtil::int_to_str(int x, char str[], int d)
{
    int i = 0;
    while (x)
    {
        str[i++] = (x%10) + '0';
        x = x/10;
    }
 
    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';
 
    rev_str(str, i);
    str[i] = '\0';
    return i;
}
 
// Converts a double floating point number to string.
void HDF5CFUtil::dtoa(double n, char *res, int afterpoint)
{
    // Extract integer part
    auto ipart = (int)n;
 
    // Extract the double part
    double fpart = n - (double)ipart;
 
    // convert integer part to string
    int i = int_to_str(ipart, res, 0);
 
    // check for display option after point
    if (afterpoint != 0)
    {
        res[i] = '.';  // add dot
 
        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter is needed
        // to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);
 
        // A round-error of 1 is found when casting to the integer for some numbers.
        // We need to correct it.
        auto final_fpart = (int)fpart;
        if(fpart -(int)fpart >0.5)
            final_fpart = (int)fpart +1;
        int_to_str(final_fpart, res + i + 1, afterpoint);
    }
}


// Used to generate EOS geolocation cache name
string HDF5CFUtil::get_int_str(int x) {

   string str;
   if(x > 0 && x <10)   
      str.push_back((char)x+'0');
   
   else if (x >10 && x<100) {
      str.push_back((char)(x/10)+'0');
      str.push_back((char)(x%10)+'0');
   }
   else {
      int num_digit = 0;
      int abs_x = (x<0)?-x:x;
      while(abs_x/=10) 
         num_digit++;
      if(x<=0)
         num_digit++;
      vector<char> buf;
      buf.resize(num_digit);
      snprintf(buf.data(),num_digit,"%d",x);
      str.assign(buf.data());

   }      

   return str;

}
 
//Used to generate EOS5 lat/lon cache name
string HDF5CFUtil::get_double_str(double x,int total_digit,int after_point) {
    
    string str;
    if(x!=0) {
        vector<char> res;
        res.resize(total_digit);
        for(int i = 0; i<total_digit;i++)
           res[i] = '\0';
        if (x<0) { 
           str.push_back('-');
           dtoa(-x,res.data(),after_point);
           for(int i = 0; i<total_digit;i++) {
               if(res[i] != '\0')
                  str.push_back(res[i]);
           }
        }
        else {
           dtoa(x, res.data(), after_point);
           for(int i = 0; i<total_digit;i++) {
               if(res[i] != '\0')
                  str.push_back(res[i]);
           }
        }
    
    }
    else 
       str.push_back('0');
        
    return str;

}


// This function is adapted from the HDF-EOS library.
int GDij2ll(int projcode, int zonecode, double projparm[],
        int spherecode, int xdimsize, int ydimsize,
        double upleftpt[], double lowrightpt[],
        int npnts, int row[], int col[],
        double longitude[], double latitude[], EOS5GridPRType pixcen, EOS5GridOriginType pixcnr)
{

    int errorcode = 0;
    // In the original GCTP library, the function pointer names should be inv_trans and for_trans.
    // However, since Hyrax supports both GDAL(including the HDF-EOS2 driver) and HDF handlers,
    // on some machines, the functions inside the HDF-EOS2 driver will be called in run-time and wrong lat/lon
    // values may be generated. To avoid, we change the function pointer names inside the GCTP library.
    int(*hinv_trans[100]) (double,double,double*,double*);  
    int(*hfor_trans[100]) (double,double,double*,double*);  /* GCTP function pointer */
    double        arg1;
    double        arg2;
    double        pixadjX  = 0.;  /* Pixel adjustment (x)                 */
    double        pixadjY  = 0.;  /* Pixel adjustment (y)                 */
    double        lonrad0  = 0.;  /* Longitude in radians of upleft point */
    double          latrad0  = 0.;  /* Latitude in radians of upleft point  */
    double          scaleX   = 0.;  /* X scale factor                       */
    double          scaleY   = 0.;  /* Y scale factor                       */
    double          lonrad   = 0.;  /* Longitude in radians of point        */
    double          latrad   = 0.;  /* Latitude in radians of point         */
    double          xMtr0;
    double          yMtr0;
    double          xMtr1;
    double          yMtr1;



    /* Compute adjustment of position within pixel */
  /* ------------------------------------------- */
  if (pixcen == HE5_HDFE_CENTER)
  {
      /* Pixel defined at center */
      /* ----------------------- */
      pixadjX = 0.5;
      pixadjY = 0.5;
  }
  else
  {
      switch (pixcnr)
      {
  
        case HE5_HDFE_GD_UL:
        {
            /* Pixel defined at upper left corner */
            /* ---------------------------------- */
            pixadjX = 0.0;
            pixadjY = 0.0;
            break;
          }

        case HE5_HDFE_GD_UR:
        {
            /* Pixel defined at upper right corner */
            /* ----------------------------------- */
            pixadjX = 1.0;
            pixadjY = 0.0;
            break;
        }

        case HE5_HDFE_GD_LL:
        {
            /* Pixel defined at lower left corner */
            /* ---------------------------------- */
            pixadjX = 0.0;
            pixadjY = 1.0;
            break;
        }

        case HE5_HDFE_GD_LR:
        {

            /* Pixel defined at lower right corner */
            /* ----------------------------------- */
            pixadjX = 1.0;
            pixadjY = 1.0;
            break;
        }

        default:
        {
            string msg = "Unknown pixel corner to retrieve lat/lon from a grid.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
    }
  }



  // If projection not GEO or BCEA call GCTP initialization routine 
  if (projcode != HE5_GCTP_GEO && projcode != HE5_GCTP_BCEA)
  {

      scaleX = (lowrightpt[0] - upleftpt[0]) / xdimsize;
      scaleY = (lowrightpt[1] - upleftpt[1]) / ydimsize;
      string eastFile = HDF5RequestHandler::get_stp_east_filename();
      string northFile = HDF5RequestHandler::get_stp_north_filename();

      hinv_init(projcode, zonecode, projparm, spherecode, (char*)eastFile.c_str(), (char*)northFile.c_str(), 
       &errorcode, hinv_trans);


      /* Report error if any */
      /* ------------------- */
      if (errorcode != 0)
      {
            string msg = "GCTP hinv_init Error to retrieve lat/lon from a grid.";
            throw InternalErr(__FILE__,__LINE__, msg);
      }
      else
      {
        /* For each point ... */
        /* ------------------ */
        for (int i = 0; i < npnts; i++)
            {
        /* Convert from meters to lon/lat (radians) using GCTP */
        /* --------------------------------------------------- */
#if 0
      /*errorcode = hinv_trans[projcode] ((col[i] + pixadjX) * scaleX + upleftpt[0], (row[i] + pixadjY) * scaleY + upleftpt[1], &lonrad, &latrad);*/
#endif

       /* modified previous line to the following for the linux64 with -fPIC in cmpilation. 
 Whithout the change col[] and row[] values are ridiclous numbers, resulting a strange 
 number (very big) for arg1 and arg2. But with (int) typecast they become normal integers,
 resulting in a acceptable values for arg1 and arg2. The problem was discovered during the
 lat/lon geolocating of an hdfeos5 file with 64-bit hadview plug-in, developped for linux64.
      */
        arg1 = ((col[i] + pixadjX) * scaleX + upleftpt[0]);
        arg2 = ((row[i] + pixadjY) * scaleY + upleftpt[1]);
        errorcode = hinv_trans[projcode] (arg1, arg2, &lonrad, &latrad);

        /* Report error if any */
        /* ------------------- */
        if (errorcode != 0)
        {

            if(projcode == HE5_GCTP_LAMAZ) {
                longitude[i] = 1.0e51;
                latitude[i] = 1.0e51;
            }
            else {
               string msg = "GCTP hinv_trans Error to retrieve lat/lon from a grid.";
               throw InternalErr(__FILE__,__LINE__, msg);
            }
        }
        else
        {

            /* Convert from radians to decimal degrees */
            /* --------------------------------------- */
            longitude[i] = HE5_EHconvAng(lonrad, HE5_HDFE_RAD_DEG);
            latitude[i]  = HE5_EHconvAng(latrad, HE5_HDFE_RAD_DEG);

        }
      }
    }
  }
  else if (projcode == HE5_GCTP_BCEA)
  {
      /* BCEA projection */
      /* -------------- */
 
      /* Note: upleftpt and lowrightpt are in packed degrees, so they
 must be converted to meters for this projection */

      /* Initialize forward transformation */
      /* --------------------------------- */
      hfor_init(projcode, zonecode, projparm, spherecode, nullptr, nullptr,&errorcode, hfor_trans);

      /* Report error if any */
      /* ------------------- */
      if (errorcode != 0)
      {
          string msg = "GCTP hfor_init Error to retrieve lat/lon from a grid.";
          throw InternalErr(__FILE__,__LINE__, msg);
      }

      /* Convert upleft and lowright X coords from DMS to radians */
      /* -------------------------------------------------------- */
      lonrad0 =HE5_EHconvAng(upleftpt[0], HE5_HDFE_DMS_RAD);
      lonrad = HE5_EHconvAng(lowrightpt[0], HE5_HDFE_DMS_RAD);

      /* Convert upleft and lowright Y coords from DMS to radians */
      /* -------------------------------------------------------- */
      latrad0 = HE5_EHconvAng(upleftpt[1], HE5_HDFE_DMS_RAD);
      latrad = HE5_EHconvAng(lowrightpt[1], HE5_HDFE_DMS_RAD);

      /* Convert form lon/lat to meters(or whatever unit is, i.e unit
          of r_major and r_minor) using GCTP */
      /* ----------------------------------------- */
      errorcode = hfor_trans[projcode] (lonrad0, latrad0, &xMtr0, &yMtr0);

      /* Report error if any */
      if (errorcode != 0)
      {
          string msg = "GCTP hfor_trans Error to retrieve lat/lon from a grid.";
          throw InternalErr(__FILE__,__LINE__, msg);

      }

      /* Convert from lon/lat to meters or whatever unit is, i.e unit
         of r_major and r_minor) using GCTP */
      /* ----------------------------------------- */
      errorcode = hfor_trans[projcode] (lonrad, latrad, &xMtr1, &yMtr1);

      /* Report error if any */
      if (errorcode != 0)
      {
          string msg = "GCTP hfor_trans Error to retrieve lat/lon from a grid.";
          throw InternalErr(__FILE__,__LINE__, msg);
      }

      /* Compute x scale factor */
      /* ---------------------- */
      scaleX = (xMtr1 - xMtr0) / xdimsize;

      /* Compute y scale factor */
      /* ---------------------- */
      scaleY = (yMtr1 - yMtr0) / ydimsize;

      /* Initialize inverse transformation */
      /* --------------------------------- */
      hinv_init(projcode, zonecode, projparm, spherecode, nullptr, nullptr, &errorcode, hinv_trans);
      /* Report error if any */
      /* ------------------- */
      if (errorcode != 0)
      {
          string msg = "GCTP hinv_init Error to retrieve lat/lon from a grid.";
          throw InternalErr(__FILE__,__LINE__, msg);
      }
      /* For each point ... */
      /* ------------------ */
      for (int i = 0; i < npnts; i++)
      {
        /* Convert from meters (or any units that r_major and
           r_minor has) to lon/lat (radians) using GCTP */
        /* --------------------------------------------------- */
        errorcode = hinv_trans[projcode] (
                       (col[i] + pixadjX) * scaleX + xMtr0,
                       (row[i] + pixadjY) * scaleY + yMtr0,
                       &lonrad, &latrad);

        /* Report error if any */
        /* ------------------- */
        if (errorcode != 0)
        {
#if 0
              /* status = -1;
 sprintf(errbuf, "GCTP Error: %li\n", errorcode);
 H5Epush(__FILE__, "HE5_GDij2ll", __LINE__, H5E_ARGS, H5E_BADVALUE , errbuf);
 HE5_EHprint(errbuf, __FILE__, __LINE__);
                 return (status); */
#endif
          longitude[i] = 1.0e51; /* PGSd_GCT_IN_ERROR */
          latitude[i] = 1.0e51; /* PGSd_GCT_IN_ERROR */
        }

        /* Convert from radians to decimal degrees */
        /* --------------------------------------- */
        longitude[i] = HE5_EHconvAng(lonrad, HE5_HDFE_RAD_DEG);
        latitude[i] = HE5_EHconvAng(latrad, HE5_HDFE_RAD_DEG);
    }
  }

  else if (projcode == HE5_GCTP_GEO)
  {
      /* GEO projection */
      /* -------------- */

      /*
       * Note: lonrad, lonrad0, latrad, latrad0 are actually in degrees for
       * the GEO projection case.
       */


      /* Convert upleft and lowright X coords from DMS to degrees */
      /* -------------------------------------------------------- */
      lonrad0 = HE5_EHconvAng(upleftpt[0], HE5_HDFE_DMS_DEG);
      lonrad  = HE5_EHconvAng(lowrightpt[0], HE5_HDFE_DMS_DEG);

      /* Compute x scale factor */
      /* ---------------------- */
      scaleX = (lonrad - lonrad0) / xdimsize;

      /* Convert upleft and lowright Y coords from DMS to degrees */
      /* -------------------------------------------------------- */
      latrad0 = HE5_EHconvAng(upleftpt[1], HE5_HDFE_DMS_DEG);
      latrad  = HE5_EHconvAng(lowrightpt[1], HE5_HDFE_DMS_DEG);

      /* Compute y scale factor */
      /* ---------------------- */
      scaleY = (latrad - latrad0) / ydimsize;

      /* For each point ... */
      /* ------------------ */
      for (int i = 0; i < npnts; i++)
      {
        /* Convert to lon/lat (decimal degrees) */
        /* ------------------------------------ */
        longitude[i] = (col[i] + pixadjX) * scaleX + lonrad0;
        latitude[i]  = (row[i] + pixadjY) * scaleY + latrad0;
      }
   }


#if 0
    hinv_init(projcode, zonecode, projparm, spherecode, eastFile, northFile,
                 (int *)&errorcode, hinv_trans);

    for (int i = 0; i < npnts; i++)
          {
            /* Convert from meters (or any units that r_major and
 *                r_minor has) to lon/lat (radians) using GCTP */
            /* --------------------------------------------------- */
            errorcode =
              hinv_trans[projcode] (
                                   upleftpt[0],
                                   upleftpt[1],
                                   &lonrad, &latrad);

    }
#endif

    return 0;

}


// convert angle degree to radian.
double 
HE5_EHconvAng(double inAngle, int code)
{
    long      min = 0;        /* Truncated Minutes      */
    long      deg = 0;        /* Truncated Degrees      */

    double    sec      = 0.;  /* Seconds                */
    double    outAngle = 0.;  /* Angle in desired units */
    double    pi  = 3.14159265358979324;/* Pi           */
    double    r2d = 180 / pi;     /* Rad-deg conversion */
    double    d2r = 1 / r2d;      /* Deg-rad conversion */

    switch (code)
    {

        /* Convert radians to degrees */
        /* -------------------------- */
    case HE5_HDFE_RAD_DEG:
        outAngle = inAngle * r2d;
        break;

        /* Convert degrees to radians */
        /* -------------------------- */
    case HE5_HDFE_DEG_RAD:
        outAngle = inAngle * d2r;
        break;


        /* Convert packed degrees to degrees */
        /* --------------------------------- */
    case HE5_HDFE_DMS_DEG:
        deg = (long)(inAngle / 1000000);
        min = (long)((inAngle - deg * 1000000) / 1000);
        sec = (inAngle - deg * 1000000 - min * 1000);
        outAngle = deg + min / 60.0 + sec / 3600.0;
        break;


        /* Convert degrees to packed degrees */
        /* --------------------------------- */
    case HE5_HDFE_DEG_DMS:
    {
        deg = (long)inAngle;
        min = (long)((inAngle - deg) * 60);
        sec = (inAngle - deg - min / 60.0) * 3600;
#if 0
/*
        if ((int)sec == 60)
        {
            sec = sec - 60;
            min = min + 1;
        }
*/
#endif
        if ( fabs(sec - 0.0) < 1.e-7 )
        {
            sec = 0.0;
        }

        if ( (fabs(sec - 60) < 1.e-7 ) || ( sec > 60.0 ))
        {
            sec = sec - 60;
            min = min + 1;
            if(sec < 0.0)
            {
                sec = 0.0;
            }
        }
        if (min == 60)
        {
            min = min - 60;
            deg = deg + 1;
        }
        outAngle = deg * 1000000 + min * 1000 + sec;
     }
        break;


        /* Convert radians to packed degrees */
        /* --------------------------------- */
    case HE5_HDFE_RAD_DMS:
    {
        inAngle = inAngle * r2d;
        deg = (long)inAngle;
        min = (long)((inAngle - deg) * 60);
        sec = ((inAngle - deg - min / 60.0) * 3600);
#if 0
/*
        if ((int)sec == 60)
        {
            sec = sec - 60;
            min = min + 1;
        }
*/
#endif
        if ( fabs(sec - 0.0) < 1.e-7 )
        {
            sec = 0.0;
        }

        if ( (fabs(sec - 60) < 1.e-7 ) || ( sec > 60.0 ))
        {
            sec = sec - 60;
            min = min + 1;
            if(sec < 0.0)
            {
                sec = 0.0;
            }
        }
        if (min == 60)
        {
            min = min - 60;
            deg = deg + 1;
        }
        outAngle = deg * 1000000 + min * 1000 + sec;
    }
        break;


        /* Convert packed degrees to radians */
        /* --------------------------------- */
    case HE5_HDFE_DMS_RAD:
        deg = (long)(inAngle / 1000000);
        min = (long)((inAngle - deg * 1000000) / 1000);
        sec = (inAngle - deg * 1000000 - min * 1000);
        outAngle = deg + min / 60.0 + sec / 3600.0;
        outAngle = outAngle * d2r;
        break;
    default:
        break;
    }
    return outAngle;
}





#if 0
/// This inline routine will translate N dimensions into 1 dimension.
inline size_t
HDF5CFUtil::INDEX_nD_TO_1D (const std::vector <size_t > &dims,
                const std::vector < size_t > &pos)
{
    //
    //  int a[10][20][30];  // & a[1][2][3] == a + (20*30+1 + 30*2 + 1 *3);
    //  int b[10][2]; // &b[1][2] == b + (20*1 + 2);
    // 
    if(dims.size () != pos.size ()) {
        string msg = "Dimension error in INDEX_nD_TO_1D routine.";
        throw InternalErr(__FILE__,__LINE__, msg);       
    }
    size_t sum = 0;
    size_t  start = 1;

    for (size_t p = 0; p < pos.size (); p++) {
        size_t m = 1;

        for (size_t j = start; j < dims.size (); j++)
            m *= dims[j];
        sum += m * pos[p];
        start++;
    }
    return sum;
}
#endif

//! Getting a subset of a variable
//
//      \param input Input variable
//       \param dim dimension info of the input
//       \param start start indexes of each dim
//       \param stride stride of each dim
//       \param edge count of each dim
//       \param poutput output variable
// \parrm index dimension index
//       \return 0 if successful. -1 otherwise.
//
//
#if 0
template<typename T>  
int HDF5CFUtil::subset(
    const T input[],
    int rank,
    vector<int> & dim,
    int start[],
    int stride[],
    int edge[],
    std::vector<T> *poutput,
    vector<int>& pos,
    int index)
{
    for(int k=0; k<edge[index]; k++) 
    {
        pos[index] = start[index] + k*stride[index];
        if(index+1<rank)
            subset(input, rank, dim, start, stride, edge, poutput,pos,index+1);
        if(index==rank-1)
        {
            poutput->push_back(input[INDEX_nD_TO_1D( dim, pos)]);
        }
    } // end of for
    return 0;
} // end of template<typename T> static int 
#endif

// Need to wrap a 'read buffer' from a pure file call here since read() is also a DAP function to read DAP data.
ssize_t HDF5CFUtil::read_buffer_from_file(int fd,  void*buf, size_t total_read) {

    const int max_read_buf_size = 1073741824;
    ssize_t ret_val = -1;

    if (total_read >max_read_buf_size) {
  
        ssize_t bytes_read = 0;
        int one_read_buf_size = max_read_buf_size;
        while (bytes_read < total_read) {
            if (total_read <(bytes_read+max_read_buf_size))
                one_read_buf_size = total_read - bytes_read;

            ret_val = read(fd,buf,one_read_buf_size);
            if (ret_val !=one_read_buf_size) {
                ret_val = -1;
                break;
            }
            bytes_read +=ret_val;
            buf = (char *)buf + ret_val;

        }
        if (ret_val != -1)
            ret_val = total_read;
    }
    else {
       ret_val = read(fd,buf,total_read);
       if (ret_val !=total_read)
            ret_val = -1;
    }
    return ret_val;
}

// Obtain the cache name. The clashing is rare given that fname is unique.The "_" may cause clashing in theory.
string HDF5CFUtil::obtain_cache_fname(const string & fprefix, const string &fname, const string &vname) {

     string cache_fname = fprefix;

     string correct_fname = fname;
     std::replace(correct_fname.begin(),correct_fname.end(),'/','_');
     
     string correct_vname = vname;

     // Replace the '/' to '_'
     std::replace(correct_vname.begin(),correct_vname.end(),'/','_');

     // Replace the ' ' to to '_" since space is not good for a file name
     std::replace(correct_vname.begin(),correct_vname.end(),' ','_');


     cache_fname = cache_fname +correct_fname +correct_vname;

     return cache_fname;
}
size_t INDEX_nD_TO_1D (const std::vector < size_t > &dims,
                                 const std::vector < size_t > &pos){
    //
    //  "int a[10][20][30]  // & a[1][2][3] == a + (20*30+1 + 30*2 + 1 *3)"
    //  "int b[10][2] // &b[1][2] == b + (20*1 + 2)"
    // 
    if (dims.size () != pos.size ()) {
        string msg = "Dimension error in INDEX_nD_TO_1D routine.";
        throw InternalErr(__FILE__,__LINE__, msg);       
    }
    size_t sum = 0;
    size_t  start = 1;

    for (const auto &pos_ele:pos) {
        size_t m = 1;

        for (size_t j = start; j < dims.size (); j++)
            m *= dims[j];
        sum += m * pos_ele;
        start++;
    }
    return sum;
}

// Supposed string temp_str contains several relpath, Obtain all the positions of relpath in temp_str.
// The positions are stored in a vector of size_t and returns.
void HDF5CFUtil::get_relpath_pos(const string& temp_str, const string& relpath, vector<size_t>&s_pos) {


    //s_pos holds all the positions that relpath appears within the temp_str

    size_t pos = temp_str.find(relpath, 0);
    while(pos != string::npos)
    {
        s_pos.push_back(pos);
#if 0
//cout<<"pos is "<<pos <<endl;
#endif
        pos = temp_str.find(relpath,pos+1);
    }
#if 0
//cout<<"pos.size() is "<<s_pos.size() <<endl;
#endif

}


void HDF5CFUtil::cha_co(string &co,const string & vpath) {

    string sep="/";
    string rp_sep="../";
    if(vpath.find(sep,1)!=string::npos) {
        if(co.find(sep)!=string::npos) {
            // if finding '../', reduce the path.
            if(co.find(rp_sep)!=string::npos) {
                vector<size_t>var_sep_pos;
                get_relpath_pos(vpath,sep,var_sep_pos);
                vector<size_t>co_rp_sep_pos;
                get_relpath_pos(co,rp_sep,co_rp_sep_pos);
                // We only support when "../" is at position 0. 
                if(co_rp_sep_pos[0]==0) {
                    // Obtain the '../' position at co 
                    if(co_rp_sep_pos.size() <var_sep_pos.size()) {
                        // We obtain the suffix of CO and the path from vpath.
                        size_t var_prefix_pos=var_sep_pos[var_sep_pos.size()-co_rp_sep_pos.size()-1];
                        string var_prefix=vpath.substr(1,var_prefix_pos);
                        string co_suffix = co.substr(co_rp_sep_pos[co_rp_sep_pos.size()-1]+rp_sep.size());
                        co = var_prefix+co_suffix;
#if 0
//cout<<"var_prefix_pos is "<<var_prefix_pos <<endl;
//cout<<"var_prefix is "<<var_prefix <<endl;
//cout<<"co_suffix is "<<co_suffix <<endl;
//cout<<"co is "<<co<<endl;;
#endif
                    }
                }
            }

        }
        else {// co no path, add fullpath
            string var_prefix = obtain_string_before_lastslash(vpath).substr(1);
            co = var_prefix +co;
#if 0
//cout<<"co full is " <<co <<endl;
#endif
        }
    }
}

