#include "HDFCFUtil.h"
#include <BESDebug.h>
#include <BESLog.h>
#include <math.h>

#define SIGNED_BYTE_TO_INT32 1
using namespace std;
using namespace libdap;

// Check the BES key. 
// This function will check a BES key specified at the file h4.conf.in.
// If the key's value is either true or yes. The handler claims to find
// a key and will do some operations. Otherwise, will do different operations.
// For example, One may find a line H4.EnableCF=true at h4.conf.in.
// That means, the HDF4 handler will handle the HDF4 files by following CF conventions.
bool 
HDFCFUtil::check_beskeys(const string& key) {

    bool found = false;
    string doset ="";
    const string dosettrue ="true";
    const string dosetyes = "yes";

    TheBESKeys::TheKeys()->get_value( key, doset, found ) ;
    if( true == found ) {
        doset = BESUtil::lowercase( doset ) ;
        if( dosettrue == doset  || dosetyes == doset )
            return true;
    }
    return false;

}


// From a string separated by a separator to a list of string,
// for example, split "ab,c" to {"ab","c"}
void 
HDFCFUtil::Split(const char *s, int len, char sep, std::vector<std::string> &names)
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
HDFCFUtil::Split(const char *sz, char sep, std::vector<std::string> &names)
{
    Split(sz, (int)strlen(sz), sep, names);
}

// This is a safer way to insert and update a c++ map value.
// Otherwise, the local testsuite at The HDF Group will fail for HDF-EOS2 data
//  under iMac machine platform.
// The implementation replaces the element even if the key exists.
// This function is equivalent to map[key]=value
bool
HDFCFUtil::insert_map(std::map<std::string,std::string>& m, string key, string val)
{
    pair<map<string,string>::iterator, bool> ret;
    ret = m.insert(make_pair(key, val));
    if(ret.second == false){
        m.erase(key);
        ret = m.insert(make_pair(key, val));
        if(ret.second == false){
            BESDEBUG("h4","insert_map():insertion failed on Key=" << key << " Val=" << val << endl);
        }
    }
    return ret.second;
}

// Obtain CF string
string
HDFCFUtil::get_CF_string(string s)
{

    if(""==s) 
        return s;
    string insertString(1,'_');

    // Always start with _ if the first character is not a letter
    if (true == isdigit(s[0]))
        s.insert(0,insertString);

    // New name conventions drop the first '/' from the path.
    if ('/' ==s[0])
        s.erase(0,1);

    for(unsigned int i=0; i < s.length(); i++)
        if((false == isalnum(s[i])) &&  (s[i]!='_'))
            s[i]='_';

    return s;

}

