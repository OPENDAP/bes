/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf4 data handler for the OPeNDAP data server.
//
// Author:   Hyo-Kyung Lee <hyoklee@hdfgroup.org>
// Copyright (c) 2009 The HDF Group
/////////////////////////////////////////////////////////////////////////////
// #define DODS_DEBUG
#include <iostream>

#include <string.h>

#include <debug.h>
#include <util.h>

#include "HDFEOS.h"

using namespace std;

struct  yy_buffer_state;

// Functions to parse structMetadata file.
// The parser can identify whether the HDF-EOS2 file has Grids or Swaths.
// It can also check for a valid projection which can determine whether
// DAP Grids or Arrays should be used.
int hdfeos2lex();
int hdfeos2parse(void *arg);
yy_buffer_state *hdfeos2_scan_string(const char *str);
void hdfeos2_delete_buffer(void *buf);


HDFEOS::HDFEOS()
{
    _is_grid = false;
    _is_orthogonal = false;
    _is_swath = false;
    _is_ydimmajor = false;
    _parsed = false;
    _shared_dimension = false;
    _valid = false;

    bmetadata_Struct = false;
    borigin_upper = false;      // Is map upside or down?
    dimension_data = NULL;
    gradient_x = 0.0;
    gradient_y = 0.0;
    path_name = "";
    point_lower = 0.0;
    point_upper = 0.0;
    point_left = 0.0;
    point_right = 0.0;
    xdimsize = 0;
    ydimsize = 0;

    eos_to_cf_map["XDim"] = "lon";
    eos_to_cf_map["YDim"] = "lat";
    eos_to_cf_map["nCandidate"] = "lev";
    eos_to_cf_map["MissingValue"] = "missing_value";
    eos_to_cf_map["Units"] = "units";
    eos_to_cf_map["Offset"] = "add_offset";
    eos_to_cf_map["ScaleFactor"] = "scale_factor";
    eos_to_cf_map["ValidRange"] = "valid_range";

    eos_to_cf_map_is_swath["Longitude"] = "lon";
    eos_to_cf_map_is_swath["Latitude"] = "lat";
    eos_to_cf_map_is_swath["GeoTrack"] = "lat"; //  AIRS
    eos_to_cf_map_is_swath["GeoXTrack"] = "lon";
    eos_to_cf_map_is_swath["Cell_Along_Swath_5km"] = "lat"; // MODIS
    eos_to_cf_map_is_swath["Cell_Across_Swath_5km"] = "lon";
    eos_to_cf_map_is_swath["StdPressureLev"] = "pressStd";

    cf_to_eos_map["lon"] = "XDim";
    cf_to_eos_map["lat"] = "YDim";
    cf_to_eos_map["lev"] = "nCandidate";

    cf_to_eos_map_is_swath["lon"] = "Longitude";
    cf_to_eos_map_is_swath["lat"] = "Latitude";
}

HDFEOS::~HDFEOS()
{
    if (dimension_data) {
        for (unsigned int i = 0; i < dimensions.size(); ++i) {
            dods_float32 *convbuf = dimension_data[i];
            delete [] convbuf;
        }
        delete [] dimension_data;
    }
}

void HDFEOS::add_data_path(string full_path)
{

#ifdef SHORT_PATH
    full_path = get_short_name(full_path);
#endif

    DBG(cerr << "Full path is:" << full_path << endl);
    full_data_paths.push_back(full_path);
}


void HDFEOS::add_dimension_list(string full_path, string dimension_list)
{

#ifdef SHORT_PATH
    full_path = get_short_name(full_path);
    dimension_list = get_short_name(dimension_list);
#endif

    full_data_path_to_dimension_list_map[full_path] = dimension_list;
    DBG(cerr << "Dimension List is:" <<
        full_data_path_to_dimension_list_map[full_path] << endl);
}

