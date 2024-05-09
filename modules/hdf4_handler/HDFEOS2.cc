/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
//
//  Authors:   Kent Yang <myang6@hdfgroup.org> Choonghwan Lee 
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifdef USE_HDFEOS2_LIB

#include <sstream>
#include <algorithm>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include <cfloat>
#include <math.h>
#include <sys/stat.h>

#include "HDFCFUtil.h"
#include "HDFEOS2.h"
#include "HDF4RequestHandler.h"

// Names to be searched by
//   get_geodim_x_name()
//   get_geodim_y_name()
//   get_latfield_name()
//   get_lonfield_name()
//   get_geogrid_name()

// Possible XDim names
const char *HDFEOS2::File::_geodim_x_names[] = {"XDim", "LonDim","nlon"};

// Possible YDim names.
const char *HDFEOS2::File::_geodim_y_names[] = {"YDim", "LatDim","nlat"};

// Possible latitude names.
const char *HDFEOS2::File::_latfield_names[] = {
    "Latitude", "Lat","YDim", "LatCenter"
};

// Possible longitude names.
const char *HDFEOS2::File::_lonfield_names[] = {
    "Longitude", "Lon","XDim", "LonCenter"
};

// For some grid products, latitude and longitude are put under a special geogrid.
// One possible name is "location".
const char *HDFEOS2::File::_geogrid_names[] = {"location"};

using namespace HDFEOS2;
using namespace std;

// The following is for generating exception messages.
template<typename T, typename U, typename V, typename W, typename X>
static void _throw5(const char *fname, int line, int numarg,
                    const T &a1, const U &a2, const V &a3, const W &a4,
                    const X &a5)
{
    ostringstream ss;
    ss << fname << ":" << line << ":";
    for (int i = 0; i < numarg; ++i) {
        ss << " ";
        switch (i) {
        case 0: ss << a1; break;
        case 1: ss << a2; break;
        case 2: ss << a3; break;
        case 3: ss << a4; break;
        case 4: ss << a5; break;
        default:
            ss<<"  Argument number is beyond 5 ";
        }
    }
    throw Exception(ss.str());
}

/// The followings are convenient functions to throw exceptions with different
// number of arguments.
/// We assume that the maximum number of arguments is 5.
#define throw1(a1)  _throw5(__FILE__, __LINE__, 1, a1, 0, 0, 0, 0)
#define throw2(a1, a2)  _throw5(__FILE__, __LINE__, 2, a1, a2, 0, 0, 0)
#define throw3(a1, a2, a3)  _throw5(__FILE__, __LINE__, 3, a1, a2, a3, 0, 0)
#define throw4(a1, a2, a3, a4)  _throw5(__FILE__, __LINE__, 4, a1, a2, a3, a4, 0)
#define throw5(a1, a2, a3, a4, a5)  _throw5(__FILE__, __LINE__, 5, a1, a2, a3, a4, a5)

#define assert_throw0(e)  do { if (!(e)) throw1("assertion failure"); } while (false)
#define assert_range_throw0(e, ge, l)  assert_throw0((ge) <= (e) && (e) < (l))

// Convenient function used in destructors.
struct delete_elem
{
    template<typename T> void operator()(T *ptr)
    {
        delete ptr;
    }
};

// Destructor for class File.
File::~File()
{
    if (gridfd !=-1) {
        for (auto i:grids)
            delete i;
        // Grid file IDs will be closed in HDF4RequestHandler.cc.
    }

    if (swathfd !=-1) {
        for (auto i:swaths)
            delete i;
    }

}

/// Read all the information in this file from the EOS2 APIs.
File * File::Read(const char *path, int32 mygridfd, int32 myswathfd) 
{

    auto file = new File(path);
    if (file == nullptr)
        throw1("Memory allocation for file class failed. ");

    file->gridfd = mygridfd;
    file->swathfd = myswathfd;

    vector<string> gridlist;
    if (!Utility::ReadNamelist(file->path.c_str(), GDinqgrid, gridlist)) {
        delete file;
        throw1("Grid ReadNamelist failed.");
    }

    try {
        for (const auto &grid: gridlist)
            file->grids.push_back(GridDataset::Read(file->gridfd, grid));
    }
    catch(...) {
        delete file;
        throw1("GridDataset Read failed");
    }

    vector<string> swathlist;
    if (!Utility::ReadNamelist(file->path.c_str(), SWinqswath, swathlist)){
        delete file;
        throw1("Swath ReadNamelist failed.");
    }

    try {
        for (const auto &swath:swathlist)
            file->swaths.push_back(SwathDataset::Read(file->swathfd, swath));
    }
    catch(...) {
        delete file;
        throw1("SwathDataset Read failed.");
    }


    // We only obtain the name list of point objects but not don't provide
    // other information of these objects.
    // The client will only get the name list of point objects.
    vector<string> pointlist;
    if (!Utility::ReadNamelist(file->path.c_str(), PTinqpoint, pointlist)){
        delete file;
        throw1("Point ReadNamelist failed.");
    }

    //See if I can make coverity happy because it doesn't understand throw macro.
    for (const auto&point: pointlist)
        file->points.push_back(PointDataset::Read(-1, point));

    // If this is not an HDF-EOS2 file, returns exception as false.
    if (file->grids.empty() && file->swaths.empty()
                            && file->points.empty()) {
        Exception e("Not an HDF-EOS2 file");
        e.setFileType(false);
        delete file;
        throw e;  
    }
    return file;
}


// A grid's X-dimension can have different names: XDim, LatDim, etc.
// This function returns the name of X-dimension which is used in the given file.
//  For better performance, we check the first grid or swath only.
string File::get_geodim_x_name()
{
    if (!_geodim_x_name.empty())
        return _geodim_x_name;
    _find_geodim_names();
    return _geodim_x_name;
}

// A grid's Y-dimension can have different names: YDim, LonDim, etc.
// This function returns the name of Y-dimension which is used in the given file.
//  For better performance, we check the first grid or swath only.
string File::get_geodim_y_name()
{
    if (!_geodim_y_name.empty())
        return _geodim_y_name;
    _find_geodim_names();
    return _geodim_y_name;
}

// In some cases, values of latitude and longitude are stored in data fields.
// Since the latitude field and longitude field do not have unique names
// (e.g., latitude field can be "latitude", "Lat", ...),
//  we need to retrieve the field name.
// The following two functions does this job.
// For better performance, we check the first grid or swath only.

string File::get_latfield_name()
{
    if (!_latfield_name.empty())
        return _latfield_name;
    _find_latlonfield_names();
    return _latfield_name;
}

string File::get_lonfield_name()
{
    if (!_lonfield_name.empty())
        return _lonfield_name;
    _find_latlonfield_names();
    return _lonfield_name;
}

// In some cases, a dedicated grid is used to store the values of
// latitude and longitude. The following function finds the name
// of the geo grid.

string File::get_geogrid_name()
{
    if (!_geogrid_name.empty())
        return _geogrid_name;
    _find_geogrid_name();
    return _geogrid_name;
}

// Internal function used by  
// get_geodim_x_name and get_geodim_y_name functions.
// This function is not intended to be used outside the 
// get_geodim_x_name and get_geodim_y_name functions.

void File::_find_geodim_names()
{
    set<string> geodim_x_name_set;
    for(size_t i = 0; i<sizeof(_geodim_x_names) / sizeof(const char *); i++)
        geodim_x_name_set.emplace(_geodim_x_names[i]);

    set<string> geodim_y_name_set;
    for (size_t i = 0; i<sizeof(_geodim_y_names) / sizeof(const char *); i++)
        geodim_y_name_set.emplace(_geodim_y_names[i]);

    const size_t gs = grids.size();
    // For performance, we're checking this for the first grid 
    if (gs >0)
    {
        const Dataset *dataset=nullptr;
        dataset = static_cast<Dataset*>(grids[0]);

        const vector<Dimension *>& dims = dataset->getDimensions();
        for (const auto &dim:dims)  
        {
            // Essentially this code will grab any dimension names that is
            // NOT predefined "XDim","LonDim","nlon" for geodim_x_name;
            // any dimension names that is NOT predefined "YDim","LatDim","nlat"
            // for geodim_y_name. This is in theory not right. Given the
            // fact that this works with the current HDF-EOS2 products and there
            // will be no more HDF-EOS2 products. We will leave the code this way.
            if (geodim_x_name_set.find(dim->getName()) != geodim_x_name_set.end())
                _geodim_x_name = dim->getName();
            else if (geodim_y_name_set.find(dim->getName()) != geodim_y_name_set.end())
                _geodim_y_name = dim->getName();
        }
    }
    if (_geodim_x_name.empty())
        _geodim_x_name = _geodim_x_names[0];
    if (_geodim_y_name.empty())
        _geodim_y_name = _geodim_y_names[0];
}

// Internal function used by  
// get_latfield_name and get_lonfield_name functions.
// This function is not intended to be used outside 
// the get_latfield_name and get_lonfield_name functions.

void File::_find_latlonfield_names()
{
    set<string> latfield_name_set;
    for(size_t i = 0; i<sizeof(_latfield_names) / sizeof(const char *); i++)
        latfield_name_set.emplace(_latfield_names[i]);

    set<string> lonfield_name_set;
    for(size_t i = 0; i<sizeof(_lonfield_names) / sizeof(const char *); i++)
        lonfield_name_set.emplace(_lonfield_names[i]);

    const size_t gs = grids.size();
    const size_t ss = swaths.size();
    for(size_t i=0;i<1 ;i++)
    {
        const Dataset *dataset = nullptr;
        SwathDataset *sw = nullptr;
        if (i<gs)
            dataset = static_cast<Dataset*>(grids[i]);
        else if (i < gs + ss)
        {
            sw = swaths[i-gs];
            dataset = static_cast<Dataset*>(sw);
        }
        else
            break;

        const vector<Field *>& fields = dataset->getDataFields();
        for (const auto &field:fields)
        {
            if (latfield_name_set.find(field->getName()) != latfield_name_set.end())
                _latfield_name = field->getName();
            else if (lonfield_name_set.find(field->getName()) != lonfield_name_set.end())
                _lonfield_name = field->getName();
        }

        if (sw)
        {
            const vector<Field *>& geofields = dataset->getDataFields();
            for(const auto &gfield:geofields)  
            {
                if (latfield_name_set.find(gfield->getName()) != latfield_name_set.end())
                    _latfield_name = gfield->getName();
                else if (lonfield_name_set.find(gfield->getName()) != lonfield_name_set.end())
                    _lonfield_name = gfield->getName();
            }
        }
    }
    if (_latfield_name.empty())
        _latfield_name = _latfield_names[0];
    if (_lonfield_name.empty())
        _lonfield_name = _lonfield_names[0];

}

// Internal function used by
// the get_geogrid_name function.
// This function is not intended to be used outside the get_geogrid_name function.

void File::_find_geogrid_name()
{
    set<string> geogrid_name_set;
    for(size_t i = 0; i<sizeof(_geogrid_names) / sizeof(const char *); i++)
        geogrid_name_set.emplace(_geogrid_names[i]);

    const size_t gs = grids.size();
    const size_t ss = swaths.size();
    for(size_t i=0; ;i++)
    {
        const Dataset *dataset = nullptr;
        if (i<gs)
            dataset = static_cast<Dataset*>(grids[i]);
        else if (i < gs + ss)
            dataset = static_cast<Dataset*>(swaths[i-gs]);
        else
            break;

        if (geogrid_name_set.find(dataset->getName()) != geogrid_name_set.end())
            _geogrid_name = dataset->getName();
    }
    if (_geogrid_name.empty())
        _geogrid_name = "location";
}

// Check if we have the dedicated lat/lon grid.
void File::check_onelatlon_grids() {

    // 0. obtain "Latitude","Longitude" and "location" set.
    string LATFIELDNAME = this->get_latfield_name();
    string LONFIELDNAME = this->get_lonfield_name();
    
    // Now only there is only one geo grid name "location", so don't call it know.
    string GEOGRIDNAME = "location";

    //only one lat/lon and it is under GEOGRIDNAME
    int onellcount = 0; 

    // Check if lat/lon is found under other grids.
    int morellcount = 0; 

    // Loop through all grids
    for (const auto &grid:grids){

        // Loop through all fields
        for (const auto &field:grid->getDataFields()) {
            if (grid->getName()==GEOGRIDNAME){
                if (field->getName()==LATFIELDNAME){
                    onellcount++;
                    grid->latfield = field;
                }
                if (field->getName()==LONFIELDNAME){ 
                    onellcount++;
                    grid->lonfield = field;
                }
                if (onellcount == 2) 
                    break;//Finish this grid
            }
            else {// Here we assume that lat and lon are always in pairs.
                if ((field->getName()==LATFIELDNAME)||(field->getName()==LONFIELDNAME)){ 
                    grid->ownllflag = true;
                    morellcount++;
                    break;
                }
            }
        }
    }

    if (morellcount ==0 && onellcount ==2) 
        this->onelatlon = true; 
}

// For one grid, need to handle the third-dimension(both existing and missing) coordinate variables
void File::handle_one_grid_zdim(GridDataset* gdset) {

    // Obtain "XDim","YDim"
    string DIMXNAME = this->get_geodim_x_name();
    string DIMYNAME = this->get_geodim_y_name();   

    bool missingfield_unlim_flag = false;
    const Field *missingfield_unlim = nullptr;

    // This is a big assumption, it may be wrong since not every 1-D field 
    // with the third dimension(name and size) is a coordinate
    // variable. We have to watch the products we've supported. KY 2012-6-13

    // Unique 1-D field's dimension name list.
    set<string> tempdimlist; 
    pair<set<string>::iterator,bool> tempdimret;

    for (const auto &field:gdset->getDataFields()) { 
        //We only need to search those 1-D fields

        if (field->getRank()==1){

            // DIMXNAME and DIMYNAME correspond to latitude and longitude.
            // They should NOT be treated as dimension names missing fields. It will be handled differently.
            if ((field->getDimensions())[0]->getName()!=DIMXNAME && (field->getDimensions())[0]->getName()!=DIMYNAME){

                tempdimret = tempdimlist.insert((field->getDimensions())[0]->getName());

                // Kent: The following implementation may not be always right. This essentially is the flaw of the 
                // data product if a file encounters this case. Only unique 1-D third-dimension field should be provided.
                // Only pick up the first 1-D field that the third-dimension 
                if (tempdimret.second == true) {

                    HDFCFUtil::insert_map(gdset->dimcvarlist, (field->getDimensions())[0]->getName(),
                                          field->getName());
                    field->fieldtype = 3;
                    if (field->getName() == "Time") 
                        field->fieldtype = 5;// IDV can handle 4-D fields when the 4th dim is Time.
                }
            }
        }
    } 

    // Add the missing Z-dimension field.
    // Some dimension name's corresponding fields are missing, 
    // so add the missing Z-dimension fields based on the dimension names. When the real
    // data is read, nature number 1,2,3,.... will be filled!
    // NOTE: The latitude and longitude dim names are not handled yet.  

    // The above handling is also based on  a big assumption. This is the best the 
    // handler can do without having the extra information outside the HDF-EOS2 file. KY 2012-6-12
    // Loop through all dimensions of this grid.
    for (const auto &gdim:gdset->getDimensions()) { 

        // Don't handle DIMXNAME and DIMYNAME yet.
        if (gdim->getName()!=DIMXNAME && gdim->getName()!=DIMYNAME){

            // This dimension needs a field
            if ((tempdimlist.find(gdim->getName())) == tempdimlist.end()){
                      
                // Need to create a new data field vector element with the name and dimension as above.
                auto missingfield = new Field();
                missingfield->name = gdim->getName();
                missingfield->rank = 1;
                
                //This is an HDF constant.the data type is always integer.
                missingfield->type = DFNT_INT32;

                // Dimension of the missing field
                auto dim = new Dimension(gdim->getName(),gdim->getSize());

                // only 1 dimension
                missingfield->dims.push_back(dim);

                if (0 == gdim->getSize()) {
                    missingfield_unlim_flag = true;
                    missingfield_unlim = missingfield;
                }
                
                // Provide information for the missing data, since we need to calculate the data, so
                // the information is different than a normal field.

                // added Z-dimension coordinate variable with nature number
                missingfield->fieldtype = 4; 

                // input data is empty now. We need to review this approach in the future.
                // The data will be retrieved in HDFEOS2ArrayMissGeoField.cc. KY 2013-06-14
                gdset->datafields.push_back(missingfield);
                HDFCFUtil::insert_map(gdset->dimcvarlist, (missingfield->getDimensions())[0]->getName(), 
                                   missingfield->name);
 
            }
        }
    }

    //Correct the unlimited dimension size.
    bool temp_missingfield_unlim_flag = missingfield_unlim_flag;
    if (true == temp_missingfield_unlim_flag) {
        for (unsigned int i =0; i<gdset->getDataFields().size(); i++) {

            for (const auto &gdim:gdset->getDimensions()) {
                
                if (gdim->getName() == (missingfield_unlim->getDimensions())[0]->getName()) {
                    if (gdim->getSize()!= 0) {
                        Dimension *dim = missingfield_unlim->getDimensions()[0];

                        // The unlimited dimension size is updated.
                        dim->dimsize = gdim->getSize();
                        missingfield_unlim_flag = false;
                        break;
                    }
                }

            }
            if (false == missingfield_unlim_flag) 
                break;
        }
    }

}

