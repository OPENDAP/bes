//////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// Copyright (c) 2010-2012 The HDF Group
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
#include "escaping.h"
#include "InternalErr.h"
#include "HDFCFUtil.h"
#include <iomanip>

// Private member functions
bool 
HE2CF::get_vgroup_field_refids(const string& _gname,
                               int32* _ref_df,
                               int32* _ref_gf)
{

    int32 vrefid = Vfind(file_id, (char*)_gname.c_str());
    if (FAIL == vrefid) {
        Vend(file_id);
        ostringstream error;
        error <<"cannot obtain the reference number for vgroup "<<_gname;
        throw_error(error.str());
        return false;
    }

    int32 vgroup_id = Vattach(file_id, vrefid, "r");
    if (FAIL == vgroup_id) {
        Vend(file_id);
        ostringstream error;
        error <<"cannot obtain the group id for vgroup "<<_gname;
        throw_error(error.str());
        return false;
    }

    int32 npairs = Vntagrefs(vgroup_id);
    int32 ref_df = -1;
    int32 ref_gf = -1;;

    if(npairs < 0){
        Vdetach(vgroup_id);
        Vend(file_id);
        ostringstream error;
        error << "Got " << npairs
              << " npairs for " << _gname;
        throw_error(error.str());
        return false;
    }

    for (int i = 0; i < npairs; ++i) {
        
        int32 tag, ref;
        
        if (Vgettagref(vgroup_id, i, &tag, &ref) < 0){

            // Previously just output stderr, it doesn't throw an internal error.
            // I believe this is wrong. Use DAP's internal error to throw an error.KY 2011-4-26 
            Vdetach(vgroup_id);
            Vend(file_id);
            ostringstream error;
            error  << "failed to get tag / ref";
            throw_error(error.str());
            return false;            
        }
        
        if(Visvg(vgroup_id, ref)){
            char  cvgroup_name[VGNAMELENMAX*4]; // the child vgroup name
            int32 istat;
            int32 vgroup_cid; // Vgroup id 

            vgroup_cid = Vattach(file_id, ref,"r");

            if (FAIL == vgroup_cid) {

                Vdetach(vgroup_id);
                Vend(file_id);
                ostringstream error;
                error  << "cannot obtain the vgroup id";
                throw_error(error.str());
                return false;

            }
            
            istat = Vgetname(vgroup_cid,cvgroup_name);

            if (FAIL == istat) {
                Vdetach(vgroup_cid);
                Vdetach(vgroup_id);
                Vend(file_id);
                ostringstream error;
                error  << "cannot obtain the vgroup id";
                throw_error(error.str());
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
                ostringstream error;
                error  << "cannot close the vgroup "<< cvgroup_name <<"Successfully";
                throw_error(error.str());
                return false;
            }

        }
    }
    *_ref_df = ref_df;
    *_ref_gf = ref_gf;

    if (FAIL == Vdetach(vgroup_id)) {
        Vend(file_id);
        ostringstream error;
        error  << "cannot close the vgroup "<< _gname <<"Successfully";
        throw_error(error.str());
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
        
        ostringstream error;
        error << "Failed to call SDfileinfo() on "
              << _filename
              <<  " file.";
        throw_error(error.str());
        return false;
    }
    return true;
}

bool 
HE2CF::open_vgroup(const string& _filename,const int file_id_in)
{

    file_id = file_id_in;
    if (Vstart(file_id) < 0){
        ostringstream error;        
        error << "Failed to call Vstart on " << _filename << endl;
        throw_error(error.str());        
        return false;
    }
    return true;
}