// Obtain the unique name for the clashed names and save it to set namelist.
// This is a recursive call. A unique name list is represented as a set.
// If we find that a name already exists in the nameset, we will add a number 
// at the end of the name to form a new name. If the new name still exists
// in the nameset, we will increase the index number and check again until 
// a unique name is generated.
void 
HDFCFUtil::gen_unique_name(string &str,set<string>& namelist, int&clash_index) {

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

// General routine to handle the name clashing
// The input parameters include:
// name vector -> newobjnamelist(The name vector is changed to a unique name list
// a pre-allocated object name set ->objnameset(can be used to determine if a name exists)
void
HDFCFUtil::Handle_NameClashing(vector<string>&newobjnamelist,set<string>&objnameset) {

    pair<set<string>::iterator,bool> setret;
    set<string>::iterator iss;

    vector<string> clashnamelist;
    vector<string>::iterator ivs;

    // clash index to original index mapping
    map<int,int> cl_to_ol;
    int ol_index = 0;
    int cl_index = 0;

    vector<string>::const_iterator irv;

    for (irv = newobjnamelist.begin(); irv != newobjnamelist.end(); ++irv) {
        setret = objnameset.insert((*irv));
        if (false == setret.second ) {
            clashnamelist.insert(clashnamelist.end(),(*irv));
            cl_to_ol[cl_index] = ol_index;
            cl_index++;
        }
        ol_index++;
    }

    // Now change the clashed elements to unique elements; 
    // Generate the set which has the same size as the original vector.
    for (ivs=clashnamelist.begin(); ivs!=clashnamelist.end(); ivs++) {
        int clash_index = 1;
        string temp_clashname = *ivs +'_';
        HDFCFUtil::gen_unique_name(temp_clashname,objnameset,clash_index);
        *ivs = temp_clashname;
    }

    // Now go back to the original vector, make it unique.
    for (unsigned int i =0; i <clashnamelist.size(); i++)
        newobjnamelist[cl_to_ol[i]] = clashnamelist[i];

}

// General routine to handle the name clashing
// The input parameter just includes:
// name vector -> newobjnamelist(The name vector is changed to a unique name list
void
HDFCFUtil::Handle_NameClashing(vector<string>&newobjnamelist) {

    set<string> objnameset;
    Handle_NameClashing(newobjnamelist,objnameset);
}

// Borrowed codes from ncdas.cc in netcdf_handle4 module.
string
HDFCFUtil::print_attr(int32 type, int loc, void *vals)
{
    ostringstream rep;

    union {
        char *cp;
        unsigned char *ucp;
        short *sp;
        unsigned short *usp;
        int32 /*nclong*/ *lp;
        unsigned int *ui;
        float *fp;
        double *dp;
    } gp;

    switch (type) {

    // Mapping both DFNT_UINT8 and DFNT_INT8 to unsigned char 
    // may cause overflow. Documented at jira ticket HFRHANDLER-169.
    case DFNT_UINT8:
        {
            unsigned char uc;
            gp.ucp = (unsigned char *) vals;

            uc = *(gp.ucp+loc);
            rep << (int)uc;
            return rep.str();
        }

    case DFNT_INT8:
        {
            char c;
            gp.cp = (char *) vals;

            c = *(gp.cp+loc);
            rep << (int)c;
            return rep.str();
        }

    case DFNT_UCHAR:
    case DFNT_CHAR:
        {
            // Use the customized escattr function. Don't escape \n,\t and \r. KY 2013-10-14
            return escattr(static_cast<const char*>(vals));
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

// Print datatype in string. This is used to generate DAS.
string
HDFCFUtil::print_type(int32 type)
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
// ADD the macro
    {
#ifndef SIGNED_BYTE_TO_INT32
        return BYTE;
#else
        return INT32;
#endif
    }
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


// Subset of latitude and longitude to follow the parameters from the DAP expression constraint
// Somehow this function doesn't work. Now it is not used. Still keep it here for the future investigation.
template < typename T >
void HDFCFUtil::LatLon2DSubset (T * outlatlon,
                                int majordim,
                                int minordim,
                                T * latlon,
                                int32 * offset,
                                int32 * count,
                                int32 * step)
{

    //  float64 templatlon[majordim][minordim];
    T (*templatlonptr)[majordim][minordim] = (typeof templatlonptr) latlon;
    int i, j, k;

    // do subsetting
    // Find the correct index
    int dim0count = count[0];
    int dim1count = count[1];
    int dim0index[dim0count], dim1index[dim1count];

    for (i = 0; i < count[0]; i++)      // count[0] is the least changing dimension 
        dim0index[i] = offset[0] + i * step[0];


    for (j = 0; j < count[1]; j++)
        dim1index[j] = offset[1] + j * step[1];

    // Now assign the subsetting data
    k = 0;

    for (i = 0; i < count[0]; i++) {
        for (j = 0; j < count[1]; j++) {
            outlatlon[k] = (*templatlonptr)[dim0index[i]][dim1index[j]];
            k++;

        }
    }
}

// CF requires the _FillValue attribute datatype is the same as the corresponding field datatype. 
// For some NASA files, this is not true.
// So we need to check if the _FillValue's datatype is the same as the attribute's. 
// If not, we need to correct them.
void HDFCFUtil::correct_fvalue_type(AttrTable *at,int32 dtype) {

    AttrTable::Attr_iter it = at->attr_begin();
    bool find_fvalue = false;
    while (it!=at->attr_end() && false==find_fvalue) {
        if (at->get_name(it) =="_FillValue")
        {                            
            find_fvalue = true;
            string fillvalue =""; 
            string fillvalue_type ="";
            if((*at->get_attr_vector(it)).size() !=1) 
                throw InternalErr(__FILE__,__LINE__,"The number of _FillValue must be 1.");
            fillvalue =  (*at->get_attr_vector(it)->begin());
            fillvalue_type = at->get_type(it);
            string var_type = HDFCFUtil::print_type(dtype);

            if(fillvalue_type != var_type){ 

                at->del_attr("_FillValue");

                if (fillvalue_type == "String") {

                    // String fillvalue is always represented as /+octal numbers when its type is forced to
                    // change to string(check HFRHANDLER-223). So we have to retrieve it back.
                    if(fillvalue.size() >1) {

                        long int fillvalue_int = 0;
                        vector<char> fillvalue_temp(fillvalue.size());
                        char *pEnd;
                        fillvalue_int = strtol((fillvalue.substr(1)).c_str(),&pEnd,8);
                        stringstream convert_str;
                        convert_str << fillvalue_int;
                        at->append_attr("_FillValue",var_type,convert_str.str());               
                    }
                    else {

                        // If somehow the fillvalue type is DFNT_CHAR or DFNT_UCHAR, and the size is 1,
                        // that means the fillvalue type is wrongly defined, we treat as a 8-bit integer number.
                        // Note, the code will only assume the value ranges from 0 to 128.(JIRA HFRHANDLER-248).
                        // KY 2014-04-24
                          
                        short fillvalue_int = fillvalue.at(0);

                        stringstream convert_str;
                        convert_str << fillvalue_int;
                        if(fillvalue_int <0 || fillvalue_int >128)
                            throw InternalErr(__FILE__,__LINE__,
                            "If the fillvalue is a char type, the value must be between 0 and 128.");


                        at->append_attr("_FillValue",var_type,convert_str.str());               
                    }
                }
                
                else 
                    at->append_attr("_FillValue",var_type,fillvalue);               
            }
        }
        it++;
    }

}

// CF requires the scale_factor and add_offset attribute datatypes must be the same as the corresponding field datatype. 
// For some NASA files, this is not true.
// So we need to check if the _FillValue's datatype is the same as the attribute's. 
// If not, we need to correct them.
void HDFCFUtil::correct_scale_offset_type(AttrTable *at) {

    AttrTable::Attr_iter it = at->attr_begin();
    bool find_scale  = false;
    bool find_offset = false;

    // Declare scale_factor,add_offset, fillvalue and valid_range type in string format.
    string scale_factor_type; 
    string add_offset_type;
    string scale_factor_value;
    string add_offset_value;

    while (it!=at->attr_end() &&((find_scale!=true) ||(find_offset!=true))) {
        if (at->get_name(it) =="scale_factor")
        {                            
            find_scale = true;
            scale_factor_value = (*at->get_attr_vector(it)->begin());
            scale_factor_type = at->get_type(it);
        }

        if(at->get_name(it)=="add_offset")
        {
            find_offset = true;
            add_offset_value = (*at->get_attr_vector(it)->begin());
            add_offset_type = at->get_type(it);
        }

        it++;
    }

    // Change offset type to the scale type
    if((true==find_scale) && (true==find_offset)) {
        if(scale_factor_type != add_offset_type) {
            at->del_attr("add_offset");
            at->append_attr("add_offset",scale_factor_type,add_offset_value);
        }
    }
}
    

#ifdef USE_HDFEOS2_LIB

// For MODIS (confirmed by level 1B) products, values between 65500(MIN_NON_SCALE_SPECIAL_VALUE)  
// and 65535(MAX_NON_SCALE_SPECIAL_VALUE) are treated as
// special values. These values represent non-physical data values caused by various failures.
// For example, 65533 represents "when Detector is saturated".
bool HDFCFUtil::is_special_value(int32 dtype, float fillvalue, float realvalue) {

    bool ret_value = false;

    if (DFNT_UINT16 == dtype) {

        int fillvalue_int = (int)fillvalue;
        if (MAX_NON_SCALE_SPECIAL_VALUE == fillvalue_int) {
            int realvalue_int = (int)realvalue;
            if (realvalue_int <= MAX_NON_SCALE_SPECIAL_VALUE && realvalue_int >=MIN_NON_SCALE_SPECIAL_VALUE)
                ret_value = true;
        }
    }

    return ret_value;

}

// Check if the MODIS file has dimension map and return the number of dimension maps
// Note: This routine is only applied to a MODIS geo-location file when the corresponding
// MODIS swath uses dimension maps and has the MODIS geo-location file under the same 
// file directory. This is also restricted by turning on H4.EnableCheckMODISGeoFile to be true(file h4.conf.in).
// By default, this key is turned off. Also this function is only used once in one service. So it won't
// affect performance. KY 2014-02-18
int HDFCFUtil::check_geofile_dimmap(const string & geofilename) {

    int32 fileid = SWopen(const_cast<char*>(geofilename.c_str()),DFACC_READ);
    if (fileid < 0) 
        return -1;
    string swathname = "MODIS_Swath_Type_GEO";
    int32 datasetid = SWattach(fileid,const_cast<char*>(swathname.c_str()));
    if (datasetid < 0) {
        SWclose(fileid);
        return -1;
    }

    int32 nummaps = 0;
    int32 bufsize;

    // Obtain number of dimension maps and the buffer size.
    if ((nummaps = SWnentries(datasetid, HDFE_NENTMAP, &bufsize)) == -1) {
        SWdetach(datasetid);
        SWclose(fileid);
        return -1;
    }

    SWdetach(datasetid);
    SWclose(fileid);
    return nummaps;

}

// Check if we need to change the datatype for MODIS fields. The datatype needs to be changed
// mainly because of non-CF scale and offset rules. To avoid violating CF conventions, we apply
// the non-CF MODIS scale and offset rule to MODIS data. So the final data type may be different
// than the original one due to this operation. For example, the original datatype may be int16.
// After applying the scale/offset rule, the datatype may become float32.
// The following are useful notes about MODIS SCALE OFFSET HANDLING
// Note: MODIS Scale and offset handling needs to re-organized. But it may take big efforts.
// Instead, I remove the global variable mtype, and _das; move the old calculate_dtype code
// back to HDFEOS2.cc. The code is a little better organized. If possible, we may think to overhaul
// the handling of MODIS scale-offset part. KY 2012-6-19
// 
bool HDFCFUtil::change_data_type(DAS & das, SOType scaletype, const string &new_field_name) 
{

    AttrTable *at = das.get_table(new_field_name);

    // The following codes do these:
    // For MODIS level 1B(using the swath name), check radiance,reflectance etc. 
    // If the DISABLESCALE key is true, no need to check scale and offset for other types.
    // Otherwise, continue checking the scale and offset names.
    // KY 2013-12-13

    if(scaletype!=DEFAULT_CF_EQU && at!=NULL)
    {
        AttrTable::Attr_iter it = at->attr_begin();
        string  scale_factor_value="";
        string  add_offset_value="0";
        string  radiance_scales_value="";
        string  radiance_offsets_value="";
        string  reflectance_scales_value=""; 
        string  reflectance_offsets_value="";
        string  scale_factor_type, add_offset_type;
        while (it!=at->attr_end())
        {
            if(at->get_name(it)=="radiance_scales")
                radiance_scales_value = *(at->get_attr_vector(it)->begin());
            if(at->get_name(it)=="radiance_offsets")
                radiance_offsets_value = *(at->get_attr_vector(it)->begin());
            if(at->get_name(it)=="reflectance_scales")
                reflectance_scales_value = *(at->get_attr_vector(it)->begin());
            if(at->get_name(it)=="reflectance_offsets")
                reflectance_offsets_value = *(at->get_attr_vector(it)->begin());

            // Ideally may just check if the attribute name is scale_factor.
            // But don't know if some products use attribut name like "scale_factor  "
            // So now just filter out the attribute scale_factor_err. KY 2012-09-20
            if(at->get_name(it).find("scale_factor")!=string::npos){
                string temp_attr_name = at->get_name(it);
                if (temp_attr_name != "scale_factor_err") {
                    scale_factor_value = *(at->get_attr_vector(it)->begin());
                    scale_factor_type = at->get_type(it);
                }
            }
            if(at->get_name(it).find("add_offset")!=string::npos)
            {
                string temp_attr_name = at->get_name(it);
                if (temp_attr_name !="add_offset_err") {
                    add_offset_value = *(at->get_attr_vector(it)->begin());
                    add_offset_type = at->get_type(it);
                }
            }
            it++;
        }

        if((radiance_scales_value.length()!=0 && radiance_offsets_value.length()!=0) 
            || (reflectance_scales_value.length()!=0 && reflectance_offsets_value.length()!=0))
            return true;
		
        if(scale_factor_value.length()!=0) 
        {
            if(!(atof(scale_factor_value.c_str())==1 && atof(add_offset_value.c_str())==0)) 
                return true;
        }
    }

    return false;
}

// Obtain the MODIS swath dimension map info.
void HDFCFUtil::obtain_dimmap_info(const string& filename,HDFEOS2::Dataset*dataset,  
                                   vector<struct dimmap_entry> & dimmaps, 
                                   string & modis_geofilename, bool& geofile_has_dimmap) {


    HDFEOS2::SwathDataset *sw = static_cast<HDFEOS2::SwathDataset *>(dataset);
    const vector<HDFEOS2::SwathDataset::DimensionMap*>& origdimmaps = sw->getDimensionMaps();
    vector<HDFEOS2::SwathDataset::DimensionMap*>::const_iterator it_dmap;
    struct dimmap_entry tempdimmap;

    // if having dimension maps, we need to retrieve the dimension map info.
    for(size_t i=0;i<origdimmaps.size();i++){
        tempdimmap.geodim = origdimmaps[i]->getGeoDimension();
        tempdimmap.datadim = origdimmaps[i]->getDataDimension();
        tempdimmap.offset = origdimmaps[i]->getOffset();
        tempdimmap.inc    = origdimmaps[i]->getIncrement();
        dimmaps.push_back(tempdimmap);
    }

    string check_modis_geofile_key ="H4.EnableCheckMODISGeoFile";
    bool check_geofile_key = false;
    check_geofile_key = HDFCFUtil::check_beskeys(check_modis_geofile_key);

    // Only when there is dimension map, we need to consider the additional MODIS geolocation files.
    // Will check if the check modis_geo_location file key is turned on.
    if((origdimmaps.size() != 0) && (true == check_geofile_key) ) {

        // Has to use C-style since basename and dirname are not C++ routines.
        char*tempcstr;
        tempcstr = new char [filename.size()+1];
        strncpy (tempcstr,filename.c_str(),filename.size());
        string basefilename = basename(tempcstr);
        string dirfilename = dirname(tempcstr);
        delete [] tempcstr;

        // If the current file is a MOD03 or a MYD03 file, we don't need to check the extra MODIS geolocation file at all.
        bool is_modis_geofile = false;
        if(basefilename.size() >5) {
            if((0 == basefilename.compare(0,5,"MOD03")) || (0 == basefilename.compare(0,5,"MYD03"))) 
                is_modis_geofile = true;
        }

        // This part is implemented specifically for supporting MODIS dimension map data.
        // MODIS Aqua Swath dimension map geolocation file always starts with MYD03
        // MODIS Terra Swath dimension map geolocation file always starts with MOD03
        // We will check the first three characters to see if the dimension map geolocation file exists.
        // An example MODIS swath filename is MOD05_L2.A2008120.0000.005.2008121182723.hdf
        // An example MODIS geo-location file name is MOD03.A2008120.0000.005.2010003235220.hdf
        // Notice that the "A2008120.0000" in the middle of the name is the "Acquistion Date" of the data
        // So the geo-location file name should have exactly the same string. We will use this
        // string to identify if a MODIS geo-location file exists or not.
        // Note the string size is 14(.A2008120.0000).
        // More information of naming convention, check http://modis-atmos.gsfc.nasa.gov/products_filename.html
        // KY 2010-5-10


        // Obtain string "MYD" or "MOD"
        // Here we need to consider when MOD03 or MYD03 use the dimension map. 
        if ((false == is_modis_geofile) && (basefilename.size() >3)) {

            string fnameprefix = basefilename.substr(0,3);

            if(fnameprefix == "MYD" || fnameprefix =="MOD") {
                size_t fnamemidpos = basefilename.find(".A");
                if(fnamemidpos != string::npos) {
       	            string fnamemiddle = basefilename.substr(fnamemidpos,14);
                    if(fnamemiddle.size()==14) {
                        string geofnameprefix = fnameprefix+"03";
                        // geofnamefp will be something like "MOD03.A2008120.0000"
                        string geofnamefp = geofnameprefix + fnamemiddle;
                        DIR *dirp;
                        struct dirent* dirs;
    
                        // Go through the directory to see if we have the corresponding MODIS geolocation file
                        dirp = opendir(dirfilename.c_str());
                        if (NULL == dirp) 
                            throw InternalErr(__FILE__,__LINE__,"opendir fails.");

                        while ((dirs = readdir(dirp))!= NULL){
                            if(strncmp(dirs->d_name,geofnamefp.c_str(),geofnamefp.size())==0){
                                modis_geofilename = dirfilename + "/"+ dirs->d_name;
                                int num_dimmap = HDFCFUtil::check_geofile_dimmap(modis_geofilename);
                                if (num_dimmap < 0)  {
                                    closedir(dirp);
                                    throw InternalErr(__FILE__,__LINE__,"this file is not a MODIS geolocation file.");
                                }
                                geofile_has_dimmap = (num_dimmap >0)?true:false;
                                break;
                            }
                        }
                        closedir(dirp);
                    }
                }
            }
        }
    }
}

// This is for the case that the separate MODIS geo-location file is used.
// Some geolocation names at the MODIS data file are not consistent with
// the names in the MODIS geo-location file. So need to correct them.
bool HDFCFUtil::is_modis_dimmap_nonll_field(string & fieldname) {

    bool modis_dimmap_nonll_field = false;
    vector<string> modis_dimmap_nonll_fieldlist; 

    modis_dimmap_nonll_fieldlist.push_back("Height");
    modis_dimmap_nonll_fieldlist.push_back("SensorZenith");
    modis_dimmap_nonll_fieldlist.push_back("SensorAzimuth");
    modis_dimmap_nonll_fieldlist.push_back("Range");
    modis_dimmap_nonll_fieldlist.push_back("SolarZenith");
    modis_dimmap_nonll_fieldlist.push_back("SolarAzimuth");
    modis_dimmap_nonll_fieldlist.push_back("Land/SeaMask");
    modis_dimmap_nonll_fieldlist.push_back("gflags");
    modis_dimmap_nonll_fieldlist.push_back("Solar_Zenith");
    modis_dimmap_nonll_fieldlist.push_back("Solar_Azimuth");
    modis_dimmap_nonll_fieldlist.push_back("Sensor_Azimuth");
    modis_dimmap_nonll_fieldlist.push_back("Sensor_Zenith");

    map<string,string>modis_field_to_geofile_field;
    map<string,string>::iterator itmap;
    modis_field_to_geofile_field["Solar_Zenith"] = "SolarZenith";
    modis_field_to_geofile_field["Solar_Azimuth"] = "SolarAzimuth";
    modis_field_to_geofile_field["Sensor_Zenith"] = "SensorZenith";
    modis_field_to_geofile_field["Solar_Azimuth"] = "SolarAzimuth";

    for (unsigned int i = 0; i <modis_dimmap_nonll_fieldlist.size(); i++) {

        if (fieldname == modis_dimmap_nonll_fieldlist[i]) {
            itmap = modis_field_to_geofile_field.find(fieldname);
            if (itmap !=modis_field_to_geofile_field.end())
                fieldname = itmap->second;
            modis_dimmap_nonll_field = true;
            break;
        }
    }

    return modis_dimmap_nonll_field;
}

/// Add comments.
void HDFCFUtil::handle_modis_special_attrs_disable_scale_comp(AttrTable *at, 
                                                              const string &filename, 
                                                              bool is_grid, 
                                                              const string & newfname, 
                                                              SOType sotype){

    // Declare scale_factor,add_offset, fillvalue and valid_range type in string format.
    string scale_factor_type;
    string add_offset_type;

    // Scale and offset values
    string scale_factor_value=""; 
    float  orig_scale_value = 1;
    string add_offset_value="0"; 
    float  orig_offset_value = 0;
    bool add_offset_found = false;


    // Go through all attributes to find scale_factor,add_offset,_FillValue,valid_range
    // Number_Type information
    AttrTable::Attr_iter it = at->attr_begin();
    while (it!=at->attr_end())
    {
        if(at->get_name(it)=="scale_factor")
        {
            scale_factor_value = (*at->get_attr_vector(it)->begin());
            orig_scale_value = atof(scale_factor_value.c_str());
            scale_factor_type = at->get_type(it);
        }

        if(at->get_name(it)=="add_offset")
        {
            add_offset_value = (*at->get_attr_vector(it)->begin());
            orig_offset_value = atof(add_offset_value.c_str());
            add_offset_type = at->get_type(it);
            add_offset_found = true;
        }

        it++;
    } 

    // According to our observations, it seems that MODIS products ALWAYS use the "scale" factor to 
    // make bigger values smaller.
    // So for MODIS_MUL_SCALE products, if the scale of some variable is greater than 1, 
    // it means that for this variable, the MODIS type for this variable may be MODIS_DIV_SCALE.
    // For the similar logic, we may need to change MODIS_DIV_SCALE to MODIS_MUL_SCALE and MODIS_EQ_SCALE
    // to MODIS_DIV_SCALE.
    // We indeed find such a case. HDF-EOS2 Grid MODIS_Grid_1km_2D of MOD(or MYD)09GA is a MODIS_EQ_SCALE.
    // However,
    // the scale_factor of the variable Range_1 in the MOD09GA product is 25. According to our observation,
    // this variable should be MODIS_DIV_SCALE.We verify this is true according to MODIS 09 product document
    // http://modis-sr.ltdri.org/products/MOD09_UserGuide_v1_3.pdf.
    // Since this conclusion is based on our observation, we would like to add a BESlog to detect if we find
    // the similar cases so that we can verify with the corresponding product documents to see if this is true.
    // More information, 
    // We just verified with the data producer, the scale_factor for Range_1 and Range_c is 25 but the
    // equation is still multiplication instead of division. So we have to make this as a special case that
    // we don't want to change the scale and offset settings. 
    // KY 2014-01-13

    
    if(scale_factor_value.length()!=0) {
        if (MODIS_EQ_SCALE == sotype || MODIS_MUL_SCALE == sotype) {
            if (orig_scale_value > 1) {

                bool need_change_scale = true;
                if(true == is_grid) {
                    if ((filename.size() >5) && ((filename.compare(0,5,"MOD09") == 0)|| (filename.compare(0,5,"MYD09")==0))) {
                        if ((newfname.size() >5) && newfname.find("Range") != string::npos)
                            need_change_scale = false;
                    }
                }
                if(true == need_change_scale)  {
                    sotype = MODIS_DIV_SCALE;
                    (*BESLog::TheLog())<< "The field " << newfname << " scale factor is "<< scale_factor_value << endl
                                   << " But the original scale factor type is MODIS_MUL_SCALE or MODIS_EQ_SCALE. " << endl
                                   << " Now change it to MODIS_DIV_SCALE. "<<endl;
                }
            }
        }

        if (MODIS_DIV_SCALE == sotype) {
            if (orig_scale_value < 1) {
                sotype = MODIS_MUL_SCALE;
                (*BESLog::TheLog())<< "The field " << newfname << " scale factor is "<< scale_factor_value << endl
                                   << " But the original scale factor type is MODIS_DIV_SCALE. " << endl
                                   << " Now change it to MODIS_MUL_SCALE. "<<endl;
            }
        }


        if ((MODIS_MUL_SCALE == sotype) &&(true == add_offset_found)) {

            //string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&orig_scale_value));
            at->del_attr("scale_factor");
            //at->append_attr("scale_factor", HDFCFUtil::print_type(DFNT_FLOAT32), print_rep);
            // Since scale_factor should always be smaller than 1, so this is fine. We just need to
            // change the type.
            at->append_attr("scale_factor", HDFCFUtil::print_type(DFNT_FLOAT32), scale_factor_value);
            
            if (true == add_offset_found) {
                float new_offset_value = (orig_offset_value ==0)?0:(-1 * orig_offset_value *orig_scale_value); 
                string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&new_offset_value));
                at->del_attr("add_offset");
                at->append_attr("add_offset", HDFCFUtil::print_type(DFNT_FLOAT32), print_rep);
            }
        }    

        if ((MODIS_DIV_SCALE == sotype)) {

            float new_scale_value = 1.0/orig_scale_value;
            string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&new_scale_value));
            at->del_attr("scale_factor");
            //at->append_attr("scale_factor", HDFCFUtil::print_type(DFNT_FLOAT32), print_rep);
            // Since scale_factor should always be smaller than 1, so this is fine. We just need to
            // change the type.
            at->append_attr("scale_factor", HDFCFUtil::print_type(DFNT_FLOAT32), print_rep);
            
            if (true == add_offset_found) {
                float new_offset_value = (orig_offset_value==0)?0:(-1 * orig_offset_value *new_scale_value); 
                string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&new_offset_value));
                at->del_attr("add_offset");
                at->append_attr("add_offset", HDFCFUtil::print_type(DFNT_FLOAT32), print_rep);
            }

        }
    }

}