void HDFEOS::add_dimension_map(string dimension_name, int dimension)
{
    bool has_dimension = false;
#ifdef SHORT_PATH
    dimension_name = get_short_name(dimension_name);
#endif

    int i;
    for (i = 0; i < (int) dimensions.size(); i++) {
        std::string str = dimensions.at(i);
        if (str == dimension_name) {
            has_dimension = true;
            break;
        }
    }

    if (!has_dimension) {
        dimensions.push_back(dimension_name);
        dimension_map[dimension_name] = dimension;
    }
}

string HDFEOS::get_CF_name(const string &str)
{
#ifdef USE_HDFEOS2_LIB
    if (is_swath()) {
        map<string,string>::iterator pos = eos_to_cf_map_is_swath.find(str);
        if (pos != eos_to_cf_map_is_swath.end())
            return pos->second;
    }
#endif
    map<string,string>::iterator pos = eos_to_cf_map.find(str);
    if (pos != eos_to_cf_map.end())
	return pos->second;
    else{
	return str;
    }
}

string HDFEOS::get_EOS_name(const string &str)
{
#ifdef USE_HDFEOS2_LIB
    if (is_swath()) {
        map<string,string>::iterator pos = eos_to_cf_map_is_swath.find(str);
        if (pos != eos_to_cf_map.end())
            return pos->second;
    }
#endif
    map<string,string>::iterator pos = eos_to_cf_map.find(str);
    if (pos != eos_to_cf_map.end())
        return pos->second;
    else
        return str;
}

void HDFEOS::get_all_dimensions(vector < string > &tokens)
{
    int j;
    for (j = 0; j < (int) dimensions.size(); j++) {
        string dim_name = dimensions.at(j);
        tokens.push_back(dim_name);
        DBG(cerr << "=get_all_dimensions():Dim name = " << dim_name <<
            std::endl);
    }
}


int HDFEOS::get_dimension_data_location(string dimension_name)
{
    int j;
    for (j = 0; j < (int) dimensions.size(); j++) {
        string dim_name = dimensions.at(j);
        if (dim_name == dimension_name)
            return j;
    }
    return -1;
}

int HDFEOS::get_dimension_size(string name)
{
    return dimension_map[name];
}

void HDFEOS::get_dimensions(string name, vector < string > &tokens)
{
    string str = full_data_path_to_dimension_list_map[name];
    // Skip delimiters at the beginning.
    string::size_type lastPos = str.find_first_not_of(',', 0);
    // Find the first "non-delimiter".
    string::size_type pos = str.find_first_of(',', lastPos);

    while (string::npos != pos || string::npos != lastPos) {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip the delimiters. Note the "not_of"
        lastPos = str.find_first_not_of(',', pos);
        // Find the next "non-delimiter".
        pos = str.find_first_of(',', lastPos);
    }
}

string HDFEOS::get_grid_name(string full_path)
{
    int end = full_path.find("/", 14);
    return full_path.substr(0, end + 1);        // Include the last "/".
}

#ifdef SHORT_PATH
// This is a private member function.
string HDFEOS::get_short_name(string varname)
{
    int pos = varname.find_last_of('/', varname.length() - 1);
    return varname.substr(pos + 1);
}
#endif

bool HDFEOS::is_grid()
{
    return _is_grid;
}

bool HDFEOS::is_grid(string name)
{
    int i;
    DBG(cerr << ">HDFEOS::is_grid() Name:" << name << endl);
    for (i = 0; i < (int) full_data_paths.size(); i++) {
        std::string str = full_data_paths.at(i);
        DBG(cerr
            << "=HDFEOS::is_grid() Name:" << name
            << " Datapath: " << str
            << endl);
        if (str == name) {
            return true;
        }
    }
    return false;
}


bool HDFEOS::is_shared_dimension_set()
{
    return _shared_dimension;
}

bool HDFEOS::is_valid()
{
    return _valid;
}