// For one grid, need to handle lat/lon(both existing lat/lon and calculated lat/lon from EOS2 APIs)
void File::handle_one_grid_latlon(GridDataset* gdset) 
{

    // Obtain "XDim","YDim","Latitude","Longitude" 
    string DIMXNAME = this->get_geodim_x_name();
    string DIMYNAME = this->get_geodim_y_name();

    string LATFIELDNAME = this->get_latfield_name();
    string LONFIELDNAME = this->get_lonfield_name(); 


    // This grid has its own latitude/longitude
    if (gdset->ownllflag) {

        // Searching the lat/lon field from the grid. 
        for (const auto &field:gdset->getDataFields()) {
 
            if (field->getName() == LATFIELDNAME) {

                // set the flag to tell if this is lat or lon. 
                // The unit will be different for lat and lon.
                field->fieldtype = 1;

                // Latitude rank should not be greater than 2.
                // Here I don't check if the rank of latitude is the same as the longitude. 
                // Hopefully it never happens for HDF-EOS2 cases.
                // We are still investigating if Java clients work 
                // when the rank of latitude and longitude is greater than 2.
                if (field->getRank() > 2) 
                    throw3("The rank of latitude is greater than 2",
                            gdset->getName(),field->getName());

                if (field->getRank() != 1) {

                    // Obtain the major dim. For most cases, it is YDim Major. 
                    // But still need to watch out the rare cases.
                    field->ydimmajor = gdset->getCalculated().isYDimMajor();

                    // If the 2-D lat/lon can be condensed to 1-D.
                    // For current HDF-EOS2 files, only GEO and CEA can be condensed. 
                    // To gain performance,
                    // we don't check the real latitude values.
                    int32 projectioncode = gdset->getProjection().getCode();
                    if (projectioncode == GCTP_GEO || projectioncode ==GCTP_CEA) {
                        field->condenseddim = true;
                    }

                    // Now we want to handle the dim and the var lists.
                    // If the lat/lon can be condensed to 1-D array, 
                    // COARD convention needs to be followed.
                    // Since COARD requires that the dimension name of lat/lon is the same as the field name of lat/lon,
                    // we still need to handle this case in the later step(in function handle_grid_coards).
                    // Regardless of dimension  major, always lat->YDim, lon->XDim;
                    // We don't need to adjust the dimension rank.
                    for (const auto &dim:field->getDimensions()) {
                        if (dim->getName() == DIMYNAME) 
                            HDFCFUtil::insert_map(gdset->dimcvarlist, dim->getName(), field->getName());
                    }
                }
                // This is the 1-D case, just inserting  the dimension, field pair.
                else { 
                    HDFCFUtil::insert_map(gdset->dimcvarlist, ((field->getDimensions())[0])->getName(), 
                                           field->getName());
                }
            } 
            else if (field->getName() == LONFIELDNAME) {

                // set the flag to tell if this is lat or lon. The unit will be different for lat and lon.
                field->fieldtype = 2;

                // longitude rank should not be greater than 2.
                // Here I don't check if the rank of latitude and longitude is the same. 
                // Hopefully it never happens for HDF-EOS2 cases.
                // We are still investigating if Java clients work when the rank of latitude and longitude is greater than 2.
                if (field->getRank() >2) 
                    throw3("The rank of Longitude is greater than 2",gdset->getName(),field->getName());

                if (field->getRank() != 1) {

                    // Obtain the major dim. For most cases, it is YDim Major. But still need to check for rare cases.
                    field->ydimmajor = gdset->getCalculated().isYDimMajor();

                    // If the 2-D lat/lon can be condensed to 1-D.
                    //For current HDF-EOS2 files, only GEO and CEA can be condensed. To gain performance,
                    // we don't check with real values.
                    int32 projectioncode = gdset->getProjection().getCode();
                    if (projectioncode == GCTP_GEO || projectioncode ==GCTP_CEA) {
                        field->condenseddim = true;
                    }

                    // Now we want to handle the dim and the var lists.
                    // If the lat/lon can be condensed to 1-D array, COARD convention needs to be followed.
                    // Since COARD requires that the dimension name of lat/lon is the same as the field name of lat/lon,
                    // we still need to handle this case at last.
                    // When the field can be condensed, regardless of dimension major, the EOS convention is always lat->YDim, lon->XDim;
                    // We don't need to adjust the dimension rank.
                    // For 2-D lat/lon case: since dimension order doesn't matter, so we always assume lon->XDim, lat->YDim.
                    for (const auto &dim:field->getDimensions()) {
                        if (dim->getName() == DIMXNAME) 
                            HDFCFUtil::insert_map(gdset->dimcvarlist, dim->getName(), field->getName());
                    }
                }
                // This is the 1-D case, just inserting  the dimension, field pair.
                else { 
                    HDFCFUtil::insert_map(gdset->dimcvarlist, ((field->getDimensions())[0])->getName(), 
                                          field->getName());
                }
            }
        } 
    }
    else { // this grid's lat/lon has to be calculated.

        // Latitude and Longitude 
        auto latfield = new Field();
        auto lonfield = new Field();

        latfield->name = LATFIELDNAME;
        lonfield->name = LONFIELDNAME;

        // lat/lon is a 2-D array
        latfield->rank = 2;
        lonfield->rank = 2;

        // The data type is always float64. DFNT_FLOAT64 is the equivalent float64 type.
        latfield->type = DFNT_FLOAT64;
        lonfield->type = DFNT_FLOAT64;

        // Latitude's fieldtype is 1
        latfield->fieldtype = 1;

        // Longitude's fieldtype is 2
        lonfield->fieldtype = 2;

        // Check if YDim is the major order. 
        // Obtain the major dim. For most cases, it is YDim Major. But some cases may be not. Still need to check.
        latfield->ydimmajor = gdset->getCalculated().isYDimMajor();
        lonfield->ydimmajor = latfield->ydimmajor;

        // Obtain XDim and YDim size.
        int xdimsize = gdset->getInfo().getX();
        int ydimsize = gdset->getInfo().getY();

        // Add dimensions. If it is YDim major,the dimension name list is "YDim XDim", otherwise, it is "XDim YDim". 
        // For LAMAZ projection, Y dimension is always supposed to be major for calculating lat/lon, 
        // but for dimension order, it should be consistent with data fields. (LD -2012/01/16
        bool dmajor=(gdset->getProjection().getCode()==GCTP_LAMAZ)? gdset->getCalculated().DetectFieldMajorDimension()
                                                                  : latfield->ydimmajor;

        if (dmajor) { 
            auto dimlaty = new Dimension(DIMYNAME,ydimsize);
            latfield->dims.push_back(dimlaty);
            auto dimlony = new Dimension(DIMYNAME,ydimsize);
            lonfield->dims.push_back(dimlony);
            auto dimlatx = new Dimension(DIMXNAME,xdimsize);
            latfield->dims.push_back(dimlatx);
            auto dimlonx = new Dimension(DIMXNAME,xdimsize);
            lonfield->dims.push_back(dimlonx);
        }
        else {
            auto dimlatx = new Dimension(DIMXNAME,xdimsize);
            latfield->dims.push_back(dimlatx);
            auto dimlonx = new Dimension(DIMXNAME,xdimsize);
            lonfield->dims.push_back(dimlonx);
            auto dimlaty = new Dimension(DIMYNAME,ydimsize);
            latfield->dims.push_back(dimlaty);
            auto dimlony = new Dimension(DIMYNAME,ydimsize);
            lonfield->dims.push_back(dimlony);
        }

        // Obtain info upleft and lower right for special longitude.

        const float64* upleft = gdset->getInfo().getUpLeft();
        const float64* lowright = gdset->getInfo().getLowRight();

        // SOme special longitude is from 0 to 360.We need to check this case.
        int32 projectioncode = gdset->getProjection().getCode();
        if (((int)lowright[0]>180000000) && ((int)upleft[0]>-1)) {
            // We can only handle geographic projection now.
            // This is the only case we can handle.
            if (projectioncode == GCTP_GEO) {// Will handle when data is read.
                lonfield->speciallon = true;
                // When HDF-EOS2 cache is involved, we have to also set the 
                // speciallon flag for the latfield since the cache file
                // includes both lat and lon fields, and even the request 
                // is only to generate the lat field, the lon field also needs to 
                // be updated to write the proper cache. KY 2016-03-16 
                if (HDF4RequestHandler::get_enable_eosgeo_cachefile() == true) 
                    latfield->speciallon = true;
            }
        }

        // Some MODIS MCD files don't follow standard format for lat/lon (DDDMMMSSS);
        // they simply represent lat/lon as -180.0000000 or -90.000000.
        // HDF-EOS2 library won't give the correct value based on these value.
        // These need to be remembered and resumed to the correct format when retrieving the data.
        // Since so far we haven't found region of satellite files is within 0.1666 degree(1 minute)
        // so, we divide the corner coordinate by 1000 and see if the integral part is 0.
        // If it is 0, we know this file uses special lat/lon coordinate.

        if (((int)(lowright[0]/1000)==0) &&((int)(upleft[0]/1000)==0) 
            && ((int)(upleft[1]/1000)==0) && ((int)(lowright[1]/1000)==0)) {
            if (projectioncode == GCTP_GEO){
                lonfield->specialformat = 1;
                latfield->specialformat = 1;
            }             
        }

        // Some TRMM CERES Grid Data have "default" to be set for the corner coordinate,
        // which they really mean for the whole globe(-180 - 180 lon and -90 - 90 lat). 
        // We will remember the information and change
        // those values when we read the lat and lon.

        if (((int)(lowright[0])==0) &&((int)(upleft[0])==0)
            && ((int)(upleft[1])==0) && ((int)(lowright[1])==0)) {
            if (projectioncode == GCTP_GEO){
                lonfield->specialformat = 2;
                latfield->specialformat = 2;
                gdset->addfvalueattr = true;
            }
        }
                
        //One MOD13C2 file doesn't provide projection code
        // The upperleft and lowerright coordinates are all -1
        // We have to calculate lat/lon by ourselves.
        // Since it doesn't provide the project code, we double check their information
        // and find that it covers the whole globe with 0.05 degree resolution.
        // Lat. is from 90 to -90 and Lon is from -180 to 180.

        if (((int)(lowright[0])==-1) &&((int)(upleft[0])==-1)
            && ((int)(upleft[1])==-1) && ((int)(lowright[1])==-1)) {
            lonfield->specialformat = 3;
            latfield->specialformat = 3;
            lonfield->condenseddim = true;
            latfield->condenseddim = true;
        }

   
        // We need to handle SOM projection in a different way.
        if (GCTP_SOM == projectioncode) {
            lonfield->specialformat = 4;
            latfield->specialformat = 4;
        }

        // Check if the 2-D lat/lon can be condensed to 1-D.
        //For current HDF-EOS2 files, only GEO and CEA can be condensed. To gain performance,
        // we just check the projection code, don't check with real values.
        if (projectioncode == GCTP_GEO || projectioncode ==GCTP_CEA) {
            lonfield->condenseddim = true;
            latfield->condenseddim = true;
        }

        // Add latitude and longitude fields to the field list.
        gdset->datafields.push_back(latfield);
        gdset->datafields.push_back(lonfield);

        // Now we want to handle the dim and the var lists.
        // If the lat/lon can be condensed to 1-D array, COARD convention needs to be followed.
        // Since COARD requires that the dimension name of lat/lon is the same as the field name of lat/lon,
        // we still need to handle this case later(function handle_grid_coards).

        // There are two cases, 
        // 1) lat/lon can be condensed to 1-D array. lat to YDim, Lon to XDim, we don't need to adjust the rank.
        // 2) 2-D lat./lon. The dimension order doesn't matter. So always assume lon to XDim, lat to YDim.
        // So we can handle them with one loop.
        for (const auto &dim:lonfield->getDimensions()) {
            if (dim->getName() == DIMXNAME) 
                HDFCFUtil::insert_map(gdset->dimcvarlist, dim->getName(), lonfield->getName());

            if (dim->getName() == DIMYNAME) 
                HDFCFUtil::insert_map(gdset->dimcvarlist, dim->getName(), latfield->getName());
        }
    }
} 

// For the case of which all grids have one dedicated lat/lon grid,
// this function shows how to handle lat/lon fields.
void File::handle_onelatlon_grids() {

    // Obtain "XDim","YDim","Latitude","Longitude" and "location" set.
    string DIMXNAME = this->get_geodim_x_name();       
    string DIMYNAME = this->get_geodim_y_name();       
    string LATFIELDNAME = this->get_latfield_name();       
    string LONFIELDNAME = this->get_lonfield_name();       

    // Now only there is only one geo grid name "location", so don't call it now.
    // string GEOGRIDNAME = this->get_geogrid_name();
    string GEOGRIDNAME = "location";

    //Dimension name and the corresponding field name when only one lat/lon is used for all grids.
    map<string,string>temponelatlondimcvarlist;

    // First we need to obtain dimcvarlist for the grid that contains lat/lon.
    for (const auto &grid:this->grids) {

        // Set the horizontal dimension name "dimxname" and "dimyname"
        // This will be used to detect the dimension major order.
        grid->setDimxName(DIMXNAME);
        grid->setDimyName(DIMYNAME);

        // Handle lat/lon. Note that other grids need to point to this lat/lon.
        if (grid->getName()==GEOGRIDNAME) {

            // Figure out dimension order,2D or 1D for lat/lon
            // if lat/lon field's pointed value is changed, the value of the lat/lon field is also changed.
            // set the flag to tell if this is lat or lon. The unit will be different for lat and lon.
            grid->lonfield->fieldtype = 2;
            grid->latfield->fieldtype = 1;

            // latitude and longitude rank must be equal and should not be greater than 2.
            if (grid->lonfield->rank >2 || grid->latfield->rank >2) 
                throw2("Either the rank of lat or the lon is greater than 2",grid->getName());
            if (grid->lonfield->rank !=grid->latfield->rank) 
                throw2("The rank of the latitude is not the same as the rank of the longitude",grid->getName());

            // For 2-D lat/lon arrays
            if (grid->lonfield->rank != 1) {

                // Obtain the major dim. For most cases, it is YDim Major. 
                //But for some cases it is not. So still need to check.
                grid->lonfield->ydimmajor = grid->getCalculated().isYDimMajor();
                grid->latfield->ydimmajor = grid->lonfield->ydimmajor;

                // Check if the 2-D lat/lon can be condensed to 1-D. 
                //For current HDF-EOS2 files, only GEO and CEA can be condensed. To gain performance,
                // we just check the projection code, don't check the real values.
                int32 projectioncode = grid->getProjection().getCode();
                if (projectioncode == GCTP_GEO || projectioncode ==GCTP_CEA) {
                    grid->lonfield->condenseddim = true;
                    grid->latfield->condenseddim = true;
                }

                // Now we want to handle the dim and the var lists.  
                // If the lat/lon can be condensed to 1-D array, COARD convention needs to be followed. 
                // Since COARD requires that the dimension name of lat/lon is the same as the field name of lat/lon,
                // we still need to handle this case later(function handle_grid_coards). Now we do the first step.  
                
                // There are two cases, 
                // 1) lat/lon can be condensed to 1-D array. lat to YDim, Lon to XDim, we don't need to adjust the rank.
                // 2) 2-D lat./lon. The dimension order doesn't matter. So always assume lon to XDim, lat to YDim.
                // So we can handle them with one loop.
 
                for (const auto &dim:grid->lonfield->getDimensions()) {
                    if (dim->getName() == DIMXNAME) {
                        HDFCFUtil::insert_map(grid->dimcvarlist, dim->getName(), 
                                              grid->lonfield->getName());
                    }
                    if (dim->getName() == DIMYNAME) {
                        HDFCFUtil::insert_map(grid->dimcvarlist, dim->getName(), 
                                              grid->latfield->getName());
                    }
                }
            }
            else { // This is the 1-D case, just inserting the dimension, field pair.
                HDFCFUtil::insert_map(grid->dimcvarlist, (grid->lonfield->getDimensions())[0]->getName(),
                                   grid->lonfield->getName());
                HDFCFUtil::insert_map(grid->dimcvarlist, (grid->latfield->getDimensions())[0]->getName(),
                                   grid->latfield->getName());
            }              
            temponelatlondimcvarlist = grid->dimcvarlist;
            break;

        }

    }

    // Now we need to assign the dim->cvar relation for lat/lon(xdim->lon,ydim->lat) to grids that don't contain lat/lon
    for (const auto &grid:this->grids) {

        string templatlonname1;
        string templatlonname2;

        if (grid->getName() != GEOGRIDNAME) {

            map<string,string>::iterator tempmapit;

            // Find DIMXNAME field
            tempmapit = temponelatlondimcvarlist.find(DIMXNAME);
            if (tempmapit != temponelatlondimcvarlist.end()) 
                templatlonname1= tempmapit->second;
            else 
                throw2("cannot find the dimension field of XDim", grid->getName());

            HDFCFUtil::insert_map(grid->dimcvarlist, DIMXNAME, templatlonname1);

            // Find DIMYNAME field
            tempmapit = temponelatlondimcvarlist.find(DIMYNAME);
            if (tempmapit != temponelatlondimcvarlist.end()) 
                templatlonname2= tempmapit->second;
            else
                throw2("cannot find the dimension field of YDim", grid->getName());
            HDFCFUtil::insert_map(grid->dimcvarlist, DIMYNAME, templatlonname2);
        }
    }

}

// Handle the dimension name to coordinate variable map for grid.
void File::handle_grid_dim_cvar_maps() {

    // Obtain "XDim","YDim","Latitude","Longitude" and "location" set.
    string DIMXNAME = this->get_geodim_x_name();

    string DIMYNAME = this->get_geodim_y_name();

    string LATFIELDNAME = this->get_latfield_name();       
                     
    string LONFIELDNAME = this->get_lonfield_name();       
                    

    // Now only there is only one geo grid name "location", so don't call it know.
    // string GEOGRIDNAME = this->get_geogrid_name();
    string GEOGRIDNAME = "location";

    //// ************* START HANDLING the name clashings for the field names. ******************
    //// Using a string vector for new field names.
    //// ******************

    // 1. Handle name clashings
    // 1.1 build up a temp. name list
    // Note here: we don't include grid and swath names(simply (*j)->name) due to the products we observe
    // Adding the grid/swath names makes the names artificially long. Will check user's feedback
    // and may change them later. KY 2012-6-25
    // The above assumption is purely for practical reason. Field names for all NASA multi-grid/swath products
    // (AIRS, AMSR-E, some MODIS, MISR) can all be distinguished regardless of grid/swath names. However,
    // this needs to be carefully watched out. KY 2013-07-08
    vector <string> tempfieldnamelist;
    for (const auto &grid:this->grids) {
        for (const auto &field:grid->getDataFields())
            tempfieldnamelist.push_back(HDFCFUtil::get_CF_string(field->name));
    }
    HDFCFUtil::Handle_NameClashing(tempfieldnamelist);

    // 2. Create a map for dimension field name <original field name, corrected field name>
    // Also assure the uniqueness of all field names,save the new field names.

    //the original dimension field name to the corrected dimension field name
    map<string,string>tempncvarnamelist;
    string tempcorrectedlatname;
    string tempcorrectedlonname;
      
    int total_fcounter = 0;

    for (const auto &grid:this->grids) {

        // Here we can't use getDataFields call since for no lat/lon fields 
        // are created for one global lat/lon case. We have to use the dimcvarnamelist 
        // map we just created.
        for (const auto &field:grid->getDataFields()) 
        {
            field->newname = tempfieldnamelist[total_fcounter];
            total_fcounter++;  
               
            // If this field is a dimension field, save the name/new name pair. 
            if (field->fieldtype!=0) {

                tempncvarnamelist.insert(make_pair(field->getName(), field->newname));

                // For one latlon case, remember the corrected latitude and longitude field names.
                if ((this->onelatlon)&&((grid->getName())==GEOGRIDNAME)) {
                    if (field->getName()==LATFIELDNAME) 
                        tempcorrectedlatname = field->newname;
                    if (field->getName()==LONFIELDNAME) 
                        tempcorrectedlonname = field->newname;
                }
            }
        }

        grid->ncvarnamelist = tempncvarnamelist;
        tempncvarnamelist.clear();
    }

    // For one lat/lon case, we have to add the lat/lon field name to other grids.
    // We know the original lat and lon names. So just retrieve the corrected lat/lon names from
    // the geo grid(GEOGRIDNAME).
    if (this->onelatlon) {
        for (const auto &grid:this->grids) {
            // Lat/lon names must be in this group.
            if (grid->getName()!=GEOGRIDNAME){
                HDFCFUtil::insert_map(grid->ncvarnamelist, LATFIELDNAME, tempcorrectedlatname);
                HDFCFUtil::insert_map(grid->ncvarnamelist, LONFIELDNAME, tempcorrectedlonname);
            }
        }
    }

    // 3. Create a map for dimension name < original dimension name, corrected dimension name>
    map<string,string>tempndimnamelist;//the original dimension name to the corrected dimension name

    //// ***** Handling the dimension name clashing ******************
    vector <string>tempalldimnamelist;
    for (const auto &grid:this->grids) {
        for (map<string,string>::const_iterator j =
            grid->dimcvarlist.begin(); j!= grid->dimcvarlist.end();++j)
            tempalldimnamelist.push_back(HDFCFUtil::get_CF_string((*j).first));
    }

    HDFCFUtil::Handle_NameClashing(tempalldimnamelist);
      
    // Since DIMXNAME and DIMYNAME are not in the original dimension name list, we use the dimension name,field map 
    // we just formed. 
    int total_dcounter = 0;
    for (const auto &grid:this->grids) {

        for (map<string,string>::const_iterator j =
            grid->dimcvarlist.begin(); j!= grid->dimcvarlist.end();++j){

            // We have to handle DIMXNAME and DIMYNAME separately.
            if ((DIMXNAME == (*j).first || DIMYNAME == (*j).first) && (true==(this->onelatlon))) 
                HDFCFUtil::insert_map(tempndimnamelist, (*j).first,(*j).first);
            else
                HDFCFUtil::insert_map(tempndimnamelist, (*j).first, tempalldimnamelist[total_dcounter]);
            total_dcounter++;
        }

        grid->ndimnamelist = tempndimnamelist;
        tempndimnamelist.clear();   
    }
}