// These routines will handle scale_factor,add_offset,valid_min,valid_max and other attributes 
// such as Number_Type to make sure the CF is followed.
// For example, For the case that the scale and offset rule doesn't follow CF, the scale_factor and add_offset attributes are renamed
// to orig_scale_factor and orig_add_offset to keep the original field info.
// Note: This routine should only be called when MODIS Scale and offset rules don't follow CF.
// Input parameters:
// AttrTable at - DAS attribute table
// string newfname - field name: this is used to identify special MODIS level 1B emissive and refSB fields
// SOType sotype - MODIS scale and offset type
// bool gridname_change_valid_range - Flag to check if this is the special MODIS VIP product
// bool changedtype - Flag to check if the datatype of this field needs to be changed
// bool change_fvtype - Flag to check if the attribute _FillValue needs to change to data type

/// Future  CODING reminder
//  Divide this function into smaller functions:
//
void HDFCFUtil::handle_modis_special_attrs(AttrTable *at, const string & filename, 
                                           bool is_grid,const string & newfname, 
                                           SOType sotype,  bool gridname_change_valid_range, 
                                           bool changedtype, bool & change_fvtype){

    // Declare scale_factor,add_offset, fillvalue and valid_range type in string format.
    string scale_factor_type;
    string add_offset_type;
    string fillvalue_type;
    string valid_range_type;


    // Scale and offset values
    string scale_factor_value=""; 
    float  orig_scale_value = 1;
    string add_offset_value="0"; 
    float  orig_offset_value = 0;
    bool   add_offset_found = false;

    // Fillvalue
    string fillvalue="";

    // Valid range value
    string valid_range_value="";

    bool has_valid_range = false;

    // We may need to change valid_range to valid_min and valid_max. Initialize them here.
    float orig_valid_min = 0;
    float orig_valid_max = 0;

    // Number_Type also needs to be adjusted when datatype is changed
    string number_type_value="";
    string number_type_dap_type="";

    // Go through all attributes to find scale_factor,add_offset,_FillValue,valid_range
    // Number_Type information
    AttrTable::Attr_iter it = at->attr_begin();
    while (it!=at->attr_end())
    {
        if(at->get_name(it)=="scale_factor")
        {
            scale_factor_value = (*at->get_attr_vector(it)->begin());
            orig_scale_value = atof(scale_factor_value.c_str());
            scale_factor_type = at->get_type(it);
        }

        if(at->get_name(it)=="add_offset")
        {
            add_offset_value = (*at->get_attr_vector(it)->begin());
            orig_offset_value = atof(add_offset_value.c_str());
            add_offset_type = at->get_type(it);
            add_offset_found = true;
        }

        if(at->get_name(it)=="_FillValue")
        {
            fillvalue =  (*at->get_attr_vector(it)->begin());
            fillvalue_type = at->get_type(it);
        }

        if(at->get_name(it)=="valid_range")
        {
            vector<string> *avalue = at->get_attr_vector(it);
            vector<string>::iterator ait = avalue->begin();
            while(ait!=avalue->end())
            {
                valid_range_value += *ait;
                ait++;	
                if(ait!=avalue->end())
                    valid_range_value += ", ";
            }
            valid_range_type = at->get_type(it);
            if (false == gridname_change_valid_range) {
                orig_valid_min = (float)(atof((avalue->at(0)).c_str()));
                orig_valid_max = (float)(atof((avalue->at(1)).c_str()));
            }
            has_valid_range = true;
        }

        if(true == changedtype && (at->get_name(it)=="Number_Type"))
        {
            number_type_value = (*at->get_attr_vector(it)->begin());
            number_type_dap_type= at->get_type(it);
        }

        it++;
    } 

    // Rename scale_factor and add_offset attribute names. Otherwise, they will be 
    // misused by CF tools to generate wrong data values based on the CF scale and offset rule.
    if(scale_factor_value.length()!=0)
    {
        if(!(atof(scale_factor_value.c_str())==1 && atof(add_offset_value.c_str())==0)) //Rename them.
        {
            at->del_attr("scale_factor");
            at->append_attr("orig_scale_factor", scale_factor_type, scale_factor_value);
            if(add_offset_found)
            {
                at->del_attr("add_offset");
                at->append_attr("orig_add_offset", add_offset_type, add_offset_value);
            }
        }
    }

    // Change _FillValue datatype
    if(true == changedtype && fillvalue.length()!=0 && fillvalue_type!="Float32" && fillvalue_type!="Float64") 
    {
        change_fvtype = true;
        at->del_attr("_FillValue");
        at->append_attr("_FillValue", "Float32", fillvalue);
    }
 
    float valid_max = 0;
    float valid_min = 0;

    it = at->attr_begin();
    bool handle_modis_l1b = false;

    // MODIS level 1B's Emissive and RefSB fields' scale_factor and add_offset 
    // get changed according to different vertical levels.
    // So we need to handle them specifically.
    if (sotype == MODIS_MUL_SCALE && true ==changedtype) {

        // Emissive should be at the end of the field name such as "..._Emissive"
        string emissive_str = "Emissive";
        string RefSB_str = "RefSB";
        bool is_emissive_field = false;
        bool is_refsb_field = false;
        if(newfname.find(emissive_str)!=string::npos) {
            if(0 == newfname.compare(newfname.size()-emissive_str.size(),emissive_str.size(),emissive_str))
                is_emissive_field = true;
        }

        if(newfname.find(RefSB_str)!=string::npos) {
            if(0 == newfname.compare(newfname.size()-RefSB_str.size(),RefSB_str.size(),RefSB_str))
                is_refsb_field = true;
        }
  
        // We will calculate the maximum and minimum scales and offsets within all the vertical levels.
        if ((true == is_emissive_field) || (true== is_refsb_field)){

            float scale_max = 0;
            float scale_min = 100000;

            float offset_max = 0;
            float offset_min = 0;

            float temp_var_val = 0;

            string orig_long_name_value;
            string modify_long_name_value;
            string str_removed_from_long_name=" Scaled Integers";
            string radiance_units_value;
                        
            while (it!=at->attr_end()) 
	    {
                // Radiance field(Emissive field)
                if(true == is_emissive_field) {

                    if ("radiance_scales" == (at->get_name(it))) {
                        vector<string> *avalue = at->get_attr_vector(it);
                        for (vector<string>::const_iterator ait = avalue->begin();ait !=avalue->end();++ait) {
                            temp_var_val = (float)(atof((*ait).c_str())); 
                            if (temp_var_val > scale_max) 
                                scale_max = temp_var_val;
                            if (temp_var_val < scale_min)
                                scale_min = temp_var_val;
                        }
                    }

                    if ("radiance_offsets" == (at->get_name(it))) {
                        vector<string> *avalue = at->get_attr_vector(it);
                        for (vector<string>::const_iterator ait = avalue->begin();ait !=avalue->end();++ait) {
                            temp_var_val = (float)(atof((*ait).c_str())); 
                            if (temp_var_val > offset_max) 
                                offset_max = temp_var_val;
                            if (temp_var_val < scale_min)
                                offset_min = temp_var_val;
                        }
                    }

                    if ("long_name" == (at->get_name(it))) {
                        orig_long_name_value = (*at->get_attr_vector(it)->begin());
                        if (orig_long_name_value.find(str_removed_from_long_name)!=string::npos) {
                            if(0 == orig_long_name_value.compare(orig_long_name_value.size()-str_removed_from_long_name.size(), 
                                                                 str_removed_from_long_name.size(),str_removed_from_long_name)) {

                                modify_long_name_value = 
                                                orig_long_name_value.substr(0,orig_long_name_value.size()-str_removed_from_long_name.size()); 
                                at->del_attr("long_name");
                                at->append_attr("long_name","String",modify_long_name_value);
                                at->append_attr("orig_long_name","String",orig_long_name_value);
                            }
                        }
                    }

                    if ("radiance_units" == (at->get_name(it))) 
                        radiance_units_value = (*at->get_attr_vector(it)->begin());
                }

                if (true == is_refsb_field) {
                    if ("reflectance_scales" == (at->get_name(it))) {
                        vector<string> *avalue = at->get_attr_vector(it);
                        for (vector<string>::const_iterator ait = avalue->begin();ait !=avalue->end();++ait) {
                            temp_var_val = (float)(atof((*ait).c_str())); 
                            if (temp_var_val > scale_max) 
                                scale_max = temp_var_val;
                            if (temp_var_val < scale_min)
                                scale_min = temp_var_val;
                        }
                    }

                    if ("reflectance_offsets" == (at->get_name(it))) {

                        vector<string> *avalue = at->get_attr_vector(it);
                        for (vector<string>::const_iterator ait = avalue->begin();ait !=avalue->end();++ait) {
                            temp_var_val = (float)(atof((*ait).c_str())); 
                            if (temp_var_val > offset_max) 
                                offset_max = temp_var_val;
                            if (temp_var_val < scale_min)
                                offset_min = temp_var_val;
                        }
                    }

                    if ("long_name" == (at->get_name(it))) {
                        orig_long_name_value = (*at->get_attr_vector(it)->begin());
                        if (orig_long_name_value.find(str_removed_from_long_name)!=string::npos) {
                            if(0 == orig_long_name_value.compare(orig_long_name_value.size()-str_removed_from_long_name.size(), 
                                                                 str_removed_from_long_name.size(),str_removed_from_long_name)) {

                                modify_long_name_value = 
                                    orig_long_name_value.substr(0,orig_long_name_value.size()-str_removed_from_long_name.size()); 
                                at->del_attr("long_name");
                                at->append_attr("long_name","String",modify_long_name_value);
                                at->append_attr("orig_long_name","String",orig_long_name_value);
                            }
                        }
                    }
                }
                it++;
            }

            // Calculate the final valid_max and valid_min.
            if (scale_min <= 0)
                throw InternalErr(__FILE__,__LINE__,"the scale factor should always be greater than 0.");

            if (orig_valid_max > offset_min) 
                valid_max = (orig_valid_max-offset_min)*scale_max;
            else 
                valid_max = (orig_valid_max-offset_min)*scale_min;

            if (orig_valid_min > offset_max)
                valid_min = (orig_valid_min-offset_max)*scale_min;
            else 
                valid_min = (orig_valid_min -offset_max)*scale_max;

            // These physical variables should be greater than 0
            if (valid_min < 0)
                valid_min = 0;

            string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&valid_min));
            at->append_attr("valid_min","Float32",print_rep);
            print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&valid_max));
            at->append_attr("valid_max","Float32",print_rep);
            at->del_attr("valid_range");  
            handle_modis_l1b = true;
                        
            // Change the units for the emissive field
            if (true == is_emissive_field && radiance_units_value.size() >0) {
                at->del_attr("units");
                at->append_attr("units","String",radiance_units_value);
            }  
        }
    }

    // Handle other valid_range attributes
    if(true == changedtype && true == has_valid_range && false == handle_modis_l1b) {

        // If the gridname_change_valid_range is true, call a special function to handle this.
        if (true == gridname_change_valid_range) 
            HDFCFUtil::handle_modis_vip_special_attrs(valid_range_value,scale_factor_value,valid_min,valid_max);
        else if(scale_factor_value.length()!=0) {

            // We found MODIS products always scale to a smaller value. If somehow the original scale factor
            // is smaller than 1, the scale/offset should be the multiplication rule.(new_data =scale*(old_data-offset))
            // If the original scale factor is greater than 1, the scale/offset rule should be division rule.
            // New_data = (old_data-offset)/scale.
            // We indeed find such a case. HDF-EOS2 Grid MODIS_Grid_1km_2D of MOD(or MYD)09GA is a MODIS_EQ_SCALE.
            // However,
            // the scale_factor of the variable Range_1 in the MOD09GA product is 25. According to our observation,
            // this variable should be MODIS_DIV_SCALE.We verify this is true according to MODIS 09 product document
            // http://modis-sr.ltdri.org/products/MOD09_UserGuide_v1_3.pdf.
            // Since this conclusion is based on our observation, we would like to add a BESlog to detect if we find
            // the similar cases so that we can verify with the corresponding product documents to see if this is true.
            // More information, 
            // We just verified with the data producer, the scale_factor for Range_1 and Range_c is 25 but the
            // equation is still multiplication instead of division. So we have to make this as a special case that
            // we don't want to change the scale and offset settings. 
            // KY 2014-01-13

            if (MODIS_EQ_SCALE == sotype || MODIS_MUL_SCALE == sotype) {
                if (orig_scale_value > 1) {
                  
                    bool need_change_scale = true;
                    if(true == is_grid) {
                        if ((filename.size() >5) && ((filename.compare(0,5,"MOD09") == 0)|| (filename.compare(0,5,"MYD09")==0))) {
                            if ((newfname.size() >5) && newfname.find("Range") != string::npos)
                                need_change_scale = false;
                        }
                    }
                    if(true == need_change_scale) {
                        sotype = MODIS_DIV_SCALE;
                        (*BESLog::TheLog())<< "The field " << newfname << " scale factor is "<< orig_scale_value << endl
                                 << " But the original scale factor type is MODIS_MUL_SCALE or MODIS_EQ_SCALE. " << endl
                                 << " Now change it to MODIS_DIV_SCALE. "<<endl;
                    }
                }
            }

            if (MODIS_DIV_SCALE == sotype) {
                if (orig_scale_value < 1) {
                    sotype = MODIS_MUL_SCALE;
                    (*BESLog::TheLog())<< "The field " << newfname << " scale factor is "<< orig_scale_value << endl
                                 << " But the original scale factor type is MODIS_DIV_SCALE. " << endl
                                 << " Now change it to MODIS_MUL_SCALE. "<<endl;
                }
            }

            if(sotype == MODIS_MUL_SCALE) {
                valid_min = (orig_valid_min - orig_offset_value)*orig_scale_value;
                valid_max = (orig_valid_max - orig_offset_value)*orig_scale_value;
            }
            else if (sotype == MODIS_DIV_SCALE) {
                valid_min = (orig_valid_min - orig_offset_value)/orig_scale_value;
                valid_max = (orig_valid_max - orig_offset_value)/orig_scale_value;
            }
            else if (sotype == MODIS_EQ_SCALE) {
                valid_min = orig_valid_min * orig_scale_value + orig_offset_value;
                valid_max = orig_valid_max * orig_scale_value + orig_offset_value;
            }
        }
        else {// This is our current assumption.
            if (MODIS_MUL_SCALE == sotype || MODIS_DIV_SCALE == sotype) {
                valid_min = orig_valid_min - orig_offset_value;
                valid_max = orig_valid_max - orig_offset_value;
            }
            else if (MODIS_EQ_SCALE == sotype) {
                valid_min = orig_valid_min + orig_offset_value;
                valid_max = orig_valid_max + orig_offset_value;
            }
        }

        // Append the corrected valid_min and valid_max. (valid_min,valid_max) is equivalent to valid_range.
        string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&valid_min));
        at->append_attr("valid_min","Float32",print_rep);
        print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&valid_max));
        at->append_attr("valid_max","Float32",print_rep);
        at->del_attr("valid_range");
    }

    // We also find that there is an attribute called "Number_Type" that will stores the original attribute
    // datatype. If the datatype gets changed, the attribute is confusion. here we can change the attribute
    // name to "Number_Type_Orig"
    if(true == changedtype && number_type_dap_type !="" ) {
        at->del_attr("Number_Type");
        at->append_attr("Number_Type_Orig",number_type_dap_type,number_type_value);
    }
}

 // This routine makes the MeaSUREs VIP attributes follow CF.