bool HDFEOS::parse_struct_metadata(const char* str_metadata)
{
    if(!_parsed){
        void *buf = hdfeos2_scan_string(str_metadata);
        hdfeos2parse(this);
	hdfeos2_delete_buffer(buf); // added jhrg 8/6/09 to fix ememory leak
        _parsed = true;
        _valid = _parsed;
    }

    return _valid;
}

void HDFEOS::print()
{
#ifdef USE_HDFEOS2_LIB
    DBG(
        cerr
        << ">HDFEOS::print() "
        << "Total number of variables=" << full_data_paths.size()
        << endl
        );

    for (int i = 0; i < (int) full_data_paths.size(); i++) {
        DBG(cerr <<  "Variable " << full_data_paths.at(i) << endl);
    }

    if(is_grid()){
        if (!set_dimension_array()) {
            cerr << "set_dimension_array fail" << endl;
        }
    }

    if (dimension_data) {
        for (unsigned int i = 0; i < dimensions.size(); ++i) {
            dods_float32 *convbuf = dimension_data[i];
            if (!convbuf) continue;
            const string &name = dimensions.at(i);
            int dimsize = dimension_map[name];
#ifdef DODS_DEBUG
            cout << "dimension_data[" << name << "] size " << dimsize << endl;
            for (int j = 0; j < dimsize; ++j)
                cout << convbuf[j] << ", ";
            cout << endl;
#endif // #ifdef DODS_DEBUG
        }
    }
#endif // #ifdef USE_HDFEOS2_LIB
    DBG(cerr
        << "Left = " << point_left << endl
        << "Right = " << point_right << endl
        << "Lower = " << point_lower << endl
        << "Upper = " << point_upper << endl
        << "Total number of paths = " << full_data_paths.size() << endl);

#ifdef DODS_DEBUG
      for (int i = 0; i < (int) full_data_paths.size(); i++) {
          cout << "Element " << full_data_paths.at(i) << endl;
      }
#endif

}

void HDFEOS::reset()
{
    int j;
    _parsed = false;
    _valid = false;
    _shared_dimension = false;
    _is_swath = false;
    _is_grid = false;
#ifdef USE_HDFEOS2_LIB
    _is_orthogonal = false;
    _is_ydimmajor = false;
    path_name = "";
#endif
    xdimsize = 0;
    ydimsize = 0;
    point_lower = 0.0;
    point_upper = 0.0;
    point_left = 0.0;
    point_right = 0.0;
    gradient_x = 0.0;
    gradient_y = 0.0;
    bmetadata_Struct = false;

    if(dimension_data != NULL){
        for (j = 0; j < (int) dimensions.size(); j++) {
            if(dimension_data[j] != NULL)
                delete [] dimension_data[j];
        }
        delete [] dimension_data;
        dimension_data = NULL;
    }

    if(!dimension_map.empty()){
        dimension_map.clear();
    }
    if(!full_data_path_to_dimension_list_map.empty()){
        full_data_path_to_dimension_list_map.clear();
    }
    
    if(!dimensions.empty()){
        dimensions.clear();
    }
    if(!full_data_paths.empty()){
        full_data_paths.clear();
    }
    // Clear the contents of the metadata string buffer.
    strcpy(metadata_Struct, "");
#ifdef USE_HDFEOS2_LIB
    eos2.reset();
#endif

}