#if 0
// Borrowed codes from ncdas.cc in netcdf_handle4 module.
string
HE2CF::print_attr(int32 type, int loc, void *vals)
{
    ostringstream rep;
    
    union {
        char *cp;
        short *sp;
        unsigned short *usp;
        int32 /*nclong*/ *lp;
        unsigned int *ui;
        float *fp;
        double *dp;
    } gp;

    switch (type) {
        
    case DFNT_UINT8:
    case DFNT_INT8:        
        {
            unsigned char uc;
            gp.cp = (char *) vals;

            uc = *(gp.cp+loc);
            rep << (int)uc;
            return rep.str();
        }

    case DFNT_CHAR:
        {
            // Using the customized escattr. Don't escape \n,\r and \t. KY 2013-10-14
            return HDFCFUtil::escattr(static_cast<const char*>(vals));
        }

    case DFNT_INT16:
        {
            gp.sp = (short *) vals;
            rep << *(gp.sp+loc);
            return rep.str();
        }

    case DFNT_UINT16:        
        {
            gp.usp = (unsigned short *) vals;
            rep << *(gp.usp+loc);
            return rep.str();
        }

    case DFNT_INT32:
        {
            gp.lp = (int32 *) vals; 
            rep << *(gp.lp+loc);
            return rep.str();
        }

    case DFNT_UINT32:        
        {
            gp.ui = (unsigned int *) vals; 
            rep << *(gp.ui+loc);
            return rep.str();
        }

    case DFNT_FLOAT:
        {
            gp.fp = (float *) vals;
            rep << showpoint;
            rep << setprecision(10);
            rep << *(gp.fp+loc);
            if (rep.str().find('.') == string::npos
                && rep.str().find('e') == string::npos)
                rep << ".";
            return rep.str();
        }
        
    case DFNT_DOUBLE:
        {
            gp.dp = (double *) vals;
            rep << std::showpoint;
            rep << std::setprecision(17);
            rep << *(gp.dp+loc);
            if (rep.str().find('.') == string::npos
                && rep.str().find('e') == string::npos)
                rep << ".";
            return rep.str();
            break;
        }
    default:
        return string("UNKNOWN");
    }
    
}

string
HE2CF::print_type(int32 type)
{

    // I expanded the list based on libdap/AttrTable.h.
    static const char UNKNOWN[]="Unknown";    
    static const char BYTE[]="Byte";
    static const char INT16[]="Int16";
    static const char UINT16[]="UInt16";        
    static const char INT32[]="Int32";
    static const char UINT32[]="UInt32";
    static const char FLOAT32[]="Float32";
    static const char FLOAT64[]="Float64";    
    static const char STRING[]="String";
    
    // I got different cases from hntdefs.h.
    switch (type) {
        
    case DFNT_CHAR:
	return STRING;
        
    case DFNT_UCHAR8:
	return STRING;        
        
    case DFNT_UINT8:
	return BYTE;
        
    case DFNT_INT8:
	return INT32;
        
    case DFNT_UINT16:
	return UINT16;
        
    case DFNT_INT16:
	return INT16;

    case DFNT_INT32:
	return INT32;

    case DFNT_UINT32:
	return UINT32;
        
    case DFNT_FLOAT:
	return FLOAT32;

    case DFNT_DOUBLE:
	return FLOAT64;
        
    default:
	return UNKNOWN;
    }
    
}
#endif
    

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
            ostringstream error;
            error <<  "Fail to obtain SDS global attribute info."  << endl;
            throw_error(error.str());
        }
       
        string attr_namestr(temp_name);
//cerr<<"namestr "<<attr_namestr <<endl;

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
        //char* attr_data  = new char [(attr_count+1) *DFKNTsize(attr_type)];
        vector<char>attr_data;
        attr_data.resize((attr_count+1) *DFKNTsize(attr_type));
#if 0
        if(attr_data == NULL){
            Vend(file_id);
            ostringstream error;
            error <<  "Fail to calloc memory"  << endl;
            throw_error(error.str());
        }
#endif

        //if(SDreadattr(sd_id, i, attr_data) == FAIL){
        if(SDreadattr(sd_id, i, &attr_data[0]) == FAIL){
            Vend(file_id);
            //delete[] attr_data;
            ostringstream error;
            error <<  "Fail to read SDS global attributes"  << endl;
            throw_error(error.str());

        }