// valid_range attribute uses char type instead of the raw data's datatype.
void HDFCFUtil::handle_modis_vip_special_attrs(const std::string& valid_range_value, 
                                               const std::string& scale_factor_value,
                                               float& valid_min, float &valid_max) {

    int16 vip_orig_valid_min = 0;
    int16 vip_orig_valid_max = 0;

    size_t found = valid_range_value.find_first_of(",");
    size_t found_from_end = valid_range_value.find_last_of(",");
    if (string::npos == found) 
        throw InternalErr(__FILE__,__LINE__,"should find the separator ,");
    if (found != found_from_end)
        throw InternalErr(__FILE__,__LINE__,"There should be only one separator.");
                        
    //istringstream(valid_range_value.substr(0,found))>>orig_valid_min;
    //istringstream(valid_range_value.substr(found+1))>>orig_valid_max;

    vip_orig_valid_min = atoi((valid_range_value.substr(0,found)).c_str());
    vip_orig_valid_max = atoi((valid_range_value.substr(found+1)).c_str());

    int16 scale_factor_number = 1;

    //istringstream(scale_factor_value)>>scale_factor_number;
    scale_factor_number = atoi(scale_factor_value.c_str());

    valid_min = (float)(vip_orig_valid_min/scale_factor_number);
    valid_max = (float)(vip_orig_valid_max/scale_factor_number);
}

// Make AMSR-E attributes follow CF.
// Change SCALE_FACTOR and OFFSET to CF names: scale_factor and add_offset.
void HDFCFUtil::handle_amsr_attrs(AttrTable *at) {

    AttrTable::Attr_iter it = at->attr_begin();
    string scale_factor_value="", add_offset_value="0";
    string scale_factor_type, add_offset_type;
    bool OFFSET_found = false;
    bool Scale_found = false;
    bool SCALE_FACTOR_found = false;

    while (it!=at->attr_end()) 
    {
        if(at->get_name(it)=="SCALE_FACTOR")
        {
            scale_factor_value = (*at->get_attr_vector(it)->begin());
            scale_factor_type = at->get_type(it);
            SCALE_FACTOR_found = true;
        }

        if(at->get_name(it)=="Scale")
        {
            scale_factor_value = (*at->get_attr_vector(it)->begin());
            scale_factor_type = at->get_type(it);
            Scale_found = true;
        }

        if(at->get_name(it)=="OFFSET")
        {
            add_offset_value = (*at->get_attr_vector(it)->begin());
            add_offset_type = at->get_type(it);
            OFFSET_found = true;
        }
        it++;
    }

    if (true == SCALE_FACTOR_found) {
        at->del_attr("SCALE_FACTOR");
        at->append_attr("scale_factor",scale_factor_type,scale_factor_value);
    }

    if (true == Scale_found) {
        at->del_attr("Scale");
        at->append_attr("scale_factor",scale_factor_type,scale_factor_value);
    }

    if (true == OFFSET_found) {
        at->del_attr("OFFSET");
        at->append_attr("add_offset",add_offset_type,add_offset_value);
    }

}

#endif

 // Check OBPG attributes. Specifically, check if slope and intercept can be obtained from the file level. 
 // If having global slope and intercept,  obtain OBPG scaling, slope and intercept values.
void HDFCFUtil::check_obpg_global_attrs(HDFSP::File *f, std::string & scaling, 
                                        float & slope,bool &global_slope_flag,
                                        float & intercept, bool & global_intercept_flag){

    HDFSP::SD* spsd = f->getSD();

    for(vector<HDFSP::Attribute *>::const_iterator i=spsd->getAttributes().begin();i!=spsd->getAttributes().end();i++) {
       
        //We want to add two new attributes, "scale_factor" and "add_offset" to data fields if the scaling equation is linear. 
        // OBPG products use "Slope" instead of "scale_factor", "intercept" instead of "add_offset". "Scaling" describes if the equation is linear.
        // Their values will be copied directly from File attributes. This addition is for OBPG L3 only.
        // We also add OBPG L2 support since all OBPG level 2 products(CZCS,MODISA,MODIST,OCTS,SeaWiFS) we evaluate use Slope,intercept and linear equation
        // for the final data. KY 2012-09-06
	if(f->getSPType()==OBPGL3 || f->getSPType() == OBPGL2)
        {
            if((*i)->getName()=="Scaling")
            {
                string tmpstring((*i)->getValue().begin(), (*i)->getValue().end());
                scaling = tmpstring;
            }
            if((*i)->getName()=="Slope" || (*i)->getName()=="slope")
            {
                global_slope_flag = true;
			
                switch((*i)->getType())
                {
#define GET_SLOPE(TYPE, CAST) \
    case DFNT_##TYPE: \
    { \
        CAST tmpvalue = *(CAST*)&((*i)->getValue()[0]); \
        slope = (float)tmpvalue; \
    } \
    break;
                    GET_SLOPE(INT16,   int16);
                    GET_SLOPE(INT32,   int32);
                    GET_SLOPE(FLOAT32, float);
                    GET_SLOPE(FLOAT64, double);
#undef GET_SLOPE
                } ;
                //slope = *(float*)&((*i)->getValue()[0]);
            }
            if((*i)->getName()=="Intercept" || (*i)->getName()=="intercept")
            {	
                global_intercept_flag = true;
                switch((*i)->getType())
                {
#define GET_INTERCEPT(TYPE, CAST) \
    case DFNT_##TYPE: \
    { \
        CAST tmpvalue = *(CAST*)&((*i)->getValue()[0]); \
        intercept = (float)tmpvalue; \
    } \
    break;
                    GET_INTERCEPT(INT16,   int16);
                    GET_INTERCEPT(INT32,   int32);
                    GET_INTERCEPT(FLOAT32, float);
                    GET_INTERCEPT(FLOAT64, double);
#undef GET_INTERCEPT
                } ;
                //intercept = *(float*)&((*i)->getValue()[0]);
            }
        }
    }
}