bool HDFEOS::set_dimension_array()
{
#ifdef USE_HDFEOS2_LIB
    if(set_dimension_array_hdfeos2()){
        return true;
    }
#endif
    int i = 0;
    int j = 0;
    int size = dimensions.size();

    dods_float32 *convbuf = NULL;

    if (libdap::size_ok(sizeof(dods_float32), size))
        dimension_data = new dods_float32*[size];
    else
        throw InternalErr(__FILE__, __LINE__,
                          "Unable to allocate memory.");
    DBG(cerr << ">set_dimension_array():Dimensions size = " << size <<
        endl);
    for (j = 0; j < (int) dimensions.size(); j++) {
        string dim_name = dimensions.at(j);
        int dim_size = dimension_map[dim_name];

        DBG(cerr << "=set_dimension_array():Dim name = " << dim_name <<
            std::endl);
        DBG(cerr << "=set_dimension_array():Dim size = " << dim_size <<
            std::endl);

        if (dim_size > 0) {
            if (libdap::size_ok(sizeof(dods_float32), dim_size))
                convbuf = new dods_float32[dim_size];
            else
                throw InternalErr(__FILE__, __LINE__,
                                  "Unable to allocate memory.");

            if ((dim_name.find("XDim", (int) dim_name.size() - 4)) !=
                string::npos) {
                gradient_x =
                    (point_right - point_left) / (float) (dim_size);
                for (i = 0; i < dim_size; i++) {
                    convbuf[i] = (dods_float32)
                        (point_left + (float) i * gradient_x +
                         (gradient_x / 2.0)) / 1000000.0;
                }
            } else if ((dim_name.find("YDim", (int) dim_name.size() - 4))
                       != string::npos) {
                gradient_y =
                    (point_upper - point_lower) / (float) (dim_size);
                for (i = 0; i < dim_size; i++) {
                    float sign = -1;
                    convbuf[i] = (dods_float32)
                        (sign) * (point_lower + (float) i * gradient_y +
                                  (gradient_y / 2.0)) / 1000000.0;
                }
            } else {
                for (i = 0; i < dim_size; i++) {
                    convbuf[i] = (dods_float32) i;      // meaningless number.
                }
            }
        }                       // if dim_size > 0
        else {
            DBG(cerr << "Negative dimension " << endl);
            return false;
        }
        dimension_data[j] = convbuf;
    }                           // for
    DBG(cerr << "<set_dimension_array()" << endl);
    return true;
}

void HDFEOS::set_grid(bool flag)
{
    _is_grid = flag;
}


void HDFEOS::set_shared_dimension()
{
    _shared_dimension = true;
}



#ifdef USE_HDFEOS2_LIB
void HDFEOS::add_type(int type)
{
    switch(type){
    case DFNT_INT8:
        types.push_back(dods_byte_c);
        break;

    case DFNT_UINT8:
        types.push_back(dods_byte_c);
        //  Fix it later to dods_int16_c.
        break;

    case DFNT_INT16:
        types.push_back(dods_int16_c);
        break;

    case DFNT_UINT16:
        types.push_back(dods_uint16_c);
        break;

    case DFNT_INT32:
        types.push_back(dods_int32_c);
        break;

    case DFNT_UINT32:
        types.push_back(dods_uint32_c);
        break;

    case DFNT_FLOAT32:
        types.push_back(dods_float32_c);
        break;

    case DFNT_FLOAT64:
        types.push_back(dods_float64_c);
        break;

    default:
        DBG(cerr << "Unknown type:" << type << endl);
        types.push_back(dods_null_c);
        break;
    }
}

void HDFEOS::add_variable(string var_name)
{
    full_data_paths.push_back(var_name);
}

// Override the CF version of is_swath() function.
bool HDFEOS::is_swath()
{
    return _is_swath;
}


bool HDFEOS::is_orthogonal()
{
    return _is_orthogonal;
}

bool HDFEOS::is_ydimmajor()
{
    return _is_ydimmajor;
}


int HDFEOS::get_data_grid(string grid_name, char** val)
{
    DBG(cerr
        << ">HDFEOS::get_data_grid():grid_name="
        << grid_name
        << " Grid size=" << eos2->getGrids().size()
        << endl);

    for (unsigned int i = 0; i < eos2->getGrids().size(); ++i) {
        HDFEOS2::GridDataset*  grid = eos2->getGrids()[i];
        DBG(cerr
            << "=HDFEOS::get_data_grid():Grid size "
            << grid->getDataFields().size()
            << endl);
        for (unsigned int j = 0; j < grid->getDataFields().size(); ++j) {
            DBG(cerr
                << "=HDFEOS::get_data_grid():Datafield name "
                << grid->getDataFields()[j]->getName()
                << endl);
            if(grid->getDataFields()[j]->getName() == grid_name){
                DBG(cerr << "=HDFEOS::get_data_grid(): Match found" << endl);
                *val = (char*)grid->getDataFields()[j]->getData().get();
                if(val == NULL){
                    return 0;
                }
                else{
                    return grid->getDataFields()[j]->getData().length();
                }
            }
        }
    }

}