// Follow COARDS for grids.
void File::handle_grid_coards() {

    // Obtain "XDim","YDim","Latitude","Longitude" and "location" set.
    string DIMXNAME = this->get_geodim_x_name();       
    string DIMYNAME = this->get_geodim_y_name();       
    string LATFIELDNAME = this->get_latfield_name();       
    string LONFIELDNAME = this->get_lonfield_name();       

    // Now only there is only one geo grid name "location", so don't call it know.
#if 0
    // string GEOGRIDNAME = this->get_geogrid_name();
#endif
    string GEOGRIDNAME = "location";


    // Revisit the lat/lon fields to check if 1-D COARD convention needs to be followed.
    vector<Dimension*> correcteddims;
    string tempcorrecteddimname;

    // grid name to the corrected latitude field name
    map<string,string> tempnewxdimnamelist;

    // grid name to the corrected longitude field name
    map<string,string> tempnewydimnamelist;
     
    // temporary dimension pointer
    Dimension *correcteddim;
     
    for (const auto &grid:this->grids) {
        for (const auto &field:grid->getDataFields()) {

            // Now handling COARD cases, since latitude/longitude can be either 1-D or 2-D array. 
            // So we need to correct both cases.
            // 2-D lat to 1-D COARD lat
            if (field->getName()==LATFIELDNAME && field->getRank()==2 &&field->condenseddim) {

                string templatdimname;
                map<string,string>::iterator tempmapit;

                // Find the new name of LATFIELDNAME
                tempmapit = grid->ncvarnamelist.find(LATFIELDNAME);
                if (tempmapit != grid->ncvarnamelist.end()) 
                    templatdimname= tempmapit->second;
                else 
                    throw2("cannot find the corrected field of Latitude", grid->getName());

                for (const auto &dim:field->getDimensions()) {

                    // Since hhis is the latitude, we create the corrected dimension with the corrected latitude field name
                    // latitude[YDIM]->latitude[latitude]
                    if (dim->getName()==DIMYNAME) {
                        correcteddim = new Dimension(templatdimname,dim->getSize());
                        correcteddims.push_back(correcteddim);
                        field->setCorrectedDimensions(correcteddims);
                        HDFCFUtil::insert_map(tempnewydimnamelist, grid->getName(), templatdimname);
                    }
                }
                field->iscoard = true;
                grid->iscoard = true;
                if (this->onelatlon) 
                    this->iscoard = true;

                // Clear the local temporary vector
                correcteddims.clear();
            }

            // 2-D lon to 1-D COARD lon
            else if (field->getName()==LONFIELDNAME && field->getRank()==2 &&field->condenseddim){
               
                string templondimname;
                map<string,string>::iterator tempmapit;

                // Find the new name of LONFIELDNAME
                tempmapit = grid->ncvarnamelist.find(LONFIELDNAME);
                if (tempmapit != grid->ncvarnamelist.end()) 
                    templondimname= tempmapit->second;
                else 
                    throw2("cannot find the corrected field of Longitude", grid->getName());

                for (const auto &dim:field->getDimensions()) {

                    // Since this is the longitude, we create the corrected dimension with the corrected longitude field name
                    // longitude[XDIM]->longitude[longitude]
                    if (dim->getName()==DIMXNAME) {
                        correcteddim = new Dimension(templondimname,dim->getSize());
                        correcteddims.push_back(correcteddim);
                        field->setCorrectedDimensions(correcteddims);
                        HDFCFUtil::insert_map(tempnewxdimnamelist, grid->getName(), templondimname);
                    }
                }

                field->iscoard = true;
                grid->iscoard = true;
                if (this->onelatlon) 
                    this->iscoard = true;
                correcteddims.clear();
            }
            // 1-D lon to 1-D COARD lon 
            // (this code can be combined with the 2-D lon to 1-D lon case, should handle this later, KY 2013-07-10).
            else if ((field->getRank()==1) &&(field->getName()==LONFIELDNAME) ) {

                string templondimname;
                map<string,string>::iterator tempmapit;

                // Find the new name of LONFIELDNAME
                tempmapit = grid->ncvarnamelist.find(LONFIELDNAME);
                if (tempmapit != grid->ncvarnamelist.end()) 
                    templondimname= tempmapit->second;
                else 
                    throw2("cannot find the corrected field of Longitude", grid->getName());

                correcteddim = new Dimension(templondimname,(field->getDimensions())[0]->getSize());
                correcteddims.push_back(correcteddim);
                field->setCorrectedDimensions(correcteddims);
                field->iscoard = true;
                grid->iscoard = true;
                if (this->onelatlon) 
                    this->iscoard = true; 
                correcteddims.clear();

                if (((field->getDimensions())[0]->getName()!=DIMXNAME)
                    &&(((field->getDimensions())[0]->getName())!=DIMYNAME)){
                    throw3("the dimension name of longitude should not be ",
                           (field->getDimensions())[0]->getName(),grid->getName()); 
                }
                if (((field->getDimensions())[0]->getName())==DIMXNAME) {
                    HDFCFUtil::insert_map(tempnewxdimnamelist, grid->getName(), templondimname);
                }
                else {
                    HDFCFUtil::insert_map(tempnewydimnamelist, grid->getName(), templondimname);
                }
            }
            // 1-D lat to 1-D COARD lat
            // (this case can be combined with the 2-D lat to 1-D lat case, should handle this later. KY 2013-7-10).
            else if ((field->getRank()==1) &&(field->getName()==LATFIELDNAME) ) {

                string templatdimname;
                map<string,string>::iterator tempmapit;

                // Find the new name of LATFIELDNAME
                tempmapit = grid->ncvarnamelist.find(LATFIELDNAME);
                if (tempmapit != grid->ncvarnamelist.end()) 
                    templatdimname= tempmapit->second;
                else 
                    throw2("cannot find the corrected field of Latitude", grid->getName());

                correcteddim = new Dimension(templatdimname,(field->getDimensions())[0]->getSize());
                correcteddims.push_back(correcteddim);
                field->setCorrectedDimensions(correcteddims);
              
                field->iscoard = true;
                grid->iscoard = true;
                if (this->onelatlon) 
                    this->iscoard = true;
                correcteddims.clear();

                if ((((field->getDimensions())[0]->getName())!=DIMXNAME)
                    &&(((field->getDimensions())[0]->getName())!=DIMYNAME))
                    throw3("the dimension name of latitude should not be ",
                           (field->getDimensions())[0]->getName(),grid->getName());
                if (((field->getDimensions())[0]->getName())==DIMXNAME){
                    HDFCFUtil::insert_map(tempnewxdimnamelist, grid->getName(), templatdimname);
                }
                else {
                    HDFCFUtil::insert_map(tempnewydimnamelist, grid->getName(), templatdimname);
                }
            }
        }
    }
      
    // If COARDS follows, apply the new DIMXNAME and DIMYNAME name to the  ndimnamelist 
    // One lat/lon for all grids.
    if (true == this->onelatlon){ 

        // COARDS is followed.
        if (true == this->iscoard){

            // For this case, only one pair of corrected XDim and YDim for all grids.
            string tempcorrectedxdimname;
            string tempcorrectedydimname;

            if ((int)(tempnewxdimnamelist.size())!= 1) 
                throw1("the corrected dimension name should have only one pair");
            if ((int)(tempnewydimnamelist.size())!= 1) 
                throw1("the corrected dimension name should have only one pair");

            map<string,string>::iterator tempdimmapit = tempnewxdimnamelist.begin();
            tempcorrectedxdimname = tempdimmapit->second;      
            tempdimmapit = tempnewydimnamelist.begin();
            tempcorrectedydimname = tempdimmapit->second;
       
            for (const auto &grid:this->grids) {

                // Find the DIMXNAME and DIMYNAME in the dimension name list.  
                map<string,string>::iterator tempmapit;
                tempmapit = grid->ndimnamelist.find(DIMXNAME);
                if (tempmapit != grid->ndimnamelist.end()) {
                    HDFCFUtil::insert_map(grid->ndimnamelist, DIMXNAME, tempcorrectedxdimname);
                }
                else 
                    throw2("cannot find the corrected dimension name", grid->getName());
                tempmapit = grid->ndimnamelist.find(DIMYNAME);
                if (tempmapit != grid->ndimnamelist.end()) {
                    HDFCFUtil::insert_map(grid->ndimnamelist, DIMYNAME, tempcorrectedydimname);
                }
                else 
                    throw2("cannot find the corrected dimension name", grid->getName());
            }
        }
    }
    else {// We have to search each grid
        for (const auto &grid:this->grids) {

            if (grid->iscoard){

                string tempcorrectedxdimname;
                string tempcorrectedydimname;

                // Find the DIMXNAME and DIMYNAME in the dimension name list.
                map<string,string>::iterator tempdimmapit;
                map<string,string>::iterator tempmapit;
                tempdimmapit = tempnewxdimnamelist.find(grid->getName());
                if (tempdimmapit != tempnewxdimnamelist.end()) 
                    tempcorrectedxdimname = tempdimmapit->second;
                else 
                    throw2("cannot find the corrected COARD XDim dimension name", grid->getName());
                tempmapit = grid->ndimnamelist.find(DIMXNAME);
                if (tempmapit != grid->ndimnamelist.end()) {
                    HDFCFUtil::insert_map(grid->ndimnamelist, DIMXNAME, tempcorrectedxdimname);
                }
                else 
                    throw2("cannot find the corrected dimension name", grid->getName());

                tempdimmapit = tempnewydimnamelist.find(grid->getName());
                if (tempdimmapit != tempnewydimnamelist.end()) 
                    tempcorrectedydimname = tempdimmapit->second;
                else 
                    throw2("cannot find the corrected COARD YDim dimension name", grid->getName());

                tempmapit = grid->ndimnamelist.find(DIMYNAME);
                if (tempmapit != grid->ndimnamelist.end()) {
                    HDFCFUtil::insert_map(grid->ndimnamelist, DIMYNAME, tempcorrectedydimname);
                }
                else 
                    throw2("cannot find the corrected dimension name", grid->getName());
            }
        }
    }

      
    // For 1-D lat/lon cases, Make the third (other than lat/lon coordinate variable) dimension to follow COARD conventions. 

    for (const auto &grid:this->grids){
        for (map<string,string>::const_iterator j =
            grid->dimcvarlist.begin(); j!= grid->dimcvarlist.end();++j){

            // It seems that the condition for onelatlon case is if (this->iscoard) is true instead if
            // this->onelatlon is true.So change it. KY 2010-7-4
            if ((this->iscoard||grid->iscoard) && (*j).first !=DIMXNAME && (*j).first !=DIMYNAME) {
                string tempnewdimname;
                map<string,string>::iterator tempmapit;

                // Find the new field name of the corresponding dimennsion name 
                tempmapit = grid->ncvarnamelist.find((*j).second);
                if (tempmapit != grid->ncvarnamelist.end()) 
                    tempnewdimname= tempmapit->second;
                else 
                    throw3("cannot find the corrected field of ", (*j).second,grid->getName());

                // Make the new field name to the correponding dimension name 
                tempmapit =grid->ndimnamelist.find((*j).first);
                if (tempmapit != grid->ndimnamelist.end()) 
                    HDFCFUtil::insert_map(grid->ndimnamelist, (*j).first, tempnewdimname);
                else 
                    throw3("cannot find the corrected dimension name of ", (*j).first,grid->getName());

            }
        }
    }
}

// Create the corrected dimension vector for each field when COARDS is not followed.
void File::update_grid_field_corrected_dims() {

    // Revisit the lat/lon fields to check if 1-D COARD convention needs to be followed.
    vector<Dimension*> correcteddims;
    string tempcorrecteddimname;
    // temporary dimension pointer
    Dimension *correcteddim;

    for (const auto &grid:this->grids) {

        for (const auto &field:grid->getDataFields()) {

            // When the corrected dimension name of lat/lon has been updated,
            if (field->iscoard == false) {

                // Just obtain the corrected dim names  and save the corrected dimensions for each field.
                for (const auto &dim:field->getDimensions()){

                    map<string,string>::iterator tempmapit;

                    // Find the new name of this field
                    tempmapit = grid->ndimnamelist.find(dim->getName());
                    if (tempmapit != grid->ndimnamelist.end())
                        tempcorrecteddimname= tempmapit->second;
                    else
                        throw4("cannot find the corrected dimension name", grid->getName(),field->getName(),dim->getName());
                    correcteddim = new Dimension(tempcorrecteddimname,dim->getSize());
                    correcteddims.push_back(correcteddim);
                }
                field->setCorrectedDimensions(correcteddims);
                correcteddims.clear();
            }
        }
    }

}

void File::handle_grid_cf_attrs() {

    // Create "coordinates" ,"units"  attributes. The attribute "units" only applies to latitude and longitude.
    // This is the last round of looping through everything, 
    // we will match dimension name list to the corresponding dimension field name 
    // list for every field. 
           
    for (const auto &grid:this->grids) {
        for (const auto &field:grid->getDataFields()) {
                 
            // Real fields: adding coordinate attributesinate attributes
            if (field->fieldtype == 0)  {
                string tempcoordinates="";
                string tempfieldname="";
                string tempcorrectedfieldname="";
                int tempcount = 0;
                for (const auto &dim:field->getDimensions()) {

                    // Handle coordinates attributes
                    map<string,string>::iterator tempmapit;
                    map<string,string>::iterator tempmapit2;
              
                    // Find the dimension field name
                    tempmapit = (grid->dimcvarlist).find(dim->getName());
                    if (tempmapit != (grid->dimcvarlist).end()) 
                        tempfieldname = tempmapit->second;
                    else 
                        throw4("cannot find the dimension field name",
                               grid->getName(),field->getName(),dim->getName());

                    // Find the corrected dimension field name
                    tempmapit2 = (grid->ncvarnamelist).find(tempfieldname);
                    if (tempmapit2 != (grid->ncvarnamelist).end()) 
                        tempcorrectedfieldname = tempmapit2->second;
                    else 
                        throw4("cannot find the corrected dimension field name",
                                grid->getName(),field->getName(),dim->getName());

                    if (tempcount == 0) 
                        tempcoordinates= tempcorrectedfieldname;
                    else 
                        tempcoordinates = tempcoordinates +" "+tempcorrectedfieldname;
                    tempcount++;
                }
                field->setCoordinates(tempcoordinates);
            }

            // Add units for latitude and longitude
            if (field->fieldtype == 1) {// latitude,adding the "units" degrees_north.
                string tempunits = "degrees_north";
                field->setUnits(tempunits);
            }
            if (field->fieldtype == 2) { // longitude, adding the units degrees_east.
                string tempunits = "degrees_east";
                field->setUnits(tempunits);
            }

            // Add units for Z-dimension, now it is always "level"
            // This also needs to be corrected since the Z-dimension may not always be "level".
            // KY 2012-6-13
            // We decide not to touch "units" when the Z-dimension is an existing field(fieldtype =3).
            if (field->fieldtype == 4) {
                string tempunits ="level";
                field->setUnits(tempunits);
            }
            
            // The units of the time is not right. KY 2012-6-13(documented at jira HFRHANDLER-167)
            if (field->fieldtype == 5) {
                string tempunits ="days since 1900-01-01 00:00:00";
                field->setUnits(tempunits);
            }

            // We meet a really special case for CERES TRMM data. We make it to the specialformat 2 case
            // since the corner coordinate is set to default in HDF-EOS2 structmetadata. We also find that there are
            // values such as 3.4028235E38 that is the maximum single precision floating point value. This value
            // is a fill value but the fillvalue attribute is not set. So we add the fillvalue attribute for this case.
            // We may find such cases for other products and will tackle them also.
            if (true == grid->addfvalueattr) {
                if (((field->getFillValue()).empty()) && (field->getType()==DFNT_FLOAT32 )) {
                    float tempfillvalue = FLT_MAX;  // Replaced HUGE with FLT_MAX. jhrg 12/3/20
                    field->addFillValue(tempfillvalue);
                    field->setAddedFillValue(true);
                }
            }
        }
    }
}

