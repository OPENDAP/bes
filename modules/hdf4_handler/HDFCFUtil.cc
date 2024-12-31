#include "config.h"
#include "config_hdf.h"

#include "HDFCFUtil.h"
#include <BESDebug.h>
#include <BESLog.h>
#include <math.h>
#include"dodsutil.h"
#include"HDFSPArray_RealField.h"
#include"HDFSPArrayMissField.h"
#include"HDFEOS2GeoCFProj.h"
#include"HDFEOS2GeoCF1D.h"

#include <libdap/debug.h>

#define SIGNED_BYTE_TO_INT32 1

// HDF datatype headers for both the default and the CF options
#include "HDFByte.h"
#include "HDFInt16.h"
#include "HDFUInt16.h"
#include "HDFInt32.h"
#include "HDFUInt32.h"
#include "HDFFloat32.h"
#include "HDFFloat64.h"
#include "HDFStr.h"
#include "HDF4RequestHandler.h"

//
using namespace std;
using namespace libdap;

#define ERR_LOC1(x) #x
#define ERR_LOC2(x) ERR_LOC1(x)
#define ERR_LOC __FILE__ " : " ERR_LOC2(__LINE__)


/**
 * Copied from stack overflow because valgrind finds the code
 * below (Split) overruns memory, generating errors. jhrg 6/22/15
 */
static void
split_helper(vector<string> &tokens, const string &text, const char sep)
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
HDFCFUtil::Split(const char *s, int len, char sep, std::vector<std::string> &names)
{
    names.clear();
    split_helper(names, string(s, len), sep);
}

