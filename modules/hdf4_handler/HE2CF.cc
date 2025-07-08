//////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// Copyright (c) The HDF Group
//
// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org>
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
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
//////////////////////////////////////////////////////////////////////////////
#include "HE2CF.h"
#include <libdap/escaping.h>
#include <BESInternalError.h>
#include "HDFCFUtil.h"
#include <iomanip>

using namespace libdap;
using namespace std;

// Private member functions
bool 
HE2CF::get_vgroup_field_refids(const string& _gname,
                               int32* _ref_df,
                               int32* _ref_gf)
{

    int32 vrefid = Vfind(file_id, (char*)_gname.c_str());
    if (FAIL == vrefid) {
        Vend(file_id);
        string msg ="Cannot obtain the reference number for vgroup "+_gname+".";
        throw_error(msg);
        return false;
    }

    int32 vgroup_id = Vattach(file_id, vrefid, "r");
    if (FAIL == vgroup_id) {
        Vend(file_id);
        string msg = "Cannot obtain the group id for vgroup "+_gname+".";
        throw_error(msg);
        return false;
    }

    int32 npairs = Vntagrefs(vgroup_id);
    int32 ref_df = -1;
    int32 ref_gf = -1;

    if(npairs < 0){
        Vdetach(vgroup_id);
        Vend(file_id);
        string msg = "Got "+to_string(npairs)+" npairs for " + _gname +".";
        throw_error(msg);
        return false;
    }

    for (int i = 0; i < npairs; ++i) {
        
        int32 tag = 0;
        int32 ref = 0;
        
        if (Vgettagref(vgroup_id, i, &tag, &ref) < 0){

            // Previously just output stderr, it doesn't throw an internal error.
            // I believe this is wrong. Use DAP's internal error to throw an error.KY 2011-4-26 
            Vdetach(vgroup_id);
            Vend(file_id);
            string msg = "Failed to get tag / ref.";
            throw_error(msg);
            return false;            
        }
        
        if(Visvg(vgroup_id, ref)){

            char  cvgroup_name[VGNAMELENMAX*4]; // the child vgroup name
            int32 istat = 0;
            int32 vgroup_cid = 0; // Vgroup id 

            vgroup_cid = Vattach(file_id, ref,"r");
            if (FAIL == vgroup_cid) {
                Vdetach(vgroup_id);
                Vend(file_id);
                string msg = "Cannot obtain the vgroup id.";
                throw_error(msg);
                return false;
            }
            
            istat = Vgetname(vgroup_cid,cvgroup_name);
            if (FAIL == istat) {
                Vdetach(vgroup_cid);
                Vdetach(vgroup_id);
                Vend(file_id);
                string msg = "Cannot obtain the vgroup id.";
                throw_error(msg);
                return false;
            }

            if(strncmp(cvgroup_name, "Data Fields",  11) == 0){
                ref_df = ref;
            }
                
            if(strncmp(cvgroup_name, "Geolocation Fields",  18) == 0){
                ref_gf = ref;
            }
            
            if (FAIL == Vdetach(vgroup_cid)) {
                Vdetach(vgroup_id);
                Vend(file_id);
                ostringstream msg;
                msg  << "cannot close the vgroup "<< cvgroup_name <<"Successfully";
                throw_error(msg.str());
                return false;
            }

        }
    }
    *_ref_df = ref_df;
    *_ref_gf = ref_gf;

    if (FAIL == Vdetach(vgroup_id)) {
        Vend(file_id);
        string msg ="cannot close the vgroup "+ _gname + "Successfully.";
        throw_error(msg);
        return false;
    }   
    return true;
}

bool 
HE2CF::open_sd(const string& _filename,const int sd_id_in)
{
    int32 num_datasets = -1;
    sd_id = sd_id_in;
    if(SDfileinfo(sd_id, &num_datasets, &num_global_attributes)
        == FAIL){
        if(file_id != -1) 
            Vend(file_id);
        string msg = "Failed to call SDfileinfo() on " +_filename + " file.";
        throw_error(msg);
        return false;
    }
    return true;
}

bool 
HE2CF::open_vgroup(const string& _filename,const int file_id_in)
{

    file_id = file_id_in;
    if (Vstart(file_id) < 0){
        string msg = "Failed to call Vstart on " + _filename +".";
        throw_error(msg);        
        return false;
    }
    return true;
}


void HE2CF::set_DAS(DAS* _das)
{
    das = _das;
}