// Special handling SOM(Space Oblique Mercator) projection files
void File::handle_grid_SOM_projection() {

    // since the latitude and longitude of the SOM projection are 3-D, so we need to handle this projection in a special way. 
    // Based on our current understanding, the third dimension size is always 180. 
    // If the size is not 180, the latitude and longitude will not be calculated correctly.
    // This is according to the MISR Lat/lon calculation document 
    // at http://eosweb.larc.nasa.gov/PRODOCS/misr/DPS/DPS_v50_RevS.pdf
    // KY 2012-6-12

    for (const auto &grid:this->grids) {
        if (GCTP_SOM == grid->getProjection().getCode()) {
                
            // 0. Getting the SOM dimension for latitude and longitude.

            // Obtain SOM's dimension name.
            string som_dimname;
            for (const auto &dim:grid->getDimensions()) {

                // NBLOCK is from misrproj.h. It is the number of block that MISR team support for the SOM projection.
                if (NBLOCK == dim->getSize()) {

                    // To make sure we catch the right dimension, check the first three characters of the dim. name
                    // It should be SOM
                    if (dim->getName().compare(0,3,"SOM") == 0) {
                        som_dimname = dim->getName();
                        break;
                    }
                }
            }

            if (""== som_dimname) 
                throw4("Wrong number of block: The number of block of MISR SOM Grid ",
                        grid->getName()," is not ",NBLOCK);

            map<string,string>::iterator tempmapit;

            // Find the corrected (CF) dimension name
            string cor_som_dimname;
            tempmapit = grid->ndimnamelist.find(som_dimname);
            if (tempmapit != grid->ndimnamelist.end()) 
                cor_som_dimname = tempmapit->second;
            else 
                throw2("cannot find the corrected dimension name for ", som_dimname);

            // Find the corrected(CF) name of this field
            string cor_som_cvname;

            // Here we cannot use getDataFields() since the returned elements cannot be modified. KY 2012-6-12
            // Here we cannot simply change the vector with for range loop since we need to remove an element. KY 2022-06-17
            for (vector<Field *>::iterator j = grid->datafields.begin();
                j != grid->datafields.end(); ) {
                    
                // Only 6-7 fields, so just loop through 
                // 1. Set the SOM dimension for latitude and longitude
                if (1 == (*j)->fieldtype || 2 == (*j)->fieldtype) {
                        
                    auto newdim = new Dimension(som_dimname,NBLOCK);
                    auto newcor_dim = new Dimension(cor_som_dimname,NBLOCK);
                    vector<Dimension *>::iterator it_d;

                    it_d = (*j)->dims.begin();
                    (*j)->dims.insert(it_d,newdim);

                    it_d = (*j)->correcteddims.begin();
                    (*j)->correcteddims.insert(it_d,newcor_dim);


                } 

                // 2. Remove the added coordinate variable for the SOM dimension
                // The added variable is a variable with the nature number
                // Why removing it? Since we cannot follow the general rule to create coordinate variables for MISR products.
                // The third-dimension belongs to lat/lon rather than a missing coordinate variable.
                if ( 4 == (*j)->fieldtype) {
                    cor_som_cvname = (*j)->newname;
                    delete (*j);
                    j = grid->datafields.erase(j);
                }
                else {
                   ++j;
                }
            }

            // 3. Fix the "coordinates" attribute: remove the SOM CV name from the coordinate attribute. 
            // Notice this is a little inefficient. Since we only have a few fields and non-SOM projection products
            // won't be affected, and more importantly, to keep the SOM projection handling in a central place,
            // I handle the adjustment of "coordinates" attribute here. KY 2012-6-12

            // MISR data cannot be visualized by Panoply and IDV. So the coordinates attribute
            // created here reflects the coordinates of this variable more accurately. KY 2012-6-13 
            for (const auto &field:grid->getDataFields()) {

                if ( 0 == field->fieldtype) {

                    string temp_coordinates = field->coordinates; 

                    size_t found;
                    found = temp_coordinates.find(cor_som_cvname);

                    if (0 == found) {
                        // Need also to remove the space after the SOM CV name.
                        if (temp_coordinates.size() >cor_som_cvname.size())
                            temp_coordinates.erase(found,cor_som_cvname.size()+1);
                        else 
                            temp_coordinates.erase(found,cor_som_cvname.size());
                    }
                    else if (found != string::npos) 
                        temp_coordinates.erase(found-1,cor_som_cvname.size()+1);
                    else 
                        throw4("cannot find the coordinate variable ",cor_som_cvname,
                               "from ",temp_coordinates);

                    field->setCoordinates(temp_coordinates);   

                }
            }
        }
    }
}

// Check if we need to handle dim. map and set handle_swath_dimmap if necessary.
// The input parameter is the number of swath.
void File::check_swath_dimmap(int numswath) {

    if (HDF4RequestHandler::get_disable_swath_dim_map() == true) 
        return;

    // Check if there are dimension maps and if the num of dim. maps is odd in this case.
    int tempnumdm = 0;
    int temp_num_map = 0;
    bool odd_num_map = false;
    for (const auto &swath:this->swaths) {
        temp_num_map = swath->get_num_map();
        tempnumdm += temp_num_map;
        if (temp_num_map%2!=0) { 
            odd_num_map =true;
            break;
        }
    }

    // We only handle even number of dimension maps like MODIS(2-D lat/lon) 
    if (tempnumdm != 0 && odd_num_map == false) 
        handle_swath_dimmap = true;
       
    // MODATML2 and MYDATML2 in year 2010 include dimension maps. But the dimension map
    // is not used. Furthermore, they provide additional latitude/longtiude 
    // for 10 KM under the data field. So we have to handle this differently.
    // MODATML2 in year 2000 version doesn't include dimension map, so we 
    // have to consider both with dimension map and without dimension map cases.
    // The swath name is atml2.

    bool fakedimmap = false;

    if (numswath == 1) {// Start special atml2-like handling

        if ((this->swaths[0]->getName()).find("atml2")!=string::npos){

            if (tempnumdm >0) 
                fakedimmap = true;
            int templlflag = 0;

            for (const auto &gfield:this->swaths[0]->getGeoFields()) {
                if (gfield->getName() == "Latitude" || gfield->getName() == "Longitude") {
                    if (gfield->getType() == DFNT_UINT16 ||
                        gfield->getType() == DFNT_INT16)
                        gfield->type = DFNT_FLOAT32;
                    templlflag ++;
                    if (templlflag == 2) 
                        break;
                }
            }

            templlflag = 0;

            for (const auto &dfield:this->swaths[0]->getDataFields()) {

                // We meet a very speical MODIS case.
                // The latitude and longitude types are int16.
                // The number are in thousand. The scale factor
                // attribute is 0.01. This attribute cannot be
                // retrieved by EOS2 APIs. So we have to hard code this.
                //  We can only use the swath name to 
                // identify this case. 
                // The swath name is atml2. It has only one swath.
                // We have to change lat and lon to float type array;
                // since after applying the scale factor, the array becomes 
                // float data.
                // KY-2010-7-12

                if ((dfield->getName()).find("Latitude") != string::npos){

                    if (dfield->getType() == DFNT_UINT16 ||
                        dfield->getType() == DFNT_INT16)
                        dfield->type = DFNT_FLOAT32;

                    dfield->fieldtype = 1;

                    // Also need to link the dimension to the coordinate variable list
                    if (dfield->getRank() != 2) 
                        throw2("The lat/lon rank must be  2 for Java clients to work",
                    dfield->getRank());
                    HDFCFUtil::insert_map(this->swaths[0]->dimcvarlist, 
                                       ((dfield->getDimensions())[0])->getName(),dfield->getName());
                    templlflag ++;
                }

                if ((dfield->getName()).find("Longitude")!= string::npos) {

                    if (dfield->getType() == DFNT_UINT16 ||
                       dfield->getType() == DFNT_INT16)
                       dfield->type = DFNT_FLOAT32;

                    dfield->fieldtype = 2;
                    if (dfield->getRank() != 2) 
                        throw2("The lat/lon rank must be  2 for Java clients to work",
                                dfield->getRank());
                    HDFCFUtil::insert_map(this->swaths[0]->dimcvarlist, 
                                          ((dfield->getDimensions())[1])->getName(), dfield->getName());
                    templlflag ++;
                }

                if (templlflag == 2) 
                    break;
            }
        }
    }// End of special atml2 handling

    // Although this file includes dimension maps, it doesn't use it at all. So set
    // handle_swath_dimmap to 0.
    if (true == fakedimmap) 
        handle_swath_dimmap = false;
    return;

}

// If dim. map needs to be handled, we need to check if we fall into the case
// that backward compatibility of MODIS Level 1B etc. should be supported.
void File::check_swath_dimmap_bk_compat(int numswath){ 

    if (true == handle_swath_dimmap) {

        if (numswath == 1 && (((this->swaths)[0])->name== "MODIS_SWATH_Type_L1B"))
            backward_handle_swath_dimmap = true;
        else {
            // If the number of dimmaps is 2 for every swath 
            // and latitude/longitude need to be interpolated,
            // this also falls back to the backward compatibility case.
            // GeoDim_in_vars needs to be checked first.
            bool all_2_dimmaps_no_geodim = true;
            for (const auto &swath:this->swaths) {
                if (swath->get_num_map() !=2 || swath->GeoDim_in_vars == true)  {                    
                    all_2_dimmaps_no_geodim = false;
                    break;
                }
            }
            if (true == all_2_dimmaps_no_geodim)
                backward_handle_swath_dimmap = true;
        }
    }
    return;
}

// Create the dimension name to coordinate variable name map for lat/lon. 
void File::create_swath_latlon_dim_cvar_map(){

    vector<Field*> ori_lats;
    vector<Field*> ori_lons;
    if (handle_swath_dimmap == true && backward_handle_swath_dimmap == false) {

        // We need to check if "Latitude and Longitude" both exist in all swaths under GeoFields.
        // The latitude and longitude must be 2-D arrays.
        // This is the basic requirement to handle our defined multiple dimension map case.
        multi_dimmap = true;

        for (const auto &swath:this->swaths) {

            bool has_cf_lat = false;
            bool has_cf_lon = false;

            for (const auto &gfield:swath->getGeoFields()) {

                // Here we assume it is always lat[f0][f1] and lon [f0][f1]. 
                // lat[f0][f1] and lon[f1][f0] should not occur.
                // So far only "Latitude" and "Longitude" are used as standard names of lat and lon for swath.
                if (gfield->getName()=="Latitude" && gfield->getRank() == 2){
                    has_cf_lat = true;
                    ori_lats.push_back(gfield);
                }
                else if (gfield->getName()=="Longitude" && gfield->getRank() == 2){
                    has_cf_lon = true;
                    ori_lons.push_back(gfield);
                }
                if (has_cf_lat == true && has_cf_lon == true) 
                    break;
            }
            if (has_cf_lat == false || has_cf_lon == false) {
                multi_dimmap = false;
                break;
            }
        }
    }

    // By our best knowledge so far, we know we come to a multiple dimension map case
    // that we can handle. We will create dim to coordinate variable map for lat and lon
    // with the following block and finish this function.
    if (true == multi_dimmap) {

        int ll_count = 0;
        for (const auto &swath:this->swaths) {
            create_swath_latlon_dim_cvar_map_for_dimmap(swath,ori_lats[ll_count],ori_lons[ll_count]);
            ll_count++;
        }
        return;

    }

    // For the cases that multi_dimmap is not true, do the following:
    // 1. Prepare the right dimension name and the dimension field list for each swath. 
    // The assumption is that within a swath, the dimension name is unique.
    // The dimension field name(even with the added Z-like field) is unique. 
    // A map <dimension name, dimension field name> will be created.
    // The name clashing handling for multiple swaths will not be done in this step. 

    // 1.1 Obtain the dimension names corresponding to the latitude and longitude,
    // save them to the <dimname, dimfield> map.

    // We found a special MODIS product: the Latitude and Longitude are put under the Data fields 
    // rather than GeoLocation fields.
    // So we need to go to the "Data Fields" to grab the "Latitude and Longitude".

    bool lat_in_geofields = false;
    bool lon_in_geofields = false;

    for (const auto &swath:this->swaths) {

        int tempgeocount = 0;
        for (const auto &gfield:swath->getGeoFields()) {  

            // Here we assume it is always lat[f0][f1] and lon [f0][f1]. No lat[f0][f1] and lon[f1][f0] occur.
            // So far only "Latitude" and "Longitude" are used as standard names of lat and lon for swath.
            if (gfield->getName()=="Latitude" ){
                if (gfield->getRank() > 2) 
                    throw2("Currently the lat/lon rank must be 1 or 2 for Java clients to work",
                            gfield->getRank());

                lat_in_geofields = true;

                // Since under our assumption, lat/lon are always 2-D for a swath and 
                // dimension order doesn't matter for Java clients,
                // so we always map Latitude to the first dimension and longitude to the second dimension.
                // Save this information in the coordinate variable name and field map.
                // For rank =1 case, we only handle the cross-section along the same 
                // longitude line. So Latitude should be the dimension name.
                HDFCFUtil::insert_map(swath->dimcvarlist, ((gfield->getDimensions())[0])->getName(), "Latitude");

                // Have dimension map, we want to remember the dimension and remove it from the list.
                if (handle_swath_dimmap == true) {

                    // We need to keep the backward compatibility when handling MODIS level 1B etc. 
                    if (true == backward_handle_swath_dimmap) {

                        // We have to loop through the dimension map
                        for (const auto &dmap:swath->getDimensionMaps()) {

                            // This dimension name will be replaced by the mapped dimension name, 
                            // the mapped dimension name can be obtained from the getDataDimension() method.
                            if ((gfield->getDimensions()[0])->getName() == dmap->getGeoDimension()) {
                                HDFCFUtil::insert_map(swath->dimcvarlist, dmap->getDataDimension(), "Latitude");
                                break;
                            }
                        }
                    }
                }
                
                gfield->fieldtype = 1;
                tempgeocount ++;
            }

            if (gfield->getName()=="Longitude"){
                if (gfield->getRank() > 2) 
                    throw2("Currently the lat/lon rank must be  1 or 2 for Java clients to work",
                            gfield->getRank());

                // Only lat-level cross-section(for Panoply)is supported 
                // when longitude/latitude is 1-D, so ignore the longitude as the dimension field.
                lon_in_geofields = true;
                if (gfield->getRank() == 1) {
                    tempgeocount++;
                    continue;
                }

                // Since under our assumption, lat/lon are almost always 2-D for 
                // a swath and dimension order doesn't matter for Java clients,
                // we always map Latitude to the first dimension and longitude to the second dimension.
                // Save this information in the dimensiion name and coordinate variable map.
                HDFCFUtil::insert_map(swath->dimcvarlist, 
                                      ((gfield->getDimensions())[1])->getName(), "Longitude");
                if (handle_swath_dimmap == true) {
                    if (true == backward_handle_swath_dimmap) {

                        // We have to loop through the dimension map
                        for (const auto &dmap:swath->getDimensionMaps()) {

                            // This dimension name will be replaced by the mapped dimension name,
                            // This name can be obtained by getDataDimension() fuction of 
                            // dimension map class. 
                            if ((gfield->getDimensions()[1])->getName() == 
                                dmap->getGeoDimension()) {
                                HDFCFUtil::insert_map(swath->dimcvarlist, 
                                                      dmap->getDataDimension(), "Longitude");
                                break;
                            }
                        }
                    }
                }
                gfield->fieldtype = 2;
                tempgeocount++;
            }
            if (tempgeocount == 2) 
                break;
        }
    }// end of creating the <dimname,dimfield> map.

    // If lat and lon are not together, throw an error.
    if (lat_in_geofields!=lon_in_geofields) 
        throw1("Latitude and longitude must be both under Geolocation fields or Data fields");
            
    // Check if they are under data fields(The code may be combined with the above, see HFRHANDLER-166)
    if (!lat_in_geofields && !lon_in_geofields) {

        bool lat_in_datafields = false;
        bool lon_in_datafields = false;

        for (const auto &swath:this->swaths) {

            int tempgeocount = 0;
            for (const auto &dfield:swath->getDataFields()) {

                // Here we assume it is always lat[f0][f1] and lon [f0][f1]. 
                // No lat[f0][f1] and lon[f1][f0] occur.
                // So far only "Latitude" and "Longitude" are used as 
                // standard names of Lat and lon for swath.
                if (dfield->getName()=="Latitude" ){
                    if (dfield->getRank() > 2) { 
                        throw2("Currently the lat/lon rank must be 1 or 2 for Java clients to work",
                                dfield->getRank());
                    }
                    lat_in_datafields = true;

                    // Since under our assumption, lat/lon are always 2-D 
                    // for a swath and dimension order doesn't matter for Java clients,
                    // we always map Latitude the first dimension and longitude the second dimension.
                    // Save this information in the coordinate variable name and field map.
                    // For rank =1 case, we only handle the cross-section along the same longitude line. 
                    // So Latitude should be the dimension name.
                    HDFCFUtil::insert_map(swath->dimcvarlist, 
                                          ((dfield->getDimensions())[0])->getName(), "Latitude");

                    if (handle_swath_dimmap == true) {
                        if (true == backward_handle_swath_dimmap) {
                            // We have to loop through the dimension map
                            for (const auto &dmap:swath->getDimensionMaps()) {
                                // This dimension name will be replaced by the mapped dimension name, 
                                // the mapped dimension name can be obtained from the getDataDimension() method.
                                if ((dfield->getDimensions()[0])->getName() == dmap->getGeoDimension()) {
                                    HDFCFUtil::insert_map(swath->dimcvarlist, dmap->getDataDimension(), "Latitude");
                                    break;
                                }
                            }
                        }
                    }
                    dfield->fieldtype = 1;
                    tempgeocount ++;
                }

                if (dfield->getName()=="Longitude"){

                    if (dfield->getRank() > 2) { 
                        throw2("Currently the lat/lon rank must be  1 or 2 for Java clients to work",
                                dfield->getRank());
                    }

                    // Only lat-level cross-section(for Panoply)is supported when 
                    // longitude/latitude is 1-D, so ignore the longitude as the dimension field.
                    lon_in_datafields = true;
                    if (dfield->getRank() == 1) {
                        tempgeocount++;
                        continue;
                    }

                    // Since under our assumption, 
                    // lat/lon are almost always 2-D for a swath and dimension order doesn't matter for Java clients,
                    // we always map Latitude the first dimension and longitude the second dimension.
                    // Save this information in the dimensiion name and coordinate variable map.
                    HDFCFUtil::insert_map(swath->dimcvarlist, 
                                                      ((dfield->getDimensions())[1])->getName(), "Longitude");
                    if (handle_swath_dimmap == true) {
                       if (true == backward_handle_swath_dimmap) {
                            // We have to loop through the dimension map
                            for (const auto &dmap:swath->getDimensionMaps()) {
                                // This dimension name will be replaced by the mapped dimension name,
                                // This name can be obtained by getDataDimension() fuction of dimension map class. 
                                if ((dfield->getDimensions()[1])->getName() == dmap->getGeoDimension()) {
                                    HDFCFUtil::insert_map(swath->dimcvarlist, 
                                                          dmap->getDataDimension(), "Longitude");
                                    break;
                                }
                            }
                        }
                    }
                    dfield->fieldtype = 2;
                    tempgeocount++;
                }
                if (tempgeocount == 2) 
                    break;
            }
        }// end of creating the <dimname,dimfield> map.

        // If lat and lon are not together, throw an error.
        if (lat_in_datafields!=lon_in_datafields) 
            throw1("Latitude and longitude must be both under Geolocation fields or Data fields");

    }

}

