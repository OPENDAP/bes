//////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// Copyright (c) 2010 The HDF Group
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
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
//////////////////////////////////////////////////////////////////////////////
#include "HE2CF.h"
#include "escaping.h"
#include "InternalErr.h"
#include <iomanip>

// Private member functions
bool 
HE2CF::get_vgroup_field_refids(const string& _gname,
                               int32* _ref_df,
                               int32* _ref_gf)
{
    int32 vrefid = Vfind(file_id, (char*)_gname.c_str());
    int32 vgroup_id = Vattach(file_id, vrefid, "r");
    int32 npairs = Vntagrefs(vgroup_id);
    int32 tag_df = -1;
    int32 ref_df = -1;
    int32 tag_gf = -1;
    int32 ref_gf = -1;;

    if(npairs < 0){
        ostringstream error;
        error << "Got " << npairs
              << " npairs for " << _gname.c_str();
        write_error(error.str());
        return false;
    }

    for (int i = 0; i < npairs; ++i) {
        
        int32 tag, ref;
        
        if (Vgettagref(vgroup_id, i, &tag, &ref) < 0){
            cerr << "failed get tag / ref" << endl;
            return false;            
        }
        
        if(Visvg(vgroup_id, ref)){
            char  cvgroup_name[VGNAMELENMAX*4]; // the child vgroup name
            int32 istat;
            int32 vgroup_cid; // Vgroup id 

            vgroup_cid = Vattach(file_id, ref,"r");
            
            istat = Vgetname(vgroup_cid,cvgroup_name);

            if(strncmp(cvgroup_name, "Data Fields",  11) == 0){
                tag_df = tag;
                ref_df = ref;
            }
                
            if(strncmp(cvgroup_name, "Geolocation Fields",  18) == 0){
                tag_gf = tag;
                ref_gf = ref;
            }
            Vdetach(vgroup_cid);
                
        }
    }
    *_ref_df = ref_df;
    *_ref_gf = ref_gf;

    Vdetach(vgroup_id);    
    return true;
}

bool 
HE2CF::open_sd(const string& _filename)
{
    int32 num_datasets = -1;

    sd_id = SDstart(_filename.c_str(), DFACC_READ);
    if(sd_id == FAIL){
        ostringstream error;
        error << "Failed to call SDstart() on "
              << _filename
              <<  " file.";
        write_error(error.str());        
        return false;
    }
    
    if(SDfileinfo(sd_id, &num_datasets, &num_global_attributes)
       == FAIL){
        ostringstream error;
        error << "Failed to call SDfileinfo() on "
              << _filename
              <<  " file.";
        write_error(error.str());
        return false;
    }
    return true;
}

bool 
HE2CF::open_vgroup(const string& _filename)
{

    if ((file_id = Hopen(_filename.c_str(), DFACC_RDONLY, 0)) < 0){
        ostringstream error;        
        error << "Failed to call Hopen on " << _filename << endl;
        write_error(error.str());
        return false;
    }
    if (Vstart(file_id) < 0){
        ostringstream error;        
        error << "Failed to call Vstart on " << _filename << endl;
        write_error(error.str());        
        return false;
    }
    return true;
}