// For some OBPG files that only provide slope and intercept at the file level, 
// global slope and intercept are needed to add to all fields and their names are needed to be changed to scale_factor and add_offset.
// For OBPG files that provide slope and intercept at the field level,  slope and intercept are needed to rename to scale_factor and add_offset.
void HDFCFUtil::add_obpg_special_attrs(HDFSP::File*f,DAS &das,
                                       HDFSP::SDField *onespsds, 
                                       string& scaling, float& slope, 
                                       bool& global_slope_flag, 
                                       float& intercept,
                                       bool & global_intercept_flag) {

    AttrTable *at = das.get_table(onespsds->getNewName());
    if (!at)
        at = das.add_table(onespsds->getNewName(), new AttrTable);

    //For OBPG L2 and L3 only.Some OBPG products put "slope" and "Intercept" etc. in the field attributes
    // We still need to handle the scale and offset here.
    bool scale_factor_flag = false;
    bool add_offset_flag = false;
    bool slope_flag = false;
    bool intercept_flag = false;

    if(f->getSPType()==OBPGL3 || f->getSPType() == OBPGL2) {// Begin OPBG CF attribute handling(Checking "slope" and "intercept") 
        for(vector<HDFSP::Attribute *>::const_iterator i=onespsds->getAttributes().begin();
                                                       i!=onespsds->getAttributes().end();i++) {
            if(global_slope_flag != true && ((*i)->getName()=="Slope" || (*i)->getName()=="slope"))
            {
                slope_flag = true;
			
                switch((*i)->getType())
                {
#define GET_SLOPE(TYPE, CAST) \
    case DFNT_##TYPE: \
    { \
        CAST tmpvalue = *(CAST*)&((*i)->getValue()[0]); \
        slope = (float)tmpvalue; \
    } \
    break;

                GET_SLOPE(INT16,   int16);
                GET_SLOPE(INT32,   int32);
                GET_SLOPE(FLOAT32, float);
                GET_SLOPE(FLOAT64, double);
#undef GET_SLOPE
                } ;
                    //slope = *(float*)&((*i)->getValue()[0]);
            }
            if(global_intercept_flag != true && ((*i)->getName()=="Intercept" || (*i)->getName()=="intercept"))
            {	
                intercept_flag = true;
                switch((*i)->getType())
                {
#define GET_INTERCEPT(TYPE, CAST) \
    case DFNT_##TYPE: \
    { \
        CAST tmpvalue = *(CAST*)&((*i)->getValue()[0]); \
        intercept = (float)tmpvalue; \
    } \
    break;
                GET_INTERCEPT(INT16,   int16);
                GET_INTERCEPT(INT32,   int32);
                GET_INTERCEPT(FLOAT32, float);
                GET_INTERCEPT(FLOAT64, double);
#undef GET_INTERCEPT
                } ;
                    //intercept = *(float*)&((*i)->getValue()[0]);
            }
        }
    } // End of checking "slope" and "intercept"

    // Checking if OBPG has "scale_factor" ,"add_offset", generally checking for "long_name" attributes.
    for(vector<HDFSP::Attribute *>::const_iterator i=onespsds->getAttributes().begin();i!=onespsds->getAttributes().end();i++) {       

        if((f->getSPType()==OBPGL3 || f->getSPType() == OBPGL2)  && (*i)->getName()=="scale_factor")
            scale_factor_flag = true;		

        if((f->getSPType()==OBPGL3 || f->getSPType() == OBPGL2) && (*i)->getName()=="add_offset")
            add_offset_flag = true;
    }
        
    // Checking if the scale and offset equation is linear, this is only for OBPG level 3.
    // Also checking if having the fill value and adding fill value if not having one for some OBPG data type
    if((f->getSPType() == OBPGL2 || f->getSPType()==OBPGL3 )&& onespsds->getFieldType()==0)
    {
            
        if((OBPGL3 == f->getSPType() && (scaling.find("linear")!=string::npos)) || OBPGL2 == f->getSPType() ) {

            if(false == scale_factor_flag && (true == slope_flag || true == global_slope_flag))
            {
                string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32, 0, (void*)&(slope));
                at->append_attr("scale_factor", HDFCFUtil::print_type(DFNT_FLOAT32), print_rep);
            }

            if(false == add_offset_flag && (true == intercept_flag || true == global_intercept_flag))
            {
                string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32, 0, (void*)&(intercept));
                at->append_attr("add_offset", HDFCFUtil::print_type(DFNT_FLOAT32), print_rep);
            }
        }

        bool has_fill_value = false;
        for(vector<HDFSP::Attribute *>::const_iterator i=onespsds->getAttributes().begin();i!=onespsds->getAttributes().end();i++) {
            if ("_FillValue" == (*i)->getNewName()){
                has_fill_value = true; 
                break;
            }
        }
         

        // Add fill value to some type of OBPG data. 16-bit integer, fill value = -32767, unsigned 16-bit integer fill value = 65535
        // This is based on the evaluation of the example files. KY 2012-09-06
        if ((false == has_fill_value) &&(DFNT_INT16 == onespsds->getType())) {
            short fill_value = -32767;
            string print_rep = HDFCFUtil::print_attr(DFNT_INT16,0,(void*)&(fill_value));
            at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_INT16),print_rep);
        }

        if ((false == has_fill_value) &&(DFNT_UINT16 == onespsds->getType())) {
            unsigned short fill_value = 65535;
            string print_rep = HDFCFUtil::print_attr(DFNT_UINT16,0,(void*)&(fill_value));
            at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_UINT16),print_rep);
        }

    }// Finish OBPG handling

}

// Handle HDF4 OTHERHDF products that follow SDS dimension scale model. 
// The special handling of AVHRR data is also included.
void HDFCFUtil::handle_otherhdf_special_attrs(HDFSP::File*f,DAS &das) {

    // For some HDF4 files that follow HDF4 dimension scales, P.O. DAAC's AVHRR files.
    // The "otherHDF" category can almost make AVHRR files work, except
    // that AVHRR uses the attribute name "unit" instead of "units" for latitude and longitude,
    // I have to correct the name to follow CF conventions(using "units"). I won't check
    // the latitude and longitude values since latitude and longitude values for some files(LISO files)   
    // are not in the standard range(0-360 for lon and 0-180 for lat). KY 2011-3-3
    const vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();
    vector<HDFSP::SDField *>::const_iterator it_g;

    if(f->getSPType() == OTHERHDF) {

        bool latflag = false;
        bool latunitsflag = false; //CF conventions use "units" instead of "unit"
        bool lonflag = false;
        bool lonunitsflag = false; // CF conventions use "units" instead of "unit"
        int llcheckoverflag = 0;

        // Here I try to condense the code within just two for loops
        // The outer loop: Loop through all SDS objects
        // The inner loop: Loop through all attributes of this SDS
        // Inside two inner loops(since "units" are common for other fields), 
        // inner loop 1: when an attribute's long name value is L(l)atitude,mark it.
        // inner loop 2: when an attribute's name is units, mark it.
        // Outside the inner loop, when latflag is true, and latunitsflag is false,
        // adding a new attribute called units and the value should be "degrees_north".
        // doing the same thing for longitude.

        for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

            // Ignore ALL coordinate variables if this is "OTHERHDF" case and some dimensions 
            // don't have dimension scale data.
            if ( true == f->Has_Dim_NoScale_Field() && ((*it_g)->getFieldType() !=0) && ((*it_g)->IsDimScale() == false))
                continue;

            // Ignore the empty(no data) dimension variable.
            if (OTHERHDF == f->getSPType() && true == (*it_g)->IsDimNoScale())
                continue;

            AttrTable *at = das.get_table((*it_g)->getNewName());
            if (!at)
                at = das.add_table((*it_g)->getNewName(), new AttrTable);

            for(vector<HDFSP::Attribute *>::const_iterator i=(*it_g)->getAttributes().begin();i!=(*it_g)->getAttributes().end();i++) {
                if((*i)->getType()==DFNT_UCHAR || (*i)->getType() == DFNT_CHAR){

                    if((*i)->getName() == "long_name") {
                        string tempstring2((*i)->getValue().begin(),(*i)->getValue().end());
                        string tempfinalstr= string(tempstring2.c_str());// This may remove some garbage characters
                        if(tempfinalstr=="latitude" || tempfinalstr == "Latitude") // Find long_name latitude
                            latflag = true;
                        if(tempfinalstr=="longitude" || tempfinalstr == "Longitude") // Find long_name latitude
                            lonflag = true;
                    }
                }
            }

            if(latflag) {
                for(vector<HDFSP::Attribute *>::const_iterator i=(*it_g)->getAttributes().begin();i!=(*it_g)->getAttributes().end();i++) {
                    if((*i)->getName() == "units") 
                        latunitsflag = true;
                }
            }

            if(lonflag) {
                for(vector<HDFSP::Attribute *>::const_iterator i=(*it_g)->getAttributes().begin();i!=(*it_g)->getAttributes().end();i++) {
                    if((*i)->getName() == "units") 
                        lonunitsflag = true;
                }
            }
            if(latflag && !latunitsflag){ // No "units" for latitude, add "units"
                at->append_attr("units","String","degrees_north");
                latflag = false;
                latunitsflag = false;
                llcheckoverflag++;
            }

            if(lonflag && !lonunitsflag){ // No "units" for latitude, add "units"
                at->append_attr("units","String","degrees_east");
                lonflag = false;
                latunitsflag = false;
                llcheckoverflag++;
            }
            if(llcheckoverflag ==2) break;

        }

    }

}