// Create the dimension name to coordinate variable name map for coordinate variables that are not lat/lon. 
// The input parameter is the number of dimension maps in this file.
void File:: create_swath_nonll_dim_cvar_map() 
{
    // Handle existing and missing fields 
    for (const auto &swath:this->swaths) {
                
        // Since we find multiple 1-D fields with the same dimension names for some Swath files(AIRS level 1B),
        // we currently always treat the third dimension field as a missing field, this may be corrected later.
        // Corrections for the above: MODIS data do include the unique existing Z fields, so we have to search
        // the existing Z field. KY 2010-8-11
        // Our correction is to search all 1-D fields with the same dimension name within one swath,
        // if only one field is found, we use this field  as the third dimension.
        // 1.1 Add the missing Z-dimension field.
        // Some dimension name's corresponding fields are missing, 
        // so add the missing Z-dimension fields based on the dimension name. When the real
        // data is read, nature number 1,2,3,.... will be filled!
        // NOTE: The latitude and longitude dim names are not handled yet.  
               
        // Build a unique 1-D dimension name list.
        // Now the list only includes dimension names of "latitude" and "longitude".
    
        pair<set<string>::iterator,bool> tempdimret;
        for (map<string,string>::const_iterator j = swath->dimcvarlist.begin(); 
            j!= swath->dimcvarlist.end();++j){ 
            tempdimret = swath->nonmisscvdimlist.insert((*j).first);
        }

        // Search the geofield group and see if there are any existing 1-D Z dimension data.
        //  If 1-D field data with the same dimension name is found under GeoField, 
        // we still search if that 1-D field  is the dimension
        // field of a dimension name.
        for (const auto &gfield:swath->getGeoFields()) {
             
            if (gfield->getRank()==1) {
                if (swath->nonmisscvdimlist.find(((gfield->getDimensions())[0])->getName()) == swath->nonmisscvdimlist.end()){
                    tempdimret = swath->nonmisscvdimlist.insert(((gfield->getDimensions())[0])->getName());
                    if (gfield->getName() =="Time") 
                        gfield->fieldtype = 5;// This is for IDV.

                    HDFCFUtil::insert_map(swath->dimcvarlist, ((gfield->getDimensions())[0])->getName(), gfield->getName());
                    gfield->fieldtype = 3;

                }
            }
        }

        // We will also check the third dimension inside DataFields
        // This may cause potential problems for AIRS data
        // We will double CHECK KY 2010-6-26
        // So far the tests seem okay. KY 2010-8-11
        for (const auto &dfield:swath->getDataFields()) {

            if (dfield->getRank()==1) {
                if (swath->nonmisscvdimlist.find(((dfield->getDimensions())[0])->getName()) == swath->nonmisscvdimlist.end()){
                    tempdimret = swath->nonmisscvdimlist.insert(((dfield->getDimensions())[0])->getName());
                    if (dfield->getName() =="Time") 
                        dfield->fieldtype = 5;// This is for IDV.

                    // This is for temporarily COARD fix. 
                    // For 2-D lat/lon, the third dimension should NOT follow
                    // COARD conventions. It will cause Panoply and IDV failed.
                    // KY 2010-7-21
                    HDFCFUtil::insert_map(swath->dimcvarlist, ((dfield->getDimensions())[0])->getName(), dfield->getName());
                    dfield->fieldtype = 3;

                }
            }
        }


        // S1.2.3 Handle the missing fields 
        // Loop through all dimensions of this swath to search the missing fields
        //
        bool missingfield_unlim_flag = false;
        Field *missingfield_unlim = nullptr;

        for (const auto &sdim:swath->getDimensions()) { 

            if ((swath->nonmisscvdimlist.find(sdim->getName())) == swath->nonmisscvdimlist.end()){// This dimension needs a field
                      
                // Need to create a new data field vector element with the name and dimension as above.
                auto missingfield = new Field();

                // This is for temporarily COARD fix. 
                // For 2-D lat/lon, the third dimension should NOT follow
                // COARD conventions. It will cause Panoply and IDV failed.
                // Since Swath is always 2-D lat/lon, so we are okay here. Add a "_d" for each field name.
                // KY 2010-7-21
                // netCDF-Java now first follows COARDS, change back
                // missingfield->name = sdim->getName()+"_d";
                Dimension *dim;
                // When we can handle multiple dimension maps and the
                // number of swath is >1, we add the swath name as suffix to
                // avoid the name clashing.
                if (true == multi_dimmap && (this->swaths.size() != 1)) {
                    missingfield->name = sdim->getName()+"_"+swath->name;
                    dim = new Dimension(missingfield->name,sdim->getSize());
                }
                else {
                    missingfield->name = sdim->getName();
                    dim = new Dimension(sdim->getName(),sdim->getSize());
                }
                missingfield->rank = 1;
                missingfield->type = DFNT_INT32;//This is an HDF constant.the data type is always integer.

                // only 1 dimension
                missingfield->dims.push_back(dim);

                // Provide information for the missing data, since we need to calculate the data, so
                // the information is different than a normal field.
                // int missingdimsize[1]; //unused variable. SBL 2/7/20
                // missingdimsize[0]= sdim->getSize();
                
                if (0 == sdim->getSize()) {
                    missingfield_unlim_flag = true;
                    missingfield_unlim = missingfield;
                }

                //added Z-dimension coordinate variable with nature number
                missingfield->fieldtype = 4; 

                swath->geofields.push_back(missingfield);
                HDFCFUtil::insert_map(swath->dimcvarlist, 
                                     (missingfield->getDimensions())[0]->getName(), missingfield->name);
            }
        }

        // Correct the unlimited dimension size.
        // The code on the following is ok. 
        // However, coverity is picky about changing the missingfield_unlim_flag in the middle.
        // use a temporary variable for the if block.
        // The following code correct the dimension size of unlimited dimension.

        bool temp_missingfield_unlim_flag = missingfield_unlim_flag;
        if (true == temp_missingfield_unlim_flag) {
             for (const auto &dfield:swath->getDataFields()) {

                for (const auto &fdim:dfield->getDimensions()) {
                
                    if (fdim->getName() == (missingfield_unlim->getDimensions())[0]->getName()) {
                        if (fdim->getSize()!= 0) {
                            Dimension *dim = missingfield_unlim->getDimensions()[0];
                            // Correct the dimension size.
                            dim->dimsize = fdim->getSize();
                            missingfield_unlim_flag = false;
                            break;
                        }
                    }
                }
                if (false == missingfield_unlim_flag) 
                    break;
            }
        }

        swath->nonmisscvdimlist.clear();// clear this set.

    }// End of handling non-latlon cv 

}

// Handle swath dimension name to coordinate variable name maps. 
void File::handle_swath_dim_cvar_maps() {

    // Start handling name clashing
    vector <string> tempfieldnamelist;
    for (const auto &swath:this->swaths) {
                 
        // First handle geofield, all dimension fields are under the geofield group.
        for (const auto &gfield:swath->getGeoFields()) {
            if (gfield->fieldtype == 0 && (this->swaths.size() !=1) &&
               (true == handle_swath_dimmap) && 
               (backward_handle_swath_dimmap == false)){
                string new_field_name = gfield->name+"_"+swath->name;
                tempfieldnamelist.push_back(HDFCFUtil::get_CF_string(new_field_name));   
            }
            else 
                tempfieldnamelist.push_back(HDFCFUtil::get_CF_string(gfield->name));   
        }

        for (const auto &dfield:swath->getDataFields()) {
            if (dfield->fieldtype == 0 && (this->swaths.size() !=1) &&
                true == multi_dimmap){
                // If we can handle multi dim. maps fro multi swaths, we 
                // create the field name with the swath name as suffix to 
                // avoid name clashing.
                string new_field_name = dfield->name+"_"+swath->name;
                tempfieldnamelist.push_back(HDFCFUtil::get_CF_string(new_field_name));   
            }
            else 
                tempfieldnamelist.push_back(HDFCFUtil::get_CF_string(dfield->name));
        }
    }

    HDFCFUtil::Handle_NameClashing(tempfieldnamelist);

    int total_fcounter = 0;
      
    // Create a map for dimension field name <original field name, corrected field name>
    // Also assure the uniqueness of all field names,save the new field names.
    //the original dimension field name to the corrected dimension field name
    map<string,string>tempncvarnamelist;
    for (const auto &swath:this->swaths) {

        // First handle geofield, all dimension fields are under the geofield group.
        for (const auto &gfield:swath->getGeoFields()) 
        {
               
            gfield->newname = tempfieldnamelist[total_fcounter];
            total_fcounter++;

            // If this field is a dimension field, save the name/new name pair. 
            if (gfield->fieldtype!=0) 
                HDFCFUtil::insert_map(swath->ncvarnamelist, gfield->getName(), gfield->newname);
            
        }
 
        for (const auto &dfield:swath->getDataFields()) 
        {
            dfield->newname = tempfieldnamelist[total_fcounter];
            total_fcounter++;

            // If this field is a dimension field, save the name/new name pair.
            if (dfield->fieldtype!=0) {
                HDFCFUtil::insert_map(swath->ncvarnamelist, dfield->getName(), dfield->newname);
            }
        }
    } // end of creating a map for dimension field name <original field name, corrected field name>

    // Create a map for dimension name < original dimension name, corrected dimension name>

    vector <string>tempalldimnamelist;

    for (const auto &swath:this->swaths) {
        for (map<string,string>::const_iterator j =
            swath->dimcvarlist.begin(); j!= swath->dimcvarlist.end();++j)
            tempalldimnamelist.push_back(HDFCFUtil::get_CF_string((*j).first));
    }

    // Handle name clashing will make the corrected dimension name follow CF
    HDFCFUtil::Handle_NameClashing(tempalldimnamelist);

    int total_dcounter = 0;

    for (const auto &swath:this->swaths) {
        for (map<string,string>::const_iterator j =
            swath->dimcvarlist.begin(); j!= swath->dimcvarlist.end();++j){
            HDFCFUtil::insert_map(swath->ndimnamelist, (*j).first, tempalldimnamelist[total_dcounter]);
            total_dcounter++;
        }
    }

    // Create corrected dimension vectors.
    vector<Dimension*> correcteddims;
    string tempcorrecteddimname;
    Dimension *correcteddim;

    for (const auto &swath:this->swaths) {

        // First the geofield. 
        for (const auto &gfield:swath->getGeoFields()) { 

            for (const auto &gdim:gfield->getDimensions()) {

                map<string,string>::iterator tempmapit;

                // No dimension map or dimension names were handled. just obtain the new dimension name.
                if (handle_swath_dimmap == false || multi_dimmap == true) {

                    // Find the new name of this field
                    tempmapit = swath->ndimnamelist.find(gdim->getName());
                    if (tempmapit != swath->ndimnamelist.end()) 
                        tempcorrecteddimname= tempmapit->second;
                    else 
                        throw4("cannot find the corrected dimension name", 
                    swath->getName(),gfield->getName(),gdim->getName());

                    correcteddim = new Dimension(tempcorrecteddimname,gdim->getSize());
                }
                else { 
                    // have dimension map, use the datadim and datadim size to replace the geodim and geodim size. 
                    bool isdimmapname = false;

                    // We have to loop through the dimension map
                    for (const auto &sdmap:swath->getDimensionMaps()) {

                        // This dimension name is the geo dimension name in the dimension map, 
                        // replace the name with data dimension name.
                        if (gdim->getName() == sdmap->getGeoDimension()) {

                            isdimmapname = true;
                            gfield->dmap = true;
                            string temprepdimname = sdmap->getDataDimension();

                            // Find the new name of this data dimension name
                            tempmapit = swath->ndimnamelist.find(temprepdimname);
                            if (tempmapit != swath->ndimnamelist.end()) 
                                tempcorrecteddimname= tempmapit->second;
                            else 
                                throw4("cannot find the corrected dimension name", swath->getName(),
                                       gfield->getName(),gdim->getName());
                                    
                            // Find the size of this data dimension name
                            // We have to loop through the Dimensions of this swath
                            bool ddimsflag = false;
                            for (const auto &sdim:swath->getDimensions()) {
                                if (sdim->getName() == temprepdimname) { 
                                    // Find the dimension size, create the correcteddim
                                    correcteddim = new Dimension(tempcorrecteddimname,sdim->getSize());
                                    ddimsflag = true;
                                    break;
                                }
                            }
                            if (!ddimsflag) 
                                throw4("cannot find the corrected dimension size", swath->getName(),
                                        gfield->getName(),gdim->getName());
                            break;
                        }
                    }
                    if (false == isdimmapname) { // Still need to assign the corrected dimensions.
                        // Find the new name of this field
                        tempmapit = swath->ndimnamelist.find(gdim->getName());
                        if (tempmapit != swath->ndimnamelist.end()) 
                            tempcorrecteddimname= tempmapit->second;
                        else 
                            throw4("cannot find the corrected dimension name", 
                        swath->getName(),gfield->getName(),gdim->getName());

                        correcteddim = new Dimension(tempcorrecteddimname,gdim->getSize());

                    }
                }         

                correcteddims.push_back(correcteddim);
            }
            gfield->setCorrectedDimensions(correcteddims);
            correcteddims.clear();
        }// End of creating the corrected dimension vectors for GeoFields.
 
        // Then the data field.
        for (const auto &dfield:swath->getDataFields()) {

            for (const auto &fdim:dfield->getDimensions()) {

                if ((handle_swath_dimmap == false) || multi_dimmap == true) {

                    map<string,string>::iterator tempmapit;
                    // Find the new name of this field
                    tempmapit = swath->ndimnamelist.find(fdim->getName());
                    if (tempmapit != swath->ndimnamelist.end()) 
                        tempcorrecteddimname= tempmapit->second;
                    else 
                        throw4("cannot find the corrected dimension name", swath->getName(),
                    dfield->getName(),fdim->getName());

                    correcteddim = new Dimension(tempcorrecteddimname,fdim->getSize());
                }
                else {
                    map<string,string>::iterator tempmapit;
                    bool isdimmapname = false;

                    // We have to loop through dimension map
                    for (const auto &smap:swath->getDimensionMaps()) {
                        // This dimension name is the geo dimension name in the dimension map, 
                        // replace the name with data dimension name.
                        if (fdim->getName() == smap->getGeoDimension()) {
                            isdimmapname = true;
                            dfield->dmap = true;
                            string temprepdimname = smap->getDataDimension();
                   
                            // Find the new name of this data dimension name
                            tempmapit = swath->ndimnamelist.find(temprepdimname);
                            if (tempmapit != swath->ndimnamelist.end()) 
                                tempcorrecteddimname= tempmapit->second;
                            else 
                                throw4("cannot find the corrected dimension name", 
                                        swath->getName(),dfield->getName(),fdim->getName());
                                    
                            // Find the size of this data dimension name
                            // We have to loop through the Dimensions of this swath
                            bool ddimsflag = false;
                            for (const auto &sdim:swath->getDimensions()) {

                                // Find the dimension size, create the correcteddim
                                if (sdim->getName() == temprepdimname) { 
                                    correcteddim = new Dimension(tempcorrecteddimname,sdim->getSize());
                                    ddimsflag = true;
                                    break;
                                }
                            }
                            if (!ddimsflag) 
                                throw4("cannot find the corrected dimension size", 
                                        swath->getName(),dfield->getName(),fdim->getName());
                            break;
                        }
                    }
                    // Not a dimension with dimension map; Still need to assign the corrected dimensions.
                    if (!isdimmapname) { 

                        // Find the new name of this field
                        tempmapit = swath->ndimnamelist.find(fdim->getName());
                        if (tempmapit != swath->ndimnamelist.end()) 
                            tempcorrecteddimname= tempmapit->second;
                        else 
                            throw4("cannot find the corrected dimension name", 
                                    swath->getName(),dfield->getName(),fdim->getName());

                        correcteddim = new Dimension(tempcorrecteddimname,fdim->getSize());
                    }

                }
                correcteddims.push_back(correcteddim);
            }
            dfield->setCorrectedDimensions(correcteddims);
            correcteddims.clear();
        }// End of creating the dimensions for data fields.
    }
}
 