#if 0
if (1==attr_count && DFNT_INT32==attr_type) {
cerr<<"attr_name is "<< attr_namestr<<endl;
cerr<<"attr value is "<<*(int*)&attr_data[0] <<endl;
}
#endif


        // Handle character type attribute as a string.
	if (attr_type == DFNT_CHAR || attr_type == DFNT_UCHAR) {
	    //*(value + n_values) = '\0';
        //    value[n_values] = '\0';
	    //n_values = 1;
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
                string print_rep = HDFCFUtil::print_attr(attr_type, loc, (void*)&attr_data[0] );
                at->append_attr(attr_namestr, HDFCFUtil::print_type(attr_type), print_rep);
        }

        


        //delete[] attr_data;                    
        //attr_data = NULL;

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
            ostringstream error;
            error <<  "Fail to obtain SDS global attribute info."  << endl;
            throw_error(error.str());
        }

        string temp_name_str(temp_name);

        // Find the basename, arrange the metadata name list.
        if(temp_name_str.find(metadata_basename)==0) {
//cerr<<"ECS metadata name "<<temp_name_str <<endl;
            arrange_list(one_dot_names,two_dots_names,non_number_names,temp_name_str,list_flag);
        }
    }

    list<string>::const_iterator lit;
 
    // list_flag = 0, no suffix
    // list_flag = 1, only .0, coremetadata.0
    // list_flag = 2, coremetadata.0, coremetadata.1 etc
    // list_flag = 3, coremeatadata.0, coremetadata.0.1 etc