// Add  missing CF attributes for non-CV varibles
void
HDFCFUtil::add_missing_cf_attrs(HDFSP::File*f,DAS &das) {


    const vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();
    vector<HDFSP::SDField *>::const_iterator it_g;


    // TRMM level 3 grid
    if(TRMML3A_V6== f->getSPType() || TRMML3C_V6==f->getSPType() || TRMML3S_V7 == f->getSPType() || TRMML3M_V7 == f->getSPType()) {

        for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){
            if((*it_g)->getFieldType() == 0 && (*it_g)->getType()==DFNT_FLOAT32) {

                AttrTable *at = das.get_table((*it_g)->getNewName());
                if (!at)
                    at = das.add_table((*it_g)->getNewName(), new AttrTable);
                string print_rep = "-9999.9";
                at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_FLOAT32),print_rep);

            }
        }

        for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){
            if((*it_g)->getFieldType() == 0 && (((*it_g)->getType()==DFNT_INT32) || ((*it_g)->getType()==DFNT_INT16))) {

                AttrTable *at = das.get_table((*it_g)->getNewName());
                if (!at)
                    at = das.add_table((*it_g)->getNewName(), new AttrTable);
                string print_rep = "-9999";
                if((*it_g)->getType()==DFNT_INT32)
                    at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_INT32),print_rep);
                else 
                    at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_INT16),print_rep);

            }
        }

        // nlayer for TRMM single grid version 7, the units should be "km"
        if(TRMML3S_V7 == f->getSPType()) {
            for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){
                if((*it_g)->getFieldType() == 6 && (*it_g)->getNewName()=="nlayer") {

                    AttrTable *at = das.get_table((*it_g)->getNewName());
                    if (!at)
                        at = das.add_table((*it_g)->getNewName(), new AttrTable);
                    at->append_attr("units","String","km");

                }
                else if((*it_g)->getFieldType() == 4) {

                    if ((*it_g)->getNewName()=="nh3" ||
                        (*it_g)->getNewName()=="ncat3" ||
                        (*it_g)->getNewName()=="nthrshZO" ||
                        (*it_g)->getNewName()=="nthrshHB" ||
                        (*it_g)->getNewName()=="nthrshSRT")
                     {

                        string references =
                           "http://pps.gsfc.nasa.gov/Documents/filespec.TRMM.V7.pdf";
                        string comment;

                        AttrTable *at = das.get_table((*it_g)->getNewName());
                        if (!at)
                            at = das.add_table((*it_g)->getNewName(), new AttrTable);
 
                        if((*it_g)->getNewName()=="nh3") {
                            comment="Index number to represent the fixed heights above the earth ellipsoid,";
                            comment= comment + " at 2, 4, 6 km plus one for path-average.";
                        }

                        else if((*it_g)->getNewName()=="ncat3") {
                            comment="Index number to represent catgories for probability distribution functions.";
                            comment=comment + "Check more information from the references.";
                         }

                         else if((*it_g)->getNewName()=="nthrshZO") 
                            comment="Q-thresholds for Zero order used for probability distribution functions.";

                         else if((*it_g)->getNewName()=="nthrshHB") 
                            comment="Q-thresholds for HB used for probability distribution functions.";

                         else if((*it_g)->getNewName()=="nthrshSRT") 
                            comment="Q-thresholds for SRT used for probability distribution functions.";
                    
                        at->append_attr("comment","String",comment);
                        at->append_attr("references","String",references);

                    }

                }
                
            }


            // 3A26 use special values such as -666, -777,-999 in their fields. 
            // Although the document doesn't provide range for some fields, the meaning of those fields should be greater than 0.
            // So add valid_min = 0 and fill_value = -999 .
            string base_filename;
            size_t last_slash_pos = f->getPath().find_last_of("/");
            if(last_slash_pos != string::npos)
                base_filename = f->getPath().substr(last_slash_pos+1);
            if(""==base_filename)
                base_filename = f->getPath();
            bool t3a26_flag = ((base_filename.find("3A26")!=string::npos)?true:false);
 
            if(true == t3a26_flag) {
            
                for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

                    if((*it_g)->getFieldType() == 0 && ((*it_g)->getType()==DFNT_FLOAT32)) {
                         AttrTable *at = das.get_table((*it_g)->getNewName());
                        if (!at)
                            at = das.add_table((*it_g)->getNewName(), new AttrTable);
                        at->del_attr("_FillValue");
                        at->append_attr("_FillValue","Float32","-999");
                        at->append_attr("valid_min","Float32","0");

                    }
                }
            }

        }

        // nlayer for TRMM single grid version 7, the units should be "km"
        if(TRMML3M_V7 == f->getSPType()) {
            for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

                if((*it_g)->getFieldType() == 4 ) {

                    string references ="http://pps.gsfc.nasa.gov/Documents/filespec.TRMM.V7.pdf";
                    if ((*it_g)->getNewName()=="nh1") {

                        AttrTable *at = das.get_table((*it_g)->getNewName());
                        if (!at)
                            at = das.add_table((*it_g)->getNewName(), new AttrTable);
                
                        string comment="Number of fixed heights above the earth ellipsoid,";
                               comment= comment + " at 2, 4, 6, 10, and 15 km plus one for path-average.";

                        at->append_attr("comment","String",comment);
                        at->append_attr("references","String",references);

                    }
                    if ((*it_g)->getNewName()=="nh3") {

                        AttrTable *at = das.get_table((*it_g)->getNewName());
                        if (!at)
                            at = das.add_table((*it_g)->getNewName(), new AttrTable);
                
                        string comment="Number of fixed heights above the earth ellipsoid,";
                               comment= comment + " at 2, 4, 6 km plus one for path-average.";

                        at->append_attr("comment","String",comment);
                        at->append_attr("references","String",references);

                    }

                    if ((*it_g)->getNewName()=="nang") {

                        AttrTable *at = das.get_table((*it_g)->getNewName());
                        if (!at)
                            at = das.add_table((*it_g)->getNewName(), new AttrTable);
                
                        string comment="Number of fixed incidence angles, at 0, 5, 10 and 15 degree and all angles.";
                        references = "http://pps.gsfc.nasa.gov/Documents/ICSVol4.pdf";

                        at->append_attr("comment","String",comment);
                        at->append_attr("references","String",references);

                    }

                    if ((*it_g)->getNewName()=="ncat2") {

                        AttrTable *at = das.get_table((*it_g)->getNewName());
                        if (!at)
                            at = das.add_table((*it_g)->getNewName(), new AttrTable);
                
                        string comment="Second number of categories for histograms (30). "; 
                        comment=comment + "Check more information from the references.";

                        at->append_attr("comment","String",comment);
                        at->append_attr("references","String",references);

                    }

                }
            }

        }

    }

    // TRMM level 2 swath
    else if(TRMML2_V7== f->getSPType()) {

        string base_filename;
        size_t last_slash_pos = f->getPath().find_last_of("/");
        if(last_slash_pos != string::npos)
            base_filename = f->getPath().substr(last_slash_pos+1);
        if(""==base_filename)
            base_filename = f->getPath();
        bool t2b31_flag = ((base_filename.find("2B31")!=string::npos)?true:false);
        bool t2a21_flag = ((base_filename.find("2A21")!=string::npos)?true:false);
        bool t2a12_flag = ((base_filename.find("2A12")!=string::npos)?true:false);
        // 2A23 is temporarily not supported perhaps due to special fill values
        //bool t2a23_flag = ((base_filename.find("2A23")!=string::npos)?true:false);
        bool t2a25_flag = ((base_filename.find("2A25")!=string::npos)?true:false);
        bool t1c21_flag = ((base_filename.find("1C21")!=string::npos)?true:false);
        bool t1b21_flag = ((base_filename.find("1B21")!=string::npos)?true:false);
        bool t1b11_flag = ((base_filename.find("1B11")!=string::npos)?true:false);
        bool t1b01_flag = ((base_filename.find("1B01")!=string::npos)?true:false);

        // Handle scale and offset 

        // group 1: 2B31,2A12,2A21
        if(t2b31_flag || t2a12_flag || t2a21_flag) { 

            // special for 2B31 
            if(t2b31_flag) {

                for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

         
                    if((*it_g)->getFieldType() == 0 && (*it_g)->getType()==DFNT_INT16) {

                        AttrTable *at = das.get_table((*it_g)->getNewName());
                        if (!at)
                            at = das.add_table((*it_g)->getNewName(), new AttrTable);

                        AttrTable::Attr_iter it = at->attr_begin();
                        while (it!=at->attr_end()) {
                            if(at->get_name(it)=="scale_factor")
                            {
                                // Scale type and value
                                string scale_factor_value=""; 
                                string scale_factor_type;

                                scale_factor_value = (*at->get_attr_vector(it)->begin());
                                scale_factor_type = at->get_type(it);

                                if(scale_factor_type == "Float64") {
                                    double new_scale = 1.0/strtod(scale_factor_value.c_str(),NULL);
                                    at->del_attr("scale_factor");
                                    string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT64,0,(void*)(&new_scale));
                                    at->append_attr("scale_factor", scale_factor_type,print_rep);

                                }
                               
                                if(scale_factor_type == "Float32") {
                                    float new_scale = 1.0/strtof(scale_factor_value.c_str(),NULL);
                                    at->del_attr("scale_factor");
                                    string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&new_scale));
                                    at->append_attr("scale_factor", scale_factor_type,print_rep);

                                }
                            
                                break;
                            }
                            ++it;
                        }
                    }
                }
            }

            // Special for 2A12
            if(t2a12_flag==true) {

                for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

                    if((*it_g)->getFieldType() == 6 && (*it_g)->getNewName()=="nlayer") {

                        AttrTable *at = das.get_table((*it_g)->getNewName());
                        if (!at)
                            at = das.add_table((*it_g)->getNewName(), new AttrTable);
                        at->append_attr("units","String","km");

                    }

                    // signed char maps to int32, so use int32 for the fillvalue.
                    if((*it_g)->getFieldType() == 0 && (*it_g)->getType()==DFNT_INT8) {

                        AttrTable *at = das.get_table((*it_g)->getNewName());
                        if (!at)
                            at = das.add_table((*it_g)->getNewName(), new AttrTable);
                        at->append_attr("_FillValue","Int32","-99");

                    }

                }
            }

 
            // for all 2A12,2A21 and 2B31
            // Add fillvalues for float32 and int32.
            for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){
                if((*it_g)->getFieldType() == 0 && (*it_g)->getType()==DFNT_FLOAT32) {

                    AttrTable *at = das.get_table((*it_g)->getNewName());
                    if (!at)
                        at = das.add_table((*it_g)->getNewName(), new AttrTable);
                    string print_rep = "-9999.9";
                    at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_FLOAT32),print_rep);

                }
            }

            for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

         
                if((*it_g)->getFieldType() == 0 && (*it_g)->getType()==DFNT_INT16) {

                    AttrTable *at = das.get_table((*it_g)->getNewName());
                    if (!at)
                        at = das.add_table((*it_g)->getNewName(), new AttrTable);

                    string print_rep = "-9999";
                    at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_INT32),print_rep);

                }
            }

        }

        // group 2: 2A21 and 2A25.
        else if(t2a21_flag == true || t2a25_flag == true) {

            // 2A25: handle reflectivity and rain rate scales
            if(t2a25_flag == true) {

                unsigned char handle_scale = 0;
                for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

                    if((*it_g)->getFieldType() == 0 && (*it_g)->getType()==DFNT_INT16) {
                               bool has_dBZ = false;
                    bool has_rainrate = false;
                    bool has_scale = false;
                    string scale_factor_value;
                    string scale_factor_type;

                    AttrTable *at = das.get_table((*it_g)->getNewName());
                    if (!at)
                        at = das.add_table((*it_g)->getNewName(), new AttrTable);
                    AttrTable::Attr_iter it = at->attr_begin();
                    while (it!=at->attr_end()) {
                        if(at->get_name(it)=="units"){
                            string units_value = (*at->get_attr_vector(it)->begin());
                            if("dBZ" == units_value) { 
                                has_dBZ = true;
                            }

                            else if("mm/hr" == units_value){
                                has_rainrate = true;
                            }
                        }
                            if(at->get_name(it)=="scale_factor")
                        {
                            scale_factor_value = (*at->get_attr_vector(it)->begin());
                            scale_factor_type = at->get_type(it);
                                has_scale = true;
                        }
                        ++it;
 
                    }
                        
                    if((true == has_rainrate || true == has_dBZ) && true == has_scale) {

                        handle_scale++;
                        short valid_min = 0; 
                        short valid_max = 0;
                                 
                        // Here just use 32-bit floating-point for the scale_factor, should be okay.
                        if(true == has_rainrate) 
                            valid_max = (short)(300*strtof(scale_factor_value.c_str(),NULL));
                        else if(true == has_dBZ) 
                            valid_max = (short)(80*strtof(scale_factor_value.c_str(),NULL));

                        string print_rep = HDFCFUtil::print_attr(DFNT_INT16,0,(void*)(&valid_min));
                        at->append_attr("valid_min","Int16",print_rep);
                        print_rep = HDFCFUtil::print_attr(DFNT_INT16,0,(void*)(&valid_max));
                        at->append_attr("valid_max","Int16",print_rep);

                        at->del_attr("scale_factor");
                        if(scale_factor_type == "Float64") {
                            double new_scale = 1.0/strtod(scale_factor_value.c_str(),NULL);
                            string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT64,0,(void*)(&new_scale));
                            at->append_attr("scale_factor", scale_factor_type,print_rep);

                        }
                               
                        if(scale_factor_type == "Float32") {
                            float new_scale = 1.0/strtof(scale_factor_value.c_str(),NULL);
                            string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&new_scale));
                            at->append_attr("scale_factor", scale_factor_type,print_rep);

                        }


                    }

                    if(2 == handle_scale)
                        break;

                    }
                }
            }
        }

        // 1B21,1C21 and 1B11
        else if(t1b21_flag || t1c21_flag || t1b11_flag) {

            // 1B21,1C21 scale_factor to CF and valid_range for dBm and dBZ.
            if(t1b21_flag || t1c21_flag) {

                for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

                    if((*it_g)->getFieldType() == 0 && (*it_g)->getType()==DFNT_INT16) {

                        bool has_dBm = false;
                        bool has_dBZ = false;

                        AttrTable *at = das.get_table((*it_g)->getNewName());
                        if (!at)
                            at = das.add_table((*it_g)->getNewName(), new AttrTable);
                        AttrTable::Attr_iter it = at->attr_begin();

                        while (it!=at->attr_end()) {
                            if(at->get_name(it)=="units"){
                             
                                string units_value = (*at->get_attr_vector(it)->begin());
                                if("dBm" == units_value) { 
                                    has_dBm = true;
                                    break;
                                }

                                else if("dBZ" == units_value){
                                    has_dBZ = true;
                                    break;
                                }
                            }
                            ++it;
                        }

                        if(has_dBm == true || has_dBZ == true) {
                            it = at->attr_begin();
                            while (it!=at->attr_end()) {
                                if(at->get_name(it)=="scale_factor")
                                {

                                    string scale_value = (*at->get_attr_vector(it)->begin());
                            
                                    if(true == has_dBm) {
                                       short valid_min = (short)(-120 *strtof(scale_value.c_str(),NULL));
                                       short valid_max = (short)(-20 *strtof(scale_value.c_str(),NULL));
                                       string print_rep = HDFCFUtil::print_attr(DFNT_INT16,0,(void*)(&valid_min));
                                       at->append_attr("valid_min","Int16",print_rep);
                                       print_rep = HDFCFUtil::print_attr(DFNT_INT16,0,(void*)(&valid_max));
                                       at->append_attr("valid_max","Int16",print_rep);
                                       break;
                                      
                                    }

                                    else if(true == has_dBZ){
                                       short valid_min = (short)(-20 *strtof(scale_value.c_str(),NULL));
                                       short valid_max = (short)(80 *strtof(scale_value.c_str(),NULL));
                                       string print_rep = HDFCFUtil::print_attr(DFNT_INT16,0,(void*)(&valid_min));
                                       at->append_attr("valid_min","Int16",print_rep);
                                       print_rep = HDFCFUtil::print_attr(DFNT_INT16,0,(void*)(&valid_max));
                                       at->append_attr("valid_max","Int16",print_rep);
                                       break;
 
                                    }
                                }
                                ++it;
                            }

                        }
                    }
                }
            }

            // For all 1B21,1C21 and 1B11 int16-bit products,change scale to follow CF
            // I find that one 1B21 variable binStormHeight has fillvalue -9999,
            // so add _FillValue -9999 for int16-bit variables.
            for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

                if((*it_g)->getFieldType() == 0 && (*it_g)->getType()==DFNT_INT16) {

                    AttrTable *at = das.get_table((*it_g)->getNewName());
                    if (!at)
                        at = das.add_table((*it_g)->getNewName(), new AttrTable);
                    AttrTable::Attr_iter it = at->attr_begin();


                    while (it!=at->attr_end()) {

                        if(at->get_name(it)=="scale_factor")
                        {
                            // Scale type and value
                            string scale_factor_value=""; 
                            string scale_factor_type;

                            scale_factor_value = (*at->get_attr_vector(it)->begin());
                            scale_factor_type = at->get_type(it);

                            if(scale_factor_type == "Float64") {
                                double new_scale = 1.0/strtod(scale_factor_value.c_str(),NULL);
                                at->del_attr("scale_factor");
                                string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT64,0,(void*)(&new_scale));
                                at->append_attr("scale_factor", scale_factor_type,print_rep);

                            }
                               
                            if(scale_factor_type == "Float32") {
                                float new_scale = 1.0/strtof(scale_factor_value.c_str(),NULL);
                                at->del_attr("scale_factor");
                                string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&new_scale));
                                at->append_attr("scale_factor", scale_factor_type,print_rep);

                            }

                            break;
                            
                        }
                        ++it;

                    }

                    at->append_attr("_FillValue","Int16","-9999");

                }
            }
        }

        // For 1B01 product, just add the fillvalue.
        else if(t1b01_flag == true) {
            for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

                if((*it_g)->getFieldType() == 0 && (*it_g)->getType()==DFNT_FLOAT32) {
                                           AttrTable *at = das.get_table((*it_g)->getNewName());
                    if (!at)
                        at = das.add_table((*it_g)->getNewName(), new AttrTable);

                    at->append_attr("_FillValue","Float32","-9999.9");

                }
            }

        }

        AttrTable *at = das.get_table("HDF_GLOBAL");
        if (!at)
            at = das.add_table("HDF_GLOBAL", new AttrTable);
        string references ="http://pps.gsfc.nasa.gov/Documents/filespec.TRMM.V7.pdf";
        string comment="The HDF4 OPeNDAP handler adds _FillValue, valid_min and valid_max for some TRMM level 1 and level 2 products.";
        comment= comment + " It also changes scale_factor to follow CF conventions. ";

        at->append_attr("comment","String",comment);
        at->append_attr("references","String",references);
    
    }

}
                