// Handle CF attributes for swaths. 
// The CF attributes include "coordinates", "units" for coordinate variables and "_FillValue". 
void File::handle_swath_cf_attrs() {

    // Create "coordinates" ,"units"  attributes. The attribute "units" only applies to latitude and longitude.
    // This is the last round of looping through everything, 
    // we will match dimension name list to the corresponding dimension field name 
    // list for every field. 
    // Since we find some swath files don't specify fillvalue when -9999.0 is found in the real data,
    // we specify fillvalue for those fields. This is entirely 
    // artifical and we will evaluate this approach. KY 2010-3-3
           
    for (const auto &swath:this->swaths) {

        // Handle GeoField first.
        for (const auto &gfield:swath->getGeoFields()) {
                 
            // Real fields: adding the coordinate attribute
            if (gfield->fieldtype == 0)  {// currently it is always true.
                string tempcoordinates="";
                string tempfieldname="";
                string tempcorrectedfieldname="";
                int tempcount = 0;
                bool has_ll_coord = false;
                if (swath->get_num_map() == 0)
                    has_ll_coord = true;
                else if (handle_swath_dimmap == true) {
                    if (backward_handle_swath_dimmap == true || multi_dimmap == true) 
                        has_ll_coord = true;
                }
                for (const auto &dim:gfield->getDimensions()) {

                    // handle coordinates attributes
                    map<string,string>::iterator tempmapit;
                    map<string,string>::iterator tempmapit2;
              
                    // Find the dimension field name
                    tempmapit = (swath->dimcvarlist).find(dim->getName());
                    if (tempmapit != (swath->dimcvarlist).end()) 
                        tempfieldname = tempmapit->second;
                    else 
                        throw4("cannot find the dimension field name",swath->getName(),
                               gfield->getName(),dim->getName());

                    // Find the corrected dimension field name
                    tempmapit2 = (swath->ncvarnamelist).find(tempfieldname);
                    if (tempmapit2 != (swath->ncvarnamelist).end()) 
                        tempcorrectedfieldname = tempmapit2->second;
                    else 
                        throw4("cannot find the corrected dimension field name",
                                swath->getName(),gfield->getName(),dim->getName());

                    if (false == has_ll_coord) 
                        has_ll_coord= check_ll_in_coords(tempcorrectedfieldname);

                    if (tempcount == 0) 
                        tempcoordinates= tempcorrectedfieldname;
                    else 
                        tempcoordinates = tempcoordinates +" "+tempcorrectedfieldname;
                    tempcount++;
                }
                if (true == has_ll_coord)
                    gfield->setCoordinates(tempcoordinates);
            }

            // Add units for latitude and longitude
            // latitude,adding the CF units degrees_north.
            if (gfield->fieldtype == 1) {
                string tempunits = "degrees_north";
                gfield->setUnits(tempunits);
            }

            // longitude, adding the CF units degrees_east
            if (gfield->fieldtype == 2) {  
                string tempunits = "degrees_east";
                gfield->setUnits(tempunits);
            }

            // Add units for Z-dimension, now it is always "level"
            // We decide not touch the units if the third-dimension CV exists(fieldtype =3)
            // KY 2013-02-15
            //if ((gfield->fieldtype == 3)||(gfield->fieldtype == 4)) 
            if (gfield->fieldtype == 4) {
                string tempunits ="level";
                gfield->setUnits(tempunits);
            }

            // Add units for "Time", 
            // Be aware that it is always "days since 1900-01-01 00:00:00"(JIRA HFRHANDLER-167)
            if (gfield->fieldtype == 5) {
                string tempunits = "days since 1900-01-01 00:00:00";
                gfield->setUnits(tempunits);
            }
            // Set the fill value for floating type data that doesn't have the fill value.
            // We found _FillValue attribute is missing from some swath data.
            // To cover the most cases, an attribute called _FillValue(the value is -9999.0)
            // is added to the data whose type is float32 or float64.
            if (((gfield->getFillValue()).empty()) && 
                (gfield->getType()==DFNT_FLOAT32 || gfield->getType()==DFNT_FLOAT64)) { 
                float tempfillvalue = -9999.0;
                gfield->addFillValue(tempfillvalue);
                gfield->setAddedFillValue(true);
            }
        }
 
        // Data fields
        for (const auto &dfield:swath->getDataFields()) {
                 
            // Real fields: adding coordinate attributes
            if (dfield->fieldtype == 0)  {// currently it is always true.
                string tempcoordinates="";
                string tempfieldname="";
                string tempcorrectedfieldname="";
                int tempcount = 0;
                bool has_ll_coord = false;
                if (swath->get_num_map() == 0)
                    has_ll_coord = true;
                else if (handle_swath_dimmap == true) {
                    if (backward_handle_swath_dimmap == true || multi_dimmap == true) 
                        has_ll_coord = true;
                }
                for (const auto &dim:dfield->getDimensions()) {

                    // handle coordinates attributes
                    map<string,string>::iterator tempmapit;
                    map<string,string>::iterator tempmapit2;
              
                    // Find the dimension field name
                    tempmapit = (swath->dimcvarlist).find(dim->getName());
                    if (tempmapit != (swath->dimcvarlist).end()) 
                        tempfieldname = tempmapit->second;
                    else 
                        throw4("cannot find the dimension field name",swath->getName(),
                                dfield->getName(),dim->getName());

                    // Find the corrected dimension field name
                    tempmapit2 = (swath->ncvarnamelist).find(tempfieldname);
                    if (tempmapit2 != (swath->ncvarnamelist).end()) 
                        tempcorrectedfieldname = tempmapit2->second;
                    else 
                        throw4("cannot find the corrected dimension field name",
                               swath->getName(),dfield->getName(),dim->getName());

                    if (false == has_ll_coord) 
                        has_ll_coord= check_ll_in_coords(tempcorrectedfieldname);

                    if (tempcount == 0) 
                        tempcoordinates= tempcorrectedfieldname;
                    else 
                        tempcoordinates = tempcoordinates +" "+tempcorrectedfieldname;
                    tempcount++;
                }
                if (true == has_ll_coord) 
                    dfield->setCoordinates(tempcoordinates);
            }
            // Add units for Z-dimension, now it is always "level"
            if ((dfield->fieldtype == 3)||(dfield->fieldtype == 4)) {
                string tempunits ="level";
                dfield->setUnits(tempunits);
            }

            // Add units for "Time", Be aware that it is always "days since 1900-01-01 00:00:00"
            // documented at JIRA (HFRHANDLER-167)
            if (dfield->fieldtype == 5) {
                string tempunits = "days since 1900-01-01 00:00:00";
                dfield->setUnits(tempunits);
            }

            // Set the fill value for floating type data that doesn't have the fill value.
            // We found _FillValue attribute is missing from some swath data.
            // To cover the most cases, an attribute called _FillValue(the value is -9999.0)
            // is added to the data whose type is float32 or float64.
            if (((dfield->getFillValue()).empty()) && 
                (dfield->getType()==DFNT_FLOAT32 || dfield->getType()==DFNT_FLOAT64)) { 
                float tempfillvalue = -9999.0;
                dfield->addFillValue(tempfillvalue);
                dfield->setAddedFillValue(true);
            }
        }
    }
}

// Find dimension that has the dimension name.
bool File::find_dim_in_dims(const std::vector<Dimension*>&dims,const std::string &dim_name) const {

    bool ret_value = false;
    for (const auto &dim:dims) {
        if (dim->name == dim_name) {
            ret_value = true;
            break;
        }
    }
    return ret_value;
}

// Check if the original dimension names in Lat/lon that holds the dimension maps are used by data fields.
void File::check_dm_geo_dims_in_vars() const {

    if (handle_swath_dimmap == false) 
        return;

    for (const auto &swath:this->swaths) {

        // Currently we only support swath that has 2-D lat/lon(MODIS).
        if (swath->get_num_map() > 0) {

            for (const auto &dfield:swath->getDataFields()) {

                int match_dims = 0;
                // We will only check the variables >=2D since lat/lon are 2D.
                if (dfield->rank >=2) {
                    for (const auto &dim:dfield->getDimensions()) {

                        // There may be multiple dimension maps that hold the same geo-dimension.
                        // We should not count this duplicately.
                        bool not_match_geo_dim = true;
                        for (const auto &sdmap:swath->getDimensionMaps()) {

                            if ((dim->getName() == sdmap->getGeoDimension()) && not_match_geo_dim){ 
                                match_dims++;
                                not_match_geo_dim = false;
                            }
                        }
                    }
                }
                // This variable holds the GeoDimensions,this swath 
                if (match_dims == 2) {
                    swath->GeoDim_in_vars = true;
                    break;
                }
            }

            if (swath->GeoDim_in_vars == false) {

                for (const auto &gfield:swath->getGeoFields()) {

                    int match_dims = 0;
                    // We will only check the variables >=2D since lat/lon are 2D.
                    if (gfield->rank >=2 && (gfield->name != "Latitude" && gfield->name != "Longitude")) {
                        for (const auto &dim:gfield->getDimensions())  {

                             // There may be multiple dimension maps that hold the same geo-dimension.
                             // We should not count this duplicately.
                             bool not_match_geo_dim = true;
 
                            for (const auto &sdmap:swath->getDimensionMaps()) {
                                if ((dim->getName() == sdmap->getGeoDimension()) && not_match_geo_dim){
                                    match_dims++;
                                    not_match_geo_dim = false;
                                }
                            }
                        }
                    }
                    // This variable holds the GeoDimensions,this swath 
                    if (match_dims == 2){
                        swath->GeoDim_in_vars = true;
                        break;
                    }
                }
            }
        }
    }

    return;
}

// Based on the dimension name and the mapped dimension name,obtain the offset and increment.
// return false if there is no match.
bool SwathDataset::obtain_dmap_offset_inc(const string& ori_dimname, const string & mapped_dimname,int &offset,int&inc) const{
    bool ret_value = false;
    for (const auto &sdmap:this->dimmaps) {
        if (sdmap->geodim==ori_dimname && sdmap->datadim == mapped_dimname){
            offset = sdmap->offset;
            inc = sdmap->increment;
            ret_value = true;
            break;
        }
    }
    return ret_value;
}
 
// For the multi-dimension map case, generate all the lat/lon names. The original lat/lon
// names should be used. 
// For one swath, we don't need to provide the swath name. Yes, swath name is missed. However. this is
// what users want. For multi swaths, swath names are added.
void File::create_geo_varnames_list(vector<string> & geo_varnames,const string & swathname, 
                                    const string & fieldname,int extra_ll_pairs,bool oneswath) const{
    // We will always keep Latitude and Longitude 
    if (true == oneswath)
        geo_varnames.push_back(fieldname);
    else {
        string nfieldname = fieldname+"_"+swathname;
        geo_varnames.push_back(nfieldname);
    }
    for (int i = 0; i <extra_ll_pairs;i++) {
        string nfieldname;
        stringstream si;
        si << (i+1);
        if ( true == oneswath) // No swath name is needed.
            nfieldname = fieldname+"_"+si.str();
        else 
            nfieldname = fieldname+"_"+swathname+"_"+si.str();
        geo_varnames.push_back(nfieldname);
    }

}

// Make just one routine for both latitude and longtitude dimmaps.
// In longitude part, we just ignore ..
void File::create_geo_dim_var_maps(SwathDataset*sd, Field*fd,const vector<string>& lat_names, 
                                   const vector<string>& lon_names,vector<Dimension*>& geo_var_dim1,
                                   vector<Dimension*>& geo_var_dim2) const{
    string field_lat_dim1_name =(fd->dims)[0]->name;
    string field_lat_dim2_name =(fd->dims)[1]->name;

    // Keep the original Latitude/Longitude and the dimensions when GeoDim_in_vars is true.
    if (sd->GeoDim_in_vars == true) {
        if ((this->swaths).size() >1) {
            (fd->dims)[0]->name = field_lat_dim1_name+"_"+sd->name;
            (fd->dims)[1]->name = field_lat_dim2_name+"_"+sd->name;
        }
        geo_var_dim1.push_back((fd->dims)[0]);
        geo_var_dim2.push_back((fd->dims)[1]);
    }

    // Create dimension list for the lats and lons.
    // Consider the multi-swath case and if the dimension names of orig. lat/lon
    // are used.
    // We also need to consider one geo-dim can map to multiple data dim.
    // One caveat for the current approach is that we don't consider 
    // two dimension maps are not created in order. HDFEOS2 API implies
    // the dimension maps are created in order.
    short dim1_map_count = 0;
    short dim2_map_count = 0;
    for (const auto &sdmap:sd->getDimensionMaps()) {
        if (sdmap->getGeoDimension()==field_lat_dim1_name){
            string data_dim1_name = sdmap->getDataDimension();
            int dim1_size = sd->obtain_dimsize_with_dimname(data_dim1_name);
            if ((this->swaths).size() > 1)
                data_dim1_name = data_dim1_name+"_"+sd->name;

            if (sd->GeoDim_in_vars == false && dim1_map_count == 0) {
                (fd->dims)[0]->name = data_dim1_name;          
                (fd->dims)[0]->dimsize = dim1_size;          
                geo_var_dim1.push_back((fd->dims)[0]);
            }
            else {
                auto lat_dim = new Dimension(data_dim1_name,dim1_size);
                geo_var_dim1.push_back(lat_dim);
            }
            dim1_map_count++;
        }
        else if (sdmap->getGeoDimension()==field_lat_dim2_name){
            string data_dim2_name = sdmap->getDataDimension();
            int dim2_size = sd->obtain_dimsize_with_dimname(data_dim2_name);
            if ((this->swaths).size() > 1)
                data_dim2_name = data_dim2_name+"_"+sd->name;
            if (sd->GeoDim_in_vars == false && dim2_map_count == 0) {
                (fd->dims)[1]->name = data_dim2_name;          
                (fd->dims)[1]->dimsize = dim2_size;          
                geo_var_dim2.push_back((fd->dims)[1]);
            }
            else {
                auto lon_dim = new Dimension(data_dim2_name,dim2_size);
                geo_var_dim2.push_back(lon_dim);
            }
            dim2_map_count++;
        }
    }

    // Build up dimension names to coordinate var lists.
    for(int i = 0; i<lat_names.size();i++) {
        HDFCFUtil::insert_map(sd->dimcvarlist, (geo_var_dim1[i])->name,lat_names[i]);
        HDFCFUtil::insert_map(sd->dimcvarlist, (geo_var_dim2[i])->name,lon_names[i]);
    }
     
    return;
}