// Assume sz is Null terminated string.
void
HDFCFUtil::Split(const char *sz, char sep, std::vector<std::string> &names)
{
    names.clear();

    // Split() was showing up in some valgrind runs as having an off-by-one
    // error. I added this help track it down.
    DBG(std::cerr << "HDFCFUtil::Split: sz: <" << sz << ">, sep: <" << sep << ">" << std::endl);

    split_helper(names, string(sz), sep);
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

    for(auto &si: s)
        if((false == isalnum(si)) &&  (si!='_'))
            si='_';

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

    // clash index to original index mapping
    map<int,int> cl_to_ol;
    int ol_index = 0;
    int cl_index = 0;


    for (const auto &newobjname:newobjnamelist) {
        setret = objnameset.insert(newobjname);
        if (false == setret.second ) {
            clashnamelist.insert(clashnamelist.end(),newobjname);
            cl_to_ol[cl_index] = ol_index;
            cl_index++;
        }
        ol_index++;
    }

    // Now change the clashed elements to unique elements,
    // Generate the set which has the same size as the original vector.
    for (auto &clashname:clashnamelist) {
        int clash_index = 1;
        string temp_clashname = clashname +'_';
        HDFCFUtil::gen_unique_name(temp_clashname,objnameset,clash_index);
        clashname = temp_clashname;
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
            string tmp_str = static_cast<const char*>(vals);
            return tmp_str;
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
            float attr_val = *(float*)vals;
            bool is_a_fin = isfinite(attr_val);
            gp.fp = (float *) vals;
            rep << showpoint;
            // setprecision seeme to cause the one-bit error when 
            // converting from float to string. Watch whether this
            // is an isue.
            rep << setprecision(10);
            rep << *(gp.fp+loc);
            string tmp_rep_str = rep.str();
            if (tmp_rep_str.find('.') == string::npos
                && tmp_rep_str.find('e') == string::npos
                && tmp_rep_str.find('E') == string::npos) {
                if(true == is_a_fin)
                    rep << ".";
            }
            return rep.str();
        }

    case DFNT_DOUBLE:
        {

            double attr_val = *(double*)vals;
            bool is_a_fin = isfinite(attr_val);
            gp.dp = (double *) vals;
            rep << std::showpoint;
            rep << std::setprecision(17);
            rep << *(gp.dp+loc);
            string tmp_rep_str = rep.str();
            if (tmp_rep_str.find('.') == string::npos
                && tmp_rep_str.find('e') == string::npos
                && tmp_rep_str.find('E') == string::npos) {
                if(true == is_a_fin)
                    rep << ".";
            }
            return rep.str();
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

// Obtain HDF4 datatype size. 
short
HDFCFUtil::obtain_type_size(int32 type)
{

    switch (type) {

    case DFNT_CHAR:
        return sizeof(char);

    case DFNT_UCHAR8:
        return sizeof(unsigned char);

    case DFNT_UINT8:
        return sizeof(unsigned char);

    case DFNT_INT8:
// ADD the macro
    {
#ifndef SIGNED_BYTE_TO_INT32
        return sizeof(char);
#else
        return sizeof(int);
#endif
    }
    case DFNT_UINT16:
        return sizeof(unsigned short);

    case DFNT_INT16:
        return sizeof(short);

    case DFNT_INT32:
        return sizeof(int);

    case DFNT_UINT32:
        return sizeof(unsigned int);

    case DFNT_FLOAT:
        return sizeof(float);

    case DFNT_DOUBLE:
        return sizeof(double);

    default:
        return -1;
    }

}
// Subset of latitude and longitude to follow the parameters from the DAP expression constraint
// Somehow this function doesn't work. Now it is not used. Still keep it here for the future investigation.
template < typename T >
void HDFCFUtil::LatLon2DSubset (T * outlatlon,
                                int majordim,
                                int minordim,
                                T * latlon,
                                const int32 * offset,
                                const int32 * count,
                                const int32 * step)
{

    T (*templatlonptr)[majordim][minordim] = (T *) latlon;
    int i;
    int j; 
    int k;

    // do subsetting
    // Find the correct index
    int dim0count = count[0];
    int dim1count = count[1];
    vector<int> dim0index(dim0count);
    vector<int> dim1index(dim1count);

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
                    fillvalue = escattr_fvalue(fillvalue);
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
                        // Note, the code will only assume the value ranges from 0 to 255.(JIRA HFRHANDLER-248).
                        // KY 2014-04-24
                          
                        short fillvalue_int = fillvalue.at(0);

                        stringstream convert_str;
                        convert_str << fillvalue_int;
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

        auto fillvalue_int = (int)fillvalue;
        if (MAX_NON_SCALE_SPECIAL_VALUE == fillvalue_int) {
            auto realvalue_int = (int)realvalue;
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

    if(scaletype!=SOType::DEFAULT_CF_EQU && at!=nullptr)
    {
        AttrTable::Attr_iter it = at->attr_begin();
        string  scale_factor_value="";
        string  add_offset_value="0";
        string  radiance_scales_value="";
        string  radiance_offsets_value="";
        string  reflectance_scales_value=""; 
        string  reflectance_offsets_value="";
        string  scale_factor_type;
        string  add_offset_type;

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

        if((radiance_scales_value.size()!=0 && radiance_offsets_value.size()!=0)
            || (reflectance_scales_value.size()!=0 && reflectance_offsets_value.size()!=0))
            return true;
		
        if(scale_factor_value.size()!=0)
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


    auto sw = static_cast<HDFEOS2::SwathDataset *>(dataset);
    const vector<HDFEOS2::SwathDataset::DimensionMap*>& origdimmaps = sw->getDimensionMaps();
    struct dimmap_entry tempdimmap;

    // if having dimension maps, we need to retrieve the dimension map info.
    for(const auto & origdimmap:origdimmaps){
        tempdimmap.geodim = origdimmap->getGeoDimension();
        tempdimmap.datadim = origdimmap->getDataDimension();
        tempdimmap.offset = origdimmap->getOffset();
        tempdimmap.inc    = origdimmap->getIncrement();
        dimmaps.push_back(tempdimmap);
    }

    // Only when there is dimension map, we need to consider the additional MODIS geolocation files.
    // Will check if the check modis_geo_location file key is turned on.
    if((origdimmaps.empty() != false) && (true == HDF4RequestHandler::get_enable_check_modis_geo_file()) ) {

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
                        if (nullptr == dirp) 
                            throw InternalErr(__FILE__,__LINE__,"opendir fails.");

                        while ((dirs = readdir(dirp))!= nullptr){
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

    modis_dimmap_nonll_fieldlist.emplace_back("Height");
    modis_dimmap_nonll_fieldlist.emplace_back("SensorZenith");
    modis_dimmap_nonll_fieldlist.emplace_back("SensorAzimuth");
    modis_dimmap_nonll_fieldlist.emplace_back("Range");
    modis_dimmap_nonll_fieldlist.emplace_back("SolarZenith");
    modis_dimmap_nonll_fieldlist.emplace_back("SolarAzimuth");
    modis_dimmap_nonll_fieldlist.emplace_back("Land/SeaMask");
    modis_dimmap_nonll_fieldlist.emplace_back("gflags");
    modis_dimmap_nonll_fieldlist.emplace_back("Solar_Zenith");
    modis_dimmap_nonll_fieldlist.emplace_back("Solar_Azimuth");
    modis_dimmap_nonll_fieldlist.emplace_back("Sensor_Azimuth");
    modis_dimmap_nonll_fieldlist.emplace_back("Sensor_Zenith");

    map<string,string>modis_field_to_geofile_field;
    map<string,string>::iterator itmap;
    modis_field_to_geofile_field["Solar_Zenith"] = "SolarZenith";
    modis_field_to_geofile_field["Solar_Azimuth"] = "SolarAzimuth";
    modis_field_to_geofile_field["Sensor_Zenith"] = "SensorZenith";
    modis_field_to_geofile_field["Solar_Azimuth"] = "SolarAzimuth";

    for (const auto & modis_dimmap_nonll_f:modis_dimmap_nonll_fieldlist) {

        if (fieldname == modis_dimmap_nonll_f) {
            itmap = modis_field_to_geofile_field.find(fieldname);
            if (itmap !=modis_field_to_geofile_field.end())
                fieldname = itmap->second;
            modis_dimmap_nonll_field = true;
            break;
        }
    }

    return modis_dimmap_nonll_field;
}

void HDFCFUtil::add_scale_offset_attrs(AttrTable*at,const std::string& s_type, float svalue_f, double svalue_d, bool add_offset_found,
                                       const std::string& o_type, float ovalue_f, double ovalue_d) {
    at->del_attr("scale_factor");
    string print_rep;
    if(s_type!="Float64") {
        print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&svalue_f));
        at->append_attr("scale_factor", "Float32", print_rep);
    }
    else { 
        print_rep = HDFCFUtil::print_attr(DFNT_FLOAT64,0,(void*)(&svalue_d));
        at->append_attr("scale_factor", "Float64", print_rep);
    }
               
    if (true == add_offset_found) {
        at->del_attr("add_offset");
        if(o_type!="Float64") {
            print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&ovalue_f));
            at->append_attr("add_offset", "Float32", print_rep);
        }
        else {
            print_rep = HDFCFUtil::print_attr(DFNT_FLOAT64,0,(void*)(&ovalue_d));
            at->append_attr("add_offset", "Float64", print_rep);
        }
    }
}

void HDFCFUtil::add_scale_str_offset_attrs(AttrTable*at,const std::string& s_type, const std::string& s_value_str, bool add_offset_found,
                                       const std::string& o_type, float ovalue_f, double ovalue_d) {
    at->del_attr("scale_factor");
    string print_rep;
    if(s_type!="Float64") 
        at->append_attr("scale_factor", "Float32", s_value_str);
    else 
        at->append_attr("scale_factor", "Float64", s_value_str);
               
    if (true == add_offset_found) {
        at->del_attr("add_offset");
        if(o_type!="Float64") {
            print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&ovalue_f));
            at->append_attr("add_offset", "Float32", print_rep);
        }
        else {
            print_rep = HDFCFUtil::print_attr(DFNT_FLOAT64,0,(void*)(&ovalue_d));
            at->append_attr("add_offset", "Float64", print_rep);
        }
    }
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
    float  orig_scale_value_float = 1;
    double  orig_scale_value_double = 1;
    string add_offset_value="0"; 
    float  orig_offset_value_float = 0;
    double  orig_offset_value_double = 0;
    bool add_offset_found = false;


    // Go through all attributes to find scale_factor,add_offset,_FillValue,valid_range
    // Number_Type information
    AttrTable::Attr_iter it = at->attr_begin();
    while (it!=at->attr_end())
    {
        if(at->get_name(it)=="scale_factor")
        {
            scale_factor_value = (*at->get_attr_vector(it)->begin());
            scale_factor_type = at->get_type(it);
            if(scale_factor_type =="Float64")
                orig_scale_value_double=atof(scale_factor_value.c_str());
            else 
                orig_scale_value_float = (float)(atof(scale_factor_value.c_str()));
        }

        if(at->get_name(it)=="add_offset")
        {
            add_offset_value = (*at->get_attr_vector(it)->begin());
            add_offset_type = at->get_type(it);

            if(add_offset_type == "Float64") 
                orig_offset_value_double = atof(add_offset_value.c_str());
            else 
                orig_offset_value_float = (float)(atof(add_offset_value.c_str()));
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

    
    if(scale_factor_value.size()!=0) {
        if (SOType::MODIS_EQ_SCALE == sotype || SOType::MODIS_MUL_SCALE == sotype) {
            if (orig_scale_value_float > 1 || orig_scale_value_double >1) {

                bool need_change_scale = true;
                if(true == is_grid) {
                    if ((filename.size() >5) && ((filename.compare(0,5,"MOD09") == 0)|| (filename.compare(0,5,"MYD09")==0))) {
                        if ((newfname.size() >5) && newfname.find("Range") != string::npos)
                            need_change_scale = false;
                    }
                    else if((filename.size() >7)&&((filename.compare(0,7,"MOD16A2") == 0)|| (filename.compare(0,7,"MYD16A2")==0) ||
                            (filename.compare(0,7,"MOD16A3")==0) || (filename.compare(0,7,"MYD16A3")==0))) 
                        need_change_scale = false;
                }
                if(true == need_change_scale)  {
                    sotype = SOType::MODIS_DIV_SCALE;
                    INFO_LOG("The field " + newfname + " scale factor is "+ scale_factor_value
                               + " But the original scale factor type is MODIS_MUL_SCALE or MODIS_EQ_SCALE. "
                               + " Now change it to MODIS_DIV_SCALE. ");
                }
            }
        }

        if (SOType::MODIS_DIV_SCALE == sotype) {
            if (orig_scale_value_float < 1 || orig_scale_value_double<1) {
                sotype = SOType::MODIS_MUL_SCALE;
                INFO_LOG("The field " + newfname + " scale factor is "+ scale_factor_value
                           + " But the original scale factor type is MODIS_DIV_SCALE. "
                           + " Now change it to MODIS_MUL_SCALE. ");
            }
        }


        if ((SOType::MODIS_MUL_SCALE == sotype) &&(true == add_offset_found)) {

            float new_offset_value_float=0;
            double new_offset_value_double=0;
            if(add_offset_type!="Float64")
                new_offset_value_float = (orig_offset_value_float ==0)?0:(-1 * orig_offset_value_float *orig_scale_value_float);
            else 
                new_offset_value_double = (orig_offset_value_double ==0)?0:(-1 * orig_offset_value_double *orig_scale_value_double);

            // May need to use another function to avoid the rounding error by atof.
            add_scale_str_offset_attrs(at,scale_factor_type,scale_factor_value,add_offset_found,
                                   add_offset_type,new_offset_value_float,new_offset_value_double);
      
        }    

        if (SOType::MODIS_DIV_SCALE == sotype) {

            float new_scale_value_float=1;
            double new_scale_value_double=1;
            float new_offset_value_float=0;
            double new_offset_value_double=0;
 

            if(scale_factor_type !="Float64") {
                new_scale_value_float = 1.0f/orig_scale_value_float;
                if (true == add_offset_found) {
                    if(add_offset_type !="Float64") 
                        new_offset_value_float = (orig_offset_value_float==0)?0:(-1 * orig_offset_value_float *new_scale_value_float); 
                    else 
                        new_offset_value_double = (orig_offset_value_double==0)?0:(-1 * orig_offset_value_double *new_scale_value_float); 
                }
            }

            else {
                new_scale_value_double = 1.0/orig_scale_value_double;
                if (true == add_offset_found) {
                    if(add_offset_type !="Float64") 
                        new_offset_value_float = (orig_offset_value_float==0)?0:(-1.0f * orig_offset_value_float *((float)new_scale_value_double)); 
                    else 
                        new_offset_value_double = (orig_offset_value_double==0)?0:(-1 * orig_offset_value_double *new_scale_value_double); 
                }
            }

            add_scale_offset_attrs(at,scale_factor_type,new_scale_value_float,new_scale_value_double,add_offset_found,
                                   add_offset_type,new_offset_value_float,new_offset_value_double);
 
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
            orig_scale_value = (float)(atof(scale_factor_value.c_str()));
            scale_factor_type = at->get_type(it);
        }

        if(at->get_name(it)=="add_offset")
        {
            add_offset_value = (*at->get_attr_vector(it)->begin());
            orig_offset_value = (float)(atof(add_offset_value.c_str()));
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
            auto ait = avalue->begin();
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
    if(scale_factor_value.size()!=0)
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
    if(true == changedtype && fillvalue.size()!=0 && fillvalue_type!="Float32" && fillvalue_type!="Float64")
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
    if (sotype == SOType::MODIS_MUL_SCALE && true ==changedtype) {

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
        else if(scale_factor_value.size()!=0) {

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

            if (SOType::MODIS_EQ_SCALE == sotype || SOType::MODIS_MUL_SCALE == sotype) {
                if (orig_scale_value > 1) {
                  
                    bool need_change_scale = true;
                    if(true == is_grid) {
                        if ((filename.size() >5) && ((filename.compare(0,5,"MOD09") == 0)|| (filename.compare(0,5,"MYD09")==0))) {
                            if ((newfname.size() >5) && newfname.find("Range") != string::npos)
                                need_change_scale = false;
                        }

                        else if((filename.size() >7)&&((filename.compare(0,7,"MOD16A2") == 0)|| (filename.compare(0,7,"MYD16A2")==0) ||
                            (filename.compare(0,7,"MOD16A3")==0) || (filename.compare(0,7,"MYD16A3")==0))) 
                            need_change_scale = false;
                    }
                    if(true == need_change_scale) {
                        sotype = SOType::MODIS_DIV_SCALE;
                        INFO_LOG("The field " + newfname + " scale factor is "+ std::to_string(orig_scale_value)
                                 + " But the original scale factor type is MODIS_MUL_SCALE or MODIS_EQ_SCALE."
                                 + " Now change it to MODIS_DIV_SCALE. ");
                    }
                }
            }

            if (SOType::MODIS_DIV_SCALE == sotype) {
                if (orig_scale_value < 1) {
                    sotype = SOType::MODIS_MUL_SCALE;
                    INFO_LOG("The field " + newfname + " scale factor is " + std::to_string(orig_scale_value)
                                 + " But the original scale factor type is MODIS_DIV_SCALE."
                                 + " Now change it to MODIS_MUL_SCALE.");
                }
            }

            if(sotype == SOType::MODIS_MUL_SCALE) {
                valid_min = (orig_valid_min - orig_offset_value)*orig_scale_value;
                valid_max = (orig_valid_max - orig_offset_value)*orig_scale_value;
            }
            else if (sotype == SOType::MODIS_DIV_SCALE) {
                valid_min = (orig_valid_min - orig_offset_value)/orig_scale_value;
                valid_max = (orig_valid_max - orig_offset_value)/orig_scale_value;
            }
            else if (sotype == SOType::MODIS_EQ_SCALE) {
                valid_min = orig_valid_min * orig_scale_value + orig_offset_value;
                valid_max = orig_valid_max * orig_scale_value + orig_offset_value;
            }
        }
        else {// This is our current assumption.
            if (SOType::MODIS_MUL_SCALE == sotype || SOType::MODIS_DIV_SCALE == sotype) {
                valid_min = orig_valid_min - orig_offset_value;
                valid_max = orig_valid_max - orig_offset_value;
            }
            else if (SOType::MODIS_EQ_SCALE == sotype) {
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
                        

    vip_orig_valid_min = (short) (atoi((valid_range_value.substr(0,found)).c_str()));
    vip_orig_valid_max = (short) (atoi((valid_range_value.substr(found+1)).c_str()));

    int16 scale_factor_number = 1;

    scale_factor_number = (short)(atoi(scale_factor_value.c_str()));

    if(scale_factor_number !=0) {
        valid_min = (float)(vip_orig_valid_min/scale_factor_number);
        valid_max = (float)(vip_orig_valid_max/scale_factor_number);
    }
    else
        throw InternalErr(__FILE__,__LINE__,"The scale_factor_number should not be zero.");
}

// Make AMSR-E attributes follow CF.
// Change SCALE_FACTOR and OFFSET to CF names: scale_factor and add_offset.
void HDFCFUtil::handle_amsr_attrs(AttrTable *at) {

    AttrTable::Attr_iter it = at->attr_begin();

    string scale_factor_value="";
    string  add_offset_value="0";

    string scale_factor_type;
    string  add_offset_type;

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

//This function obtains the latitude and longitude dimension info. of an
//HDF-EOS2 grid after the handler translates the HDF-EOS to CF.
// Dimension info. includes dimension name and dimension size.
void HDFCFUtil::obtain_grid_latlon_dim_info(const HDFEOS2::GridDataset* gdset, 
                                     string & dim0name, 
                                     int32 & dim0size,
                                     string & dim1name, 
                                     int32& dim1size){

    const vector<HDFEOS2::Field*>gfields = gdset->getDataFields();
    for (const auto &gf:gfields) {

        // Check the dimensions for Latitude
        if(1 == gf->getFieldType()) {
            const vector<HDFEOS2::Dimension*>& dims= gf->getCorrectedDimensions();

            //2-D latitude
            if(2 == dims.size()) {
                // Most time, it is YDim Major. We will see if we have a real case to
                // check if the handling is right for the XDim Major case.
                if(true == gf->getYDimMajor()) {
                    dim0name = dims[0]->getName();
                    dim0size = dims[0]->getSize();
                    dim1name = dims[1]->getName();
                    dim1size = dims[1]->getSize();
                }
                else {
                    dim0name = dims[1]->getName();
                    dim0size = dims[1]->getSize();
                    dim1name = dims[0]->getName();
                    dim1size = dims[0]->getSize();
                }
                break;
            }

            //1-D latitude
            if( 1 == dims.size()) {
                dim0name = dims[0]->getName();
                dim0size = dims[0]->getSize();
            }
        }

        // Longitude, if longitude is checked first, it goes here.
        if(2 == gf->getFieldType()) {
            const vector<HDFEOS2::Dimension*>& dims= gf->getCorrectedDimensions();
            if(2 == dims.size()) {

                // Most time, it is YDim Major. We will see if we have a real case to
                // check if the handling is right for the XDim Major case.
                if(true == gf->getYDimMajor()) {
                    dim0name = dims[0]->getName();
                    dim0size = dims[0]->getSize();
                    dim1name = dims[1]->getName();
                    dim1size = dims[1]->getSize();
                }
                else {
                    dim0name = dims[1]->getName();
                    dim0size = dims[1]->getSize();
                    dim1name = dims[0]->getName();
                    dim1size = dims[0]->getSize();
                }
                break;
            }
            if( 1 == dims.size()) {
                dim1name = dims[0]->getName();
                dim1size = dims[0]->getSize();
                
            }
        }
    }
    if(""==dim0name || ""==dim1name || dim0size<0 || dim1size <0)
        throw InternalErr (__FILE__, __LINE__,"Fail to obtain grid lat/lon dimension info.");

}

// This function adds the 1-D cf grid projection mapping attribute to data variables
// it is called by the function add_cf_grid_attrs. 
void HDFCFUtil::add_cf_grid_mapping_attr(DAS &das, const HDFEOS2::GridDataset*gdset,const string& cf_projection,
                                         const string & dim0name,int32 dim0size,const string &dim1name,int32 dim1size) {

    // Check >=2-D fields, check if they hold the dim0name,dim0size etc., yes, add the attribute cf_projection.
    const vector<HDFEOS2::Field*>gfields = gdset->getDataFields();

    for (const auto &gf:gfields) {
        if(0 == gf->getFieldType() && gf->getRank() >1) {
            bool has_dim0 = false;
            bool has_dim1 = false;
            const vector<HDFEOS2::Dimension*>& dims= gf->getCorrectedDimensions();
            for (const auto &dim:dims) {
                if(dim->getName()== dim0name && dim->getSize() == dim0size)
                    has_dim0 = true;
                else if(dim->getName()== dim1name && dim->getSize() == dim1size)
                    has_dim1 = true;

            }
            if(true == has_dim0 && true == has_dim1) {// Need to add the grid_mapping attribute
                AttrTable *at = das.get_table(gf->getNewName());
                if (!at)
                    at = das.add_table(gf->getNewName(), new AttrTable);

                // The dummy projection name is the value of the grid_mapping attribute
                at->append_attr("grid_mapping","String",cf_projection);
            }
        }
    }
}

//This function adds 1D grid mapping CF attributes to CV and data variables.
void HDFCFUtil::add_cf_grid_cv_attrs(DAS & das, const HDFEOS2::GridDataset *gdset) {

    //1. Check the projection information, now, we only handle sinusoidal now
    if(GCTP_SNSOID == gdset->getProjection().getCode()) {

        //2. Obtain the dimension information from latitude and longitude(fieldtype =1 or fieldtype =2)
        string dim0name;
        string dim1name;
        int32 dim0size = -1;
        int32 dim1size = -1;

        HDFCFUtil::obtain_grid_latlon_dim_info(gdset,dim0name,dim0size,dim1name,dim1size);
        
        //3. Add 1D CF attributes to the 1-D CV variables and the dummy projection variable
        AttrTable *at = das.get_table(dim0name);
        if (!at)
            at = das.add_table(dim0name, new AttrTable);
        at->append_attr("standard_name","String","projection_y_coordinate");

        string long_name="y coordinate of projection for grid "+ gdset->getName();
        at->append_attr("long_name","String",long_name);
        // Change this to meter.
        at->append_attr("units","string","meter");
        
        at->append_attr("_CoordinateAxisType","string","GeoY");

        at = das.get_table(dim1name);
        if (!at)
            at = das.add_table(dim1name, new AttrTable);
 
        at->append_attr("standard_name","String","projection_x_coordinate");
        long_name="x coordinate of projection for grid "+ gdset->getName();
        at->append_attr("long_name","String",long_name);
         
        // change this to meter.
        at->append_attr("units","string","meter");
        at->append_attr("_CoordinateAxisType","string","GeoX");
        
        // Add the attributes for the dummy projection variable.
        string cf_projection_base = "eos_cf_projection";
        string cf_projection = HDFCFUtil::get_CF_string(gdset->getName()) +"_"+cf_projection_base;
        at = das.get_table(cf_projection);
        if (!at)
            at = das.add_table(cf_projection, new AttrTable);

        at->append_attr("grid_mapping_name","String","sinusoidal");
        at->append_attr("longitude_of_central_meridian","Float64","0.0");
        at->append_attr("earth_radius","Float64","6371007.181");

        at->append_attr("_CoordinateAxisTypes","string","GeoX GeoY");

        // Fill in the data fields that contains the dim0name and dim1name dimensions with the grid_mapping
        // We only apply to >=2D data fields.
        HDFCFUtil::add_cf_grid_mapping_attr(das,gdset,cf_projection,dim0name,dim0size,dim1name,dim1size);
    }

}

// This function adds the 1-D horizontal coordinate variables as well as the dummy projection variable to the grid.
//Note: Since we don't add these artifical CF variables to our main engineering at HDFEOS2.cc, the information
// to handle DAS won't pass to DDS by the file pointer, we need to re-call the routines to check projection
// and dimension. The time to retrieve these information is trivial compared with the whole translation.
void HDFCFUtil::add_cf_grid_cvs(DDS & dds, const HDFEOS2::GridDataset *gdset) {

    //1. Check the projection information, now, we only handle sinusoidal now
    if(GCTP_SNSOID == gdset->getProjection().getCode()) {

        //2. Obtain the dimension information from latitude and longitude(fieldtype =1 or fieldtype =2)
        string dim0name;
        string dim1name;
        int32  dim0size = -1;
        int32 dim1size = -1;
        HDFCFUtil::obtain_grid_latlon_dim_info(gdset,dim0name,dim0size,dim1name,dim1size);
        
        //3. Add the 1-D CV variables and the dummy projection variable
        // Note: we just need to pass the parameters that calculate 1-D cv to the data reading function,
        // in that way, we save the open cost of HDF-EOS2.
        BaseType *bt_dim0 = nullptr;
        BaseType *bt_dim1 = nullptr;

        HDFEOS2GeoCF1D * ar_dim0 = nullptr;
        HDFEOS2GeoCF1D * ar_dim1 = nullptr;

        const float64 *upleft = nullptr;
        const float64 *lowright = nullptr;

        try {

            bt_dim0 = new(HDFFloat64)(dim0name,gdset->getName());
            bt_dim1 = new(HDFFloat64)(dim1name,gdset->getName());

            // Obtain the upleft and lowright coordinates
            upleft = gdset->getInfo().getUpLeft();
            lowright = gdset->getInfo().getLowRight();
           
            // Note ar_dim0 is y, ar_dim1 is x.
            ar_dim0 = new HDFEOS2GeoCF1D(GCTP_SNSOID,
                                         upleft[1],lowright[1],dim0size,dim0name,bt_dim0);
            ar_dim0->append_dim(dim0size,dim0name);
                                         
            ar_dim1 = new HDFEOS2GeoCF1D(GCTP_SNSOID,
                                         upleft[0],lowright[0],dim1size,dim1name,bt_dim1);
            ar_dim1->append_dim(dim1size,dim1name);
            dds.add_var(ar_dim0);
            dds.add_var(ar_dim1);
        
        }
        catch(...) {
            if(bt_dim0) 
                delete bt_dim0;
            if(bt_dim1) 
                delete bt_dim1;
            if(ar_dim0) 
                delete ar_dim0;
            if(ar_dim1) 
                delete ar_dim1;
            throw InternalErr(__FILE__,__LINE__,"Unable to allocate the HDFEOS2GeoCF1D instance.");
        }

        if(bt_dim0)
            delete bt_dim0;
        if(bt_dim1)
            delete bt_dim1;
        if(ar_dim0)
            delete ar_dim0;
        if(ar_dim1)
            delete ar_dim1;

        // Also need to add the dummy projection variable.
        string cf_projection_base = "eos_cf_projection";

        // To handle multi-grid cases, we need to add the grid name.
        string cf_projection = HDFCFUtil::get_CF_string(gdset->getName()) +"_"+cf_projection_base;

        HDFEOS2GeoCFProj * dummy_proj_cf = new HDFEOS2GeoCFProj(cf_projection,gdset->getName());
        dds.add_var(dummy_proj_cf);
        if(dummy_proj_cf)
            delete dummy_proj_cf;

    }

}
#endif

 // Check OBPG attributes. Specifically, check if slope and intercept can be obtained from the file level. 
 // If having global slope and intercept,  obtain OBPG scaling, slope and intercept values.
void HDFCFUtil::check_obpg_global_attrs(HDFSP::File *f, std::string & scaling, 
                                        float & slope,bool &global_slope_flag,
                                        float & intercept, bool & global_intercept_flag){

    HDFSP::SD* spsd = f->getSD();

    for (const auto &attr:spsd->getAttributes()) {
       
        //We want to add two new attributes, "scale_factor" and "add_offset" to data fields if the scaling equation is linear. 
        // OBPG products use "Slope" instead of "scale_factor", "intercept" instead of "add_offset". "Scaling" describes if the equation is linear.
        // Their values will be copied directly from File attributes. This addition is for OBPG L3 only.
        // We also add OBPG L2 support since all OBPG level 2 products(CZCS,MODISA,MODIST,OCTS,SeaWiFS) we evaluate use Slope,intercept and linear equation
        // for the final data. KY 2012-09-06
	if(f->getSPType()==OBPGL3 || f->getSPType() == OBPGL2)
        {
            if(attr->getName()=="Scaling")
            {
                string tmpstring(attr->getValue().begin(), attr->getValue().end());
                scaling = tmpstring;
            }
            if(attr->getName()=="Slope" || attr->getName()=="slope")
            {
                global_slope_flag = true;
			
                switch(attr->getType())
                {
#define GET_SLOPE(TYPE, CAST) \
    case DFNT_##TYPE: \
    { \
        CAST tmpvalue = *(CAST*)&(attr->getValue()[0]); \
        slope = (float)tmpvalue; \
    } \
    break;
                    GET_SLOPE(INT16,   int16)
                    GET_SLOPE(INT32,   int32)
                    GET_SLOPE(FLOAT32, float)
                    GET_SLOPE(FLOAT64, double)
                    default:
                        throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef GET_SLOPE
                } 
#if 0
                //slope = *(float*)&((*i)->getValue()[0]);
#endif
            }
            if(attr->getName()=="Intercept" || attr->getName()=="intercept")
            {	
                global_intercept_flag = true;
                switch(attr->getType())
                {
#define GET_INTERCEPT(TYPE, CAST) \
    case DFNT_##TYPE: \
    { \
        CAST tmpvalue = *(CAST*)&(attr->getValue()[0]); \
        intercept = (float)tmpvalue; \
    } \
    break;
                    GET_INTERCEPT(INT16,   int16)
                    GET_INTERCEPT(INT32,   int32)
                    GET_INTERCEPT(FLOAT32, float)
                    GET_INTERCEPT(FLOAT64, double)
                    default:
                        throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef GET_INTERCEPT
                } 
#if 0
                //intercept = *(float*)&((*i)->getValue()[0]);
#endif
            }
        }
    }
}

// For some OBPG files that only provide slope and intercept at the file level, 
// global slope and intercept are needed to add to all fields and their names are needed to be changed to scale_factor and add_offset.
// For OBPG files that provide slope and intercept at the field level,  slope and intercept are needed to rename to scale_factor and add_offset.
void HDFCFUtil::add_obpg_special_attrs(const HDFSP::File*f,DAS &das,
                                       const HDFSP::SDField *onespsds, 
                                       const string& scaling, float& slope, 
                                       const bool& global_slope_flag, 
                                       float& intercept,
                                       const bool & global_intercept_flag) {

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
#if 0
        for(vector<HDFSP::Attribute *>::const_iterator i=onespsds->getAttributes().begin();
                                                       i!=onespsds->getAttributes().end();i++) {
#endif
        for (const auto &attr:onespsds->getAttributes()) {

            if(global_slope_flag != true && (attr->getName()=="Slope" || attr->getName()=="slope"))
            {
                slope_flag = true;
			
                switch(attr->getType())
                {
#define GET_SLOPE(TYPE, CAST) \
    case DFNT_##TYPE: \
    { \
        CAST tmpvalue = *(CAST*)&(attr->getValue()[0]); \
        slope = (float)tmpvalue; \
    } \
    break;

                GET_SLOPE(INT16,   int16)
                GET_SLOPE(INT32,   int32)
                GET_SLOPE(FLOAT32, float)
                GET_SLOPE(FLOAT64, double)
                default:
                        throw InternalErr(__FILE__,__LINE__,"unsupported data type.");

#undef GET_SLOPE
                } 
#if 0
                    //slope = *(float*)&((*i)->getValue()[0]);
#endif
            }
            if(global_intercept_flag != true && (attr->getName()=="Intercept" || attr->getName()=="intercept"))
            {	
                intercept_flag = true;
                switch(attr->getType())
                {
#define GET_INTERCEPT(TYPE, CAST) \
    case DFNT_##TYPE: \
    { \
        CAST tmpvalue = *(CAST*)&(attr->getValue()[0]); \
        intercept = (float)tmpvalue; \
    } \
    break;
                GET_INTERCEPT(INT16,   int16)
                GET_INTERCEPT(INT32,   int32)
                GET_INTERCEPT(FLOAT32, float)
                GET_INTERCEPT(FLOAT64, double)
                default:
                        throw InternalErr(__FILE__,__LINE__,"unsupported data type.");

#undef GET_INTERCEPT
                } 
#if 0
                    //intercept = *(float*)&((*i)->getValue()[0]);
#endif
            }
        }
    } // End of checking "slope" and "intercept"

    // Checking if OBPG has "scale_factor" ,"add_offset", generally checking for "long_name" attributes.
    for (const auto& attr:onespsds->getAttributes()) {       

        if((f->getSPType()==OBPGL3 || f->getSPType() == OBPGL2)  && attr->getName()=="scale_factor")
            scale_factor_flag = true;		

        if((f->getSPType()==OBPGL3 || f->getSPType() == OBPGL2) && attr->getName()=="add_offset")
            add_offset_flag = true;
    }
        
    // Checking if the scale and offset equation is linear, this is only for OBPG level 3.
    // Also checking if having the fill value and adding fill value if not having one for some OBPG data type
    if((f->getSPType() == OBPGL2 || f->getSPType()==OBPGL3 )&& onespsds->getFieldType()==0)
    {
            
        if((OBPGL3 == f->getSPType() && (scaling.find("linear")!=string::npos)) || OBPGL2 == f->getSPType() ) {

            if(false == scale_factor_flag && (true == slope_flag || true == global_slope_flag))
            {
                string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32, 0, (void*)&slope);
                at->append_attr("scale_factor", HDFCFUtil::print_type(DFNT_FLOAT32), print_rep);
            }

            if(false == add_offset_flag && (true == intercept_flag || true == global_intercept_flag))
            {
                string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT32, 0, (void*)&intercept);
                at->append_attr("add_offset", HDFCFUtil::print_type(DFNT_FLOAT32), print_rep);
            }
        }

        bool has_fill_value = false;
        for(const auto &attr:onespsds->getAttributes()) {
            if ("_FillValue" == attr->getNewName()){
                has_fill_value = true; 
                break;
            }
        }
         

        // Add fill value to some type of OBPG data. 16-bit integer, fill value = -32767, unsigned 16-bit integer fill value = 65535
        // This is based on the evaluation of the example files. KY 2012-09-06
        if ((false == has_fill_value) &&(DFNT_INT16 == onespsds->getType())) {
            short fill_value = -32767;
            string print_rep = HDFCFUtil::print_attr(DFNT_INT16,0,(void*)&fill_value);
            at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_INT16),print_rep);
        }

        if ((false == has_fill_value) &&(DFNT_UINT16 == onespsds->getType())) {
            unsigned short fill_value = 65535;
            string print_rep = HDFCFUtil::print_attr(DFNT_UINT16,0,(void*)&fill_value);
            at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_UINT16),print_rep);
        }

    }// Finish OBPG handling

}

// Handle HDF4 OTHERHDF products that follow SDS dimension scale model. 
// The special handling of AVHRR data is also included.
void HDFCFUtil::handle_otherhdf_special_attrs(const HDFSP::File*f,DAS &das) {

    // For some HDF4 files that follow HDF4 dimension scales, P.O. DAAC's AVHRR files.
    // The "otherHDF" category can almost make AVHRR files work, except
    // that AVHRR uses the attribute name "unit" instead of "units" for latitude and longitude,
    // I have to correct the name to follow CF conventions(using "units"). I won't check
    // the latitude and longitude values since latitude and longitude values for some files(LISO files)   
    // are not in the standard range(0-360 for lon and 0-180 for lat). KY 2011-3-3
    const vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();

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

        for (const auto &fd:spsds){

            // Ignore ALL coordinate variables if this is "OTHERHDF" case and some dimensions 
            // don't have dimension scale data.
            if ( true == f->Has_Dim_NoScale_Field() && (fd->getFieldType() !=0) && (fd->IsDimScale() == false))
                continue;

            // Ignore the empty(no data) dimension variable.
            if (OTHERHDF == f->getSPType() && true == fd->IsDimNoScale())
                continue;

            AttrTable *at = das.get_table(fd->getNewName());
            if (!at)
                at = das.add_table(fd->getNewName(), new AttrTable);

            for (const auto& attr:fd->getAttributes()) {
                if(attr->getType()==DFNT_UCHAR || attr->getType() == DFNT_CHAR){

                    if(attr->getName() == "long_name") {
                        string tempstring2(attr->getValue().begin(),attr->getValue().end());
                        string tempfinalstr= string(tempstring2.c_str());// This may remove some garbage characters
                        if(tempfinalstr=="latitude" || tempfinalstr == "Latitude") // Find long_name latitude
                            latflag = true;
                        if(tempfinalstr=="longitude" || tempfinalstr == "Longitude") // Find long_name latitude
                            lonflag = true;
                    }
                }
            }

            if(latflag) {
                for (const auto& attr:fd->getAttributes()) {
                    if (attr->getName() == "units") 
                        latunitsflag = true;
                }
            }

            if(lonflag) {
                for(const auto& attr:fd->getAttributes()) {
                    if(attr->getName() == "units") 
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
                lonunitsflag = false;
                llcheckoverflag++;
            }
            if(llcheckoverflag ==2) break;

        }

    }

}

// Add  missing CF attributes for non-CV varibles
void
HDFCFUtil::add_missing_cf_attrs(const HDFSP::File*f,DAS &das) {

    const vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();

    // TRMM level 3 grid
    if(TRMML3A_V6== f->getSPType() || TRMML3C_V6==f->getSPType() || TRMML3S_V7 == f->getSPType() || TRMML3M_V7 == f->getSPType()) {

        for(const auto &fd:spsds){
            if(fd->getFieldType() == 0 && fd->getType()==DFNT_FLOAT32) {

                AttrTable *at = das.get_table(fd->getNewName());
                if (!at)
                    at = das.add_table(fd->getNewName(), new AttrTable);
                string print_rep = "-9999.9";
                at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_FLOAT32),print_rep);

            }
        }

        for(const auto &fd:spsds){
            if(fd->getFieldType() == 0 && ((fd->getType()==DFNT_INT32) || (fd->getType()==DFNT_INT16))) {

                AttrTable *at = das.get_table(fd->getNewName());
                if (!at)
                    at = das.add_table(fd->getNewName(), new AttrTable);
                string print_rep = "-9999";
                if(fd->getType()==DFNT_INT32)
                    at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_INT32),print_rep);
                else 
                    at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_INT16),print_rep);

            }
        }

        // nlayer for TRMM single grid version 7, the units should be "km"
        if(TRMML3S_V7 == f->getSPType()) {
            for(const auto &fd:spsds){
                if(fd->getFieldType() == 6 && fd->getNewName()=="nlayer") {

                    AttrTable *at = das.get_table(fd->getNewName());
                    if (!at)
                        at = das.add_table(fd->getNewName(), new AttrTable);
                    at->append_attr("units","String","km");

                }
                else if(fd->getFieldType() == 4) {

                    if (fd->getNewName()=="nh3" ||
                        fd->getNewName()=="ncat3" ||
                        fd->getNewName()=="nthrshZO" ||
                        fd->getNewName()=="nthrshHB" ||
                        fd->getNewName()=="nthrshSRT")
                     {

                        string references =
                           "http://pps.gsfc.nasa.gov/Documents/filespec.TRMM.V7.pdf";
                        string comment;

                        AttrTable *at = das.get_table(fd->getNewName());
                        if (!at)
                            at = das.add_table(fd->getNewName(), new AttrTable);
 
                        if(fd->getNewName()=="nh3") {
                            comment="Index number to represent the fixed heights above the earth ellipsoid,";
                            comment= comment + " at 2, 4, 6 km plus one for path-average.";
                        }

                        else if(fd->getNewName()=="ncat3") {
                            comment="Index number to represent catgories for probability distribution functions.";
                            comment=comment + "Check more information from the references.";
                        }

                        else if(fd->getNewName()=="nthrshZO") 
                            comment="Q-thresholds for Zero order used for probability distribution functions.";

                        else if(fd->getNewName()=="nthrshHB") 
                            comment="Q-thresholds for HB used for probability distribution functions.";

                        else if(fd->getNewName()=="nthrshSRT") 
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
            
                for(const auto &fd:spsds){

                    if(fd->getFieldType() == 0 && (fd->getType()==DFNT_FLOAT32)) {
                         AttrTable *at = das.get_table(fd->getNewName());
                        if (!at)
                            at = das.add_table(fd->getNewName(), new AttrTable);
                        at->del_attr("_FillValue");
                        at->append_attr("_FillValue","Float32","-999");
                        at->append_attr("valid_min","Float32","0");

                    }
                }
            }

        }

        // nlayer for TRMM single grid version 7, the units should be "km"
        if(TRMML3M_V7 == f->getSPType()) {
            for(const auto &fd:spsds){

                if(fd->getFieldType() == 4 ) {

                    string references ="http://pps.gsfc.nasa.gov/Documents/filespec.TRMM.V7.pdf";
                    if (fd->getNewName()=="nh1") {

                        AttrTable *at = das.get_table(fd->getNewName());
                        if (!at)
                            at = das.add_table(fd->getNewName(), new AttrTable);
                
                        string comment="Number of fixed heights above the earth ellipsoid,";
                               comment= comment + " at 2, 4, 6, 10, and 15 km plus one for path-average.";

                        at->append_attr("comment","String",comment);
                        at->append_attr("references","String",references);

                    }
                    if (fd->getNewName()=="nh3") {

                        AttrTable *at = das.get_table(fd->getNewName());
                        if (!at)
                            at = das.add_table(fd->getNewName(), new AttrTable);
                
                        string comment="Number of fixed heights above the earth ellipsoid,";
                               comment= comment + " at 2, 4, 6 km plus one for path-average.";

                        at->append_attr("comment","String",comment);
                        at->append_attr("references","String",references);

                    }

                    if (fd->getNewName()=="nang") {

                        AttrTable *at = das.get_table(fd->getNewName());
                        if (!at)
                            at = das.add_table(fd->getNewName(), new AttrTable);
                
                        string comment="Number of fixed incidence angles, at 0, 5, 10 and 15 degree and all angles.";
                        references = "http://pps.gsfc.nasa.gov/Documents/ICSVol4.pdf";

                        at->append_attr("comment","String",comment);
                        at->append_attr("references","String",references);

                    }

                    if (fd->getNewName()=="ncat2") {

                        AttrTable *at = das.get_table(fd->getNewName());
                        if (!at)
                            at = das.add_table(fd->getNewName(), new AttrTable);
                
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

                for (const auto &fd:spsds){

                    if(fd->getFieldType() == 0 && fd->getType()==DFNT_INT16) {

                        AttrTable *at = das.get_table(fd->getNewName());
                        if (!at)
                            at = das.add_table(fd->getNewName(), new AttrTable);

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
                                    double new_scale = 1.0/strtod(scale_factor_value.c_str(),nullptr);
                                    at->del_attr("scale_factor");
                                    string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT64,0,(void*)(&new_scale));
                                    at->append_attr("scale_factor", scale_factor_type,print_rep);

                                }
                               
                                if(scale_factor_type == "Float32") {
                                    float new_scale = 1.0f/strtof(scale_factor_value.c_str(),nullptr);
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
            if (t2a12_flag==true) {

                for (const auto &fd:spsds){

                    if (fd->getFieldType() == 6 && fd->getNewName()=="nlayer") {

                        AttrTable *at = das.get_table(fd->getNewName());
                        if (!at)
                            at = das.add_table(fd->getNewName(), new AttrTable);
                        at->append_attr("units","String","km");

                    }

                    // signed char maps to int32, so use int32 for the fillvalue.
                    if (fd->getFieldType() == 0 && fd->getType()==DFNT_INT8) {

                        AttrTable *at = das.get_table(fd->getNewName());
                        if (!at)
                            at = das.add_table(fd->getNewName(), new AttrTable);
                        at->append_attr("_FillValue","Int32","-99");

                    }
                }
            }

            // for all 2A12,2A21 and 2B31
            // Add fillvalues for float32 and int32.
            for (const auto & fd:spsds){
                if (fd->getFieldType() == 0 && fd->getType()==DFNT_FLOAT32) {

                    AttrTable *at = das.get_table(fd->getNewName());
                    if (!at)
                        at = das.add_table(fd->getNewName(), new AttrTable);
                    string print_rep = "-9999.9";
                    at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_FLOAT32),print_rep);

                }
            }

            for (const auto &fd:spsds){

                if(fd->getFieldType() == 0 && fd->getType()==DFNT_INT16) {

                    AttrTable *at = das.get_table(fd->getNewName());
                    if (!at)
                        at = das.add_table(fd->getNewName(), new AttrTable);

                    string print_rep = "-9999";
                    at->append_attr("_FillValue",HDFCFUtil::print_type(DFNT_INT32),print_rep);

                }
            }
        }

        // group 2: 2A21 and 2A25.
        else if(t2a21_flag == true || t2a25_flag == true) {

            // 2A25: handle reflectivity and rain rate scales
            if (t2a25_flag == true) {

                unsigned char handle_scale = 0;

                for (const auto &fd:spsds){

                    if (fd->getFieldType() == 0 && fd->getType()==DFNT_INT16) {
                        bool has_dBZ = false;
                        bool has_rainrate = false;
                        bool has_scale = false;
                        string scale_factor_value;
                        string scale_factor_type;

                        AttrTable *at = das.get_table(fd->getNewName());
                        if (!at)
                            at = das.add_table(fd->getNewName(), new AttrTable);
                        AttrTable::Attr_iter it = at->attr_begin();
                        while (it!=at->attr_end()) {
                            if(at->get_name(it)=="units"){
                                string units_value = *at->get_attr_vector(it)->begin();
                                if("dBZ" == units_value) { 
                                    has_dBZ = true;
                                }

                                else if("mm/hr" == units_value){
                                    has_rainrate = true;
                                }
                            }
                            if(at->get_name(it)=="scale_factor")
                            {
                                scale_factor_value = *at->get_attr_vector(it)->begin();
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
                                valid_max = (short)(300*strtof(scale_factor_value.c_str(),nullptr));
                            else if(true == has_dBZ) 
                                valid_max = (short)(80*strtof(scale_factor_value.c_str(),nullptr));
    
                            string print_rep1 = HDFCFUtil::print_attr(DFNT_INT16,0,(void*)(&valid_min));
                            at->append_attr("valid_min","Int16",print_rep1);
                            print_rep1 = HDFCFUtil::print_attr(DFNT_INT16,0,(void*)(&valid_max));
                            at->append_attr("valid_max","Int16",print_rep1);
    
                            at->del_attr("scale_factor");
                            if(scale_factor_type == "Float64") {
                                double new_scale = 1.0/strtod(scale_factor_value.c_str(),nullptr);
                                string print_rep2 = HDFCFUtil::print_attr(DFNT_FLOAT64,0,(void*)(&new_scale));
                                at->append_attr("scale_factor", scale_factor_type,print_rep2);
    
                            }
                                   
                            if(scale_factor_type == "Float32") {
                                float new_scale = 1.0/strtof(scale_factor_value.c_str(),nullptr);
                                string print_rep3 = HDFCFUtil::print_attr(DFNT_FLOAT32,0,(void*)(&new_scale));
                                at->append_attr("scale_factor", scale_factor_type,print_rep3);
    
                            }
                        }

                        if(2 == handle_scale)
                            break;

                    }
                }
            }
        }

        // 1B21,1C21 and 1B11
        else if (t1b21_flag || t1c21_flag || t1b11_flag) {

            // 1B21,1C21 scale_factor to CF and valid_range for dBm and dBZ.
            if (t1b21_flag || t1c21_flag) {

                for (const auto &fd:spsds){

                    if(fd->getFieldType() == 0 && fd->getType()==DFNT_INT16) {

                        bool has_dBm = false;
                        bool has_dBZ = false;

                        AttrTable *at = das.get_table(fd->getNewName());
                        if (!at)
                            at = das.add_table(fd->getNewName(), new AttrTable);
                        AttrTable::Attr_iter it = at->attr_begin();

                        while (it!=at->attr_end()) {
                            if(at->get_name(it)=="units"){
                             
                                string units_value = *at->get_attr_vector(it)->begin();
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

                                    string scale_value = *at->get_attr_vector(it)->begin();
                            
                                    if(true == has_dBm) {
                                       auto valid_min = (short)(-120 *strtof(scale_value.c_str(),nullptr));
                                       auto valid_max = (short)(-20 *strtof(scale_value.c_str(),nullptr));
                                       string print_rep = HDFCFUtil::print_attr(DFNT_INT16,0,(void*)(&valid_min));
                                       at->append_attr("valid_min","Int16",print_rep);
                                       print_rep = HDFCFUtil::print_attr(DFNT_INT16,0,(void*)(&valid_max));
                                       at->append_attr("valid_max","Int16",print_rep);
                                       break;
                                      
                                    }

                                    else if(true == has_dBZ){
                                       auto valid_min = (short)(-20 *strtof(scale_value.c_str(),nullptr));
                                       auto valid_max = (short)(80 *strtof(scale_value.c_str(),nullptr));
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
            for(const auto &fd:spsds){

                if(fd->getFieldType() == 0 && fd->getType()==DFNT_INT16) {

                    AttrTable *at = das.get_table(fd->getNewName());
                    if (!at)
                        at = das.add_table(fd->getNewName(), new AttrTable);
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
                                double new_scale = 1.0/strtod(scale_factor_value.c_str(),nullptr);
                                at->del_attr("scale_factor");
                                string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT64,0,(void*)(&new_scale));
                                at->append_attr("scale_factor", scale_factor_type,print_rep);

                            }
                               
                            if(scale_factor_type == "Float32") {
                                float new_scale = 1.0f/strtof(scale_factor_value.c_str(),nullptr);
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
        else if (t1b01_flag == true) {

            for (const auto &fd:spsds){

                if(fd->getFieldType() == 0 && fd->getType()==DFNT_FLOAT32) {

                    AttrTable *at = das.get_table(fd->getNewName());
                    if (!at)
                        at = das.add_table(fd->getNewName(), new AttrTable);

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
 
void HDFCFUtil::handle_merra_ceres_attrs_with_bes_keys(const HDFSP::File *f, DAS &das,const string& filename) {

    string base_filename = filename.substr(filename.find_last_of("/")+1);

#if 0 
    string check_ceres_merra_short_name_key="H4.EnableCERESMERRAShortName";
    bool turn_on_ceres_merra_short_name_key= false;

    turn_on_ceres_merra_short_name_key = HDFCFUtil::check_beskeys(check_ceres_merra_short_name_key);
#endif

    bool merra_is_eos2 = false;
    if (0== (base_filename.compare(0,5,"MERRA"))) {

        for (const auto & fd:f->getSD()->getAttributes()) {
            // Check if this MERRA file is an HDF-EOS2 or not.
            if((fd->getName().compare(0, 14, "StructMetadata" )== 0) ||
                (fd->getName().compare(0, 14, "structmetadata" )== 0)) {
                merra_is_eos2 = true;
                break;
            }

        }
    }

    if (true == HDF4RequestHandler::get_enable_ceres_merra_short_name() && (CER_ES4 == f->getSPType() || CER_SRB == f->getSPType()
        || CER_CDAY == f->getSPType() || CER_CGEO == f->getSPType() 
        || CER_SYN == f->getSPType() || CER_ZAVG == f->getSPType()
        || CER_AVG == f->getSPType() || (true == merra_is_eos2))) {

        const vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();
        for (const auto & fd:spsds){

            AttrTable *at = das.get_table(fd->getNewName());
            if (!at)
                at = das.add_table(fd->getNewName(), new AttrTable);

            at->append_attr("fullpath","String",fd->getSpecFullPath());

        }
    }
}


// Handle the attributes when the BES key EnableVdataDescAttr is enabled..
void HDFCFUtil::handle_vdata_attrs_with_desc_key(const HDFSP::File*f,libdap::DAS &das) {

    // Check the EnableVdataDescAttr key. If this key is turned on, the handler-added attribute VDdescname and
    // the attributes of vdata and vdata fields will be outputed to DAS. Otherwise, these attributes will
    // not outputed to DAS. The key will be turned off by default to shorten the DAP output. KY 2012-09-18

#if 0
    string check_vdata_desc_key="H4.EnableVdataDescAttr";
    bool turn_on_vdata_desc_key= false;

    turn_on_vdata_desc_key = HDFCFUtil::check_beskeys(check_vdata_desc_key);
#endif

    string VDdescname = "hdf4_vd_desc";
    string VDdescvalue = "This is an HDF4 Vdata.";
    string VDfieldprefix = "Vdata_field_";
    string VDattrprefix = "Vdata_attr_";
    string VDfieldattrprefix ="Vdata_field_attr_";

    // To speed up the performance for handling CERES data, we turn off some CERES vdata fields, this should be resumed in the future version with BESKeys.
#if 0
    string check_ceres_vdata_key="H4.EnableCERESVdata";
    bool turn_on_ceres_vdata_key= false;
    turn_on_ceres_vdata_key = HDFCFUtil::check_beskeys(check_ceres_vdata_key);
#endif
                
    bool output_vdata_flag = true;
    if (false == HDF4RequestHandler::get_enable_ceres_vdata() && 
        (CER_AVG == f->getSPType() || 
         CER_ES4 == f->getSPType() ||   
         CER_SRB == f->getSPType() ||
         CER_ZAVG == f->getSPType()))
        output_vdata_flag = false;

    if (true == output_vdata_flag) {

        for (const auto &vd:f->getVDATAs()) {

            AttrTable *at = das.get_table(vd->getNewName());
            if(!at)
                at = das.add_table(vd->getNewName(),new AttrTable);
 
            if (true == HDF4RequestHandler::get_enable_vdata_desc_attr()) {
                // Add special vdata attributes
                bool emptyvddasflag = true;
                if (!(vd->getAttributes().empty())) 
                    emptyvddasflag = false;
                if (vd->getTreatAsAttrFlag())
                    emptyvddasflag = false;
                else {
                    for (const auto &vfd:vd->getFields()) {
                        if(!(vfd->getAttributes().empty())) {
                            emptyvddasflag = false;
                            break;
                        }
                    }
                }

                if(emptyvddasflag) 
                    continue;
                at->append_attr(VDdescname, "String" , VDdescvalue);

                for(const auto &va:vd->getAttributes()) {

                    if(va->getType()==DFNT_UCHAR || va->getType() == DFNT_CHAR){

                        string tempstring2(va->getValue().begin(),va->getValue().end());
                        string tempfinalstr= string(tempstring2.c_str());
                        at->append_attr(VDattrprefix+va->getNewName(), "String" , tempfinalstr);
                    }
                    else {
                        for (int loc=0; loc < va->getCount() ; loc++) {
                            string print_rep = HDFCFUtil::print_attr(va->getType(), loc, (void*) &(va->getValue()[0]));
                            at->append_attr(VDattrprefix+va->getNewName(), HDFCFUtil::print_type(va->getType()), print_rep);
                        }
                    }
                }
            }

            if(false == (vd->getTreatAsAttrFlag())){ 

                if (true == HDF4RequestHandler::get_enable_vdata_desc_attr()) {

                    //NOTE: for vdata field, we assume that no special characters are found. We need to escape the special characters when the data type is char. 
                    // We need to create a DAS container for each field so that the attributes can be put inside.
                    for (const auto &vdf:vd->getFields()) {

                        // This vdata field will NOT be treated as attributes, only save the field attribute as the attribute
                        // First check if the field has attributes, if it doesn't have attributes, no need to create a container.
                        
                        if (vdf->getAttributes().empty() ==false) {

                            AttrTable *at_v = das.get_table(vdf->getNewName());                           
                            if(!at_v) 
                                at_v = das.add_table(vdf->getNewName(),new AttrTable);

                            for (const auto &va:vdf->getAttributes()) {

                                if(va->getType()==DFNT_UCHAR || va->getType() == DFNT_CHAR){

                                    string tempstring2(va->getValue().begin(),va->getValue().end());
                                    string tempfinalstr= string(tempstring2.c_str());
                                    at_v->append_attr(va->getNewName(), "String" , tempfinalstr);
                                }
                                else {
                                    for (int loc=0; loc < va->getCount() ; loc++) {
                                        string print_rep = HDFCFUtil::print_attr(va->getType(), loc, (void*) &(va->getValue()[0]));
                                        at_v->append_attr(va->getNewName(), HDFCFUtil::print_type(va->getType()), print_rep);
                                    }
                                }

                            }
                        }
                    }
                }

            }

            else {

                for(const auto & vdf:vd->getFields()) {
 
                    if(vdf->getFieldOrder() == 1) {
                        if(vdf->getType()==DFNT_UCHAR || vdf->getType() == DFNT_CHAR){
                            string tempfinalstr;
                            tempfinalstr.resize(vdf->getValue().size());
                            copy(vdf->getValue().begin(),vdf->getValue().end(),tempfinalstr.begin());
                            at->append_attr(VDfieldprefix+vdf->getNewName(), "String" , tempfinalstr);
                        }
                        else {
                            for ( int loc=0; loc < vdf->getNumRec(); loc++) {
                                string print_rep = HDFCFUtil::print_attr(vdf->getType(), loc, (void*) &(vdf->getValue()[0]));
                                at->append_attr(VDfieldprefix+vdf->getNewName(), HDFCFUtil::print_type(vdf->getType()), print_rep);
                            }
                        }
                    }
                    else {//When field order is greater than 1,we want to print each record in group with single quote,'0 1 2','3 4 5', etc.

                        if (vdf->getValue().size() != (unsigned int)(DFKNTsize(vdf->getType())*(vdf->getFieldOrder())*(vdf->getNumRec()))){
                            throw InternalErr(__FILE__,__LINE__,"the vdata field size doesn't match the vector value");
                        }

                        if(vdf->getNumRec()==1){
                            if(vdf->getType()==DFNT_UCHAR || vdf->getType() == DFNT_CHAR){
                                string tempstring2(vdf->getValue().begin(),vdf->getValue().end());
                                auto tempfinalstr= string(tempstring2.c_str());
                                at->append_attr(VDfieldprefix+vdf->getNewName(),"String",tempfinalstr);
                            }
                            else {
                                for (int loc=0; loc < vdf->getFieldOrder(); loc++) {
                                    string print_rep = HDFCFUtil::print_attr(vdf->getType(), loc, (void*) &(vdf->getValue()[0]));
                                    at->append_attr(VDfieldprefix+vdf->getNewName(), HDFCFUtil::print_type(vdf->getType()), print_rep);
                                }
                            }

                        }

                        else {
                            if(vdf->getType()==DFNT_UCHAR || vdf->getType() == DFNT_CHAR){

                                for(int tempcount = 0; tempcount < vdf->getNumRec()*DFKNTsize(vdf->getType());tempcount ++) {
                                    vector<char>::const_iterator tempit;
                                    tempit = vdf->getValue().begin()+tempcount*(vdf->getFieldOrder());
                                    string tempstring2(tempit,tempit+vdf->getFieldOrder());
                                    auto tempfinalstr= string(tempstring2.c_str());
                                    string tempoutstring = "'"+tempfinalstr+"'";
                                    at->append_attr(VDfieldprefix+vdf->getNewName(),"String",tempoutstring);
                                }
                            }

                            else {
                                for(int tempcount = 0; tempcount < vdf->getNumRec();tempcount ++) {
                                    at->append_attr(VDfieldprefix+vdf->getNewName(),HDFCFUtil::print_type(vdf->getType()),"'");
                                    for (int loc=0; loc < vdf->getFieldOrder(); loc++) {
                                        string print_rep = HDFCFUtil::print_attr(vdf->getType(), loc, (void*) &(vdf->getValue()[tempcount*(vdf->getFieldOrder())]));
                                        at->append_attr(VDfieldprefix+vdf->getNewName(), HDFCFUtil::print_type(vdf->getType()), print_rep);
                                    }
                                    at->append_attr(VDfieldprefix+vdf->getNewName(),HDFCFUtil::print_type(vdf->getType()),"'");
                                }
                            }
                        }
                    }

         
                    if (true == HDF4RequestHandler::get_enable_vdata_desc_attr()) {
                        for(const auto &va:vdf->getAttributes()) {

                            if(va->getType()==DFNT_UCHAR || va->getType() == DFNT_CHAR){

                                string tempstring2(va->getValue().begin(),va->getValue().end());
                                auto tempfinalstr= string(tempstring2.c_str());
                                at->append_attr(VDfieldattrprefix+va->getNewName(), "String" , tempfinalstr);
                            }
                            else {
                                for (int loc=0; loc < va->getCount() ; loc++) {
                                    string print_rep = HDFCFUtil::print_attr(va->getType(), loc, (void*) &(va->getValue()[0]));
                                    at->append_attr(VDfieldattrprefix+va->getNewName(), HDFCFUtil::print_type(va->getType()), print_rep);
                                }
                            }
                        }
                    }
                }
            }

        }
    } 

}

void HDFCFUtil::map_eos2_objects_attrs(libdap::DAS &das,const string &filename) {
    
    intn   status_n =-1;     
    int32  status_32 = -1;  
    int32  file_id = -1;
    int32  vgroup_id = -1;
    int32  num_of_lones = 0;    
    uint16 name_len = 0;

    file_id = Hopen (filename.c_str(), DFACC_READ, 0); 
    if(file_id == FAIL) 
        throw InternalErr(__FILE__,__LINE__,"Hopen failed.");

    status_n = Vstart (file_id);
    if(status_n == FAIL) {
        Hclose(file_id);
        throw InternalErr(__FILE__,__LINE__,"Vstart failed.");
    }

    string err_msg;
    bool unexpected_fail = false;
    //Get and print the names and class names of all the lone vgroups.
    // First, call Vlone with num_of_lones set to 0 to get the number of
    // lone vgroups in the file, but not to get their reference numbers.
    num_of_lones = Vlone (file_id, nullptr, num_of_lones );

    //
    // Then, if there are any lone vgroups, 
    if (num_of_lones > 0)
    {
        // Use the num_of_lones returned to allocate sufficient space for the
        // buffer ref_array to hold the reference numbers of all lone vgroups,
        vector<int32> ref_array;
        ref_array.resize(num_of_lones);

      
        // and call Vlone again to retrieve the reference numbers into 
        // the buffer ref_array.
      
        num_of_lones = Vlone (file_id, ref_array.data(), num_of_lones);

        // Loop the name and class of each lone vgroup.
        for (int lone_vg_number = 0; lone_vg_number < num_of_lones; 
                                                            lone_vg_number++)
        {
         
            // Attach to the current vgroup then get and display its
            // name and class. Note: the current vgroup must be detached before
            // moving to the next.
            vgroup_id = Vattach(file_id, ref_array[lone_vg_number], "r");
            if(vgroup_id == FAIL) {
                unexpected_fail = true;
                err_msg = string(ERR_LOC) + " Vattach failed. ";
                goto cleanFail;
            }
            
            status_32 = Vgetnamelen(vgroup_id, &name_len);
            if(status_32 == FAIL) {
                unexpected_fail = true;
                Vdetach(vgroup_id);
                err_msg = string(ERR_LOC) + " Vgetnamelen failed. ";
                goto cleanFail;
            }
 
            vector<char> vgroup_name;
            vgroup_name.resize(name_len+1);
            status_32 = Vgetname (vgroup_id, vgroup_name.data());
            if(status_32 == FAIL) {
                unexpected_fail = true;
                Vdetach(vgroup_id);
                err_msg = string(ERR_LOC) + " Vgetname failed. ";
                goto cleanFail;
            }

            status_32 = Vgetclassnamelen(vgroup_id, &name_len);
            if(status_32 == FAIL) {
                unexpected_fail = true;
                Vdetach(vgroup_id);
                err_msg = string(ERR_LOC) + " Vgetclassnamelen failed. ";
                goto cleanFail;
            }
           
            vector<char>vgroup_class;
            vgroup_class.resize(name_len+1);
            status_32 = Vgetclass (vgroup_id, vgroup_class.data());
            if(status_32 == FAIL) {
                unexpected_fail = true;
                Vdetach(vgroup_id);
                err_msg = string(ERR_LOC) + " Vgetclass failed. ";
                goto cleanFail;
            }
            
            string vgroup_name_str(vgroup_name.begin(),vgroup_name.end());
            vgroup_name_str = vgroup_name_str.substr(0,vgroup_name_str.size()-1);

            string vgroup_class_str(vgroup_class.begin(),vgroup_class.end());
            vgroup_class_str = vgroup_class_str.substr(0,vgroup_class_str.size()-1);
            try {
                if(vgroup_class_str =="GRID") 
                    map_eos2_one_object_attrs_wrapper(das,file_id,vgroup_id,vgroup_name_str,true);
                else if(vgroup_class_str =="SWATH")
                    map_eos2_one_object_attrs_wrapper(das,file_id,vgroup_id,vgroup_name_str,false);
            }
            catch(...) {
                Vdetach(vgroup_id);
                Vend(file_id);
                Hclose(file_id);
                throw InternalErr(__FILE__,__LINE__,"map_eos2_one_object_attrs_wrapper failed.");
            }
            Vdetach (vgroup_id);
        }// for  
    }// if 

    //Terminate access to the V interface and close the file.
cleanFail:
    Vend (file_id);
    Hclose (file_id);
    if(true == unexpected_fail)
        throw InternalErr(__FILE__,__LINE__,err_msg);

    return;

}

void HDFCFUtil::map_eos2_one_object_attrs_wrapper(libdap:: DAS &das,int32 file_id,int32 vgroup_id, const string& vgroup_name,bool is_grid) {

    int32 num_gobjects = Vntagrefs (vgroup_id);
    if(num_gobjects < 0) 
        throw InternalErr(__FILE__,__LINE__,"Cannot obtain the number of objects under a vgroup.");
    
    for(int i = 0; i<num_gobjects;i++) {

        int32 obj_tag;
        int32 obj_ref;
        if (Vgettagref (vgroup_id, i, &obj_tag, &obj_ref) == FAIL) 
            throw InternalErr(__FILE__,__LINE__,"Failed to obtain the tag and reference of an object under a vgroup.");

        if (Visvg (vgroup_id, obj_ref) == TRUE) {
                                                                      
            int32 object_attr_vgroup = Vattach(file_id,obj_ref,"r");
            if(object_attr_vgroup == FAIL) 
                throw InternalErr(__FILE__,__LINE__,"Failed to attach an EOS2 vgroup.");

            uint16 name_len = 0;
            int32 status_32 = Vgetnamelen(object_attr_vgroup, &name_len);
            if(status_32 == FAIL) {
                Vdetach(object_attr_vgroup);
                throw InternalErr(__FILE__,__LINE__,"Failed to obtain an EOS2 vgroup name length.");
            }
            vector<char> attr_vgroup_name; 
            attr_vgroup_name.resize(name_len+1);
            status_32 = Vgetname (object_attr_vgroup, attr_vgroup_name.data());
            if(status_32 == FAIL) {
                Vdetach(object_attr_vgroup);
                throw InternalErr(__FILE__,__LINE__,"Failed to obtain an EOS2 vgroup name. ");
            }

            string attr_vgroup_name_str(attr_vgroup_name.begin(),attr_vgroup_name.end());
            attr_vgroup_name_str = attr_vgroup_name_str.substr(0,attr_vgroup_name_str.size()-1);

            try {
                if(true == is_grid && attr_vgroup_name_str=="Grid Attributes"){
                    map_eos2_one_object_attrs(das,file_id,object_attr_vgroup,vgroup_name);
                    Vdetach(object_attr_vgroup);
                    break;
                }
                else if(false == is_grid && attr_vgroup_name_str=="Swath Attributes") {
                    map_eos2_one_object_attrs(das,file_id,object_attr_vgroup,vgroup_name);
                    Vdetach(object_attr_vgroup);
                    break;
                }
            }
            catch(...) {
                Vdetach(object_attr_vgroup);
                throw InternalErr(__FILE__,__LINE__,"Cannot map eos2 object attributes to DAP2.");
            }
            Vdetach(object_attr_vgroup);

        }

    }
}

void HDFCFUtil::map_eos2_one_object_attrs(libdap:: DAS &das,int32 file_id, int32 obj_attr_group_id, const string& vgroup_name) {

    int32 num_gobjects = Vntagrefs(obj_attr_group_id);
    if(num_gobjects < 0) 
        throw InternalErr(__FILE__,__LINE__,"Cannot obtain the number of objects under a vgroup.");

    string vgroup_cf_name = HDFCFUtil::get_CF_string(vgroup_name);
    AttrTable *at = das.get_table(vgroup_cf_name);
    if(!at)
        at = das.add_table(vgroup_cf_name,new AttrTable);

    for(int i = 0; i<num_gobjects;i++) {

        int32 obj_tag;
        int32 obj_ref;

        if (Vgettagref(obj_attr_group_id, i, &obj_tag, &obj_ref) == FAIL) 
            throw InternalErr(__FILE__,__LINE__,"Failed to obtain the tag and reference of an object under a vgroup.");

        if(Visvs(obj_attr_group_id,obj_ref)) {

            int32 vdata_id = VSattach(file_id,obj_ref,"r");
            if(vdata_id == FAIL) 
                throw InternalErr(__FILE__,__LINE__,"Failed to attach a vdata.");
            
            // EOS2 object vdatas are actually EOS2 object attributes.
            if(VSisattr(vdata_id)) {

                // According to EOS2 library, EOS2 number of field and record must be 1.
                int32 num_field = VFnfields(vdata_id);
                if(num_field !=1) { 
                    VSdetach(vdata_id);
                    throw InternalErr(__FILE__,__LINE__,"Number of vdata field for an EOS2 object must be 1.");
                }
                int32 num_record = VSelts(vdata_id);
                if(num_record !=1){
                    VSdetach(vdata_id);
                    throw InternalErr(__FILE__,__LINE__,"Number of vdata record for an EOS2 object must be 1.");
                }
                char vdata_name[VSNAMELENMAX];
                if(VSQueryname(vdata_id,vdata_name) == FAIL) {
                    VSdetach(vdata_id);
                    throw InternalErr(__FILE__,__LINE__,"Failed to obtain EOS2 object vdata name.");
                }
                string vdatanamestr(vdata_name);
                string vdataname_cfstr = HDFCFUtil::get_CF_string(vdatanamestr);
                int32 fieldsize = VFfieldesize(vdata_id,0);
                if(fieldsize == FAIL) {
                    VSdetach(vdata_id);
                    throw InternalErr(__FILE__,__LINE__,"Failed to obtain EOS2 object vdata field size.");
                }

                const char* fieldname = VFfieldname(vdata_id,0);
                if(fieldname == nullptr) {
                    VSdetach(vdata_id);
                    throw InternalErr(__FILE__,__LINE__,"Failed to obtain EOS2 object vdata field name.");
                }
                int32 fieldtype = VFfieldtype(vdata_id,0);
                if(fieldtype == FAIL) {
                    VSdetach(vdata_id);
                    throw InternalErr(__FILE__,__LINE__,"Failed to obtain EOS2 object vdata field type.");
                }

                if(VSsetfields(vdata_id,fieldname) == FAIL) {
                    VSdetach(vdata_id);
                    throw InternalErr(__FILE__,__LINE__,"EOS2 object vdata: VSsetfields failed.");
                }

                vector<char> vdata_value;
                vdata_value.resize(fieldsize);
                if(VSread(vdata_id,(uint8*)vdata_value.data(),1,FULL_INTERLACE) == FAIL) {
                    VSdetach(vdata_id);
                    throw InternalErr(__FILE__,__LINE__,"EOS2 object vdata: VSread failed.");
                }

                // Map the attributes to DAP2.
                if(fieldtype == DFNT_UCHAR || fieldtype == DFNT_CHAR){
                    string tempstring(vdata_value.begin(),vdata_value.end());
                    // Remove the nullptr term
                    auto tempstring2 = string(tempstring.c_str());
                    at->append_attr(vdataname_cfstr,"String",tempstring2);
                }
                else {
                    string print_rep = HDFCFUtil::print_attr(fieldtype, 0, (void*) vdata_value.data());
                    at->append_attr(vdataname_cfstr, HDFCFUtil::print_type(fieldtype), print_rep);
                }
            
            }
            VSdetach(vdata_id);

        }
    }

    return;
}

// The function that escapes the special characters is no longer needed after we move that functionality to libdap4.
// Will keep the following function as an #if 0/#endif block for a while and then the code should be removed. KY 2022-11-22
#if 0
// Part of a large fix for attributes. Escaping the values of the attributes
// may have been a bad idea. It breaks using JSON, for example. If this is a
// bad idea - to turn of escaping - then we'll have to figure out how to store
// 'serialized JSON' in attributes because it's being used in netcdf/hdf files.
// If we stick with this, there's clearly a more performant solution - eliminate
// the calls to this code.
// jhrg 6/25/21
#define ESCAPE_STRING_ATTRIBUTES 0

string HDFCFUtil::escattr(string s)
{
// We should not handle any special characters here. These will be handled by libdap4. Still leave the code here for the time being.
// Eventually this function will be removed. KY 2022-08-25
#if ESCAPE_STRING_ATTRIBUTES
    const string printable = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789~`!@#$%^&*()_-+={[}]|\\:;<,>.?/'\"\n\t\r";
    const string ESC = "\\";
    const string DOUBLE_ESC = ESC + ESC;
    const string QUOTE = "\"";
    const string ESCQUOTE = ESC + QUOTE;

    // escape \ with a second backslash
    size_t ind = 0;
    while ((ind = s.find(ESC, ind)) != string::npos) {
        s.replace(ind, 1, DOUBLE_ESC);
        ind += DOUBLE_ESC.size();
    }

    // escape " with backslash
    ind = 0;
    while ((ind = s.find(QUOTE, ind)) != string::npos) {
        //comment out the following line since it wastes the CPU operation.
        s.replace(ind, 1, ESCQUOTE);
        ind += ESCQUOTE.size();
    }

    size_t ind = 0;
    while ((ind = s.find_first_not_of(printable, ind)) != string::npos) {
        s.replace(ind, 1, ESC + octstring(s[ind]));
    }
#endif

    return s;
}
#endif

// This function is necessary since char is represented as string. For fillvalue, this has to be resumed. 
string HDFCFUtil::escattr_fvalue(string s)
{
    const string printable = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789~`!@#$%^&*()_-+={[}]|\\:;<,>.?/'\"\n\t\r";
    const string ESC = "\\";
    size_t ind = 0;
    while ((ind = s.find_first_not_of(printable, ind)) != string::npos) {
        s.replace(ind, 1, ESC + octstring(s[ind]));
    }

    return s;
}


void HDFCFUtil::parser_trmm_v7_gridheader(const vector<char>& value, 
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
     HDFCFUtil::Split(value.data(),sep,ind_elems);
     // The number of elements in the GridHeader is 9. 
     //The string vector will add a leftover. So the size should be 10.
     // For the  MacOS clang compiler, the string vector size may become 11.
     // So we change the condition to be "<9" is wrong.
     if(ind_elems.size()<9)
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
            lat_res = strtof(latres_str.c_str(),nullptr);
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
            lon_res = strtof(lonres_str.c_str(),nullptr);
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
            lat_north = strtof(north_bounding_str.c_str(),nullptr);
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
                lat_south = strtof(lat_south_str.c_str(),nullptr);
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
                lon_east = strtof(lon_east_str.c_str(),nullptr);
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
                lon_west = strtof(lon_west_str.c_str(),nullptr);
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
    char temp = 0;
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


string HDFCFUtil::get_double_str(double x,int total_digit,int after_point) {
    
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

string HDFCFUtil::get_int_str(int x) {

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
 
ssize_t HDFCFUtil::read_vector_from_file(int fd, vector<double> &val, size_t dtypesize) {

    ssize_t ret_val;
    ret_val = read(fd,val.data(),val.size()*dtypesize);
    
    return ret_val;
}
// Need to wrap a 'read buffer' from a pure file call here since read() is also a DAP function to read DAP data.
ssize_t HDFCFUtil::read_buffer_from_file(int fd,  void*buf, size_t total_read) {

    ssize_t ret_val;
    ret_val = read(fd,buf,total_read);
    
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

// Obtain the cache name. Since AIRS version 6 level 3 all share the same latitude and longitude,
// we provide one set of latitude and longitude cache files for all AIRS level 3 version 6 products.
string HDFCFUtil::obtain_cache_fname(const string & fprefix, const string &fname, const string &vname) {

     string cache_fname = fprefix;
     string base_file_name = basename(fname);
     // Identify this file from product name: AIRS, product level: .L3. or .L2. and version .v6. 
     if((base_file_name.size() >12)  
         && (base_file_name.compare(0,4,"AIRS") == 0)
         && (base_file_name.find(".L3.")!=string::npos)
         && (base_file_name.find(".v6.")!=string::npos)
         && ((vname == "Latitude") ||(vname == "Longitude"))) 
         cache_fname = cache_fname +"AIRS"+".L3."+".v6."+vname;
     else
         cache_fname = cache_fname + base_file_name +"_"+vname;

     return cache_fname;
}

// The current DDS cache is only for some products of which object information
// can be retrieved via HDF4 SDS APIs. Currently only AIRS version 6 products are supported.
size_t HDFCFUtil::obtain_dds_cache_size(const HDFSP::File*spf) {

    size_t total_bytes_written = 0;
    const vector<HDFSP::SDField *>& spsds = spf->getSD()->getFields();
    for (const auto &fd:spsds){

        // We will not handle when the SDS datatype is DFNT_CHAR now.
        if(DFNT_CHAR == fd->getType()) {
            total_bytes_written = 0;
            break;
        }
        else {
            // We need to store dimension names and variable names.
            int temp_rank = fd->getRank(); 
            for (const auto & dim:fd->getDimensions())
                total_bytes_written += (dim->getName()).size()+1;

            total_bytes_written +=(fd->getName()).size()+1;

            // Many a time variable new name is the same as variable name,
            // so we may just save one byte('\0') for such as a case.
            if((fd->getName()) != (fd->getNewName()))
                total_bytes_written +=(fd->getNewName()).size()+1;
            else
                total_bytes_written +=1;

            // We need to store 4 integers: rank, variable datatype, SDS reference number, fieldtype
            total_bytes_written +=(temp_rank+4)*sizeof(int);
        }
    }

    if(total_bytes_written != 0)
        total_bytes_written +=1;

    return total_bytes_written;

}

// Write the DDS of the special SDS-only HDF to a cache. 
void HDFCFUtil::write_sp_sds_dds_cache(const HDFSP::File* spf,FILE*dds_file,size_t total_bytes_dds_cache,const string &dds_filename) {

    BESDEBUG("h4"," Coming to write SDS DDS to a cache" << endl);
    char delimiter = '\0';
    char cend = '\n';
    size_t total_written_bytes_count = 0;

    // The buffer to hold information to write to a DDS cache file.
    vector<char>temp_buf;
    temp_buf.resize(total_bytes_dds_cache);
    char* temp_pointer = temp_buf.data();

    const vector<HDFSP::SDField *>& spsds = spf->getSD()->getFields();

    //Read SDS 
    for(const auto &fd:spsds){

        // First, rank, fieldtype, SDS reference number, variable datatype, dimsize(rank)
        int sds_rank = fd->getRank();
        int sds_ref = fd->getFieldRef();
        int sds_dtype = fd->getType();
        int sds_ftype = fd->getFieldType();

        vector<int32>dimsizes;
        dimsizes.resize(sds_rank);
  
        // Size for each dimension
        const vector<HDFSP::Dimension*>& dims= fd->getDimensions();
        for(int i = 0; i < sds_rank; i++) 
            dimsizes[i] = dims[i]->getSize();

        memcpy((void*)temp_pointer,(void*)&sds_rank,sizeof(int));
        temp_pointer +=sizeof(int);
        memcpy((void*)temp_pointer,(void*)&sds_ref,sizeof(int));
        temp_pointer +=sizeof(int);
        memcpy((void*)temp_pointer,(void*)&sds_dtype,sizeof(int));
        temp_pointer +=sizeof(int);
        memcpy((void*)temp_pointer,(void*)&sds_ftype,sizeof(int));
        temp_pointer +=sizeof(int);
        
        memcpy((void*)temp_pointer,(void*)dimsizes.data(),sds_rank*sizeof(int));
        temp_pointer +=sds_rank*sizeof(int);
        
        // total written bytes so far
        total_written_bytes_count +=(sds_rank+4)*sizeof(int);

        // Second, variable name,variable new name  and SDS dim name(rank)
        // we need to a delimiter to distinguish each name.
        string temp_varname = fd->getName();
        vector<char>temp_val1(temp_varname.begin(),temp_varname.end());
        memcpy((void*)temp_pointer,(void*)temp_val1.data(),temp_varname.size());
        temp_pointer +=temp_varname.size();
        memcpy((void*)temp_pointer,&delimiter,1);
        temp_pointer +=1;

        total_written_bytes_count =total_written_bytes_count +(1+temp_varname.size());

        // To save the dds cache size and the speed to retrieve variable new name
        // we only save variable cf name when the variable cf name is not the
        // same as the variable name. When they are the same, a delimiter is saved for
        // variable cf name. 
        if(fd->getName() == fd->getNewName()){
            memcpy((void*)temp_pointer,&delimiter,1);
            temp_pointer +=1;
            total_written_bytes_count +=1;
        }
        else {
            string temp_varnewname = fd->getNewName();
            vector<char>temp_val2(temp_varnewname.begin(),temp_varnewname.end());
            memcpy((void*)temp_pointer,(void*)temp_val2.data(),temp_varnewname.size());
            temp_pointer +=temp_varnewname.size();
            memcpy((void*)temp_pointer,&delimiter,1);
            temp_pointer +=1;
            total_written_bytes_count =total_written_bytes_count +(1+temp_varnewname.size());
        }
            
        // Now the name for each dimensions.
        for(int i = 0; i < sds_rank; i++) {
            string temp_dimname = dims[i]->getName();
            vector<char>temp_val3(temp_dimname.begin(),temp_dimname.end());
            memcpy((void*)temp_pointer,(void*)temp_val3.data(),temp_dimname.size());
            temp_pointer +=temp_dimname.size();
            memcpy((void*)temp_pointer,&delimiter,1);
            temp_pointer +=1;
            total_written_bytes_count =total_written_bytes_count +(1+temp_dimname.size());
        }
    }
    
    memcpy((void*)temp_pointer,&cend,1);
    total_written_bytes_count +=1;

    if(total_written_bytes_count != total_bytes_dds_cache) {
        stringstream s_total_written_count;
        s_total_written_count << total_written_bytes_count;
        stringstream s_total_bytes_dds_cache;
        s_total_bytes_dds_cache << total_bytes_dds_cache;
        string msg = "DDs cached file "+ dds_filename +" buffer size should be " + s_total_bytes_dds_cache.str()  ;
        msg = msg + ". But the real size written in the buffer is " + s_total_written_count.str();
        throw InternalErr (__FILE__, __LINE__,msg);
    }

    size_t bytes_really_written = fwrite((const void*)temp_buf.data(),1,total_bytes_dds_cache,dds_file);

    if(bytes_really_written != total_bytes_dds_cache) { 
        stringstream s_expected_bytes;
        s_expected_bytes << total_bytes_dds_cache;
        stringstream s_really_written_bytes;
        s_really_written_bytes << bytes_really_written;
        string msg = "DDs cached file "+ dds_filename +" size should be " + s_expected_bytes.str()  ;
        msg += ". But the real size written to the file is " + s_really_written_bytes.str();
        throw InternalErr (__FILE__, __LINE__,msg);
    }

}

// Read DDS of a special SDS-only HDF file from a cache.
void HDFCFUtil::read_sp_sds_dds_cache(FILE* dds_file,libdap::DDS * dds_ptr,
                                      const std::string &cache_filename, const std::string &hdf4_filename) {

    BESDEBUG("h4"," Coming to read SDS DDS from a cache" << endl);

    // Check the cache file size.
    struct stat sb;
    if(stat(cache_filename.c_str(),&sb)!=0) {
        string err_mesg="The DDS cache file " + cache_filename;
        err_mesg = err_mesg + " doesn't exist.  ";
        throw InternalErr(__FILE__,__LINE__,err_mesg);
    }

    auto bytes_expected_read = (size_t)sb.st_size;

    // Allocate the buffer size based on the file size.
    vector<char> temp_buf;
    temp_buf.resize(bytes_expected_read);
    size_t bytes_really_read = fread((void*)temp_buf.data(),1,bytes_expected_read,dds_file);

    // Now bytes_really_read should be the same as bytes_expected_read if the element size is 1.
    if(bytes_really_read != bytes_expected_read) {
        stringstream s_bytes_really_read;
        s_bytes_really_read << bytes_really_read ;
        stringstream s_bytes_expected_read;
        s_bytes_expected_read << bytes_expected_read;
        string msg = "The expected bytes to read from DDS cache file " + cache_filename +" is " + s_bytes_expected_read.str();
        msg = msg + ". But the real read size from the buffer is  " + s_bytes_really_read.str();
        throw InternalErr (__FILE__, __LINE__,msg);
    }
    char* temp_pointer = temp_buf.data();

    char delimiter = '\0';
    // The end of the whole string.
    char cend = '\n';
    bool end_file_flag = false;

   
    do {
        int sds_rank = *((int *)(temp_pointer));
        temp_pointer = temp_pointer + sizeof(int);

        int sds_ref = *((int *)(temp_pointer));
        temp_pointer = temp_pointer + sizeof(int);

        int sds_dtype = *((int *)(temp_pointer));
        temp_pointer = temp_pointer + sizeof(int);

        int sds_ftype = *((int *)(temp_pointer));
        temp_pointer = temp_pointer + sizeof(int);

        vector<int32>dimsizes;
        if(sds_rank <=0) 
            throw InternalErr (__FILE__, __LINE__,"SDS rank must be >0");
             
        dimsizes.resize(sds_rank);
        for (int i = 0; i <sds_rank;i++) {
            dimsizes[i] = *((int *)(temp_pointer));
            temp_pointer = temp_pointer + sizeof(int);
        }

        vector<string>dimnames;
        dimnames.resize(sds_rank);
        string varname;
        string varnewname;
        for (int i = 0; i <sds_rank+2;i++) {
            vector<char> temp_vchar;
            char temp_char = *temp_pointer;

            // Only apply when varnewname is stored as the delimiter.
            if(temp_char == delimiter) 
                temp_vchar.push_back(temp_char);             
            while(temp_char !=delimiter) {
                temp_vchar.push_back(temp_char);
                temp_pointer++;
                temp_char = *temp_pointer;
                //temp_char = *(++temp_pointer); work
                //temp_char = *(temp_pointer++); not working
            }
            string temp_string(temp_vchar.begin(),temp_vchar.end());
            if(i == 0) 
                varname = temp_string;
            else if( i == 1)
                varnewname = temp_string;
            else 
                dimnames[i-2] = temp_string;
            temp_pointer++;
        }

        // If varnewname is only the delimiter, varname and varnewname is the same.
        if(varnewname[0] == delimiter)
            varnewname = varname;
        // Assemble DDS. 
        // 1. Create a basetype
        BaseType *bt = nullptr;
        switch(sds_dtype) { 
#define HANDLE_CASE(tid, type) \
    case tid: \
        bt = new (type)(varnewname,hdf4_filename); \
        break; 
        HANDLE_CASE(DFNT_FLOAT32, HDFFloat32)
        HANDLE_CASE(DFNT_FLOAT64, HDFFloat64) 
        HANDLE_CASE(DFNT_CHAR, HDFStr)
#ifndef SIGNED_BYTE_TO_INT32 
        HANDLE_CASE(DFNT_INT8, HDFByte) 
#else 
        HANDLE_CASE(DFNT_INT8,HDFInt32) 
#endif 
        HANDLE_CASE(DFNT_UINT8, HDFByte) 
        HANDLE_CASE(DFNT_INT16, HDFInt16) 
        HANDLE_CASE(DFNT_UINT16, HDFUInt16) 
        HANDLE_CASE(DFNT_INT32, HDFInt32) 
        HANDLE_CASE(DFNT_UINT32, HDFUInt32) 
        HANDLE_CASE(DFNT_UCHAR8, HDFByte) 
        default: 
            throw InternalErr(__FILE__,__LINE__,"unsupported data type."); 
#undef HANDLE_CASE 
        } 

        if(nullptr == bt)
            throw InternalErr(__FILE__,__LINE__,"Cannot create the basetype when creating DDS from a cache file.");

        SPType sptype = OTHERHDF;

        // sds_ftype indicates if this is a general data field or geolocation field.
        // 4 indicates this is a missing non-latlon geo-location fields.
        if(sds_ftype != 4){
            HDFSPArray_RealField *ar = nullptr;
            try {
                // pass sds id as 0 since the sds id will be retrieved from SDStart if necessary.
                ar = new HDFSPArray_RealField(
                                              sds_rank,
                                              hdf4_filename,
                                              0,
                                              sds_ref,
                                              sds_dtype,
                                              sptype,
                                              varname,
                                              dimsizes,
                                              varnewname,
                                              bt);
            }
            catch(...) {
                delete bt;
                throw InternalErr(__FILE__,__LINE__,"Unable to allocate the HDFSPArray_RealField instance.");
            }

            for(int i = 0; i <sds_rank; i++)
                ar->append_dim(dimsizes[i],dimnames[i]);
            dds_ptr->add_var(ar);
            delete bt;
            delete ar;
        }
        else {
            HDFSPArrayMissGeoField *ar = nullptr;
            if(sds_rank == 1) {
                try {
                    ar = new HDFSPArrayMissGeoField(
                                                    sds_rank,
                                                    dimsizes[0],
                                                    varnewname,
                                                    bt);
                }
                catch(...) {
                    delete bt;
                    throw InternalErr(__FILE__,__LINE__,"Unable to allocate the HDFSPArray_RealField instance.");
                }

                ar->append_dim(dimsizes[0],dimnames[0]);
                dds_ptr->add_var(ar);
                delete bt;
                delete ar;
            }
            else 
                throw InternalErr(__FILE__,__LINE__,"SDS rank  must be 1 for the missing coordinate.");
        }

        if(*temp_pointer == cend)
            end_file_flag = true;
        if((temp_pointer - temp_buf.data()) > (int)bytes_expected_read) {
            string msg = cache_filename + " doesn't have the end-line character at the end. The file may be corrupted.";
            throw InternalErr (__FILE__, __LINE__,msg);
        }
    } while(false == end_file_flag);

    dds_ptr->set_dataset_name(basename(hdf4_filename));
}