int HDFEOS::get_data_swath(string swath_name, char** val)
{
    DBG(cerr
        << ">HDFEOS::get_data_swath():swath_name="
        << swath_name
        << " Swath size=" << eos2->getSwaths().size()
        << endl);

    for (unsigned int i = 0; i < eos2->getSwaths().size(); ++i) {
        HDFEOS2::SwathDataset*  swath = eos2->getSwaths()[i];
        DBG(cerr
            << "=HDFEOS::get_data_swath():Swath size "
            << swath->getDataFields().size()
            << endl);
        for (unsigned int j = 0; j < swath->getDataFields().size(); ++j) {
            DBG(cerr
                << "=HDFEOS::get_data_swath():Datafield name "
                << swath->getDataFields()[j]->getName()
                << endl);
            if(swath->getDataFields()[j]->getName() == swath_name){
                DBG(cerr << "=HDFEOS::get_data_swath(): Match found" << endl);
                *val = (char*)swath->getDataFields()[j]->getData().get();
                if(val == NULL){
                    return 0;
                }
                else{
                    return swath->getDataFields()[j]->getData().length();
                }
            }
        } // For DataFields
        for (unsigned int i = 0; i < swath->getGeoFields().size(); ++i){
            if(swath->getGeoFields()[i]->getName() == swath_name){
                DBG(cerr
                    << "=HDFEOS::get_data_swath(): Match found in GeoFields"
                    << endl);
                *val = (char*)swath->getGeoFields()[i]->getData().get();
                if(val == NULL){
                    return 0;
                }
                else{
                    return swath->getGeoFields()[i]->getData().length();
                }
            }

        } // For GeoFields

    }

}


int HDFEOS::get_data_type(int i)
{
    return types.at(i);
}


int HDFEOS::get_xdim_size()
{
    return (int) xdimsize;
}

int HDFEOS::get_ydim_size()
{
    return (int) ydimsize;
}



void HDFEOS::handle_attributes(const HDFEOS2::Attribute* attribute)
{
    DBG(cerr << "Attr: " << attribute->getName() << endl);

}


void HDFEOS::handle_datafield(const HDFEOS2::Field *field)
{
    // A field is equivalent to a DAP variable.
    DBG(cerr << ">HDFEOS::handle_datafield() "
        << "Field's name=" << field->getName()
        << endl);

    string varname = "";

#ifdef USE_FULL_PATH_NAME       // This macro is for future use. Ignore it.
    varname = "/" + path_name + "/" + field->getName();
#else
    varname = field->getName();
#endif

    add_variable(varname);
    add_type(field->getType());

    // Process dimensions of each variable.
    ostringstream dimlist;
    const vector<HDFEOS2::Dimension *> &dims = field->getDimensions();
    for (unsigned int i = 0; i < dims.size(); ++i) {
        const HDFEOS2::Dimension*  dim = dims[i];
        string dimname = "";

#ifdef USE_FULL_PATH_NAME  // This macro is for future use. Ignore it.
        dimname = "/" + path_name + "/" + dim->getName();
#else
        dimname = dim->getName();
#endif

        if (i > 0)
            dimlist << ",";
        dimlist << dimname;
        add_dimension_map(dimname, dim->getSize());
    }
    add_dimension_list(varname, dimlist.str());
}