//cerr<<"list_flag "<<list_flag <<endl;
    if ( list_flag >= 0 && list_flag <=2) {
        for (lit = one_dot_names.begin();lit!=one_dot_names.end();++lit) {
            set_eosmetadata_namelist(*lit);
            string cur_data;
            obtain_SD_attr_value(*lit,cur_data);
//cerr<<"metadata "<<cur_data <<endl;
            metadata.append(cur_data);
        }
    }

    if( 3== list_flag) {
        for (lit = two_dots_names.begin();lit!=two_dots_names.end();++lit){
            set_eosmetadata_namelist(*lit);
            string cur_data;
            obtain_SD_attr_value(*lit,cur_data);
            metadata.append(cur_data);
        }
    }

    if(non_number_names.size() >0) {
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
void HE2CF::arrange_list(list<string> & sl1, list<string>&sl2,vector<string>&v1,string name,int& flag) {

  
    if(name.find(".") == string::npos) {
        sl1.push_front(name);
        sl2.push_front(name);
        flag = 0;
    }
    else if (name.find_first_of(".") == name.find_last_of(".")) {

        size_t dot_pos = name.find_first_of(".");

        if((dot_pos+1)==name.size()) 
            throw InternalErr(__FILE__, __LINE__,"Should have characters or numbers after ." );

        string str_after_dot = name.substr(dot_pos+1);
        stringstream sstr(str_after_dot);
        int number_after_dot;
        sstr >> number_after_dot;
        if (!sstr) 
            v1.push_back(name);
        else if(0 == number_after_dot) {
            sl1.push_back(name);
            sl2.push_back(name);
            // For only .0 case, set flag to 1.
            if(flag!=1)
                flag =1; 
        }
        else { 
            sl1.push_back(name);
            if (3 == flag)
                throw InternalErr(__FILE__, __LINE__,
                      "ecs metadata suffix .1 and .0.1 cannot exist at the same file" );
            if (flag !=2)
                flag = 2;
        }
    }

    else {// We don't distinguish if .0.1 and .0.0.1 will appear.
        sl2.push_back(name);
        if (2 == flag)
            throw InternalErr(__FILE__, __LINE__,"ecs metadata suffix .1 and .0.1 cannot exist at the same file" );
        if (flag !=3)
            flag = 3;
    }

}

// Obtain SD attribute value
void HE2CF::obtain_SD_attr_value(const string& attrname, string &cur_data) {

    int32 sds_index = SDfindattr(sd_id, attrname.c_str());
    if(sds_index == FAIL){
        Vend(file_id);
        ostringstream error;
        error <<  "Failed to obtain the SDS global attribute"  << attrname << endl;
        throw InternalErr(__FILE__, __LINE__,error.str());
    }

    // H4_MAX_NC_NAME is from the user guide example. It's 256.
    char temp_name[H4_MAX_NC_NAME];
    int32 type = 0;
    int32 count = 0;

    if(SDattrinfo(sd_id, sds_index, temp_name, &type, &count) == FAIL) {
        Vend(file_id);
        ostringstream error;
        error <<  "Failed to obtain the SDS global attribute"  << attrname << "information" << endl;
        throw InternalErr(__FILE__, __LINE__,error.str());
    }

    vector<char> attrvalue;
    attrvalue.resize((count+1)*DFKNTsize(type));

    if(SDreadattr(sd_id, sds_index, &attrvalue[0]) == FAIL){
        Vend(file_id);
        ostringstream error;
        error <<  "Failed to read the SDS global attribute"  << attrname << endl;
        throw InternalErr(__FILE__, __LINE__,error.str());

    }
    // Remove the last NULL character
    //        string temp_data(attrvalue.begin(),attrvalue.end()-1);
    //       cur_data = temp_data;

    if(attrvalue[count] != '\0') 
        throw InternalErr(__FILE__,__LINE__,"the last character of the attribute buffer should be NULL");

    // No need to escape the special characters since they are ECS metadata. Will see. KY 2013-10-14
    cur_data.resize(attrvalue.size()-1);
    copy(attrvalue.begin(),attrvalue.end()-1,cur_data.begin());
}


bool HE2CF::set_vgroup_map(int32 _refid)
{
    // Clear existing maps first.
    vg_sd_map.clear();
    vg_vd_map.clear();
    
//cerr<<"before Vattach ref id "<<_refid <<endl;
    int32 vgroup_id = Vattach(file_id, _refid, "r");
    if (FAIL == vgroup_id) {
        Vend(file_id);
        ostringstream error;
        error <<  "Fail to attach the vgroup " ;
        throw_error(error.str());
        return false;
    }

    int32 npairs = Vntagrefs(vgroup_id);

    if (FAIL == npairs) {
        Vdetach(vgroup_id);
        Vend(file_id);
        ostringstream error;
        error <<  "Fail to obtain the number of objects in a group " ;
        throw_error(error.str());
        return false;
    }
    
    for (int i = 0; i < npairs; ++i) {

        int32 tag2, ref2;
        char buf[H4_MAX_NC_NAME];
        
        if (Vgettagref(vgroup_id, i, &tag2, &ref2) < 0){
            Vdetach(vgroup_id);
            Vend(file_id);
            ostringstream error;
            error <<  "Vgettagref failed for vgroup_id=."  << vgroup_id;
            throw_error(error.str());
            return false;
        }
        
        if(tag2 == DFTAG_NDG){

            int32 sds_index = SDreftoindex(sd_id, ref2); // index 
            if (FAIL == sds_index) {
                Vdetach(vgroup_id);
                Vend(file_id);
                ostringstream error;
                error <<  "Cannot obtain the SDS index ";
                throw_error(error.str());
                return false;
            }

            int32 sds_id = SDselect(sd_id, sds_index); // sds_id 
            if (FAIL == sds_id) {

                Vdetach(vgroup_id);
                Vend(file_id);
                ostringstream error;
                error <<  "Cannot obtain the SDS ID ";
                throw_error(error.str());
                return false;

            }
            
            int32 rank;
            int32 dimsizes[H4_MAX_VAR_DIMS];
            int32 datatype;
            int32 num_attrs;
        
            if(FAIL == SDgetinfo(sds_id, buf, &rank, dimsizes, &datatype, &num_attrs)) {

                Vdetach(vgroup_id);
                Vend(file_id);
                ostringstream error;
                error <<  "Cannot obtain the SDS info.";
                throw_error(error.str());
                return false;

            }
            vg_sd_map[string(buf)] = sds_id;
            
        }
        
        if(tag2 == DFTAG_VH){

            int vid;
            if ((vid = VSattach(file_id, ref2, "r")) < 0) {

                Vdetach(vgroup_id);
                Vend(file_id);
                ostringstream error;
                error <<  "VSattach failed for file_id=."  << file_id;
                throw_error(error.str());
            }
            if (FAIL == VSgetname(vid, buf)) {

                Vdetach(vgroup_id);
                Vend(file_id);
                ostringstream error;
                error <<  "VSgetname failed for file_id=."  << file_id;
                throw_error(error.str());
            }
            vg_vd_map[string(buf)] = ref2;
            if (FAIL == VSdetach(vid)) {

                Vdetach(vgroup_id);
                Vend(file_id);
                ostringstream error;
                error <<  "VSdetach failed for file_id=."  << file_id;
                throw_error(error.str());

            }
        }
    } // for

    if (FAIL == Vdetach(vgroup_id)){
        Vend(file_id);
        ostringstream error;
        error <<  "VSdetach failed for file_id=."  << file_id;
        throw_error(error.str());
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
HE2CF::write_attr_sd(int32 _sds_id, const string& _newfname)
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
        ostringstream error;
        error <<  "Cannot obtain the SDS info. ";
        throw_error(error.str());

    }
//cerr<<"datatype "<<datatype <<endl;
//cerr<<"attribute name " << _newfname <<endl;
    
    for (int j=0; j < num_attrs; j++){

        status = SDattrinfo(_sds_id, j, buf_attr, &datatype, &n_values);

        if (status < 0){

            Vend(file_id);
            SDendaccess(_sds_id);

            ostringstream error;
            error << "SDattrinfo() failed on " << buf_attr;
            throw_error(error.str());
            
        }        

        // AttrTable *at = das->get_table(buf_var);
        AttrTable *at = das->get_table(_newfname);

        if (!at){
            // at = das->add_table(buf_var, new AttrTable);
            at = das->add_table(_newfname, new AttrTable);
        }

        // USE VECTOR
        //char* value = new char [(n_values+1) * DFKNTsize(datatype)];
        vector<char> value;
        value.resize((n_values+1) * DFKNTsize(datatype));
        //status = SDreadattr(_sds_id, j, value);
        status = SDreadattr(_sds_id, j, &value[0]);

        if (status < 0){

            Vend(file_id);
            SDendaccess(_sds_id);
            ostringstream error;
            error << "SDreadattr() failed on " << buf_attr << endl;
            throw_error(error.str());
        }

        // Handle character type attribute as a string.
	if (datatype == DFNT_CHAR || datatype == DFNT_UCHAR) {
	    //*(value + n_values) = '\0';
            value[n_values] = '\0';
	    n_values = 1;
	}
        
        // Need to check and change if attribute names contain special characters. KY 2012-6-8

        string attr_cf_name = string(buf_attr,strlen(buf_attr));
        attr_cf_name = HDFCFUtil::get_CF_string(attr_cf_name);
	for (int loc=0; loc < n_values ; loc++) {
	    //string print_rep = HDFCFUtil::print_attr(datatype, loc, (void *)value);
	    string print_rep = HDFCFUtil::print_attr(datatype, loc, (void *)&value[0]);

            // Override any existing _FillValue attribute.
            // if(!strncmp(buf_attr, "_FillValue", H4_MAX_NC_NAME)){
               //at->del_attr(buf_attr);      
            if (attr_cf_name == "_FillValue") {
                at->del_attr(attr_cf_name);
            }
            // Override any existing long_name attribute.
            // if(!strncmp(buf_attr, "long_name", H4_MAX_NC_NAME)){
            // at->del_attr(buf_attr);      

            if (attr_cf_name == "long_name") {
                at->del_attr(attr_cf_name);
            }
	    //at->append_attr(buf_attr, print_type(datatype), print_rep);
            // No need to escape special characters since print_rep already does that.
	    at->append_attr(attr_cf_name, HDFCFUtil::print_type(datatype), print_rep);

	}
        status = SDendaccess(_sds_id); 
        //delete[] value;
    }
    return true;
}

bool HE2CF::write_attr_vdata(int32 _vd_id, const string& _newfname)
{
    int32 number_type, count, size;
    char buf[H4_MAX_NC_NAME];
    
    int vid;
    
    if ((vid = VSattach(file_id, _vd_id, "r")) < 0) {

        Vend(file_id);
       
        ostringstream error;
        error <<  "VSattach failed.";
        throw_error(error.str());
    }
    
    // Don't use VSnattrs - it returns the TOTAL number of attributes 
    // of a vdata and its fields. We should use VSfnattrs.
    count = VSfnattrs(vid, _HDF_VDATA);      

    if (FAIL == count) {

        VSdetach(vid);
        Vend(file_id);
        ostringstream error;
        error <<  "VSfnattrs failed.";
        throw_error(error.str());
    }

    AttrTable *at = das->get_table(_newfname);
    if (!at)
        at = das->add_table(_newfname, new AttrTable);


    for(int i=0; i < count; i++){
        int32 count_v = 0;
        if (VSattrinfo(vid, _HDF_VDATA, i, buf,
                       &number_type, &count_v, &size) < 0) {

            VSdetach(vid);
            Vend(file_id);
            ostringstream error;
            error << "VSattrinfo failed.";
            throw_error(error.str());
        }

     //   char *data = NULL;
        vector<char> data;     

      //  try {

        //    data = new char [(count_v+1) * DFKNTsize(number_type)];
            data.resize((count_v+1) * DFKNTsize(number_type));
            if (VSgetattr(vid, _HDF_VDATA, i, &data[0]) < 0) {

                // problem: clean up and throw an exception
                //delete []data;
                VSdetach(vid);
                Vend(file_id);
                ostringstream error;            
                error << "VSgetattr failed.";
                throw_error(error.str());
            }
            // Handle character type attribute as a string.
	    if (number_type == DFNT_CHAR || number_type == DFNT_UCHAR8) {
	        //*(data + count_v) = '\0';
                data[count_v] ='\0';  
	        count_v = 1;
	    }
        
        
            for(int j=0; j < count_v ; j++){

	        //string print_rep = HDFCFUtil::print_attr(number_type, j, (void *)data);
	        string print_rep = HDFCFUtil::print_attr(number_type, j, (void *)&data[0]);

                // Override any existing _FillValue attribute.
                if(!strncmp(buf, "_FillValue", H4_MAX_NC_NAME)){
                    at->del_attr(buf);      
                }
                // Override any existing long_name attribute.
                if(!strncmp(buf, "long_name", H4_MAX_NC_NAME)){
                    at->del_attr(buf);      
                }

                string vdataname(buf);
//cerr<<"vdata name "<<vdataname <<endl;
//cerr<<"vdata value "<<print_rep << endl;
	        at->append_attr(HDFCFUtil::get_CF_string(buf), HDFCFUtil::print_type(number_type), print_rep);

            }

           // if(data != NULL)
            //    delete[] data;
        //}

#if 0
        catch (...) {
            if (data != NULL) 
                delete[] data;
            throw_error("Cannot allocate enough memory for the data buffer");
        }
#endif
        
    } // for 
    //  We need to call VFnfields to process fields as well in near future.
    VSdetach(vid);

    return true;
}

void
HE2CF::throw_error(string _error)
{
    throw InternalErr(__FILE__, __LINE__,
                      _error);        
}


// Public member functions
HE2CF::HE2CF()
{
    num_global_attributes = -1;
    file_id = -1;
    sd_id = -1;
    metadata = "";
    gname = "";
    das = NULL;
}

HE2CF::~HE2CF()
{
    // Actually this is not necessary since C++ will clean up the string.
    metadata.clear();
}

bool
HE2CF::close()
{
    
    // Close Vgroup interface.
    int istat = Vend(file_id);
    if(istat == FAIL){
        ostringstream error;                
        error << "Failed to call Vend in HE2CF::close.";
        throw_error(error.str());        
        return false;
    }
    
    return true;
}

string
HE2CF::get_metadata(const string& _name, bool& suffix_is_number,vector<string>&meta_nonnum_names, vector<string>& meta_nonum_data)
{
    suffix_is_number = set_metadata(_name,meta_nonnum_names,meta_nonum_data);
//meta_nonnum_names.resize(1);
//meta_nonnum_names[0] ="test metadata";
    return metadata;
}

bool
HE2CF::open(const string& _filename,const int sd_id, const int file_id)
{
    if(_filename == ""){
        ostringstream error;                
        error << "=open(): filename is empty.";
        throw_error(error.str());        
        return false;
    }
    
    if(!open_vgroup(_filename,file_id)){
        ostringstream error;                
        error << "=open(): failed to open vgroup.";
        throw_error(error.str());        
        return false;        
    }
    
    if(!open_sd(_filename,sd_id)){
        ostringstream error;                
        error << "=open(): failed to open sd.";
        throw_error(error.str());        
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
    
    // This if block effectively constructs the vg_sd_map and vg_vd_map at most once
    // since starting from the second time, gname will be equal to _gname. KY 2012-7-3
    if(gname != _gname){
        // Construct field:(SD|Vdata)ref table for faster lookup.
        gname = _gname;        
        get_vgroup_field_refids(_gname, &ref_df, &ref_gf);
        
        if (ref_gf !=-1) 
            set_vgroup_map(ref_gf); 
        if (ref_df != 1) 
            set_vgroup_map(ref_df); 
    }
    
    // Use a cached table.
    int32 id = -1;

    // Using SDS name to SDS id table is fine for the HDF-EOS2 case since under one grid/swath,
    // the data field name is unique. Generally SDS name cannot be used as a key since one may have
    // the same SDS name for two different SDS objects. But in this context(HDF-EOS2 objects), it is okay.
    // KY 2012-7-3
    // Retrive sds_id for SDS
    id =  vg_sd_map[_fname];
    if(id > 0){
        write_attr_sd(id, _newfname);
    }
    // Retrieve ref_id for Vdata
    // The Vdata we retrieve here are HDF-EOS2 objects(Swath 1-D data)
    // Not all Vdata from the HDF file.
    id = vg_vd_map[_fname];
    if(id > 0){
        write_attr_vdata(id, _newfname);
    }
    return true;
}

// The application will make sure the fill value falls in the valid range of the datatype values.
bool
HE2CF::write_attribute_FillValue(const string& _varname, 
                                 int _type,
                                 float value)
{
    void* v_ptr = NULL;

    // Cast the value depending on the type.
    switch (_type) {
        
        case DFNT_UINT8:
        {
            uint8 val = (uint8) value;
            v_ptr = (void*)&val;
        }

        break;
        case DFNT_INT8:        
        {
            int8 val = (int8) value;
            v_ptr = (void*)&val;
            
        }
        break;
        case DFNT_INT16:
        {
            int16 val = (int16) value;
            v_ptr = (void*)&val;
        }
        break;

        case DFNT_UINT16:        
        {
            uint16 val = (uint16) value;
            v_ptr = (void*)&val;
        }
        break;

        case DFNT_INT32:
         {
            int32 val = (int32) value;
            v_ptr = (void*)&val;

        }
        break;
        case DFNT_UINT32:        
        {
            uint32 val = (uint32) value;
            v_ptr = (void*)&val;

        }
        break;
        case DFNT_FLOAT:
        {
            v_ptr = (void*)&value;
        }
        break;
        case DFNT_DOUBLE:
        {
            float64 val = (float64) value;
            v_ptr = (void*)&val;

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
HE2CF::write_attribute_coordinates(const string& _varname, string _coordinates)
{

    AttrTable *at = das->get_table(_varname);
    if (!at){
        at = das->add_table(_varname, new AttrTable);
    }
    at->append_attr("coordinates", "String", _coordinates);

    return true;
}

bool
HE2CF::write_attribute_units(const string& _varname, string _units)
{

    AttrTable *at = das->get_table(_varname);
    if (!at){
        at = das->add_table(_varname, new AttrTable);
    }
    at->del_attr("units");      // Override any existing units attribute.
    at->append_attr("units", "String", _units);

    return true;
}