bool HE2CF::set_non_ecsmetadata_attrs() {

    for(int i = 0; i < num_global_attributes; i++){

        // H4_MAX_NC_NAME is from the user guide example. It's 256.
        char temp_name[H4_MAX_NC_NAME];     
        int32 attr_type = 0;
        int32 attr_count = 0;
        if(SDattrinfo(sd_id, i, temp_name, &attr_type, &attr_count) == FAIL) {
            Vend(file_id);
            string msg = "Fail to obtain SDS global attribute info.";
            throw_error(msg);
        }
       
        string attr_namestr(temp_name);
        // Check if this attribute is an HDF-EOS2 metadata(coremeta etc. ) attribute  
        // If yes, ignore this attribute.
        if (true == is_eosmetadata(attr_namestr)) 
            continue;
            
        // When DisableStructMetaAttr key is true, StructMetadata.0 is not in the
        // ECS metadata list. So this routine will pick up this attribute and generate
        // the DAP output here. Anyhow, 
        // StructMetadata attribute should not be generated here. We will turn it off.
        if (attr_namestr.compare(0,14, "StructMetadata" )== 0)
            continue;

        // When DisableECSMetaDataAll key is true,  All ECS attributes(Coremetadata etc.)
        // should not be in the 
        // ECS metadata list. But this routine will pick up those attributes and generate
        // the DAP output here. Anyhow, 
        // these attributes should not be generated here. We will turn it off.
        if (attr_namestr.compare(0,12, "CoreMetadata" )== 0)
            continue;
        if (attr_namestr.compare(0,12, "coremetadata" )== 0)
            continue;
        if (attr_namestr.compare(0,15, "ArchiveMetadata" )== 0)
            continue;
        if (attr_namestr.compare(0,15, "archivemetadata" )== 0)
            continue;
        if (attr_namestr.compare(0,15, "Productmetadata" )== 0)
            continue;
        if (attr_namestr.compare(0,15, "productmetadata" )== 0)
            continue;

        // USE VECTOR
        vector<char>attr_data;
        attr_data.resize((attr_count+1) *DFKNTsize(attr_type));

        if(SDreadattr(sd_id, i, attr_data.data()) == FAIL){
            Vend(file_id);
            string msg = "Fail to read SDS global attributes.";
            throw_error(msg);

        }

        // Handle character type attribute as a string.
	if (attr_type == DFNT_CHAR || attr_type == DFNT_UCHAR) {
            attr_data[attr_count] = '\0';
            attr_count = 1;
	}
             
        AttrTable *at = das->get_table("HDF_GLOBAL");
        if (!at)
            at = das->add_table("HDF_GLOBAL", new AttrTable);

        attr_namestr = HDFCFUtil::get_CF_string(attr_namestr);

#if 0
        if(attr_type == DFNT_UCHAR || attr_type == DFNT_CHAR){
            string tempstring2(attr_data);
            string tempfinalstr= string(tempstring2.c_str());

            // Using the customized escattr. Don't escape \n,\r and \t. KY 2013-10-14
            //tempfinalstr=escattr(tempfinalstr);
            at->append_attr(attr_namestr, "String" , HDFCFUtil::escattr(tempfinalstr));
        }
    
#endif
        
        for (int loc=0; loc < attr_count ; loc++) {
                string print_rep = HDFCFUtil::print_attr(attr_type, loc, (void*)attr_data.data() );
                at->append_attr(attr_namestr, HDFCFUtil::print_type(attr_type), print_rep);
        }

    }

    return true;
}



// Combine ECS metadata coremetadata.0,coremetadata.1 etc. into one string.
bool HE2CF::set_metadata(const string&  metadata_basename,vector<string>& non_number_names, vector<string>& no_num_data)
{
    bool suffix_is_num_or_null = true;
    metadata.clear();

    // Metadata like coremetadata,coremetadata.0,coremetadata.1
    list<string> one_dot_names;

    // Metadata like coremetadata, coremetadata.0, coremetadata.0.1
    list<string> two_dots_names;

    // Metadata with the suffix as .s, .t, like productmetadta.s, productmetadata.t
    // is non_number_names passed from the function

    int list_flag = -1;
    
    for(int i = 0; i < num_global_attributes; i++){

        // H4_MAX_NC_NAME is from the user guide example. It's 256.
        char temp_name[H4_MAX_NC_NAME];     
        int32 attr_type=0;
        int32 attr_count = 0;
        if(SDattrinfo(sd_id, i, temp_name, &attr_type, &attr_count) == FAIL) {
            Vend(file_id);
            string msg = "Fail to obtain SDS global attribute info.";
            throw_error(msg);
        }

        string temp_name_str(temp_name);

        // Find the basename, arrange the metadata name list.
        if(temp_name_str.find(metadata_basename)==0) {
            arrange_list(one_dot_names,two_dots_names,non_number_names,temp_name_str,list_flag);
        }
    }

    list<string>::const_iterator lit;
 
    // list_flag = 0, no suffix
    // list_flag = 1, only .0, coremetadata.0
    // list_flag = 2, coremetadata.0, coremetadata.1 etc
    // list_flag = 3, coremeatadata.0, coremetadata.0.1 etc
    if ( list_flag >= 0 && list_flag <=2) {
        for (lit = one_dot_names.begin();lit!=one_dot_names.end();++lit) {
            set_eosmetadata_namelist(*lit);
            string cur_data;
            obtain_SD_attr_value(*lit,cur_data);
            metadata.append(cur_data);
        }
    }

    if ( 3== list_flag) {
        for (lit = two_dots_names.begin();lit!=two_dots_names.end();++lit){
            set_eosmetadata_namelist(*lit);
            string cur_data;
            obtain_SD_attr_value(*lit,cur_data);
            metadata.append(cur_data);
        }
    }

    if (non_number_names.empty() == false) {
        suffix_is_num_or_null = false;
        no_num_data.resize(non_number_names.size());
    }

    for (unsigned int i =0; i<non_number_names.size();i++) {
        set_eosmetadata_namelist(non_number_names[i]);
        obtain_SD_attr_value(non_number_names[i],no_num_data[i]);
    }

    return suffix_is_num_or_null;
    
}

