#include "HDFCFUtil.h"
#include <BESDebug.h>

bool 
HDFCFUtil::check_beskeys(const string key) {

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


void
HDFCFUtil::ClearMem (int32 * offset32, int32 * count32, int32 * step32,
					 int *offset, int *count, int *step)
{
	if(offset32 != NULL)
           delete[]offset32;
        if(count32 != NULL) 
	   delete[]count32;
        if(step32 != NULL) 
	   delete[]step32;
        if(offset != NULL) 
	   delete[]offset;
        if(count != NULL) 
	   delete[]count;
        if(step != NULL) 
	   delete[]step;
}

void
HDFCFUtil::ClearMem2 (int32 * offset32, int32 * count32, int32 * step32)
{
     if(offset32 != NULL)
	delete[]offset32;
     if(count32 != NULL) 
	delete[]count32;
     if(step32 != NULL)
	delete[]step32;
}

void
HDFCFUtil::ClearMem3 (int *offset, int *count, int *step)
{
     if (offset != NULL)
	delete[]offset;
     if (count != NULL)
	delete[]count;
     if (step != NULL)
	delete[]step;
}

void 
HDFCFUtil::Split(const char *s, int len, char sep,
                    std::vector<std::string> &names)
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

void 
HDFCFUtil::Split(const char *sz, char sep, std::vector<std::string> &names)
{
    Split(sz, (int)strlen(sz), sep, names);
}

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

string
HDFCFUtil::get_CF_string(string s)
{

    if(""==s) return s;
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

void
HDFCFUtil::Handle_NameClashing(vector<string>&newobjnamelist,set<string>&objnameset) {

    //set<string> objnameset;
    pair<set<string>::iterator,bool> setret;
    set<string>::iterator iss;

    vector<string> clashnamelist;
    vector<string>::iterator ivs;

    map<int,int> cl_to_ol;
    int ol_index = 0;
    int cl_index = 0;

    vector<string>::const_iterator irv;

    for (irv = newobjnamelist.begin();
                irv != newobjnamelist.end(); ++irv) {
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

string
HDFCFUtil::print_type(int32 type)
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

     //          float64 templatlon[majordim][minordim];
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