// Generate lat/lon variables for the multi-dimension map case for this swath.
// Original lat/lon variable information is provided.
void File::create_geo_vars(SwathDataset* sd,Field *orig_lat,Field*orig_lon,
                           const vector<string>& lat_names,const vector<string>& lon_names,
                          vector<Dimension*>&geo_var_dim1,vector<Dimension*>&geo_var_dim2){

    // Need to have ll dimension names to obtain the dimension maps
    string ll_ori_dim0_name = (orig_lon->dims)[0]->name;
    string ll_ori_dim1_name = (orig_lon->dims)[1]->name;
    int dmap_offset = 0;
    int dmap_inc = 0;
    if (sd->GeoDim_in_vars == false) {
        
        // The original lat's dim has been updated in create_geo_dim_var_maps
        // In theory , it should be reasonable to do it here. Later. 
        (orig_lon->dims)[0]->name = geo_var_dim1[0]->name;
        (orig_lon->dims)[0]->dimsize = geo_var_dim1[0]->dimsize;
        (orig_lon->dims)[1]->name = geo_var_dim2[0]->name;
        (orig_lon->dims)[1]->dimsize = geo_var_dim2[0]->dimsize;
        string ll_datadim0_name = geo_var_dim1[0]->name;
        string ll_datadim1_name = geo_var_dim2[0]->name;
        if (this->swaths.size() >1) {
            string prefix_remove = "_"+sd->name;
            ll_datadim0_name = ll_datadim0_name.substr(0,ll_datadim0_name.size()-prefix_remove.size());
            ll_datadim1_name = ll_datadim1_name.substr(0,ll_datadim1_name.size()-prefix_remove.size());
        }

        // dimension map offset and inc should be retrieved.
        if (false == sd->obtain_dmap_offset_inc(ll_ori_dim0_name,ll_datadim0_name,dmap_offset,dmap_inc)){
            throw5("Cannot retrieve dimension map offset and inc ",sd->name,
                    orig_lon->name,ll_ori_dim0_name,ll_datadim0_name);
        }
        orig_lon->ll_dim0_inc = dmap_inc;
        orig_lon->ll_dim0_offset = dmap_offset;
        orig_lat->ll_dim0_inc = dmap_inc;
        orig_lat->ll_dim0_offset = dmap_offset;

        if (false == sd->obtain_dmap_offset_inc(ll_ori_dim1_name,ll_datadim1_name,dmap_offset,dmap_inc)){
            throw5("Cannot retrieve dimension map offset and inc ",sd->name,
                    orig_lon->name,ll_ori_dim1_name,ll_datadim1_name);
        }
        orig_lon->ll_dim1_inc = dmap_inc;
        orig_lon->ll_dim1_offset = dmap_offset;
        orig_lat->ll_dim1_inc = dmap_inc;
        orig_lat->ll_dim1_offset = dmap_offset;
#if 0
cerr<<"orig_lon "<<orig_lon->name <<endl;
cerr<<"orig_lon dim1 inc "<<orig_lon->ll_dim1_inc<<endl;
cerr<<"orig_lon dim1 offset  "<<orig_lon->ll_dim1_offset<<endl;
cerr<<"orig_lon dim0 inc "<<orig_lon->ll_dim0_inc<<endl;
cerr<<"orig_lon dim0 offset  "<<orig_lon->ll_dim0_offset<<endl;
#endif

       
    }
    else {
        // if GeoDim is used, we still need to update longitude's dimension names
        // if multiple swaths. Latitude was done in create_geo_dim_var_maps().
        if ((this->swaths).size() >1) {
            (orig_lon->dims)[0]->name = (orig_lon->dims)[0]->name + "_" + sd->name;
            (orig_lon->dims)[1]->name = (orig_lon->dims)[1]->name + "_" + sd->name;
        }
    }

    // We also need to update the latitude and longitude names when num_swath is not 1. 
    if ((this->swaths).size()>1) { 
        orig_lat->name = lat_names[0];
        orig_lon->name = lon_names[0];
    }

    // Mark the original lat/lon as coordinate variables.
    orig_lat->fieldtype = 1;
    orig_lon->fieldtype = 2;

    // The added fields.
    for (int i = 1; i <lat_names.size();i++) {
        auto newlat = new Field();
        newlat->name = lat_names[i];
        (newlat->dims).push_back(geo_var_dim1[i]);
        (newlat->dims).push_back(geo_var_dim2[i]);
        newlat->fieldtype = 1;
        newlat->rank = 2;
        newlat->type = orig_lat->type;
        auto newlon = new Field();
        newlon->name = lon_names[i];
        // Here we need to create new Dimensions 
        // for Longitude.
        auto lon_dim1= 
           new Dimension(geo_var_dim1[i]->name,geo_var_dim1[i]->dimsize);
        auto lon_dim2= 
           new Dimension(geo_var_dim2[i]->name,geo_var_dim2[i]->dimsize);
        (newlon->dims).push_back(lon_dim1);
        (newlon->dims).push_back(lon_dim2);
        newlon->fieldtype = 2;
        newlon->rank = 2;
        newlon->type = orig_lon->type;

        string ll_datadim0_name = geo_var_dim1[i]->name;
        string ll_datadim1_name = geo_var_dim2[i]->name;
        if (this->swaths.size() >1) {
            string prefix_remove = "_"+sd->name;
            ll_datadim0_name = ll_datadim0_name.substr(0,ll_datadim0_name.size()-prefix_remove.size());
            ll_datadim1_name = ll_datadim1_name.substr(0,ll_datadim1_name.size()-prefix_remove.size());
        }

        // Obtain dimension map offset and inc for the new lat/lon.
        if (false == sd->obtain_dmap_offset_inc(ll_ori_dim0_name,ll_datadim0_name,dmap_offset,dmap_inc)){
            throw5("Cannot retrieve dimension map offset and inc ",sd->name,
                    newlon->name,ll_ori_dim0_name,ll_datadim0_name);
        }
        newlon->ll_dim0_inc = dmap_inc;
        newlon->ll_dim0_offset = dmap_offset;
        newlat->ll_dim0_inc = dmap_inc;
        newlat->ll_dim0_offset = dmap_offset;
        if (false == sd->obtain_dmap_offset_inc(ll_ori_dim1_name,ll_datadim1_name,dmap_offset,dmap_inc)){
            throw5("Cannot retrieve dimension map offset and inc ",sd->name,
                    newlon->name,ll_ori_dim0_name,ll_datadim1_name);
        }
        newlon->ll_dim1_inc = dmap_inc;
        newlon->ll_dim1_offset = dmap_offset;
        newlat->ll_dim1_inc = dmap_inc;
        newlat->ll_dim1_offset = dmap_offset;
#if 0
cerr<<"newlon "<<newlon->name <<endl;
cerr<<"newlon dim1 inc "<<newlon->ll_dim1_inc<<endl;
cerr<<"newlon dim1 offset  "<<newlon->ll_dim1_offset<<endl;
cerr<<"newlon dim0 inc "<<newlon->ll_dim0_inc<<endl;
cerr<<"newlon dim0 offset  "<<newlon->ll_dim0_offset<<endl;
#endif
        sd->geofields.push_back(newlat);
        sd->geofields.push_back(newlon);
    }
#if 0
//cerr<<"Latlon under swath: "<<sd->name<<endl;
for (vector<Field *>::const_iterator j =
     sd->getGeoFields().begin();
     j != sd->getGeoFields().end(); ++j) {
cerr<<"Field name: "<<(*j)->name <<endl;
cerr<<"Dimension name and size: "<<endl;
   for(vector<Dimension *>::const_iterator k=
      (*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k)
cerr<<(*k)->getName() <<": "<<(*k)->getSize() <<endl;
}
#endif

}

// We need to update the dimensions for all the swath and all the fields when
// we can handle the multi-dimension map case. This only applies to >1 swath.
// For one swath, dimension names are not needed to be updated.
void File::update_swath_dims_for_dimmap(const SwathDataset* sd,const std::vector<Dimension*>&geo_var_dim1, 
                                        const std::vector<Dimension*>&geo_var_dim2) const{

    // Loop through each field under geofields and data fields. update dimensions.
    // Obtain each dimension name + _+swath_name, if match with geo_var_dim1 or geo_var_dim2;
    // Update the dimension names with the matched one.
    for (const auto &gfield:sd->getGeoFields()) {
        // No need to update latitude/longitude 
        if (gfield->fieldtype == 1 || gfield->fieldtype == 2) 
            continue;
        for (const auto &dim:gfield->getDimensions()) {
            string new_dim_name = dim->name +"_"+sd->name;
            if (find_dim_in_dims(geo_var_dim1,new_dim_name) || 
                find_dim_in_dims(geo_var_dim2,new_dim_name)) 
                dim->name = new_dim_name;
        }
    }

    for (const auto &dfield:sd->getDataFields()){ 
        for (const auto &dim:dfield->getDimensions()) {
            string new_dim_name = dim->name +"_"+sd->name;
            if (find_dim_in_dims(geo_var_dim1,new_dim_name) || 
               find_dim_in_dims(geo_var_dim2,new_dim_name)) 
                dim->name = new_dim_name;
        }
    }

    // We also need to update the dimension name of this swath.
    for (const auto &dim:sd->getDimensions()) { 
        string new_dim_name = dim->name +"_"+sd->name;
        if (find_dim_in_dims(geo_var_dim1,new_dim_name) || 
           find_dim_in_dims(geo_var_dim2,new_dim_name)) 
            dim->name = new_dim_name;
    }

    return;

}

// This is the main function to handle the multi-dimension map case.
// It creates the lat/lon lists, handle dimension names and then
// provide the dimension name to coordinate variable map.
void File::create_swath_latlon_dim_cvar_map_for_dimmap(SwathDataset* sd, Field* ori_lat, Field* ori_lon) {

    bool one_swath = ((this->swaths).size() == 1);

    int num_extra_lat_lon_pairs = 0;

#if 0    
if (sd->GeoDim_in_vars == true) 
 cerr<<" swath name is "<<sd->name <<endl;
#endif

    // Since the original lat/lon will be kept for lat/lon with the first  dimension map..
    if (sd->GeoDim_in_vars == false) 
        num_extra_lat_lon_pairs--;

    num_extra_lat_lon_pairs += (sd->num_map)/2;

    vector<string> lat_names;
    create_geo_varnames_list(lat_names,sd->name,ori_lat->name,num_extra_lat_lon_pairs,one_swath);
    vector<string>lon_names;
    create_geo_varnames_list(lon_names,sd->name,ori_lon->name,num_extra_lat_lon_pairs,one_swath);
    vector<Dimension*> geo_var_dim1;
    vector<Dimension*> geo_var_dim2;

    // Define dimensions or obtain dimensions for new field.
    create_geo_dim_var_maps(sd, ori_lat,lat_names,lon_names,geo_var_dim1,geo_var_dim2);
    create_geo_vars(sd,ori_lat,ori_lon,lat_names,lon_names,geo_var_dim1,geo_var_dim2);

    // Update dims for vars,this is only necessary when there are multiple swaths 
    // Dimension names need to be updated to include swath names.
    if ((this->swaths).size() >1) 
        update_swath_dims_for_dimmap(sd,geo_var_dim1,geo_var_dim2);
    
}

/// Read and prepare. This is the main method to make the DAP output CF-compliant.
/// All dimension(coordinate variables) information need to be ready.
/// All special arrangements need to be done in this step.
void File::Prepare(const char *eosfile_path) 
{

    // check if this is a special HDF-EOS2 grid(MOD08_M3) that have all dimension scales
    // added by the HDF4 library but the relation between the dimension scale and the dimension is not
    // specified. If the return value  is true, we will specify  

    // Obtain the number of swaths and the number of grids
    auto numgrid = (int)(this->grids.size());
    auto numswath = (int)(this->swaths.size()); 
    
    if (numgrid < 0) 
        throw2("the number of grid is less than 0", eosfile_path);
    
    // First handle grids
    if (numgrid > 0) {

        // Obtain "XDim","YDim","Latitude","Longitude" and "location" set.
        string DIMXNAME = this->get_geodim_x_name();       
      
        string DIMYNAME = this->get_geodim_y_name();       

        string LATFIELDNAME = this->get_latfield_name();       

        string LONFIELDNAME = this->get_lonfield_name();       

        // Now only there is only one geo grid name "location", so don't call it know.
        string GEOGRIDNAME = "location";

        // Check global lat/lon for multiple grids.
        // We want to check if there is a global lat/lon for multiple grids.
        // AIRS level 3 data provides lat/lon under the GEOGRIDNAME grid.
        check_onelatlon_grids();

        // Handle the third-dimension(both existing and missing) coordinate variables
        for (const auto &grid:this->grids) 
            handle_one_grid_zdim(grid);
        
        
        // Handle lat/lon fields for the case of which all grids have one dedicated lat/lon grid.
        if (true == this->onelatlon) 
            handle_onelatlon_grids();
        else  {
            for (const auto &grid:this->grids) {

                // Set the horizontal dimension name "dimxname" and "dimyname"
                // This will be used to detect the dimension major order.
                grid->setDimxName(DIMXNAME);
                grid->setDimyName(DIMYNAME);

                // Handle lat/lon(both existing lat/lon and calculated lat/lon from EOS2 APIs)
                handle_one_grid_latlon(grid);
            }
        }

        // Handle the dimension name to coordinate variable map for grid. 
        handle_grid_dim_cvar_maps();

        // Follow COARDS for grids.
        handle_grid_coards();

        // Create the corrected dimension vector for each field when COARDS is not followed.
        update_grid_field_corrected_dims();

        // Handle CF attributes for grids. 
        handle_grid_cf_attrs();

        // Special handling SOM(Space Oblique Mercator) projection files
        handle_grid_SOM_projection();


    }// End of handling grid

    // Check and set the scale type
    for (const auto& grid:this->grids) 
        grid->SetScaleType(grid->name);
    
    if (numgrid==0) {
  
        // Now we handle swath case. 
        if (numswath > 0) {

           // Check if we need to handle dimension maps in this file. 
           check_swath_dimmap(numswath);

           // Check if GeoDim is used by variables when dimension maps are present.
           check_dm_geo_dims_in_vars();

           // If we need to handle dimension maps,check if we need to keep the old way.
           check_swath_dimmap_bk_compat(numswath);

           // Create the dimension name to coordinate variable name map for lat/lon. 
           create_swath_latlon_dim_cvar_map();

           // Create the dimension name to coordinate variable name map for non lat/lon coordinate variables.
           create_swath_nonll_dim_cvar_map();

           // Handle swath dimension name to coordinate variable name maps. 
           handle_swath_dim_cvar_maps();

           // Handle CF attributes for swaths. 
           // The CF attributes include "coordinates", "units" for coordinate variables and "_FillValue". 
           handle_swath_cf_attrs();
 
            // Check and set the scale type
            for (const auto &swath:this->swaths) 
                swath->SetScaleType(swath->name);
        }

    }// End of handling swath
   
}

bool File::check_special_1d_grid() {

    auto numgrid = (int)(this->grids.size());
    auto numswath = (int)(this->swaths.size());
    
    if (numgrid != 1 || numswath != 0) 
        return false;

    // Obtain "XDim","YDim","Latitude","Longitude" and "location" set.
    string DIMXNAME = this->get_geodim_x_name();
    string DIMYNAME = this->get_geodim_y_name();

    if (DIMXNAME != "XDim" || DIMYNAME != "YDim") 
        return false;

    int var_dimx_size = 0;
    int var_dimy_size = 0;

    const GridDataset *mygrid = (this->grids)[0];

    int field_xydim_flag = 0;
    for (const auto &dfield:mygrid->getDataFields()) {
        if (1==dfield->rank) {
            if (dfield->name == "XDim"){
                field_xydim_flag++;
                var_dimx_size = (dfield->getDimensions())[0]->getSize();
            }
            if (dfield->name == "YDim"){
                field_xydim_flag++;
                var_dimy_size = (dfield->getDimensions())[0]->getSize();
            }
        }
        if (2==field_xydim_flag)
            break;
    }

    if (field_xydim_flag !=2)
        return false;

    // Obtain XDim and YDim size.
    int xdimsize = mygrid->getInfo().getX();
    int ydimsize = mygrid->getInfo().getY();
   
    if (var_dimx_size != xdimsize || var_dimy_size != ydimsize)
        return false;

    return true;
    
}
    

bool File::check_ll_in_coords(const string& vname) {

    bool ret_val = false;
    for (const auto &swath:this->swaths) {
        for (const auto &gfield:swath->getGeoFields()) {
             // Real fields: adding the coordinate attribute
            if (gfield->fieldtype == 1 || gfield->fieldtype == 2)  {// currently it is always true.
                if (gfield->getNewName() == vname) {
                    ret_val = true;
                    break;
                }
            }
        }
        if (true == ret_val) 
            break;
        for (const auto &dfield:swath->getDataFields()) {

            // Real fields: adding the coordinate attribute
            if (dfield->fieldtype == 1 || dfield->fieldtype == 2)  {// currently it is always true.
                if (dfield->getNewName() == vname) {
                    ret_val = true;
                    break;
                }
            }
        }
        if (true == ret_val) 
            break;

    }
    return ret_val;
}


// Set scale and offset type
// MODIS data has three scale and offset rules. 
// They are 
// MODIS_EQ_SCALE: raw_data = scale*data + offset
// MODIS_MUL_SCALE: raw_data = scale*(data -offset)
// MODIS_DIV_SCALE: raw_data = (data-offset)/scale

void Dataset::SetScaleType(const string & EOS2ObjName) {


    // Group features of MODIS products.

    vector<string> modis_multi_scale_type;
    modis_multi_scale_type.emplace_back("L1B");
    modis_multi_scale_type.emplace_back("GEO");
    modis_multi_scale_type.emplace_back("BRDF");
    modis_multi_scale_type.emplace_back("0.05Deg");
    modis_multi_scale_type.emplace_back("Reflectance");
    modis_multi_scale_type.emplace_back("MOD17A2");
    modis_multi_scale_type.emplace_back("North");
    modis_multi_scale_type.emplace_back("South");
    modis_multi_scale_type.emplace_back("MOD_Swath_Sea_Ice");
    modis_multi_scale_type.emplace_back("MOD_Grid_MOD15A2");
    modis_multi_scale_type.emplace_back("MOD_Grid_MOD16A2");
    modis_multi_scale_type.emplace_back("MOD_Grid_MOD16A3");
    modis_multi_scale_type.emplace_back("MODIS_NACP_LAI");
        
    vector<string> modis_div_scale_type;

    // Always start with MOD or mod.
    modis_div_scale_type.emplace_back("VI");
    modis_div_scale_type.emplace_back("1km_2D");
    modis_div_scale_type.emplace_back("L2g_2d");
    modis_div_scale_type.emplace_back("CMG");
    modis_div_scale_type.emplace_back("MODIS SWATH TYPE L2");

    string modis_eq_scale_type   = "LST";
    string modis_equ_scale_lst_group1="MODIS_Grid_8Day_1km_LST21";
    string modis_equ_scale_lst_group2="MODIS_Grid_Daily_1km_LST21";

    string modis_divequ_scale_group = "MODIS_Grid";
    string modis_div_scale_group = "MOD_Grid";

    // The scale-offset rule for the following group may be MULTI but since add_offset is 0. So
    // the MULTI rule is equal to the EQU rule. KY 2013-01-25
    string modis_equ_scale_group  = "MODIS_Grid_1km_2D";

    if (EOS2ObjName=="mod05" || EOS2ObjName=="mod06" || EOS2ObjName=="mod07" 
                            || EOS2ObjName=="mod08" || EOS2ObjName=="atml2")
    {
        scaletype = SOType::MODIS_MUL_SCALE;
        return;
    }

    // Find one MYD09GA2012.version005 file that 
    // the grid names change to MODIS_Grid_500m_2D. 
    // So add this one. KY 2012-11-20
 
    // Find the grid name in one MCD43C1 file starts with "MCD_CMG_BRDF", 
    // however, the offset of the data is 0.
    // So we may not handle this file here since it follows the CF conventions. 
    // May need to double check them later. KY 2013-01-24 


    if (EOS2ObjName.find("MOD")==0 || EOS2ObjName.find("mod")==0) 
    {
        size_t pos = EOS2ObjName.rfind(modis_eq_scale_type);
        if (pos != string::npos && 
          (pos== (EOS2ObjName.size()-modis_eq_scale_type.size())))
        {
            scaletype = SOType::MODIS_EQ_SCALE;
            return;
        }

        for(unsigned int k=0; k<modis_multi_scale_type.size(); k++)
        {
            pos = EOS2ObjName.rfind(modis_multi_scale_type[k]);
            if (pos !=string::npos && 
               (pos== (EOS2ObjName.size()-modis_multi_scale_type[k].size())))
            {
                scaletype = SOType::MODIS_MUL_SCALE;
                return;
            }
        }

        for(unsigned int k=0; k<modis_div_scale_type.size(); k++)
        {
            pos = EOS2ObjName.rfind(modis_div_scale_type[k]);
            if (pos != string::npos && 
                (pos==(EOS2ObjName.size()-modis_div_scale_type[k].size()))){
                scaletype = SOType::MODIS_DIV_SCALE;

                // We have a case that group 
                // MODIS_Grid_1km_2D should apply the equal scale equation.
                // This will be handled after this loop.
                if (EOS2ObjName != "MODIS_Grid_1km_2D") 
                    return;
            }
        }

        // Special handling for MOD_Grid and MODIS_Grid_500m_2D. 
        // Check if the group name starts with the modis_divequ and modis_div_scale.
        pos = EOS2ObjName.find(modis_divequ_scale_group);

        // Find the "MODIS_Grid???" group. 
        // We have to separate MODIS_Grid_1km_2D(EQ),MODIS_Grid_8Day_1km_LST21
        // and MODIS_Grid_Daily_1km_LST21 from other grids(DIV). 
        if (0 == pos) { 
            size_t eq_scale_pos = EOS2ObjName.find(modis_equ_scale_group)
                                 *EOS2ObjName.find(modis_equ_scale_lst_group1)
                                 *EOS2ObjName.find(modis_equ_scale_lst_group2);
            if (0 == eq_scale_pos) 
                scaletype = SOType::MODIS_EQ_SCALE;
            else 
                scaletype = SOType::MODIS_DIV_SCALE;
        }
        else {
            size_t div_scale_pos = EOS2ObjName.find(modis_div_scale_group);

            // Find the "MOD_Grid???" group. 
            if ( 0 == div_scale_pos) 
                scaletype = SOType::MODIS_DIV_SCALE;
        }
    }

    //  MEASuRES VIP files have the grid name VIP_CMG_GRID. 
    // This applies to all VIP version 2 files. KY 2013-01-24
    if (EOS2ObjName =="VIP_CMG_GRID")
        scaletype = SOType::MODIS_DIV_SCALE;
}

int Dataset::obtain_dimsize_with_dimname(const string & dimname) const{

    int ret_value = -1;

    for (const auto & dim:this->getDimensions()) {
        if (dim->name == dimname){
            ret_value = dim->dimsize;
            break;
        }
    }

    return ret_value;
}

// Release resources
Field::~Field()
{
    for_each(this->dims.begin(), this->dims.end(), delete_elem());
    for_each(this->correcteddims.begin(), this->correcteddims.end(), delete_elem());
}

// Release resources
Dataset::~Dataset()
{
    for_each(this->dims.begin(), this->dims.end(), delete_elem());
    for_each(this->datafields.begin(), this->datafields.end(),
                  delete_elem());
    for_each(this->attrs.begin(), this->attrs.end(), delete_elem());
}

// Retrieve dimensions of grids or swaths
void Dataset::ReadDimensions(int32 (*entries)(int32, int32, int32 *),
                             int32 (*inq)(int32, char *, int32 *),
                             vector<Dimension *> &d_dims) 
{
    // Initialize number of dimensions
    int32 numdims = 0; 

    // Initialize buf size
    int32 bufsize = 0;

    // Obtain the number of dimensions and buffer size of holding ","
    // separated dimension name list.
    if ((numdims = entries(this->datasetid, HDFE_NENTDIM, &bufsize)) == -1)
        throw2("dimension entry", this->name);

    // Read all dimension information
    if (numdims > 0) {
        vector<char> namelist;
        vector<int32> dimsize;

        namelist.resize(bufsize + 1);
        dimsize.resize(numdims);

        // Inquiry dimension name lists and sizes for all dimensions
        if (inq(this->datasetid, namelist.data(), dimsize.data()) == -1)
            throw2("inquire dimension", this->name);

        vector<string> dimnames;

        // Make the "," separated name string to a string list without ",".
        // This split is for global dimension of a Swath or a Grid object.
        HDFCFUtil::Split(namelist.data(), bufsize, ',', dimnames);
        int count = 0;

        for (const auto &dimname:dimnames) {
            auto dim = new Dimension(dimname, dimsize[count]);
            d_dims.push_back(dim);
            ++count;
        }
        
    }
}

// Retrieve data field info.
void Dataset::ReadFields(int32 (*entries)(int32, int32, int32 *),
                         int32 (*inq)(int32, char *, int32 *, int32 *),
                         intn (*fldinfo)
                         (int32, char *, int32 *, int32 *, int32 *, char *),
                         intn (*getfill)(int32, char *, VOIDP),
                         bool geofield,
                         vector<Field *> &fields) 
{

    // Initalize the number of fields
    int32 numfields = 0;

    // Initalize the buffer size
    int32 bufsize = 0;

    // Obtain the number of fields and buffer size for the current Swath or
    // Grid.
    if ((numfields = entries(this->datasetid, geofield ?
                             HDFE_NENTGFLD : HDFE_NENTDFLD, &bufsize)) == -1)
        throw2("field entry", this->name);

    // Obtain the information of fields (either data fields or geo-location
    // fields of a Swath object)
    if (numfields > 0) {
        vector<char> namelist;

        namelist.resize(bufsize + 1);

        // Inquiry fieldname list of the current object
        if (inq(this->datasetid, namelist.data(), nullptr, nullptr) == -1)
            throw2("inquire field", this->name);

        vector<string> fieldnames;

        // Split the field namelist, make the "," separated name string to a
        // string list without ",".
        HDFCFUtil::Split(namelist.data(), bufsize, ',', fieldnames);
        for (const auto& fdname:fieldnames){

            auto field = new Field();
            if (field == nullptr)
                throw1("The field is nullptr");
            field->name = fdname;

            bool throw_error = false;
            string err_msg;

            // XXX: We assume the maximum number of dimension for an EOS field
            // is 16.
            int32 dimsize[16];
            char dimlist[512]; // XXX: what an HDF-EOS2 developer recommended

            // Obtain most information of a field such as rank, dimension etc.
            if ((fldinfo(this->datasetid,
                         const_cast<char *>(field->name.c_str()),
                         &field->rank, dimsize, &field->type, dimlist)) == -1){
                string fieldname_for_eh = field->name;
		throw_error = true;
		err_msg ="Obtain field info error for field name "+fieldname_for_eh;
            }

	    if (false == throw_error) {
            
                vector<string> dimnames;

                // Split the dimension name list for a field
                HDFCFUtil::Split(dimlist, ',', dimnames);
                if ((int)dimnames.size() != field->rank) {
                    throw_error = true;
                    err_msg = "Dimension names size is not consistent with field rank. ";
                    err_msg += "Field name is "+field->name;
                }
                else {
                    for (int k = 0; k < field->rank; ++k) {
                        auto dim = new Dimension(dimnames[k], dimsize[k]);
                        field->dims.push_back(dim);
                    }

                    // Get fill value of a field
                    field->filler.resize(DFKNTsize(field->type));
                    if (getfill(this->datasetid,
                        const_cast<char *>(field->name.c_str()),
                        &field->filler[0]) == -1)
                        field->filler.clear();

                    // Append the field into the fields vector.
                    fields.push_back(field);
               }
            }
            
            if (true == throw_error) {
                delete field;
                throw1(err_msg);

	    }
        }
    }
}

// Retrieve attribute info.
void Dataset::ReadAttributes(int32 (*inq)(int32, char *, int32 *),
                             intn (*attrinfo)(int32, char *, int32 *, int32 *),
                             intn (*readattr)(int32, char *, VOIDP),
                             vector<Attribute *> &obj_attrs) 
{
    // Initalize the number of attributes to be 0
    int32 numattrs = 0;

    // Initalize the buffer size to be 0
    int32 bufsize = 0;

    // Obtain the number of attributes in a Grid or Swath
    if ((numattrs = inq(this->datasetid, nullptr, &bufsize)) == -1)
        throw2("inquire attribute", this->name);

    // Obtain the list of  "name, type, value" tuple
    if (numattrs > 0) {
        vector<char> namelist;

        namelist.resize(bufsize + 1);

        // inquiry namelist and buffer size
        if (inq(this->datasetid, namelist.data(), &bufsize) == -1)
            throw2("inquire attribute", this->name);

        vector<string> attrnames;

        // Split the attribute namelist, make the "," separated name string to
        // a string list without ",".
        HDFCFUtil::Split(namelist.data(), bufsize, ',', attrnames);
        for (const auto &attrname:attrnames) {
            auto attr = new Attribute();
            attr->name = attrname;
            attr->newname = HDFCFUtil::get_CF_string(attr->name);

            int32 count = 0;

            // Obtain the datatype and byte count of this attribute
            if (attrinfo(this->datasetid, const_cast<char *>
                        (attr->name.c_str()), &attr->type, &count) == -1) {
                delete attr;
                throw3("attribute info", this->name, attr->name);
            }

            attr->count = count/DFKNTsize(attr->type);
            attr->value.resize(count);
            
                        
            // Obtain the attribute value. Note that this function just
            // provides a copy of all attribute values. 
            //The client should properly interpret to obtain individual
            // attribute value.
            if (readattr(this->datasetid,
                         const_cast<char *>(attr->name.c_str()),
                         &attr->value[0]) == -1) {
                delete attr;
                throw3("read attribute", this->name, attr->name);
            }

            // Append this attribute to attrs list.
            obj_attrs.push_back(attr);
        }
    }
}

// Release grid dataset resources
GridDataset::~GridDataset()
{
    if (this->datasetid != -1){
        GDdetach(this->datasetid);
    }
}

// Retrieve all information of the grid dataset
GridDataset * GridDataset::Read(int32 fd, const string &gridname)
{
    string err_msg;
    bool GD_fun_err = false;
    auto grid = new GridDataset(gridname);

    // Open this Grid object 
    if ((grid->datasetid = GDattach(fd, const_cast<char *>(gridname.c_str())))
        == -1) {
        err_msg = "attach grid";
        GD_fun_err = true;
        goto cleanFail;
    }

    // Obtain the size of XDim and YDim as well as latitude and longitude of
    // the upleft corner and the low right corner. 
    {
        Info &info = grid->info;
        if (GDgridinfo(grid->datasetid, &info.xdim, &info.ydim, info.upleft,
                       info.lowright) == -1) {
            err_msg = "grid info";
            GD_fun_err = true;
            goto cleanFail;
        }
    }

    // Obtain projection information.
    {
        Projection &proj = grid->proj;
        if (GDprojinfo(grid->datasetid, &proj.code, &proj.zone, &proj.sphere,
                       proj.param) == -1) { 
            err_msg = "projection info";
            GD_fun_err = true;
            goto cleanFail;
        }
        if (GDpixreginfo(grid->datasetid, &proj.pix) == -1) {
            err_msg = "pixreg info";
            GD_fun_err = true;
            goto cleanFail;
        }
        if (GDorigininfo(grid->datasetid, &proj.origin) == -1){
            err_msg = "origin info";
            GD_fun_err = true;
            goto cleanFail;
        }
    }
cleanFail: 
    if (true == GD_fun_err){
        delete grid;
        throw2(err_msg,gridname);
    }

   try {
        // Read dimensions
        grid->ReadDimensions(GDnentries, GDinqdims, grid->dims);

        // Read all fields of this Grid.
        grid->ReadFields(GDnentries, GDinqfields, GDfieldinfo, 
                         GDgetfillvalue, false, grid->datafields);

        // Read all attributes of this Grid.
        grid->ReadAttributes(GDinqattrs, GDattrinfo, GDreadattr, grid->attrs);
    }
    catch (...) {
        delete grid;
        throw;
    }
    
    return grid;
}

GridDataset::Calculated & GridDataset::getCalculated() const
{
    if (this->calculated.grid != this)
        this->calculated.grid = this;
    return this->calculated;
}

bool GridDataset::Calculated::isYDimMajor() 
{
    this->DetectMajorDimension();
    return this->ydimmajor;
}


int GridDataset::Calculated::DetectFieldMajorDimension() const
{
    int ym = -1;
	
    // Traverse all data fields
    for (const auto& gfd:this->grid->getDataFields()) { 

        int xdimindex = -1;
        int ydimindex = -1;
        int index = 0;

        // Traverse all dimensions in each data field
        for (const auto& dim:gfd->getDimensions()) { 
            if (dim->getName() == this->grid->dimxname) 
                xdimindex = index;
            else if (dim->getName() == this->grid->dimyname) 
                ydimindex = index;
            ++index;
        }
        if (xdimindex == -1 || ydimindex == -1) 
            continue;

        int major = ydimindex < xdimindex ? 1 : 0;

        if (ym == -1)
            ym = major;
        break; 

         // TO gain performance, just check one field.
        // The dimension order for all data fields in a grid should be
        // consistent.
        // Kent adds this if 0 block 2012-09-19
#if 0
        else if (ym != major)
            throw2("inconsistent XDim, YDim order", this->grid->getName());
#endif

    }
    // At least one data field should refer to XDim or YDim
    if (ym == -1)
        throw2("lack of data fields", this->grid->getName());
    
    return ym;
}

void GridDataset::Calculated::DetectMajorDimension() 
{
    int ym = -1;
    // ydimmajor := true if (YDim, XDim)
    // ydimmajor := false if (XDim, YDim)

    // Traverse all data fields
    
    for (const auto& fd:this->grid->getDataFields()) {

        int xdimindex = -1;
        int ydimindex = -1;
        int index = 0;

        // Traverse all dimensions in each data field
        for (const auto& dim:fd->getDimensions()) {
            if (dim->getName() == this->grid->dimxname) 
                xdimindex = index;
            else if (dim->getName() == this->grid->dimyname) 
                ydimindex = index;
            ++index;
        }
        if (xdimindex == -1 || ydimindex == -1) 
            continue;

        // Change the way of deciding the major dimesion (LD -2012/01/10).
        int major;
        if (this->grid->getProjection().getCode() == GCTP_LAMAZ)
            major = 1;
        else
            major = ydimindex < xdimindex ? 1 : 0;

        if (ym == -1)
            ym = major;
        break; 

         // TO gain performance, just check one field.
        // The dimension order for all data fields in a grid should be
        // consistent.
        // Kent adds this if 0 block
#if 0 
        else if (ym != major)
            throw2("inconsistent XDim, YDim order", this->grid->getName());
#endif
    }
    // At least one data field should refer to XDim or YDim
    if (ym == -1)
        throw2("lack of data fields", this->grid->getName());
    this->ydimmajor = ym != 0;
}

// The following internal utilities are not used currently, will see if
// they are necessary in the future. KY 2012-09-19
// The internal utility method to check if two vectors have overlapped.
// If not, return true.
// Not used. Temporarily comment out to avoid the compiler warnings.
#if 0
static bool IsDisjoint(const vector<Field *> &l,
                       const vector<Field *> &r)
{
    for (vector<Field *>::const_iterator i = l.begin(); i != l.end(); ++i)
    {
        if (find(r.begin(), r.end(), *i) != r.end())
            return false;
    }
    return true;
}

// The internal utility method to check if two vectors have overlapped.
// If not, return true.
static bool IsDisjoint(vector<pair<Field *, string> > &l, const vector<Field *> &r)
{
    for (vector<pair<Field *, string> >::const_iterator i =
        l.begin(); i != l.end(); ++i) {
        if (find(r.begin(), r.end(), i->first) != r.end())
            return false;
    }
    return true;
}

// The internal utility method to check if vector s is a subset of vector b.
static bool IsSubset(const vector<Field *> &s, const vector<Field *> &b)
{
    for (vector<Field *>::const_iterator i = s.begin(); i != s.end(); ++i)
    {
        if (find(b.begin(), b.end(), *i) == b.end())
            return false;
    }
    return true;
}

// The internal utility method to check if vector s is a subset of vector b.
static bool IsSubset(vector<pair<Field *, string> > &s, const vector<Field *> &b)
{
    for (vector<pair<Field *, string> >::const_iterator i
         = s.begin(); i != s.end(); ++i) {
        if (find(b.begin(), b.end(), i->first) == b.end())
            return false;
    }
    return true;
}

#endif
// Destructor, release resources
SwathDataset::~SwathDataset()
{
    if (this->datasetid != -1) {
        SWdetach(this->datasetid);
    }

    for_each(this->dimmaps.begin(), this->dimmaps.end(), delete_elem());
    for_each(this->indexmaps.begin(), this->indexmaps.end(),
            delete_elem());

    for_each(this->geofields.begin(), this->geofields.end(),
            delete_elem());
    return; 

}

// Read all information of this swath
SwathDataset * SwathDataset::Read(int32 fd, const string &swathname)
    
{
    auto swath = new SwathDataset(swathname);
    if (swath == nullptr)
        throw1("Cannot allocate HDF5 Swath object");

    // Open this Swath object
    if ((swath->datasetid = SWattach(fd,
        const_cast<char *>(swathname.c_str())))
        == -1) {
        delete swath;
        throw2("attach swath", swathname);
    }

    try {

        // Read dimensions of this Swath
        swath->ReadDimensions(SWnentries, SWinqdims, swath->dims);
        
        // Read all information related to data fields of this Swath
        swath->ReadFields(SWnentries, SWinqdatafields, SWfieldinfo, 
                          SWgetfillvalue, false, swath->datafields);
        
        // Read all information related to geo-location fields of this Swath
        swath->ReadFields(SWnentries, SWinqgeofields, SWfieldinfo, 
                          SWgetfillvalue, true, swath->geofields);
        
        // Read all attributes of this Swath
        swath->ReadAttributes(SWinqattrs, SWattrinfo, SWreadattr, swath->attrs);
        
        // Read dimension map and save the number of dimension map for dim. subsetting
        swath->set_num_map(swath->ReadDimensionMaps(swath->dimmaps));

        // Read index maps, we haven't found any files with the Index Maps.
        swath->ReadIndexMaps(swath->indexmaps);
    }
    catch (...) {
        delete swath;
        throw;
    }
    //}

    return swath;
}

// Read dimension map info.
int SwathDataset::ReadDimensionMaps(vector<DimensionMap *> &swath_dimmaps)
    
{
    int32 nummaps;
    int32 bufsize;

    // Obtain number of dimension maps and the buffer size.
    if ((nummaps = SWnentries(this->datasetid, HDFE_NENTMAP, &bufsize)) == -1)
        throw2("dimmap entry", this->name);
        
    // Will use Split utility method to generate a list of dimension map.
    // An example of original representation of a swath dimension map:(d1/d2,
    // d3/d4,...) 
    // d1:the name of this dimension of the data field , d2: the name of the
    // corresponding dimension of the geo-location field
    // The list will be decomposed from (d1/d2,d3/d4,...) to {[d1,d2,0(offset),
    // 1(increment)],[d3,d4,0(offset),1(increment)],...}
        
    if (nummaps > 0) {
        vector<char> namelist;
        vector<int32> offset;
        vector<int32> increment;

        namelist.resize(bufsize + 1);
        offset.resize(nummaps);
        increment.resize(nummaps);
        if (SWinqmaps(this->datasetid, namelist.data(), offset.data(), increment.data())
            == -1)
            throw2("inquire dimmap", this->name);

        vector<string> mapnames;
        HDFCFUtil::Split(namelist.data(), bufsize, ',', mapnames);
        int count = 0;
        for (const auto& mapname:mapnames) {
            vector<string> parts;
            HDFCFUtil::Split(mapname.c_str(), '/', parts);
            if (parts.size() != 2) 
                throw3("dimmap part", parts.size(),
                        this->name);

            DimensionMap *dimmap = new DimensionMap(parts[0], parts[1],
                                                    offset[count],
                                                    increment[count]);
            swath_dimmaps.push_back(dimmap);
            ++count;
        }
    }
    return nummaps;
}

// The following function is nevered tested and probably will never be used.
void SwathDataset::ReadIndexMaps(vector<IndexMap *> &swath_indexmaps)
    
{
    int32 numindices;
    int32 bufsize;

    if ((numindices = SWnentries(this->datasetid, HDFE_NENTIMAP, &bufsize)) ==
        -1)
        throw2("indexmap entry", this->name);
    if (numindices > 0) {
        // TODO: I have never seen any EOS2 files that have index map.
        vector<char> namelist;

        namelist.resize(bufsize + 1);
        if (SWinqidxmaps(this->datasetid, namelist.data(), nullptr) == -1)
            throw2("inquire indexmap", this->name);

        vector<string> mapnames;
        HDFCFUtil::Split(namelist.data(), bufsize, ',', mapnames);
        for (const auto& mapname: mapnames) {
            auto indexmap = new IndexMap();
            vector<string> parts;
            HDFCFUtil::Split(mapname.c_str(), '/', parts);
            if (parts.size() != 2) 
                throw3("indexmap part", parts.size(),
                                          this->name);
            indexmap->geo = parts[0];
            indexmap->data = parts[1];

            {
                int32 dimsize;
                if ((dimsize =
                     SWdiminfo(this->datasetid,
                               const_cast<char *>(indexmap->geo.c_str())))
                    == -1)
                    throw3("dimension info", this->name, indexmap->geo);
                indexmap->indices.resize(dimsize);
                if (SWidxmapinfo(this->datasetid,
                                 const_cast<char *>(indexmap->geo.c_str()),
                                 const_cast<char *>(indexmap->data.c_str()),
                                 &indexmap->indices[0]) == -1)
                    throw4("indexmap info", this->name, indexmap->geo,
                           indexmap->data);
            }

            swath_indexmaps.push_back(indexmap);
        }
    }
}


PointDataset::~PointDataset()
{
}

PointDataset * PointDataset::Read(int32 /*fd*/, const string &pointname)
{
    return nullptr;
    // We support the point data via HDF4 APIs. So the following code
    // is not necessary. Comment out to avoid the memory leaking. KY 2024-04-22
#if 0
    auto point = new PointDataset(pointname);
    return point;
#endif
}


// Read name list from the EOS2 APIs and store names into a vector
bool Utility::ReadNamelist(const char *path,
                           int32 (*inq)(char *, char *, int32 *),
                           vector<string> &names)
{
    char *fname = const_cast<char *>(path);
    int32 bufsize;
    int numobjs;

    if ((numobjs = inq(fname, nullptr, &bufsize)) == -1) return false;
    if (numobjs > 0) {
        vector<char> buffer;
        buffer.resize(bufsize + 1);
        if (inq(fname, buffer.data(), &bufsize) == -1) return false;
        HDFCFUtil::Split(buffer.data(), bufsize, ',', names);
    }
    return true;
}
#endif