//
// Many CERES products compose of multiple groups
// There are many fields in CERES data(a few hundred) and the full name(with the additional path)
// is very long. It causes Java clients choken since Java clients append names in the URL
// To improve the performance and to make Java clients access the data, we will ignore 
// the path of these fields. Users can turn off this feature by commenting out the line: H4.EnableCERESMERRAShortName=true
// or can set the H4.EnableCERESMERRAShortName=false
// We still preserve the full path as an attribute in case users need to check them. 
// Kent 2012-6-29
 
void HDFCFUtil::handle_merra_ceres_attrs_with_bes_keys(HDFSP::File *f, DAS &das,const string& filename) {


    string check_ceres_merra_short_name_key="H4.EnableCERESMERRAShortName";
    bool turn_on_ceres_merra_short_name_key= false;
    string base_filename = filename.substr(filename.find_last_of("/")+1);

    turn_on_ceres_merra_short_name_key = HDFCFUtil::check_beskeys(check_ceres_merra_short_name_key);

    bool merra_is_eos2 = false;
    if(0== (base_filename.compare(0,5,"MERRA"))) {

         for (vector < HDFSP::Attribute * >::const_iterator i = 
            f->getSD()->getAttributes ().begin ();
            i != f->getSD()->getAttributes ().end (); ++i) {

            // CHeck if this MERRA file is an HDF-EOS2 or not.
            if(((*i)->getName().compare(0, 14, "StructMetadata" )== 0) ||
                ((*i)->getName().compare(0, 14, "structmetadata" )== 0)) {
                merra_is_eos2 = true;
                break;
            }

        }
    }

    if (true == turn_on_ceres_merra_short_name_key && (CER_ES4 == f->getSPType() || CER_SRB == f->getSPType()
        || CER_CDAY == f->getSPType() || CER_CGEO == f->getSPType() 
        || CER_SYN == f->getSPType() || CER_ZAVG == f->getSPType()
        || CER_AVG == f->getSPType() || (true == merra_is_eos2))) {

        const vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();
        vector<HDFSP::SDField *>::const_iterator it_g;
        for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

            AttrTable *at = das.get_table((*it_g)->getNewName());
            if (!at)
                at = das.add_table((*it_g)->getNewName(), new AttrTable);

            at->append_attr("fullpath","String",(*it_g)->getSpecFullPath());

        }

    }

}