void HDFEOS::handle_grid(const HDFEOS2::GridDataset *grid)
{
    int xdimsize_prev = 0;
    int ydimsize_prev = 0;

    // Set all grid-related key information.
    const float64* upleft = grid->getInfo().getUpLeft();
    const float64* lowright = grid->getInfo().getLowRight();

    DBG(cerr << "Up-Left " << upleft[0] << "," << upleft[1]  << endl);
    DBG(cerr << "Low-Right " << lowright[0] << "," << lowright[1]  << endl);

    point_left = upleft[0];
    point_right = lowright[0];
    point_upper = upleft[1];
    point_lower = lowright[1];

    if(xdimsize > 0 && ydimsize > 0){ // Save the previous size.
        xdimsize_prev = xdimsize;
        ydimsize_prev = ydimsize;
    }

    xdimsize = grid->getInfo().getX();
    ydimsize = grid->getInfo().getY();

    if(xdimsize != xdimsize_prev && xdimsize_prev > 0){
        throw InternalErr("This HDF-EOS2 file has multiple Grids with \
different longitude dimension sizes.");
    }

    if(ydimsize != ydimsize_prev && ydimsize_prev > 0){
        throw InternalErr("This HDF-EOS2 file has multiple Grids with \
different latitude dimension sizes.");
    }

    gradient_x =
        (point_right - point_left) / (float) (xdimsize);

    gradient_y =
        (point_upper - point_lower) / (float) (ydimsize);

    path_name = grid->getName();

    DBG(cerr
        << ">HDFEOS::handle_grid() Grid DataFields.size="
        << grid->getDataFields().size() << endl);
    for (unsigned int i = 0; i < grid->getDataFields().size(); ++i){
        handle_datafield(grid->getDataFields()[i]);
    }

    for (unsigned int i = 0; i < grid->getAttributes().size(); ++i){
        DBG(cerr << "Gridname:" << grid->getName() << " ");
        handle_attributes(grid->getAttributes()[i]);
    }
}



void HDFEOS::handle_swath(const HDFEOS2::SwathDataset *swath)
{

    DBG(cerr << ">HDFEOS::handle_swath()"
        << " Swath name=" <<  swath->getName()
        << " DataFields.size=" << swath->getDataFields().size()
        << endl);

    for (unsigned int i = 0; i < swath->getDataFields().size(); ++i){
        std::vector<std::string> associated;
        handle_datafield(swath->getDataFields()[i]);
    }

    for (unsigned int i = 0; i < swath->getGeoFields().size(); ++i)
        handle_datafield(swath->getGeoFields()[i]);
    for (unsigned int i = 0; i < swath->getAttributes().size(); ++i){
        handle_attributes(swath->getAttributes()[i]);
    }

}

int HDFEOS::open(char* filename)
{
    try {
        // Open the file with HDF-EOS2 library.
        // Here, reset() is a member function of auto_ptr which is a standard
        // C++ template.
        eos2.reset(HDFEOS2::File::ReadAndAdjust(filename));

        if(eos2->getGrids().size() > 0){
            _is_grid = true;
        }

        if(eos2->getSwaths().size() > 0){
            _is_swath = true;
        }

        if(_is_grid && _is_swath){
            throw InternalErr("This HDFEOS2 file has both Grids and Swath.");
        }

        if(_is_grid || _is_swath)
            // Determine if it's a valid HDF-EOS2.
            _valid = true;

        for (unsigned int i = 0; i < eos2->getGrids().size(); ++i) {
            handle_grid(eos2->getGrids()[i]);
        }

        for (unsigned int i = 0; i < eos2->getSwaths().size(); ++i) {
            handle_swath(eos2->getSwaths()[i]);
        }

        return 0;
    }
    catch (const HDFEOS2::Exception &e) {
        cerr << e.what() << endl;
    }
    return -1;
}