// Borrowed codes from ncdas.cc in netcdf_handle4 module.
string
HE2CF::print_attr(int32 type, int loc, void *vals)
{
    ostringstream rep;
    
    union {
        char *cp;
        short *sp;
        int32 /*nclong*/ *lp;
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
            return escattr(static_cast<const char*>(vals));
        }

    case DFNT_INT16:
    case DFNT_UINT16:        
        {
            gp.sp = (short *) vals;
            rep << *(gp.sp+loc);
            return rep.str();
        }

    case DFNT_INT32:
    case DFNT_UINT32:        
        {
            gp.lp = (int32 *) vals; 
            rep << *(gp.lp+loc);
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
    static const char UINT16[]="Uint16";        
    static const char INT32[]="Int32";
    static const char UINT32[]="Uint32";
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
	return INT16;
        
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
    

void HE2CF::set_DAS(DAS* _das)
{
    das = _das;
}



bool HE2CF::set_metadata(const string&  _name)
{
    metadata.clear();
    
    for(int i = -1; i < num_global_attributes; i++){
        
        char name[H4_MAX_NC_NAME];
        
        if(i == -1){
            snprintf(name, H4_MAX_NC_NAME, "%s", _name.c_str());
        }
        else{
            snprintf(name, H4_MAX_NC_NAME, "%s.%d", _name.c_str(), i);
        }
        
        int32 sds_index = SDfindattr(sd_id, name);
        
        if(sds_index == FAIL){
            // cerr << "Failed to SDfindattr() for " << name << endl;
            // The sequence can skip <metadata>.0
            // For example, "coremetadata" and then "coremetadata.1".
            if(i > 1){
                break;

            }
        }
        else {
            // H4_MAX_NC_NAME is from the user guide example. It's 256.
            char temp_name[H4_MAX_NC_NAME];     
            int32 type=0;
            int32 count = 0;
            if(SDattrinfo(sd_id, sds_index, temp_name, &type, &count) == FAIL) {
                return false;
            }
            else{
                // HACK: C++ new may insert garbage characters.
                // char* data = new char[count]; 
                // <hyokyung 2010.03. 1. 15:23:51>

                char* data  = (char*) calloc((size_t)count+1, sizeof(char));
                if(data == NULL){
                    ostringstream error;
                    error <<  "Failed to calloc memory"  << endl;
                    write_error(error.str());
                }
                if(SDreadattr(sd_id, sds_index, data) == FAIL){
                    // delete[] data;
                    free(data);
                    data = NULL;
                    return false;
                }
                else{
                    data[count] = '\0';
                    metadata.append(data);
                    // delete[] data;                    
                    free(data);
                    data = NULL;

                }
            }

        }
    } // for
    return true;
    
}

bool HE2CF::set_vgroup_map(int32 _refid)
{
    // Clear existing maps first.
    vg_sd_map.clear();
    vg_vd_map.clear();
    
    int32 vgroup_id = Vattach(file_id, _refid, "r");
    int32 npairs = Vntagrefs(vgroup_id);
    
    for (int i = 0; i < npairs; ++i) {
        int32 tag2, ref2;
        char buf[H4_MAX_NC_NAME];
        
        if (Vgettagref(vgroup_id, i, &tag2, &ref2) < 0){
            ostringstream error;
            error <<  "Vgettagref failed for vgroup_id=."  << vgroup_id;
            write_error(error.str());
            return false;
        }
        
        if(tag2 == DFTAG_NDG){
            int sds_id = SDreftoindex(sd_id, ref2); // index 
            sds_id = SDselect(sd_id, sds_id); // sds_id 
            
            int32 rank;
            int32 dimsizes[H4_MAX_VAR_DIMS];
            int32 datatype;
            int32 num_attrs;
        
            SDgetinfo(sds_id, buf, &rank, dimsizes, &datatype, &num_attrs);
            vg_sd_map[string(buf)] = sds_id;
            
        }
        
        if(tag2 == DFTAG_VH){
            int vid;
            if ((vid = VSattach(file_id, ref2, "r")) < 0) {
                ostringstream error;
                error <<  "VSattach failed for file_id=."  << file_id;
                write_error(error.str());
            }
            VSgetname(vid, buf);
            vg_vd_map[string(buf)] = ref2;
            VSdetach(vid);
            
        }
    } // for

    Vdetach(vgroup_id);
    
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
//cerr<<"datatype "<<datatype <<endl;
//cerr<<"attribute name " << _newfname <<endl;
    
    for (int j=0; j < num_attrs; j++){
        status = SDattrinfo(_sds_id, j, buf_attr, &datatype, &n_values);
        if (status < 0){
            ostringstream error;
            error << "SDattrinfo() failed on " << buf_attr;
            write_error(error.str());
            
        }        

        // AttrTable *at = das->get_table(buf_var);
        AttrTable *at = das->get_table(_newfname);
        if (!at){
            // at = das->add_table(buf_var, new AttrTable);
            at = das->add_table(_newfname, new AttrTable);
        }

        //char* value = new char [(n_values+1) * sizeof(datatype)];
        char* value = new char [(n_values+1) * DFKNTsize(datatype)];
        status = SDreadattr(_sds_id, j, value);
        if (status < 0){
            ostringstream error;
            error << "SDreadattr() failed on " << buf_attr << endl;
            write_error(error.str());
        }
        // Handle character type attribute as a string.
	if (datatype == DFNT_CHAR || datatype == DFNT_UCHAR8) {
	    *(value + n_values) = '\0';
	    n_values = 1;
	}
        
	for (unsigned int loc=0; loc < n_values ; loc++) {
	    string print_rep = print_attr(datatype, loc, (void *)value);

            // Override any existing _FillValue attribute.
            if(!strncmp(buf_attr, "_FillValue", H4_MAX_NC_NAME)){
               at->del_attr(buf_attr);      
            }
            // Override any existing long_name attribute.
            if(!strncmp(buf_attr, "long_name", H4_MAX_NC_NAME)){
               at->del_attr(buf_attr);      
            }
	    at->append_attr(buf_attr, print_type(datatype), print_rep);

	}
        status = SDendaccess(_sds_id); 
        delete[] value;
    }
    return true;
}

bool HE2CF::write_attr_vdata(int32 _vd_id, const string& _newfname)
{
    int32 number_type, count, size;
    char buf[H4_MAX_NC_NAME];
    
    int vid;
    
    if ((vid = VSattach(file_id, _vd_id, "r")) < 0) {
        ostringstream error;
        error <<  "VSattach failed.";
        write_error(error.str());
    }
    
    // Don't use VSnattrs - it returns the TOTAL number of attributes 
    // of a vdata and its fields. We should use VSfnattrs.
    count = VSfnattrs(vid, _HDF_VDATA);      
    DBG(cout << "count = " << count << endl);

    AttrTable *at = das->get_table(_newfname);
    if (!at)
        at = das->add_table(_newfname, new AttrTable);


    for(int i=0; i < count; i++){
        int32 count_v = 0;
        if (VSattrinfo(vid, _HDF_VDATA, i, buf,
                       &number_type, &count_v, &size) < 0) {
            ostringstream error;
            error << "VSattrinfo failed.";
            write_error(error.str());
        }

        char* data = new char [(count_v+1) * DFKNTsize(number_type)];
        if (VSgetattr(vid, _HDF_VDATA, i, data) < 0) {
            // problem: clean up and throw an exception
            ostringstream error;            
            error << "VSgetattr failed.";
            write_error(error.str());
        }
        // Handle character type attribute as a string.
	if (number_type == DFNT_CHAR || number_type == DFNT_UCHAR8) {
	    *(data + count_v) = '\0';
	    count_v = 1;
	}
        
        
        for(int j=0; j < count_v ; j++){
	    string print_rep = print_attr(number_type, j, (void *)data);

            // Override any existing _FillValue attribute.
            if(!strncmp(buf, "_FillValue", H4_MAX_NC_NAME)){
               at->del_attr(buf);      
            }
            // Override any existing long_name attribute.
            if(!strncmp(buf, "long_name", H4_MAX_NC_NAME)){
               at->del_attr(buf);      
            }
	    at->append_attr(buf, print_type(number_type), print_rep);

        }
        delete[] data;
        
    } // for 
    //  We need to call VFnfields to process fields as well in near future.
    VSdetach(vid);
}

void
HE2CF::write_error(string _error)
{
    throw InternalErr(__FILE__, __LINE__,
                      _error);        
}
    
bool
HE2CF::write_metadata()
{
    DBG(cout << "metadata:" << metadata << endl);
    return true;
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
    metadata.clear();
}

bool
HE2CF::close()
{
    // Close SD interface.
    int32 istat = SDend(sd_id);

    if(istat == FAIL){
        ostringstream error;                
        error << "Failed to call SDend in HE2CF::close.";
        write_error(error.str());        
        return false;
    }
    
    // Close Vgroup interface.
    istat = Vend(file_id);
    if(istat == FAIL){
        ostringstream error;                
        error << "Failed to call Vend in HE2CF::close.";
        write_error(error.str());        
        return false;
    }
    
    istat =  Hclose(file_id);
    if(istat == FAIL){
        ostringstream error;                
        error << "Failed to call Hclose in HE2CF::close.";
        write_error(error.str());        
        return false;
    }
    return true;
}

string
HE2CF::get_metadata(const string& _name)
{
    set_metadata(_name);
    return metadata;
}

bool
HE2CF::open(const string& _filename)
{
    if(_filename == ""){
        ostringstream error;                
        error << "=open(): filename is empty.";
        write_error(error.str());        
        return false;
    }
    
    if(!open_vgroup(_filename)){
        ostringstream error;                
        error << "=open(): failed to open vgroup.";
        write_error(error.str());        
        return false;        
    }
    
    if(!open_sd(_filename)){
        ostringstream error;                
        error << "=open(): failed to open sd.";
        write_error(error.str());        
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
    
    if(gname != _gname){
        // Construct field:(SD|Vdata)ref table for faster lookup.
        gname = _gname;        
        get_vgroup_field_refids(_gname, &ref_df, &ref_gf);
        set_vgroup_map(ref_gf); 
        set_vgroup_map(ref_df); 
    }
    
    // Use a cached table.
    int32 id = -1;
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

bool
HE2CF::write_attribute_FillValue(const string& _varname, 
                                 int _type,
                                 float value)
{
    void* v_ptr;

    // Cast the value depending on the type.
    switch (_type) {
        
    case DFNT_UINT8:
    case DFNT_INT8:        
        {
            int8 val = (int8) value;
            v_ptr = (void*)&val;
            
        }
        break;
    case DFNT_INT16:
    case DFNT_UINT16:        
        {
            int16 val = (int16) value;
            v_ptr = (void*)&val;
        }
        break;
    case DFNT_INT32:
    case DFNT_UINT32:        
        {
            int32 val = (int32) value;
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
        write_error("Invalid FillValue Type - ");
        break;
    }

    AttrTable *at = das->get_table(_varname);
    if (!at){
        at = das->add_table(_varname, new AttrTable);
    }
    string print_rep = print_attr(_type, 0, v_ptr);
    at->append_attr("_FillValue", print_type(_type), print_rep);

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