// Handle the attributes when the BES key EnableVdataDescAttr is enabled..
void HDFCFUtil::handle_vdata_attrs_with_desc_key(HDFSP::File*f,libdap::DAS &das) {

    // Check the EnableVdataDescAttr key. If this key is turned on, the handler-added attribute VDdescname and
    // the attributes of vdata and vdata fields will be outputed to DAS. Otherwise, these attributes will
    // not outputed to DAS. The key will be turned off by default to shorten the DAP output. KY 2012-09-18

    string check_vdata_desc_key="H4.EnableVdataDescAttr";
    bool turn_on_vdata_desc_key= false;

    turn_on_vdata_desc_key = HDFCFUtil::check_beskeys(check_vdata_desc_key);

    string VDdescname = "hdf4_vd_desc";
    string VDdescvalue = "This is an HDF4 Vdata.";
    string VDfieldprefix = "Vdata_field_";
    string VDattrprefix = "Vdata_attr_";
    string VDfieldattrprefix ="Vdata_field_attr_";

    // To speed up the performance for handling CERES data, we turn off some CERES vdata fields, this should be resumed in the future version with BESKeys.
    string check_ceres_vdata_key="H4.EnableCERESVdata";
    bool turn_on_ceres_vdata_key= false;
    turn_on_ceres_vdata_key = HDFCFUtil::check_beskeys(check_ceres_vdata_key);
                
    bool output_vdata_flag = true;
    if (false == turn_on_ceres_vdata_key && 
        (CER_AVG == f->getSPType() || 
         CER_ES4 == f->getSPType() ||   
         CER_SRB == f->getSPType() ||
         CER_ZAVG == f->getSPType()))
        output_vdata_flag = false;

    //if(f->getSPType() != CER_AVG && f->getSPType() != CER_ES4 && f->getSPType() !=CER_SRB && f->getSPType() != CER_ZAVG) {
    if (true == output_vdata_flag) {

        for(vector<HDFSP::VDATA *>::const_iterator i=f->getVDATAs().begin(); i!=f->getVDATAs().end();i++) {

            AttrTable *at = das.get_table((*i)->getNewName());
            if(!at)
                at = das.add_table((*i)->getNewName(),new AttrTable);
 
            if (true == turn_on_vdata_desc_key) {
                // Add special vdata attributes
                bool emptyvddasflag = true;
                if(!((*i)->getAttributes().empty())) emptyvddasflag = false;
                if(((*i)->getTreatAsAttrFlag()))
                    emptyvddasflag = false;
                else {
                    for(vector<HDFSP::VDField *>::const_iterator j=(*i)->getFields().begin();j!=(*i)->getFields().end();j++) {
                        if(!((*j)->getAttributes().empty())) {
                            emptyvddasflag = false;
                            break;
                        }
                    }
                }

                if(emptyvddasflag) 
                    continue;
                at->append_attr(VDdescname, "String" , VDdescvalue);

                for(vector<HDFSP::Attribute *>::const_iterator it_va = (*i)->getAttributes().begin();it_va!=(*i)->getAttributes().end();it_va++) {

                    if((*it_va)->getType()==DFNT_UCHAR || (*it_va)->getType() == DFNT_CHAR){

                        string tempstring2((*it_va)->getValue().begin(),(*it_va)->getValue().end());
                        string tempfinalstr= string(tempstring2.c_str());
                        at->append_attr(VDattrprefix+(*it_va)->getNewName(), "String" , HDFCFUtil::escattr(tempfinalstr));
                    }
                    else {
                        for (int loc=0; loc < (*it_va)->getCount() ; loc++) {
                            string print_rep = HDFCFUtil::print_attr((*it_va)->getType(), loc, (void*) &((*it_va)->getValue()[0]));
                            at->append_attr(VDattrprefix+(*it_va)->getNewName(), HDFCFUtil::print_type((*it_va)->getType()), print_rep);
                        }
                    }

                }
            }

            if(false == ((*i)->getTreatAsAttrFlag())){ 

                if (true == turn_on_vdata_desc_key) {

                    //NOTE: for vdata field, we assume that no special characters are found. We need to escape the special characters when the data type is char. 
                    // We need to create a DAS container for each field so that the attributes can be put inside.
                    for(vector<HDFSP::VDField *>::const_iterator j=(*i)->getFields().begin();j!=(*i)->getFields().end();j++) {

                        // This vdata field will NOT be treated as attributes, only save the field attribute as the attribute
                        // First check if the field has attributes, if it doesn't have attributes, no need to create a container.
                        
                        if((*j)->getAttributes().size() !=0) {

                            AttrTable *at_v = das.get_table((*j)->getNewName());                           
                            if(!at_v) 
                                at_v = das.add_table((*j)->getNewName(),new AttrTable);

                            for(vector<HDFSP::Attribute *>::const_iterator it_va = (*j)->getAttributes().begin();it_va!=(*j)->getAttributes().end();it_va++) {

                                if((*it_va)->getType()==DFNT_UCHAR || (*it_va)->getType() == DFNT_CHAR){

                                    string tempstring2((*it_va)->getValue().begin(),(*it_va)->getValue().end());
                                    string tempfinalstr= string(tempstring2.c_str());
                                    at_v->append_attr((*it_va)->getNewName(), "String" , HDFCFUtil::escattr(tempfinalstr));
                                }
                                else {
                                    for (int loc=0; loc < (*it_va)->getCount() ; loc++) {
                                        string print_rep = HDFCFUtil::print_attr((*it_va)->getType(), loc, (void*) &((*it_va)->getValue()[0]));
                                        at_v->append_attr((*it_va)->getNewName(), HDFCFUtil::print_type((*it_va)->getType()), print_rep);
                                    }
                                }

                            }
                        }
                    }
                }

            }

            else {

                for(vector<HDFSP::VDField *>::const_iterator j=(*i)->getFields().begin();j!=(*i)->getFields().end();j++) {
 
                    if((*j)->getFieldOrder() == 1) {
                        if((*j)->getType()==DFNT_UCHAR || (*j)->getType() == DFNT_CHAR){
                            string tempfinalstr;
                            tempfinalstr.resize((*j)->getValue().size());
                            copy((*j)->getValue().begin(),(*j)->getValue().end(),tempfinalstr.begin());
                            at->append_attr(VDfieldprefix+(*j)->getNewName(), "String" , HDFCFUtil::escattr(tempfinalstr));
                        }
                        else {
                            for ( int loc=0; loc < (*j)->getNumRec(); loc++) {
                                string print_rep = HDFCFUtil::print_attr((*j)->getType(), loc, (void*) &((*j)->getValue()[0]));
                                at->append_attr(VDfieldprefix+(*j)->getNewName(), HDFCFUtil::print_type((*j)->getType()), print_rep);
                            }
                        }
                    }
                    else {//When field order is greater than 1,we want to print each record in group with single quote,'0 1 2','3 4 5', etc.

                        if((*j)->getValue().size() != (unsigned int)(DFKNTsize((*j)->getType())*((*j)->getFieldOrder())*((*j)->getNumRec()))){
                            throw InternalErr(__FILE__,__LINE__,"the vdata field size doesn't match the vector value");
                        }

                        if((*j)->getNumRec()==1){
                            if((*j)->getType()==DFNT_UCHAR || (*j)->getType() == DFNT_CHAR){
                                string tempstring2((*j)->getValue().begin(),(*j)->getValue().end());
                                string tempfinalstr= string(tempstring2.c_str());
                                at->append_attr(VDfieldprefix+(*j)->getNewName(),"String",HDFCFUtil::escattr(tempfinalstr));
                            }
                            else {
                                for (int loc=0; loc < (*j)->getFieldOrder(); loc++) {
                                    string print_rep = HDFCFUtil::print_attr((*j)->getType(), loc, (void*) &((*j)->getValue()[0]));
                                    at->append_attr(VDfieldprefix+(*j)->getNewName(), HDFCFUtil::print_type((*j)->getType()), print_rep);
                                }
                            }

                        }

                        else {
                            if((*j)->getType()==DFNT_UCHAR || (*j)->getType() == DFNT_CHAR){

                                for(int tempcount = 0; tempcount < (*j)->getNumRec()*DFKNTsize((*j)->getType());tempcount ++) {
                                    vector<char>::const_iterator tempit;
                                    tempit = (*j)->getValue().begin()+tempcount*((*j)->getFieldOrder());
                                    string tempstring2(tempit,tempit+(*j)->getFieldOrder());
                                    string tempfinalstr= string(tempstring2.c_str());
                                    string tempoutstring = "'"+tempfinalstr+"'";
                                    at->append_attr(VDfieldprefix+(*j)->getNewName(),"String",HDFCFUtil::escattr(tempoutstring));
                                }
                            }

                            else {
                                for(int tempcount = 0; tempcount < (*j)->getNumRec();tempcount ++) {
                                    at->append_attr(VDfieldprefix+(*j)->getNewName(),HDFCFUtil::print_type((*j)->getType()),"'");
                                    for (int loc=0; loc < (*j)->getFieldOrder(); loc++) {
                                        string print_rep = HDFCFUtil::print_attr((*j)->getType(), loc, (void*) &((*j)->getValue()[tempcount*((*j)->getFieldOrder())]));
                                        at->append_attr(VDfieldprefix+(*j)->getNewName(), HDFCFUtil::print_type((*j)->getType()), print_rep);
                                    }
                                    at->append_attr(VDfieldprefix+(*j)->getNewName(),HDFCFUtil::print_type((*j)->getType()),"'");
                                }
                            }
                        }
                    }

         
                    if (true == turn_on_vdata_desc_key) {
                        for(vector<HDFSP::Attribute *>::const_iterator it_va = (*j)->getAttributes().begin();it_va!=(*j)->getAttributes().end();it_va++) {

                            if((*it_va)->getType()==DFNT_UCHAR || (*it_va)->getType() == DFNT_CHAR){

                                string tempstring2((*it_va)->getValue().begin(),(*it_va)->getValue().end());
                                string tempfinalstr= string(tempstring2.c_str());
                                at->append_attr(VDfieldattrprefix+(*it_va)->getNewName(), "String" , HDFCFUtil::escattr(tempfinalstr));
                            }
                            else {
                                for (int loc=0; loc < (*it_va)->getCount() ; loc++) {
                                    string print_rep = HDFCFUtil::print_attr((*it_va)->getType(), loc, (void*) &((*it_va)->getValue()[0]));
                                    at->append_attr(VDfieldattrprefix+(*it_va)->getNewName(), HDFCFUtil::print_type((*it_va)->getType()), print_rep);
                                }
                            }
                        }
                    }
                }
            }

        }
    } 

}


string HDFCFUtil::escattr(string s)
{
    const string printable = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789~`!@#$%^&*()_-+={[}]|\\:;<,>.?/'\"\n\t\r";
    const string ESC = "\\";
    const string DOUBLE_ESC = ESC + ESC;
    const string QUOTE = "\"";
    const string ESCQUOTE = ESC + QUOTE;


    // escape \ with a second backslash
    string::size_type ind = 0;
    while ((ind = s.find(ESC, ind)) != s.npos) {
        s.replace(ind, 1, DOUBLE_ESC);
        ind += DOUBLE_ESC.length();
    }

    // escape non-printing characters with octal escape
    ind = 0;
    while ((ind = s.find_first_not_of(printable, ind)) != s.npos)
        s.replace(ind, 1, ESC + octstring(s[ind]));

    // escape " with backslash
    ind = 0;
    while ((ind = s.find(QUOTE, ind)) != s.npos) {
        s.replace(ind, 1, ESCQUOTE);
        ind += ESCQUOTE.length();
    }

    return s;
}


void HDFCFUtil::parser_trmm_v7_gridheader(const vector<char>& value, 
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
     HDFCFUtil::Split(&value[0],sep,ind_elems);
     
     /* The number of elements in the GridHeader is 9. The string vector will add a leftover. So the size should be 10.*/
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

// Somehow the conversion of double to c++ string with sprintf causes the memory error in
// the testing code. I used the following code to do the conversion. Most part of the code
// in reverse, int_to_str and dtoa are adapted from geeksforgeeks.org

// reverses a string 'str' of length 'len'
void HDFCFUtil::rev_str(char *str, int len)
{
    int i=0;
    int j=len-1;
    int temp = 0;
    while (i<j)
    {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++; 
        j--;
    }
}
 
// Converts a given integer x to string str[].  d is the number
// of digits required in output. If d is more than the number
// of digits in x, then 0s are added at the beginning.
int HDFCFUtil::int_to_str(int x, char str[], int d)
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
void HDFCFUtil::dtoa(double n, char *res, int afterpoint)
{
    // Extract integer part
    int ipart = (int)n;
 
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
        int final_fpart = (int)fpart;
        if(fpart -(int)fpart >0.5)
            final_fpart = (int)fpart +1;
        int_to_str(final_fpart, res + i + 1, afterpoint);
    }
}


string HDFCFUtil::get_double_str(double x,int total_digit,int after_point) {
    
    string str;
    if(x!=0) {
        char res[total_digit];
        for(int i = 0; i<total_digit;i++)
           res[i] = '\0';
        if (x<0) { 
           str.push_back('-');
           dtoa(-x,res,after_point);
           for(int i = 0; i<total_digit;i++) {
               if(res[i] != '\0')
                  str.push_back(res[i]);
           }
        }
        else {
           dtoa(x, res, after_point);
           for(int i = 0; i<total_digit;i++) {
               if(res[i] != '\0')
                  str.push_back(res[i]);
           }
        }
    
    }
    else 
       str.push_back('0');
        
//std::cerr<<"str length is "<<str.size() <<std::endl;
//std::cerr<<"str is "<<str <<std::endl;
    return str;

}

string HDFCFUtil::get_int_str(int x) {

   string str;
   if(x > 0 && x <10)   
      str.push_back(x+'0');
   
   else if (x >10 && x<100) {
      str.push_back(x/10+'0');
      str.push_back(x%10+'0');
   }
   else {
      int num_digit = 0;
      int abs_x = (x<0)?-x:x;
      while(abs_x/=10) 
         num_digit++;
      if(x<=0)
         num_digit++;
      char buf[num_digit];
      sprintf(buf,"%d",x);
      str.assign(buf);

   }      

//cerr<<"int str is "<<str<<endl;
   return str;

}
 
#if 0
template<typename T>
size_t HDFCFUtil::write_vector_to_file(const string & fname, const vector<T> &val, size_t dtypesize) {
#endif
#if 0
size_t HDFCFUtil::write_vector_to_file(const string & fname, const vector<double> &val, size_t dtypesize) {

    size_t ret_val;
    FILE* pFile;
cerr<<"Open a file with the name "<<fname<<endl;
    pFile = fopen(fname.c_str(),"wb");
    ret_val = fwrite(&val[0],dtypesize,val.size(),pFile);
cerr<<"ret_val for write is "<<ret_val <<endl;
//for (int i=0;i<val.size();i++)
//cerr<<"val["<<i<<"] is "<<val[i]<<endl;
    fclose(pFile);
    return ret_val;
}
ssize_t HDFCFUtil::write_vector_to_file2(const string & fname, const vector<double> &val, size_t dtypesize) {

    ssize_t ret_val;
    //int fd = open(fname.c_str(),O_RDWR|O_CREAT|O_TRUNC,0666);
    int fd = open(fname.c_str(),O_RDWR|O_CREAT|O_EXCL,0666);
cerr<<"The first val is "<<val[0] <<endl;
    ret_val = write(fd,&val[0],dtypesize*val.size());
    close(fd);
cerr<<"ret_val for write is "<<ret_val <<endl;
//for (int i=0;i<val.size();i++)
//cerr<<"val["<<i<<"] is "<<val[i]<<endl;
    return ret_val;
}
#endif
ssize_t HDFCFUtil::read_vector_from_file(int fd, vector<double> &val, size_t dtypesize) {

    ssize_t ret_val;
    //FILE* pFile;
    ret_val = read(fd,&val[0],val.size()*dtypesize);
    
#if 0
cerr<<"Open a file with the name "<<fname<<endl;
    pFile = fopen(fname.c_str(),"wb");
    ret_val = fwrite(&val[0],dtypesize,val.size(),pFile);
cerr<<"ret_val for write is "<<ret_val <<endl;
//for (int i=0;i<val.size();i++)
//cerr<<"val["<<i<<"] is "<<val[i]<<endl;
    fclose(pFile);
#endif
    return ret_val;
}

void HDFCFUtil::close_fileid(int32 sdfd, int32 fileid,int32 gridfd, int32 swathfd,bool pass_fileid) {

    if(false == pass_fileid) {
        if(sdfd != -1)
            SDend(sdfd);
        if(fileid != -1)
            Hclose(fileid);
#ifdef USE_HDFEOS2_LIB
        if(gridfd != -1)
            GDclose(gridfd);
        if(swathfd != -1)
            SWclose(swathfd);
        
#endif
    }

}
#if 0
void HDFCFUtil::close_fileid(int32 sdfd, int32 fileid,int32 gridfd, int32 swathfd) {

    if(sdfd != -1)
        SDend(sdfd);
    if(fileid != -1)
        Hclose(fileid);
    if(gridfd != -1)
        GDclose(gridfd);
    if(swathfd != -1)
        SWclose(swathfd);

}

void HDFCFUtil::reset_fileid(int& sdfd, int& fileid,int& gridfd, int& swathfd) {

    sdfd   = -1;
    fileid = -1;
    gridfd = -1;
    swathfd = -1;

}
#endif