bool HDFEOS::set_dimension_array_hdfeos2()
{
    int j = 0;
    int size = dimensions.size();
    DBG(cerr << "Dimensions size is " << size << endl);
    // TODO: add_dimension_map() assumed that a dimension name is unique in
    // one EOS2 file; so, the first grid is chosen to get longitude and
    // latitude.
    const vector<HDFEOS2::GridDataset *> &grids = eos2->getGrids();
    if (grids.empty())
        return false;
    const HDFEOS2::GridDataset *firstgrid = grids[0];
    dods_float32 *convbuf = NULL;

    dimension_data = new dods_float32*[size];
    // for safe delete operation
    for (j = 0; j < (int) dimensions.size(); j++)
        dimension_data[j] = 0;

    try {
        // CAUTION: When the projection is orthogonal like the Geographic
        // projection,convbuf can be correctly stored; otherwise,
        // one-dimensional convbuf cannot hold enough data.
        // Also, all dimensions other than XDim or YDim will not have
        // valid convbuf.
        const float64 *lons = firstgrid->getCalculated().getLongitude();
        const float64 *lats = firstgrid->getCalculated().getLatitude();


        _is_ydimmajor = firstgrid->getCalculated().isYDimMajor();
        _is_orthogonal = firstgrid->getCalculated().isOrthogonal();
        for (j = 0; j < (int) dimensions.size(); j++) {
            string dim_name = dimensions.at(j);
            int dim_size = dimension_map[dim_name];
            DBG(cerr << "Dim_name=" << dim_name << " Dim_size=" << dim_size
                << endl);
            if (_is_orthogonal) {
                if (dim_name == "XDim") {
                    convbuf = new dods_float32[dim_size];
                    for (int k = 0; k < dim_size; ++k) {
                        if (_is_ydimmajor)
                            convbuf[k] = static_cast<dods_float32>
                                (lons[k]);
                        else{
                            convbuf[k] = static_cast<dods_float32>
                                (lons[k * ydimsize]);
                        }
                    }
                    dimension_data[j] = convbuf;
                } // XDim
                else if (dim_name == "YDim") {
                    convbuf = new dods_float32[dim_size];
                    for (int k = 0; k < dim_size; ++k) {
                        if (ydimsize)
                            convbuf[k] = static_cast<dods_float32>
                                (lats[k * xdimsize]);
                        else
                            convbuf[k] = static_cast<dods_float32>
                                (lats[k]);
                    }
                    dimension_data[j] = convbuf;
                } // YDim
                else{
                    convbuf = new dods_float32[dim_size];
                    for (int k = 0; k < dim_size; ++k) {
                        convbuf[k] = k;	// dummy data
                    }
                    dimension_data[j] = convbuf;
                }
            } // Orthogonal
            else{
                DBG(cerr << "Not orthogonal" << endl);
                // This works for sinusoidal case only.
                if (dim_name == "XDim") {
                    convbuf = new dods_float32[xdimsize * ydimsize];
                    for (int k = 0; k < xdimsize * ydimsize; ++k) {
                        convbuf[k] = static_cast<dods_float32>(lons[k]);
                         DBG(cerr << convbuf[k] << " ");
                    }
                    cerr << endl;
                    dimension_data[j] = convbuf;
                }
                else if (dim_name == "YDim"){
                    convbuf = new dods_float32[xdimsize * ydimsize];
                    for (int k = 0; k < xdimsize * ydimsize; ++k) {
                        convbuf[k] = static_cast<dods_float32>(lats[k]);
                        DBG(cerr << convbuf[k] << " ");
                    }
                    cerr << endl;
                    dimension_data[j] = convbuf;
                }
                else{
                    convbuf = new dods_float32[dim_size];
                    for (int k = 0; k < dim_size; ++k) {
                        convbuf[k] = k;	// dummy data
                    }
                    dimension_data[j] = convbuf;
                }
            }
        }                           // for
        firstgrid->getCalculated().dropLongitudeLatitude();
        return true;
    }
    catch (const HDFEOS2::Exception &e) {
        cerr << e.what() << endl;
    }
    return false;
}

#endif // #ifdef USE_HDFEOS2_LIB