// This routine will generate three ECS metadata lists. Note in theory list sl1 and sl2 should be sorted.
// Since the ECS metadata is always written(sorted) in increasing numeric order, we don't perform this now.
// Should watch if there are any outliers. KY 2012-08-31
void HE2CF::arrange_list(list<string> & sl1, list<string>&sl2,vector<string>&v1,const string& name,int& flag) const {

    // No dot in the ECS name
    if(name.find(".") == string::npos) {
        sl1.push_front(name);
        sl2.push_front(name);
        flag = 0;
    }
    else if (name.find_first_of(".") == name.find_last_of(".")) {

        size_t dot_pos = name.find_first_of(".");

        if((dot_pos+1)==name.size()) 
            throw BESInternalError("Should have characters or numbers after.",__FILE__, __LINE__);

        string str_after_dot = name.substr(dot_pos+1);
        stringstream sstr(str_after_dot);

        int number_after_dot = 0;
        sstr >> number_after_dot;

        // No dot after ECS metadata
        if (!sstr) 
            v1.push_back(name);
        // .0 after the main name of ECS metadata
        else if(0 == number_after_dot) {
            sl1.push_back(name);
            sl2.push_back(name);
            // For only .0 case, set flag to 1.
            if(flag!=1)
                flag =1; 
        }
        else {// .1 or .2 etc. after the main ECS metadata name 
            sl1.push_back(name);
            if (3 == flag) {
                string msg = "ecs metadata suffix .1 and .0.1 cannot exist at the same file.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            if (flag !=2)
                flag = 2;
        }
    }
    else {// We don't distinguish if .0.1 and .0.0.1 will appear.
        // have two dots in the ECS name.
        sl2.push_back(name);
        if (2 == flag)
            throw BESInternalError("ecs metadata suffix .1 and .0.1 cannot exist at the same file.",__FILE__, __LINE__);
        if (flag !=3)
            flag = 3;
    }

}

// Obtain SD attribute value
void HE2CF::obtain_SD_attr_value(const string& attrname, string &cur_data) const {

    int32 sds_index = SDfindattr(sd_id, attrname.c_str());
    if(sds_index == FAIL){
        Vend(file_id);
        string msg = "Failed to obtain the SDS global attribute " + attrname +".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    // H4_MAX_NC_NAME is from the user guide example. It's 256.
    char temp_name[H4_MAX_NC_NAME];
    int32 type = 0;
    int32 count = 0;

    if(SDattrinfo(sd_id, sds_index, temp_name, &type, &count) == FAIL) {
        Vend(file_id);
        string msg = "Failed to obtain the SDS global attribute " + attrname + " information.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    vector<char> attrvalue;
    attrvalue.resize((count+1)*DFKNTsize(type));

    if(SDreadattr(sd_id, sds_index, attrvalue.data()) == FAIL){
        Vend(file_id);
        string msg = "Failed to read the SDS global attribute" + attrname +".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    // Leave the following #if 0 #endif block.
#if 0
    // Remove the last nullptr character
    //        string temp_data(attrvalue.begin(),attrvalue.end()-1);
    //       cur_data = temp_data;
#endif

    if(attrvalue[count] != '\0') 
        throw BESInternalError("The last character of the attribute buffer should be nullptr.",__FILE__,__LINE__);

    // No need to escape the special characters since they are ECS metadata. Will see. KY 2013-10-14
    cur_data.resize(attrvalue.size()-1);
    copy(attrvalue.begin(),attrvalue.end()-1,cur_data.begin());
}


bool HE2CF::set_vgroup_map(int32 _refid,bool isgeo)
{
    // Clear existing maps first.
    // Note: It should only clear the corresponding groups: Geolocation or Data.
    if(false == isgeo) {
        vg_dsd_map.clear();
        vg_dvd_map.clear();
    }
    else {
        vg_gsd_map.clear();
        vg_gvd_map.clear();
    }
    
    int32 vgroup_id = Vattach(file_id, _refid, "r");
    if (FAIL == vgroup_id) {
        Vend(file_id);
        string msg = "Fail to attach the vgroup. " ;
        throw_error(msg);
        return false;
    }

    int32 npairs = Vntagrefs(vgroup_id);
    if (FAIL == npairs) {
        Vdetach(vgroup_id);
        Vend(file_id);
        string msg = "Fail to obtain the number of objects in a group. " ;
        throw_error(msg);
        return false;
    }
    
    for (int i = 0; i < npairs; ++i) {

        int32 tag2 = 0;
        int32 ref2 = 0;
        char buf[H4_MAX_NC_NAME];
        
        if (Vgettagref(vgroup_id, i, &tag2, &ref2) < 0){
            Vdetach(vgroup_id);
            Vend(file_id);
            string msg = "Vgettagref failed for vgroup_id= " + to_string(vgroup_id)+".";
            throw_error(msg);
            return false;
        }
        
        if(tag2 == DFTAG_NDG){

            int32 sds_index = SDreftoindex(sd_id, ref2); // index 
            if (FAIL == sds_index) {
                Vdetach(vgroup_id);
                Vend(file_id);
                string msg = "Cannot obtain the SDS index.";
                throw_error(msg);
                return false;
            }

            int32 sds_id = SDselect(sd_id, sds_index); // sds_id 
            if (FAIL == sds_id) {
                Vdetach(vgroup_id);
                Vend(file_id);
                string msg = "Cannot obtain the SDS ID. ";
                throw_error(msg);
                return false;
            }
            
            int32 rank;
            int32 dimsizes[H4_MAX_VAR_DIMS];
            int32 datatype;
            int32 num_attrs;
        
            if(FAIL == SDgetinfo(sds_id, buf, &rank, dimsizes, &datatype, &num_attrs)) {
                Vdetach(vgroup_id);
                Vend(file_id);
                string msg = "Cannot obtain the SDS info.";
                throw_error(msg);
                return false;
            }
            if(false == isgeo) 
                vg_dsd_map[string(buf)] = sds_id;
            else
                vg_gsd_map[string(buf)] = sds_id;
            
        }
        
        if(tag2 == DFTAG_VH){

            int vid;
            if ((vid = VSattach(file_id, ref2, "r")) < 0) {

                Vdetach(vgroup_id);
                Vend(file_id);
                string msg = "VSattach failed for file_id= " + to_string(file_id) + ".";
                throw_error(msg);
            }
            if (FAIL == VSgetname(vid, buf)) {

                Vdetach(vgroup_id);
                Vend(file_id);
                string msg = "VSgetname failed for file_id= " +to_string(file_id) + ".";
                throw_error(msg);
            }
            if(false == isgeo) 
                vg_dvd_map[string(buf)] = ref2;
            else
                vg_gvd_map[string(buf)] = ref2;

            if (FAIL == VSdetach(vid)) {

                Vdetach(vgroup_id);
                Vend(file_id);
                string msg = "VSdetach failed for file_id= "  + to_string(file_id) + ".";
                throw_error(msg);

            }
        }// if 
    } // for

    if (FAIL == Vdetach(vgroup_id)){
        Vend(file_id);
        string msg = "VSdetach failed for file_id= "  + to_string(file_id) + ".";
        throw_error(msg);
    }
    
    return true;
}

bool HE2CF::write_attr_long_name(const string& _long_name, 
                                 const string& _varname,
                                 int _fieldtype)
{
    AttrTable *at = das->get_table(_varname);
    if (!at){
        at = das->add_table(_varname, new AttrTable);
    }
    if(_fieldtype > 3){
        at->append_attr("long_name", "String", _long_name + "(fake)");
    }
    else{
        at->append_attr("long_name", "String", _long_name);
    }
    return true;
}
bool HE2CF::write_attr_long_name(const string& _group_name, 
                                 const string& _long_name, 
                                 const string& _varname,
                                 int _fieldtype)
{
    AttrTable *at = das->get_table(_varname);
    if (!at){
        at = das->add_table(_varname, new AttrTable);
    }
    if(_fieldtype > 3){
        at->append_attr("long_name", "String", 
                        _group_name + ":" + _long_name + "(fake)");
    }
    else{
        at->append_attr("long_name", "String", 
                        _group_name + ":" + _long_name);
    }
    return true;
}


bool 
HE2CF::write_attr_sd(int32 _sds_id, const string& _newfname,int _fieldtype)
{
    char buf_var[H4_MAX_NC_NAME];
    char buf_attr[H4_MAX_NC_NAME];        
    int32 rank;
    int32 dimsizes[H4_MAX_VAR_DIMS];
    int32 datatype;
    int32 num_attrs;
    int32 n_values;
    intn status;
    
    status = SDgetinfo(_sds_id, buf_var, 
                       &rank, dimsizes, &datatype, &num_attrs);
    if (FAIL == status) {
        Vend(file_id);
        SDendaccess(_sds_id);
        string msg = "Cannot obtain the SDS info. ";
        throw_error(msg);

    }
    AttrTable *at = das->get_table(_newfname);
    if (!at){
        at = das->add_table(_newfname, new AttrTable);
    }

    //Check if having the "coordinates" attribute when fieldtype is 0
    bool v_has_coordinates = false;
    if(0 == _fieldtype) {
        if(at->simple_find("coordinates")!= at->attr_end())
            v_has_coordinates = true;
    }

    //Check if having the "units" attributes when fieldtype is 1(latitude) or 2(longitude)
    bool llcv_has_units = false;

    // See if we can ignore scale and offset.
    short llcv_ignore_scale  = 0;
    short llcv_ignore_offset = 0;
    if(1 == _fieldtype || 2 == _fieldtype) {
        if(at->simple_find("units")!= at->attr_end())
            llcv_has_units = true;
        try {
            llcv_ignore_scale  =  check_scale_offset(_sds_id,true);
        }
        catch(...) {
            SDendaccess(_sds_id);
            Vend(file_id);
            throw;
        }
        try {
            llcv_ignore_offset =  check_scale_offset(_sds_id,false);
        }
        catch(...) {
            SDendaccess(_sds_id);
            Vend(file_id);
            throw;
        }

        // We need to check if we ignore scale_factor and the scale_factor type is an integer.
        // If that's the case, we need to throw an exception since we cannot retrieve the
        // integer type scale_factor added by the HDF4 API for the latitude and longitude.
        // We don't think such a case exists. If any, we throw an exception and hope the 
        // user to report this case to us.

        // Find a case that the variable type is integer,
        //             that the scale_factor and add_offset attributes exist
        //             that the add_offset is not 0.
        // We don't support this case and highly suspect that such a case doesn't exist.
        if(-1 == llcv_ignore_offset && 2 == llcv_ignore_scale){
            SDendaccess(_sds_id);
            Vend(file_id);
            string msg = "The latitude or longitude has <scale_factor> and <add_offset> attributes, ";
            msg +=" the latitude or longitude have integer type and <add_offset> is not 0, ";
            msg +=" we don't support such a case in the current implementation, ";
            msg +=" please report to eoshelp@hdfgroup.org if you encounter this situation.";
            throw_error(msg);
        }
    }

    for (int j=0; j < num_attrs; j++){

        status = SDattrinfo(_sds_id, j, buf_attr, &datatype, &n_values);
        if (status < 0){
            Vend(file_id);
            SDendaccess(_sds_id);
            string msg = "SDattrinfo failed.";
            throw_error(msg);
        }        

        if(true == v_has_coordinates) {
           if(!strncmp(buf_attr, "coordinates", H4_MAX_NC_NAME))
              continue;
        }

        if(true == llcv_has_units) {
           if(!strncmp(buf_attr, "units", H4_MAX_NC_NAME))
              continue;
        }

        if( 1 == _fieldtype || 2 == _fieldtype) {
            if(!strncmp(buf_attr, "scale_factor", H4_MAX_NC_NAME))
                if((1 == llcv_ignore_scale) || (2==llcv_ignore_scale) )
                    continue;
            if(!strncmp(buf_attr, "add_offset", H4_MAX_NC_NAME))
                if(1 == llcv_ignore_offset)
                    continue;
        }

        vector<char> value;
        value.resize((n_values+1) * DFKNTsize(datatype));
        status = SDreadattr(_sds_id, j, value.data());
        if (status < 0){
            Vend(file_id);
            SDendaccess(_sds_id);
            ostringstream msg;
            msg << "SDreadattr() failed on " << buf_attr << endl;
            throw_error(msg.str());
        }

        // Handle character type attribute as a string.
	if (datatype == DFNT_CHAR || datatype == DFNT_UCHAR) {
            value[n_values] = '\0';
	    n_values = 1;
	}
        
        // Need to check and change if attribute names contain special characters. KY 2012-6-8
        string attr_cf_name = string(buf_attr,strlen(buf_attr));
        attr_cf_name = HDFCFUtil::get_CF_string(attr_cf_name);
	for (int loc=0; loc < n_values ; loc++) {
	    string print_rep = HDFCFUtil::print_attr(datatype, loc, (void *)value.data());

            // Override any existing _FillValue attribute.
            if (attr_cf_name == "_FillValue") {
                at->del_attr(attr_cf_name);
            }
            // Override any existing long_name attribute.
            if (attr_cf_name == "long_name") {
                at->del_attr(attr_cf_name);
            }

            // No need to escape special characters since print_rep already does that.
	    at->append_attr(attr_cf_name, HDFCFUtil::print_type(datatype), print_rep);

	}
    }

    status = SDendaccess(_sds_id); 
    if(status < 0) {
        string msg = "SDendaccess failed on variable  " + _newfname +".";
        throw_error(msg);
    }

    return true;
}

bool HE2CF::write_attr_vdata(int32 _vd_id, const string& _newfname, int _fieldtype)
{
    int32 number_type = 0;
    int32 count = 0;
    int32 size = 0;
    char buf[H4_MAX_NC_NAME];
    
    int vid = 0;
    
    if ((vid = VSattach(file_id, _vd_id, "r")) < 0) {
        Vend(file_id);
        string msg = "VSattach failed.";
        throw_error(msg);
    }
    
    // Don't use VSnattrs - it returns the TOTAL number of attributes 
    // of a vdata and its fields. We should use VSfnattrs.
    count = VSfnattrs(vid, _HDF_VDATA);      
    if (FAIL == count) {
        VSdetach(vid);
        Vend(file_id);
        string msg = "VSfnattrs failed.";
        throw_error(msg);
    }

    AttrTable *at = das->get_table(_newfname);
    if (!at)
        at = das->add_table(_newfname, new AttrTable);

    //Check if having the "coordinates" attribute when fieldtype is 0
    bool v_has_coordinates = false;
    if(0 == _fieldtype) {
        if(at->simple_find("coordinates")!= at->attr_end())
            v_has_coordinates = true;
    }

    //Check if having the "units" attributes when fieldtype is 1 or 2
    bool llcv_has_units = false;
    if(1 == _fieldtype || 2 == _fieldtype) {
        if(at->simple_find("units")!= at->attr_end())
            llcv_has_units = true;
    }


    for(int i=0; i < count; i++){
        int32 count_v = 0;
        if (VSattrinfo(vid, _HDF_VDATA, i, buf,
                       &number_type, &count_v, &size) < 0) {
            VSdetach(vid);
            Vend(file_id);
            string msg = "VSattrinfo failed.";
            throw_error(msg);
        }

        if(true == v_has_coordinates) {
           if(!strncmp(buf, "coordinates", H4_MAX_NC_NAME))
              continue;
        }

        else if(true == llcv_has_units) {
           if(!strncmp(buf, "units", H4_MAX_NC_NAME))
              continue;
        }
        
        vector<char> data;     
        data.resize((count_v+1) * DFKNTsize(number_type));
        if (VSgetattr(vid, _HDF_VDATA, i, data.data()) < 0) {

            // problem: clean up and throw an exception
            VSdetach(vid);
            Vend(file_id);
            string msg = "VSgetattr failed.";
            throw_error(msg);
        }
        // Handle character type attribute as a string.
	if (number_type == DFNT_CHAR || number_type == DFNT_UCHAR8) {
            data[count_v] ='\0';  
	    count_v = 1;
	}
        
        for(int j=0; j < count_v ; j++){

	    string print_rep = HDFCFUtil::print_attr(number_type, j, (void *)data.data());

            // Override any existing _FillValue attribute.
            if(!strncmp(buf, "_FillValue", H4_MAX_NC_NAME)){
                at->del_attr(buf);      
            }

            // Override any existing long_name attribute.
            if(!strncmp(buf, "long_name", H4_MAX_NC_NAME)){
                at->del_attr(buf);      
            }

            string vdataname(buf);
	    at->append_attr(HDFCFUtil::get_CF_string(buf), HDFCFUtil::print_type(number_type), print_rep);

        }
    } // for 
    //  We need to call VFnfields to process fields as well in near future.
    VSdetach(vid);

    return true;
}

void
HE2CF::throw_error(const string& _error)
{
    throw BESInternalError(_error,__FILE__, __LINE__);
}


// Public member functions
HE2CF::HE2CF() = default;

HE2CF::~HE2CF() = default;

bool
HE2CF::close()
{
    
    // Close Vgroup interface.
    int istat = Vend(file_id);
    if(istat == FAIL){
        string msg = "Failed to call Vend in HE2CF::close.";
        throw_error(msg);        
        return false;
    }
    
    return true;
}

string
HE2CF::get_metadata(const string& _name, bool& suffix_is_number,vector<string>&meta_nonnum_names, vector<string>& meta_nonum_data)
{
    suffix_is_number = set_metadata(_name,meta_nonnum_names,meta_nonum_data);
    return metadata;
}

bool
HE2CF::open(const string& _filename,const int hc_sd_id, const int hc_file_id)
{
    if(_filename == ""){
        string msg = "=open(): filename is empty.";
        throw_error(msg);        
        return false;
    }
    
    if(!open_vgroup(_filename,hc_file_id)){
        string msg = "=open(): failed to open vgroup.";
        throw_error(msg);        
        return false;        
    }
    
    if(!open_sd(_filename,hc_sd_id)){
        string msg = "=open(): failed to open sd.";
        throw_error(msg);        
        return false;
    }
    
    return true;
}




bool
HE2CF::write_attribute(const string& _gname, 
                       const string&  _fname,
                       const string& _newfname,
                       int _n_groups,
                       int _fieldtype)
{

    if(_n_groups > 1){
        write_attr_long_name(_gname, _fname, _newfname, _fieldtype);
    }
    else{
        write_attr_long_name(_fname, _newfname, _fieldtype);
    }
    int32 ref_df = -1; // ref id for /Data Fields/
    int32 ref_gf = -1; // ref id for /Geolocaion Fields/
    
    // This if block effectively constructs the vg_dsd_map and vg_dvd_map at most once
    // since starting from the second time, gname will be equal to _gname.
    if(gname != _gname){
        // Construct field:(SD|Vdata)ref table for faster lookup.
        gname = _gname;        
        get_vgroup_field_refids(_gname, &ref_df, &ref_gf);
        if (ref_gf !=-1) 
            set_vgroup_map(ref_gf,true); 
 
        if (ref_df != 1) 
            set_vgroup_map(ref_df,false); 
    }
    
    // Use a cached table.
    int32 id = -1;

    // Using SDS name to SDS id table is fine for the HDF-EOS2 case since under one grid/swath,
    // the data field name is unique. Generally SDS name cannot be used as a key since one may have
    // the same SDS name for two different SDS objects. But in this context(HDF-EOS2 objects), it is okay.
    // KY 2012-7-3
    // Retrive sds_id for SDS
    // Go over all possible maps (data fields and geolocation fields)
    id =  vg_dsd_map[_fname];
    if(id > 0){
        write_attr_sd(id, _newfname,_fieldtype);
    }
    // Retrieve ref_id for Vdata
    // The Vdata we retrieve here are HDF-EOS2 objects(Swath 1-D data)
    // Not all Vdata from the HDF file.
    id = vg_dvd_map[_fname];
    if(id > 0){
        write_attr_vdata(id, _newfname,_fieldtype);
    }

    id =  vg_gsd_map[_fname];
    if(id > 0){
        write_attr_sd(id, _newfname,_fieldtype);
    }
    // Retrieve ref_id for Vdata
    // The Vdata we retrieve here are HDF-EOS2 objects(Swath 1-D data)
    // Not all Vdata from the HDF file.
    id = vg_gvd_map[_fname];
    if(id > 0){
        write_attr_vdata(id, _newfname,_fieldtype);
    }   

    return true;
}

// The application will make sure the fill value falls in the valid range of the datatype values.
bool
HE2CF::write_attribute_FillValue(const string& _varname, 
                                 int _type,
                                 float value)
{
    void* v_ptr = nullptr;
    // Casting between pointers of different types may generate unexpected behavior.
    // gcc 4.8.2 reveals this problem in one test(SwathFile.HDF).
    // So what we should do is to use char* or vector<char> in this case and memcpy the value to the vector<char>. 
    // Casting the vector pointer to void *. The compiler can correctly interpret the data based on the data type.
    vector<char>v_val;

    // Cast the value depending on the type.
    switch (_type) {
        
        case DFNT_UINT8:
        {
            auto val = (uint8) value;
            v_val.resize(1);
            memcpy(v_val.data(),&val,1);
            v_ptr = (void*)v_val.data();
        }

        break;
        case DFNT_INT8:        
        {
            auto val = (int8) value;
            v_val.resize(1);
            memcpy(v_val.data(),&val,1);
            v_ptr = (void*)v_val.data();
        }
        break;
        case DFNT_INT16:
        {
            auto val = (int16) value;
            v_val.resize(sizeof(short));
            memcpy(v_val.data(),&val,sizeof(short));
            v_ptr = (void*)v_val.data();
        }
        break;

        case DFNT_UINT16:        
        {
            auto val = (uint16) value;
            v_val.resize(sizeof(unsigned short));
            memcpy(v_val.data(),&val,sizeof(unsigned short));
            v_ptr = (void*)v_val.data();
        }
        break;

        case DFNT_INT32:
        {
            auto val = (int32) value;
            v_val.resize(sizeof(int));
            memcpy(v_val.data(),&val,sizeof(int));
            v_ptr = (void*)v_val.data();
        }
        break;
        case DFNT_UINT32:        
        {
            auto val = (uint32) value;
            v_val.resize(sizeof(unsigned int));
            memcpy(v_val.data(),&val,sizeof(int));
            v_ptr = (void*)v_val.data();
        }
        break;
        case DFNT_FLOAT:
        {
            v_ptr = (void*)&value;
        }
        break;
        case DFNT_DOUBLE:
        {
            auto val = (float64) value;
            v_val.resize(sizeof(double));
            memcpy(v_val.data(),&val,sizeof(double));
            v_ptr = (void*)v_val.data();
        }
        break;
        default:
            throw_error("Invalid FillValue Type - ");
        break;
    }

    AttrTable *at = das->get_table(_varname);
    if (!at){
        at = das->add_table(_varname, new AttrTable);
    }
    string print_rep = HDFCFUtil::print_attr(_type, 0, v_ptr);
    at->append_attr("_FillValue", HDFCFUtil::print_type(_type), print_rep);

    return true;
}

bool
HE2CF::write_attribute_coordinates(const string& _varname, const string &_coordinates)
{

    AttrTable *at = das->get_table(_varname);
    if (!at){
        at = das->add_table(_varname, new AttrTable);
    }
    at->append_attr("coordinates", "String", _coordinates);

    return true;
}

bool
HE2CF::write_attribute_units(const string& _varname, const string &_units)
{

    AttrTable *at = das->get_table(_varname);
    if (!at){
        at = das->add_table(_varname, new AttrTable);
    }
    at->del_attr("units");      // Override any existing units attribute.
    at->append_attr("units", "String", _units);

    return true;
}


// Check if the scale_factor or the add_offset can be ignored.
// _sds_id is the SDS ID of the variable. 
// is_scale is true, the attribute is scale_factor.
// is_scale is false, the attribute is add_offset.
// When the attribute is scale_factor, 
// the meaning of the returned value: 
//   0: the scale_factor won't be ignored if exists.
//   1 or 2: the scale_factor can be ignored. 2 is the case when the variable type is integer. 1 is the case when the variable type is float.            
// When the attribute is add_offset,
// the meaning of the returned value:
//   0: the add_offset won't be ignored if exists. It doesn't check if add_offset attribute exists.
//   -1: the add_offset won't be ignored and the add_offset definitely exists,the value is not 0.
//   1: the add_offset will be ignored.
short
HE2CF::check_scale_offset(int32 _sds_id, bool is_scale) {

    char buf_var[H4_MAX_NC_NAME];
    char buf_attr[H4_MAX_NC_NAME];        
    int32 rank;
    int32 dimsizes[H4_MAX_VAR_DIMS];
    int32 datatype;
    int32 num_attrs;
    int32 n_values;
    intn status;
    
    short ignore_so = 0;
    status = SDgetinfo(_sds_id, buf_var, 
                       &rank, dimsizes, &datatype, &num_attrs);
    if (FAIL == status) {
        SDendaccess(_sds_id);
        string msg = "Cannot obtain the SDS info. ";
        throw_error(msg);
    }
 
    bool  has_so    = false;
    int   so_index  = -1;
    int32 so_dtype  = -1;
    int32 var_dtype = datatype;
    string so_name  =((true==is_scale)?"scale_factor":"add_offset");

    for (int j=0; j < num_attrs; j++){

        status = SDattrinfo(_sds_id, j, buf_attr, &datatype, &n_values);
        if (status < 0){
            SDendaccess(_sds_id);
            string msg =  "SDattrinfo() failed. ";
            throw_error(msg);
        }        
     
        if(!strncmp(buf_attr, so_name.c_str(), H4_MAX_NC_NAME)) {
            if(1 == n_values) {
                has_so = true;
                so_index = j;
                so_dtype = datatype;
                break;
            }
        }
    }

    // We find the attribute, now check if we can ignore this attribute.
    if(true == has_so) {
        vector<char> value;
        // We have already checked the number of attribute values. The number is 1. 
        value.resize(DFKNTsize(so_dtype));
        status = SDreadattr(_sds_id, so_index, value.data());
        if (status < 0){
            SDendaccess(_sds_id);
            string msg = "SDreadattr() failed on the attribute scale_factor.";
            throw_error(msg);
        }
        // If this attribute is "scale_factor",
        if(true == is_scale) {
            if(DFNT_FLOAT32 == so_dtype) {
                float final_scale_value = *((float*)((void*)(value.data())));
                if(final_scale_value == 1.0) 
                    ignore_so = 1;
                // This is the ugly case that the variable type is integer, the
                // scale_factor is handled by the handler reading function.
                // However, the add_offset for this case cannot be 0.
                // So we need to check this case. We may also need to
                // check the scale_factor value.
                else if(var_dtype !=DFNT_FLOAT32 && var_dtype != DFNT_FLOAT64)
                    ignore_so = 2;
            }
            else if(DFNT_FLOAT64 == so_dtype) {
                double final_scale_value = *((double*)((void*)(value.data())));
                if(final_scale_value == 1.0) 
                    ignore_so = 1;
                else if(var_dtype !=DFNT_FLOAT32 && var_dtype != DFNT_FLOAT64)
                    ignore_so = 2;
            }   
        }
        else {
            // has offset, we need to make a mark for the case when scale_factor applies to a varialbe that has an integer type  
            ignore_so = -1;
            string print_rep = HDFCFUtil::print_attr(so_dtype, 0, (void *)value.data());
            if(DFNT_FLOAT32 == so_dtype || DFNT_FLOAT64 == so_dtype) {
                if(atof(print_rep.c_str()) == 0.0) 
                    ignore_so = 1;
            }
            else { 
                if(atoi(print_rep.c_str()) == 0)
                    ignore_so = 1;
            }
        }
    }
    return ignore_so;
}

