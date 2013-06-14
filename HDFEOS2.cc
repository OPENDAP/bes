/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
//
//  Authors:   MuQun Yang <myang6@hdfgroup.org> Choonghwan Lee 
// Copyright (c) 2009 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifdef USE_HDFEOS2_LIB

//#include <BESDEBUG.h> // Should provide BESDebug info. later
#include <sstream>
#include <algorithm>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include<math.h>

#include "HDFCFUtil.h"
#include "HDFEOS2.h"

// Names to be searched by
//   get_geodim_x_name()
//   get_geodim_y_name()
//   get_latfield_name()
//   get_lonfield_name()
//   get_geogrid_name()

const char *HDFEOS2::File::_geodim_x_names[] = {"XDim", "LonDim","nlon"};
const char *HDFEOS2::File::_geodim_y_names[] = {"YDim", "LatDim","nlat"};
const char *HDFEOS2::File::_latfield_names[] = {
    "Latitude", "Lat","YDim", "LatCenter"
};
const char *HDFEOS2::File::_lonfield_names[] = {
    "Longitude", "Lon","XDim", "LonCenter"
};
const char *HDFEOS2::File::_geogrid_names[] = {"location"};

using namespace HDFEOS2;
using namespace std;

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

struct delete_elem
{
    template<typename T> void operator()(T *ptr)
    {
        delete ptr;
    }
};


File::~File()
{
    if(gridfd !=-1) {
        for (vector<GridDataset *>::const_iterator i = grids.begin();
             i != grids.end(); ++i){
            delete *i;
        }
        if((GDclose(gridfd))==-1) throw2("grid close",path);
    }

    if(swathfd !=-1) {
        for (vector<SwathDataset *>::const_iterator i = swaths.begin();
             i != swaths.end(); ++i){
            delete *i;
        }

        if((SWclose(swathfd))==-1) throw2("swath close",path);
    }
}

File * File::Read(const char *path) throw(Exception)
{
    File *file = new File(path);

    // Read information of all Grid objects in this file.
    if ((file->gridfd = GDopen(const_cast<char *>(file->path.c_str()),
                               DFACC_READ)) == -1) {
        delete file;
        throw2("grid open", path);
    }

    vector<string> gridlist;
    if (!Utility::ReadNamelist(file->path.c_str(), GDinqgrid, gridlist))
        throw2("grid namelist", path);
    for (vector<string>::const_iterator i = gridlist.begin();
         i != gridlist.end(); ++i)
        file->grids.push_back(GridDataset::Read(file->gridfd, *i));

    // Read information of all Swath objects in this file
    if ((file->swathfd = SWopen(const_cast<char *>(file->path.c_str()),
                                DFACC_READ)) == -1){
        delete file;
        throw2("swath open", path);
    }

    vector<string> swathlist;
    if (!Utility::ReadNamelist(file->path.c_str(), SWinqswath, swathlist)){
        delete file;
        throw2("swath namelist", path);
    }
    for (vector<string>::const_iterator i = swathlist.begin();
         i != swathlist.end(); ++i)
        file->swaths.push_back(SwathDataset::Read(file->swathfd, *i));

    // We only obtain the name list of point objects but not don't provide
    // other information of these objects.
    // The client will only get the name list of point objects.
    vector<string> pointlist;
    if (!Utility::ReadNamelist(file->path.c_str(), PTinqpoint, pointlist)){
        delete file;
        throw2("point namelist", path);
    }
    for (vector<string>::const_iterator i = pointlist.begin();
         i != pointlist.end(); ++i)
        file->points.push_back(PointDataset::Read(-1, *i));

    // If this is not an HDF-EOS2 file, returns exception as false.
    if(file->grids.size() == 0 && file->swaths.size() == 0 
       && file->points.size() == 0) {
        Exception e("Not an HDF-EOS2 file");
        e.setFileType(false);
        delete file;
        throw e;  
    }
    return file;
}


string File::get_geodim_x_name()
{
    if(!_geodim_x_name.empty())
        return _geodim_x_name;
    _find_geodim_names();
    return _geodim_x_name;
}

string File::get_geodim_y_name()
{
    if(!_geodim_y_name.empty())
        return _geodim_y_name;
    _find_geodim_names();
    return _geodim_y_name;
}

string File::get_latfield_name()
{
    if(!_latfield_name.empty())
        return _latfield_name;
    _find_latlonfield_names();
    return _latfield_name;
}

string File::get_lonfield_name()
{
    if(!_lonfield_name.empty())
        return _lonfield_name;
    _find_latlonfield_names();
    return _lonfield_name;
}

string File::get_geogrid_name()
{
    if(!_geogrid_name.empty())
        return _geogrid_name;
    _find_geogrid_name();
    return _geogrid_name;
}

void File::_find_geodim_names()
{
    set<string> geodim_x_name_set;
    for(size_t i = 0; i<sizeof(_geodim_x_names) / sizeof(const char *); i++)
        geodim_x_name_set.insert(_geodim_x_names[i]);

    set<string> geodim_y_name_set;
    for(size_t i = 0; i<sizeof(_geodim_y_names) / sizeof(const char *); i++)
        geodim_y_name_set.insert(_geodim_y_names[i]);

    const size_t gs = grids.size();
    const size_t ss = swaths.size();
    for(size_t i=0; ;i++)
    {
        Dataset *dataset=NULL;
        if(i<gs)
            dataset = static_cast<Dataset*>(grids[i]);
        else if(i < gs + ss)
            dataset = static_cast<Dataset*>(swaths[i-gs]);
        else
            break;

        const vector<Dimension *>& dims = dataset->getDimensions();
        for(vector<Dimension*>::const_iterator it = dims.begin();
            it != dims.end(); ++it)
        {
            if(geodim_x_name_set.find((*it)->getName()) != geodim_x_name_set.end())
                _geodim_x_name = (*it)->getName();
            else if(geodim_y_name_set.find((*it)->getName()) != geodim_y_name_set.end())
                _geodim_y_name = (*it)->getName();
        }
        // For performance, we're checking this for the first grid or swath
        break;
    }
    if(_geodim_x_name.empty())
        _geodim_x_name = _geodim_x_names[0];
    if(_geodim_y_name.empty())
        _geodim_y_name = _geodim_y_names[0];
}

void File::_find_latlonfield_names()
{
    set<string> latfield_name_set;
    for(size_t i = 0; i<sizeof(_latfield_names) / sizeof(const char *); i++)
        latfield_name_set.insert(_latfield_names[i]);

    set<string> lonfield_name_set;
    for(size_t i = 0; i<sizeof(_lonfield_names) / sizeof(const char *); i++)
        lonfield_name_set.insert(_lonfield_names[i]);

    const size_t gs = grids.size();
    const size_t ss = swaths.size();
    for(size_t i=0; ;i++)
    {
        Dataset *dataset = NULL;
        SwathDataset *sw = NULL;
        if(i<gs)
            dataset = static_cast<Dataset*>(grids[i]);
        else if(i < gs + ss)
        {
            sw = swaths[i-gs];
            dataset = static_cast<Dataset*>(sw);
        }
        else
            break;

        const vector<Field *>& fields = dataset->getDataFields();
        for(vector<Field*>::const_iterator it = fields.begin();
            it != fields.end(); ++it)
        {
            if(latfield_name_set.find((*it)->getName()) != latfield_name_set.end())
                _latfield_name = (*it)->getName();
            else if(lonfield_name_set.find((*it)->getName()) != lonfield_name_set.end())
                _lonfield_name = (*it)->getName();
        }

        if(sw)
        {
            const vector<Field *>& geofields = dataset->getDataFields();
            for(vector<Field*>::const_iterator it = geofields.begin();
                it != geofields.end(); ++it)
            {
                if(latfield_name_set.find((*it)->getName()) != latfield_name_set.end())
                    _latfield_name = (*it)->getName();
                else if(lonfield_name_set.find((*it)->getName()) != lonfield_name_set.end())
                    _lonfield_name = (*it)->getName();
            }
        }
        // For performance, we're checking this for the first grid or swath
        break;
    }
    if(_latfield_name.empty())
        _latfield_name = _latfield_names[0];
    if(_lonfield_name.empty())
        _lonfield_name = _lonfield_names[0];

}


void File::_find_geogrid_name()
{
    set<string> geogrid_name_set;
    for(size_t i = 0; i<sizeof(_geogrid_names) / sizeof(const char *); i++)
        geogrid_name_set.insert(_geogrid_names[i]);

    const size_t gs = grids.size();
    const size_t ss = swaths.size();
    for(size_t i=0; ;i++)
    {
        Dataset *dataset;
        if(i<gs)
            dataset = static_cast<Dataset*>(grids[i]);
        else if(i < gs + ss)
            dataset = static_cast<Dataset*>(swaths[i-gs]);
        else
            break;

        if(geogrid_name_set.find(dataset->getName()) != geogrid_name_set.end())
            _geogrid_name = dataset->getName();
    }
    if(_geogrid_name.empty())
        _geogrid_name = "location";
}

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
    for(vector<GridDataset *>::const_iterator i = this->grids.begin();
        i != this->grids.end(); ++i){

        // Loop through all fields
        for(vector<Field *>::const_iterator j =
            (*i)->getDataFields().begin();
            j != (*i)->getDataFields().end(); ++j) {
            if((*i)->getName()==GEOGRIDNAME){
                if((*j)->getName()==LATFIELDNAME){
                    onellcount++;
                    (*i)->latfield = *j;
                }
                if((*j)->getName()==LONFIELDNAME){ 
                    onellcount++;
                    (*i)->lonfield = *j;
                }
                if(onellcount == 2) 
                    break;//Finish this grid
            }
            else {// Here we assume that lat and lon are always in pairs.
                if(((*j)->getName()==LATFIELDNAME)||((*j)->getName()==LONFIELDNAME)){ 
                    (*i)->ownllflag = true;
                    morellcount++;
                    break;
                }
            }
        }
    }

    if(morellcount ==0 && onellcount ==2) 
        this->onelatlon = true; 
}

void File::handle_one_grid_zdim(GridDataset* gdset) {

    // 0. obtain "XDim","YDim"
    string DIMXNAME = this->get_geodim_x_name();
    string DIMYNAME = this->get_geodim_y_name();   

    // This is a big assumption, it may be wrong since not every 1-D field 
    // with the third dimension(name and size) is a coordinate
    // variable. We have to watch the products we've supported. KY 2012-6-13
    set<string> tempdimlist; // Unique 1-D field's dimension name list.
    pair<set<string>::iterator,bool> tempdimret;

    //map<string,string>tempdimcvarlist;//dimension name and the corresponding field name. 
    for (vector<Field *>::const_iterator j =
        gdset->getDataFields().begin();
        j != gdset->getDataFields().end(); ++j) {
        if ((*j)->getRank()==1){//We only need to search those 1-D fields

            // DIMXNAME and DIMYNAME correspond to latitude and longitude.
            // They should NOT be treated as dimension names missing fields. It will be handled differently.
            // Kent: The following implementation may not be always right. This essentially is the flaw of the 
            // data product if a file encounters this case.
            if(((*j)->getDimensions())[0]->getName()!=DIMXNAME && ((*j)->getDimensions())[0]->getName()!=DIMYNAME){
                tempdimret = tempdimlist.insert(((*j)->getDimensions())[0]->getName());

                // Only pick up the first 1-D field that the third-dimension 
                if(tempdimret.second == true) {
                    HDFCFUtil::insert_map(gdset->dimcvarlist, ((*j)->getDimensions())[0]->getName(),
                                          (*j)->getName());
                    (*j)->fieldtype = 3;
                    if((*j)->getName() == "Time") 
                        (*j)->fieldtype = 5;// IDV can handle 4-D fields when the 4th dim is Time.
                }
            }
        }
    } 

    // G2.2 Add the missing Z-dimension field.
    // Some dimension name's corresponding fields are missing, 
    // so add the missing Z-dimension fields based on the dimension names. When the real
    // data is read, nature number 1,2,3,.... will be filled!
    // NOTE: The latitude and longitude dim names are not handled yet.  

    // The above handling is also a big assumption. KY 2012-6-12
    // Loop through all dimensions of this grid.
    for (vector<Dimension *>::const_iterator j =
        gdset->getDimensions().begin(); j!= gdset->getDimensions().end();++j){

        if((*j)->getName()!=DIMXNAME && (*j)->getName()!=DIMYNAME){// Don't handle DIMXNAME and DIMYNAME yet.

            if((tempdimlist.find((*j)->getName())) == tempdimlist.end()){// This dimension needs a field
                      
                // Need to create a new data field vector element with the name and dimension as above.
                Field *missingfield = new Field();
                missingfield->name = (*j)->getName();
                missingfield->rank = 1;
                missingfield->type = DFNT_INT32;//This is an HDF constant.the data type is always integer.

                Dimension *dim = new Dimension((*j)->getName(),(*j)->getSize());

                // only 1 dimension
                missingfield->dims.push_back(dim);

                // Provide information for the missing data, since we need to calculate the data, so
                // the information is different than a normal field.
                int missingdatarank =1;
                int missingdatatypesize = 4;
                int missingdimsize[1];
                missingdimsize[0]= (*j)->getSize();
                missingfield->fieldtype = 4; //added Z-dimension coordinate variable with nature number

                // input data is empty now. We need to review this approach in the future.
                // The data will be retrieved in HDFEOS2ArrayMissGeoField.cc. KY 2013-06-14
                LightVector<char>inputdata;
                missingfield->data = new MissingFieldData(missingdatarank,missingdatatypesize,missingdimsize,inputdata);
                gdset->datafields.push_back(missingfield);
//                        HDFCFUtil::insert_map(tempdimcvarlist, (missingfield->getDimensions())[0]->getName(), 
//                                   missingfield->name);
                HDFCFUtil::insert_map(gdset->dimcvarlist, (missingfield->getDimensions())[0]->getName(), 
                                   missingfield->name);
 
            }
        }
    }

            //gdset->dimcvarlist = tempdimcvarlist;
            //tempdimlist.clear();// clear this temp. set.

}

void File::handle_one_grid_latlon(GridDataset* gdset) throw(Exception) 
{

    // 0. obtain "XDim","YDim","Latitude","Longitude" 
    string DIMXNAME = this->get_geodim_x_name();
    string DIMYNAME = this->get_geodim_y_name();

    string LATFIELDNAME = this->get_latfield_name();
    string LONFIELDNAME = this->get_lonfield_name(); 


    if(gdset->ownllflag) {// this grid has its own latitude/longitude
        // Searching the lat/lon field from the grid. 
        for (vector<Field *>::const_iterator j =
            gdset->getDataFields().begin();
            j != gdset->getDataFields().end(); ++j) {
 
            if((*j)->getName() == LATFIELDNAME) {

                // set the flag to tell if this is lat or lon. The unit will be different for lat and lon.
                (*j)->fieldtype = 1;

                // Latitude rank should not be greater than 2.
                // Here I don't check if the rank of latitude is the same as the longitude. 
                // Hopefully it never happens for HDF-EOS2 cases.
                // We are still investigating if Java clients work 
                // when the rank of latitude and longitude is greater than 2.
                if((*j)->getRank() > 2) 
                    throw3("The rank of latitude is greater than 2",gdset->getName(),(*j)->getName());

                if((*j)->getRank() != 1) {

                    // Obtain the major dim. For most cases, it is YDim Major. But some cases may be not. Still need to check.
                    (*j)->ydimmajor = gdset->getCalculated().isYDimMajor();

                    // If the 2-D lat/lon can be condensed to 1-D.
                    // For current HDF-EOS2 files, only GEO and CEA can be condensed. To gain performance,
                    // we don't check with real values.
                    int32 projectioncode = gdset->getProjection().getCode();
                    if(projectioncode == GCTP_GEO || projectioncode ==GCTP_CEA) {
                        (*j)->condenseddim = true;
                    }

                    // Now we want to handle the dim and the var lists.
                    // If the lat/lon can be condensed to 1-D array, COARD convention needs to be followed.
                    // Since COARD requires that the dimension name of lat/lon is the same as the field name of lat/lon,
                    // we still need to handle this case at last.
                    if((*j)->condenseddim) {// can be condensed to 1-D array.
                        // If it is YDim major, lat->YDim, lon->XDim;
                        // We don't need to adjust the dimension rank.
                        for (vector<Dimension *>::const_iterator k =
                            (*j)->getDimensions().begin(); k!= (*j)->getDimensions().end();++k){
                            if((*k)->getName() == DIMYNAME) {
                                HDFCFUtil::insert_map(gdset->dimcvarlist, (*k)->getName(), (*j)->getName());
                            }
                        }
                    }
                    else {// 2-D lat/lon case. Since dimension order doesn't matter, so we always assume lon->XDim, lat->YDim.
                        for (vector<Dimension *>::const_iterator k =
                            (*j)->getDimensions().begin(); k!= (*j)->getDimensions().end();++k){
                            if((*k)->getName() == DIMYNAME) {
                                HDFCFUtil::insert_map(gdset->dimcvarlist, (*k)->getName(), (*j)->getName());
                            }
                        }
                    }
                }
                else { // This is the 1-D case, just inserting  the dimension, field pair.
                    HDFCFUtil::insert_map(gdset->dimcvarlist, (((*j)->getDimensions())[0])->getName(), 
                                           (*j)->getName());
                }
            } 
            else if ((*j)->getName() == LONFIELDNAME) {

                // set the flag to tell if this is lat or lon. The unit will be different for lat and lon.
                (*j)->fieldtype = 2;

                // longitude rank should not be greater than 2.
                // Here I don't check if the rank of latitude and longitude is the same. Hopefully it never happens for HDF-EOS2 cases.
                // We are still investigating if Java clients work when the rank of latitude and longitude is greater than 2.
                if((*j)->getRank() >2) 
                    throw3("The rank of Longitude is greater than 2",gdset->getName(),(*j)->getName());

                if((*j)->getRank() != 1) {

                    // Obtain the major dim. For most cases, it is YDim Major. But some cases may be not. Still need to check.
                    (*j)->ydimmajor = gdset->getCalculated().isYDimMajor();

                    // If the 2-D lat/lon can be condensed to 1-D.
                    //For current HDF-EOS2 files, only GEO and CEA can be condensed. To gain performance,
                    // we don't check with real values.
                    int32 projectioncode = gdset->getProjection().getCode();
                    if(projectioncode == GCTP_GEO || projectioncode ==GCTP_CEA) {
                        (*j)->condenseddim = true;
                    }

                    // Now we want to handle the dim and the var lists.
                    // If the lat/lon can be condensed to 1-D array, COARD convention needs to be followed.
                    // Since COARD requires that the dimension name of lat/lon is the same as the field name of lat/lon,
                    // we still need to handle this case at last.
                    if((*j)->condenseddim) {// can be condensed to 1-D array.

                        //  lat->YDim, lon->XDim;
                        // We don't need to adjust the dimension rank.
                        for (vector<Dimension *>::const_iterator k =
                            (*j)->getDimensions().begin(); k!= (*j)->getDimensions().end();++k){
                            if((*k)->getName() == DIMXNAME) {
                                HDFCFUtil::insert_map(gdset->dimcvarlist, (*k)->getName(), (*j)->getName());
                            }
                        }
                    }
                    else {// 2-D lat/lon case. Since dimension order doesn't matter, so we always assume lon->XDim, lat->YDim.
                        for (vector<Dimension *>::const_iterator k =
                            (*j)->getDimensions().begin(); k!= (*j)->getDimensions().end();++k){
                            if((*k)->getName() == DIMXNAME) {
                                HDFCFUtil::insert_map(gdset->dimcvarlist, (*k)->getName(), (*j)->getName());
                            }
                        }
                    }
                }
                else { // This is the 1-D case, just inserting  the dimension, field pair.
                    HDFCFUtil::insert_map(gdset->dimcvarlist, (((*j)->getDimensions())[0])->getName(), 
                                          (*j)->getName());
                }
            }
        } 
    }
    else { // this grid's lat/lon has to be calculated.

        // Latitude and Longitude 
        Field *latfield = new Field();
        Field *lonfield = new Field();

        latfield->name = LATFIELDNAME;
        lonfield->name = LONFIELDNAME;

        latfield->rank = 2;// Assume it is a 2-D array
        lonfield->rank = 2;// Assume it is a 2-D array

        latfield->type = DFNT_FLOAT64;//This is an HDF constant.the data type is always float64.
        lonfield->type = DFNT_FLOAT64;//This is an HDF constant.the data type is always float64.

        latfield->fieldtype = 1;
        lonfield->fieldtype = 2;

        // Check if YDim is the major order. 
        // Obtain the major dim. For most cases, it is YDim Major. But some cases may be not. Still need to check.
        latfield->ydimmajor = gdset->getCalculated().isYDimMajor();
        lonfield->ydimmajor = latfield->ydimmajor;

        // Obtain XDim and YDim size.
        int xdimsize = gdset->getInfo().getX();
        int ydimsize = gdset->getInfo().getY();

        // Add dimensions. If it is YDim major,the dimension name list is "YDim XDim", otherwise, it is "XDim YDim". 
        // For LAMAZ projection, Y dimension is always supposed to be major for calculating lat/lon, but for dimension order, it should be consistent with data fields. (LD -2012/01/16
        bool dmajor=(gdset->getProjection().getCode()==GCTP_LAMAZ)? gdset->getCalculated().DetectFieldMajorDimension(): latfield->ydimmajor;
        //bool dmajor = latfield->ydimmajor;
        // latfield->ydimmajor(just keep the comment to remember the old code. 
        if(dmajor) { 
            Dimension *dimlaty = new Dimension(DIMYNAME,ydimsize);
            latfield->dims.push_back(dimlaty);
            Dimension *dimlony = new Dimension(DIMYNAME,ydimsize);
            lonfield->dims.push_back(dimlony);
            Dimension *dimlatx = new Dimension(DIMXNAME,xdimsize);
            latfield->dims.push_back(dimlatx);
            Dimension *dimlonx = new Dimension(DIMXNAME,xdimsize);
            lonfield->dims.push_back(dimlonx);
        }
        else {
            Dimension *dimlatx = new Dimension(DIMXNAME,xdimsize);
            latfield->dims.push_back(dimlatx);
            Dimension *dimlonx = new Dimension(DIMXNAME,xdimsize);
            lonfield->dims.push_back(dimlonx);
            Dimension *dimlaty = new Dimension(DIMYNAME,ydimsize);
            latfield->dims.push_back(dimlaty);
            Dimension *dimlony = new Dimension(DIMYNAME,ydimsize);
            lonfield->dims.push_back(dimlony);
        }

        latfield->data = NULL; 
        lonfield->data = NULL;

        // Obtain info upleft and lower right for special longitude.
        float64* upleft;
        float64* lowright;
        upleft = const_cast<float64 *>(gdset->getInfo().getUpLeft());
        lowright = const_cast<float64 *>(gdset->getInfo().getLowRight());

        // SOme special longitude is from 0 to 360.We need to check this case.
        int32 projectioncode = gdset->getProjection().getCode();
        if(((int)lowright[0]>180000000) && ((int)upleft[0]>-1)) {
            // We can only handle geographic projection now.
            // This is the only case we can handle.
            if(projectioncode == GCTP_GEO) // Will handle when data is read.
                lonfield->speciallon = true;
        }

        // Some MODIS MCD files don't follow standard format for lat/lon (DDDMMMSSS);
        // they simply represent lat/lon as -180.0000000 or -90.000000.
        // HDF-EOS2 library won't give the correct value based on these value.
        // These need to be remembered and resumed to the correct format when retrieving the data.
        // Since so far we haven't found region of satellite files is within 0.1666 degree(1 minute)
        // so, we divide the corner coordinate by 1000 and see if the integral part is 0.
        // If it is 0, we know this file uses special lat/lon coordinate.

        if(((int)(lowright[0]/1000)==0) &&((int)(upleft[0]/1000)==0) 
            && ((int)(upleft[1]/1000)==0) && ((int)(lowright[1]/1000)==0)) {
            if(projectioncode == GCTP_GEO){
                lonfield->specialformat = 1;
                latfield->specialformat = 1;
            }             
        }

        // Some TRMM CERES Grid Data have "default" to be set for the corner coordinate,
        // which they really mean for the whole globe(-180 - 180 lon and -90 - 90 lat). 
        // We will remember the information and change
        // those values when we read the lat and lon.

        if(((int)(lowright[0])==0) &&((int)(upleft[0])==0)
            && ((int)(upleft[1])==0) && ((int)(lowright[1])==0)) {
            if(projectioncode == GCTP_GEO){
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

        if(((int)(lowright[0])==-1) &&((int)(upleft[0])==-1)
            && ((int)(upleft[1])==-1) && ((int)(lowright[1])==-1)) {
            lonfield->specialformat = 3;
            latfield->specialformat = 3;
            lonfield->condenseddim = true;
            latfield->condenseddim = true;
        }


        if(GCTP_SOM == projectioncode) {
            lonfield->specialformat = 4;
            latfield->specialformat = 4;
        }

        // Check if the 2-D lat/lon can be condensed to 1-D.
        //For current HDF-EOS2 files, only GEO and CEA can be condensed. To gain performance,
        // we just check the projection code, don't check with real values.
        if(projectioncode == GCTP_GEO || projectioncode ==GCTP_CEA) {
            lonfield->condenseddim = true;
            latfield->condenseddim = true;
        }
        // Add latitude and longitude fields to the field list.
        gdset->datafields.push_back(latfield);
        gdset->datafields.push_back(lonfield);

        // Now we want to handle the dim and the var lists.
        // If the lat/lon can be condensed to 1-D array, COARD convention needs to be followed.
        // Since COARD requires that the dimension name of lat/lon is the same as the field name of lat/lon,
        // we still need to handle this case later.

        if(lonfield->condenseddim) {// can be condensed to 1-D array.
            //  lat->YDim, lon->XDim;
            // We don't need to adjust the dimension rank.
            for (vector<Dimension *>::const_iterator j =
                lonfield->getDimensions().begin(); j!= lonfield->getDimensions().end();++j){
                if((*j)->getName() == DIMXNAME) {
                    HDFCFUtil::insert_map(gdset->dimcvarlist, (*j)->getName(), lonfield->getName());
                }

                if((*j)->getName() == DIMYNAME) {
                    HDFCFUtil::insert_map(gdset->dimcvarlist, (*j)->getName(), latfield->getName());
                }
            }
        }
        else {// 2-D lat/lon case. Since dimension order doesn't matter, so we always assume lon->XDim, lat->YDim.
            for (vector<Dimension *>::const_iterator j =
                lonfield->getDimensions().begin(); j!= lonfield->getDimensions().end();++j){

                if((*j)->getName() == DIMXNAME){ 
                    HDFCFUtil::insert_map(gdset->dimcvarlist, (*j)->getName(), lonfield->getName());
                }

                if((*j)->getName() == DIMYNAME){
                    HDFCFUtil::insert_map(gdset->dimcvarlist, (*j)->getName(), latfield->getName());
                }
            }
        }
    }

} 

void File::handle_onelatlon_grids() throw(Exception) {

    // 0. obtain "XDim","YDim","Latitude","Longitude" and "location" set.
    string DIMXNAME = this->get_geodim_x_name();       
    string DIMYNAME = this->get_geodim_y_name();       
    string LATFIELDNAME = this->get_latfield_name();       
    string LONFIELDNAME = this->get_lonfield_name();       


    // Now only there is only one geo grid name "location", so don't call it know.
    // string GEOGRIDNAME = this->get_geogrid_name();
    string GEOGRIDNAME = "location";

    //Dimension name and the corresponding field name when only one lat/lon is used for all grids.
    map<string,string>temponelatlondimcvarlist;

    for (vector<GridDataset *>::const_iterator i = this->grids.begin();
        i != this->grids.end(); ++i){

        // G2.0. Set the horizontal dimension name "dimxname" and "dimyname"
        // G2.0. This will be used to detect the dimension major order.
        (*i)->setDimxName(DIMXNAME);
        (*i)->setDimyName(DIMYNAME);

        // Handle lat/lon. Note that other grids need to point to this lat/lon.
        if((*i)->getName()==GEOGRIDNAME) {

            // Figure out dimension order,2D or 1D for lat/lon
            // if lat/lon field's pointed value is changed, the value of the lat/lon field is also changed.
            // set the flag to tell if this is lat or lon. The unit will be different for lat and lon.
            (*i)->lonfield->fieldtype = 2;
            (*i)->latfield->fieldtype = 1;

            // latitude and longitude rank must be equal and should not be greater than 2.
            if((*i)->lonfield->rank >2 || (*i)->latfield->rank >2) 
                throw2("Either the rank of lat or the lon is greater than 2",(*i)->getName());
            if((*i)->lonfield->rank !=(*i)->latfield->rank) 
                throw2("The rank of the latitude is not the same as the rank of the longitude",(*i)->getName());

            if((*i)->lonfield->rank !=1) {// 2-D lat/lon arrays

                // obtain the major dim. For most cases, it is YDim Major. 
                //But for some cases it is not. So still need to check.
                (*i)->lonfield->ydimmajor = (*i)->getCalculated().isYDimMajor();
                (*i)->latfield->ydimmajor = (*i)->lonfield->ydimmajor;

                // Check if the 2-D lat/lon can be condensed to 1-D. 
                //For current HDF-EOS2 files, only GEO and CEA can be condensed. To gain performance,
                // we just check the projection code, don't check with real values.
                int32 projectioncode = (*i)->getProjection().getCode();
                if(projectioncode == GCTP_GEO || projectioncode ==GCTP_CEA) {
                    (*i)->lonfield->condenseddim = true;
                    (*i)->latfield->condenseddim = true;
                }

                // Now we want to handle the dim and the var lists.  
                // If the lat/lon can be condensed to 1-D array, COARD convention needs to be followed. 
                // Since COARD requires that the dimension name of lat/lon is the same as the field name of lat/lon,
                // we still need to handle this case later. Now we do the first step.  
                if((*i)->lonfield->condenseddim) {
                    // can be condensed to 1-D array.
                    //regardless of YDim major or XDim major,,always lat->YDim, lon->XDim; 
                    // We still need to remember the dimension major when we calculate the data.
                    // We don't need to adjust the dimension rank.
                    for (vector<Dimension *>::const_iterator j =
                        (*i)->lonfield->getDimensions().begin(); j!= (*i)->lonfield->getDimensions().end();++j){
                        if((*j)->getName() == DIMXNAME) {
                            HDFCFUtil::insert_map((*i)->dimcvarlist, (*j)->getName(), 
                                                  (*i)->lonfield->getName());
                        }
                        if((*j)->getName() == DIMYNAME) {
                            HDFCFUtil::insert_map((*i)->dimcvarlist, (*j)->getName(), 
                                                  (*i)->latfield->getName());
                        }
                    }
                }
                else {// 2-D lat/lon case. Since dimension order doesn't matter, so we always assume lon->XDim, lat->YDim.
                    for (vector<Dimension *>::const_iterator j =
                        (*i)->lonfield->getDimensions().begin(); j!= (*i)->lonfield->getDimensions().end();++j){
                        if((*j)->getName() == DIMXNAME) {
                            HDFCFUtil::insert_map((*i)->dimcvarlist, (*j)->getName(), 
                                                  (*i)->lonfield->getName());
                        }
                        if((*j)->getName() == DIMYNAME) {
                            HDFCFUtil::insert_map((*i)->dimcvarlist, (*j)->getName(), 
                                                  (*i)->latfield->getName());
                        }
                    }
                }
              
            }
            else { // This is the 1-D case, just inserting the dimension, field pair.
                HDFCFUtil::insert_map((*i)->dimcvarlist, ((*i)->lonfield->getDimensions())[0]->getName(),
                                   (*i)->lonfield->getName());
                HDFCFUtil::insert_map((*i)->dimcvarlist, ((*i)->latfield->getDimensions())[0]->getName(),
                                   (*i)->latfield->getName());
            }              
            temponelatlondimcvarlist = (*i)->dimcvarlist;
            break;

        }

    }

    for(vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i){

        string templatlonname1;
        string templatlonname2;

        if((*i)->getName() != GEOGRIDNAME) {

            map<string,string>::iterator tempmapit;

            // Find DIMXNAME field
            tempmapit = temponelatlondimcvarlist.find(DIMXNAME);
            if(tempmapit != temponelatlondimcvarlist.end()) 
                templatlonname1= tempmapit->second;
            else 
                throw2("cannot find the dimension field of XDim", (*i)->getName());

            HDFCFUtil::insert_map((*i)->dimcvarlist, DIMXNAME, templatlonname1);

            // Find DIMYNAME field
            tempmapit = temponelatlondimcvarlist.find(DIMYNAME);
            if(tempmapit != temponelatlondimcvarlist.end()) 
                templatlonname2= tempmapit->second;
            else
                throw2("cannot find the dimension field of YDim", (*i)->getName());
            HDFCFUtil::insert_map((*i)->dimcvarlist, DIMYNAME, templatlonname2);
        }
    }

}
//#endif

void File::handle_grid_dim_cvar_maps() throw(Exception) {

    // 0. obtain "XDim","YDim","Latitude","Longitude" and "location" set.
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

    // G3.5 Handle name clashings
    // G3.5.1 build up a temp. name list
    // Note here: we don't include grid and swath names(simply (*j)->name) due to the products we observe
    // Adding the grid/swath names makes the names artificially long. Will check user's feedback
    // and may change them later. KY 2012-6-25
    vector <string> tempfieldnamelist;
    for (vector<GridDataset *>::const_iterator i = this->grids.begin();
        i != this->grids.end(); ++i) {
        for (vector<Field *>::const_iterator j = (*i)->getDataFields().begin();
            j!= (*i)->getDataFields().end(); ++j) { 
            tempfieldnamelist.push_back(HDFCFUtil::get_CF_string((*j)->name));
        }
    }
    HDFCFUtil::Handle_NameClashing(tempfieldnamelist);

    // G4. Create a map for dimension field name <original field name, corrected field name>
    // Also assure the uniqueness of all field names,save the new field names.

    //the original dimension field name to the corrected dimension field name
    map<string,string>tempncvarnamelist;
    string tempcorrectedlatname, tempcorrectedlonname;
      
    int total_fcounter = 0;

    for (vector<GridDataset *>::const_iterator i = this->grids.begin();
        i != this->grids.end(); ++i){

        // Here we can't use getDataFields call since for no lat/lon fields 
        // are created for one global lat/lon case. We have to use the dimcvarnamelist 
        // map we just created.
        for (vector<Field *>::const_iterator j = 
            (*i)->getDataFields().begin();
            j != (*i)->getDataFields().end(); ++j) 
        {
            (*j)->newname = tempfieldnamelist[total_fcounter];
            total_fcounter++;  
               
            // If this field is a dimension field, save the name/new name pair. 
            if((*j)->fieldtype!=0) {

                tempncvarnamelist.insert(make_pair((*j)->getName(), (*j)->newname));

                // For one latlon case, remember the corrected latitude and longitude field names.
                if((this->onelatlon)&&(((*i)->getName())==GEOGRIDNAME)) {
                    if((*j)->getName()==LATFIELDNAME) 
                        tempcorrectedlatname = (*j)->newname;
                    if((*j)->getName()==LONFIELDNAME) 
                        tempcorrectedlonname = (*j)->newname;
                }
            }
        }

        (*i)->ncvarnamelist = tempncvarnamelist;
        tempncvarnamelist.clear();
    }

    // for one lat/lon case, we have to add the lat/lon field name to other grids.
    // We know the original lat and lon names. So just retrieve the corrected lat/lon names from
    // the geo grid(GEOGRIDNAME).
    if(this->onelatlon) {
        for(vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i){
            // Lat/lon names must be in this group.
            if((*i)->getName()!=GEOGRIDNAME){
                HDFCFUtil::insert_map((*i)->ncvarnamelist, LATFIELDNAME, tempcorrectedlatname);
                HDFCFUtil::insert_map((*i)->ncvarnamelist, LONFIELDNAME, tempcorrectedlonname);
            }
        }
    }

    // G5. Create a map for dimension name < original dimension name, corrected dimension name>
    map<string,string>tempndimnamelist;//the original dimension name to the corrected dimension name

    //// ***** Handling the dimension name clashing ******************
    vector <string>tempalldimnamelist;
    for (vector<GridDataset *>::const_iterator i = this->grids.begin();
        i != this->grids.end(); ++i)
        for (map<string,string>::const_iterator j =
            (*i)->dimcvarlist.begin(); j!= (*i)->dimcvarlist.end();++j)
            tempalldimnamelist.push_back(HDFCFUtil::get_CF_string((*j).first));

    HDFCFUtil::Handle_NameClashing(tempalldimnamelist);
      
    // Since DIMXNAME and DIMYNAME are not in the original dimension name list, we use the dimension name,field map 
    // we just formed. 

    int total_dcounter = 0;
    for (vector<GridDataset *>::const_iterator i = this->grids.begin();
        i != this->grids.end(); ++i){

        for (map<string,string>::const_iterator j =
            (*i)->dimcvarlist.begin(); j!= (*i)->dimcvarlist.end();++j){

            // We have to handle DIMXNAME and DIMYNAME separately.
            if((DIMXNAME == (*j).first || DIMYNAME == (*j).first) && (true==(this->onelatlon))) 
                HDFCFUtil::insert_map(tempndimnamelist, (*j).first,(*j).first);
            else
                HDFCFUtil::insert_map(tempndimnamelist, (*j).first, tempalldimnamelist[total_dcounter]);
            total_dcounter++;
        }

        (*i)->ndimnamelist = tempndimnamelist;
        tempndimnamelist.clear();   
    }
}

void File::handle_grid_coards() throw(Exception) {

    // 0. obtain "XDim","YDim","Latitude","Longitude" and "location" set.
    string DIMXNAME = this->get_geodim_x_name();       
    string DIMYNAME = this->get_geodim_y_name();       
    string LATFIELDNAME = this->get_latfield_name();       
    string LONFIELDNAME = this->get_lonfield_name();       

    // Now only there is only one geo grid name "location", so don't call it know.
    // string GEOGRIDNAME = this->get_geogrid_name();
    string GEOGRIDNAME = "location";


    // G6. Revisit the lat/lon fields to check if 1-D COARD convention needs to be followed.
    vector<Dimension*> correcteddims;
    string tempcorrecteddimname;
    map<string,string> tempnewxdimnamelist;
    map<string,string> tempnewydimnamelist;
     
    Dimension *correcteddim;
     
    for (vector<GridDataset *>::const_iterator i = this->grids.begin();
        i != this->grids.end(); ++i){
        for (vector<Field *>::const_iterator j =
            (*i)->getDataFields().begin();
            j != (*i)->getDataFields().end(); ++j) {

            // Now handling COARD cases, since latitude/longitude can be either 1-D or 2-D array. 
            // So we need to correct both cases.
            // 2-D lat to 1-D COARD lat
            if((*j)->getName()==LATFIELDNAME && (*j)->getRank()==2 &&(*j)->condenseddim) {

                string templatdimname;
                map<string,string>::iterator tempmapit;

                // Find the new name of LATFIELDNAME
                tempmapit = (*i)->ncvarnamelist.find(LATFIELDNAME);
                if(tempmapit != (*i)->ncvarnamelist.end()) 
                    templatdimname= tempmapit->second;
                else 
                    throw2("cannot find the corrected field of Latitude", (*i)->getName());

                for(vector<Dimension *>::const_iterator k =(*j)->getDimensions().begin();
                    k!=(*j)->getDimensions().end();++k){

                    // This needs to be latitude
                    if((*k)->getName()==DIMYNAME) {
                        correcteddim = new Dimension(templatdimname,(*k)->getSize());
                        correcteddims.push_back(correcteddim);
                        (*j)->setCorrectedDimensions(correcteddims);
                        HDFCFUtil::insert_map(tempnewydimnamelist, (*i)->getName(), templatdimname);
                    }
                    }
                    (*j)->iscoard = true;
                    (*i)->iscoard = true;
                    if(this->onelatlon) 
                        this->iscoard = true;

                    correcteddims.clear();//clear the local temporary vector
            }

            // 2-D lon to 1-D COARD lat
            else if((*j)->getName()==LONFIELDNAME && (*j)->getRank()==2 &&(*j)->condenseddim){
               
                string templondimname;
                map<string,string>::iterator tempmapit;

                // Find the new name of LONFIELDNAME
                tempmapit = (*i)->ncvarnamelist.find(LONFIELDNAME);
                if(tempmapit != (*i)->ncvarnamelist.end()) 
                    templondimname= tempmapit->second;
                else 
                    throw2("cannot find the corrected field of Longitude", (*i)->getName());

                for(vector<Dimension *>::const_iterator k =(*j)->getDimensions().begin();
                    k!=(*j)->getDimensions().end();++k){

                    if((*k)->getName()==DIMXNAME) {// This needs to be longitude
                        correcteddim = new Dimension(templondimname,(*k)->getSize());
                        correcteddims.push_back(correcteddim);
                        (*j)->setCorrectedDimensions(correcteddims);
                        HDFCFUtil::insert_map(tempnewxdimnamelist, (*i)->getName(), templondimname);
                    }
                }

                (*j)->iscoard = true;
                (*i)->iscoard = true;
                if(this->onelatlon) 
                    this->iscoard = true;
                correcteddims.clear();
            }
            else if(((*j)->getRank()==1) &&((*j)->getName()==LONFIELDNAME) ) {// 1-D lon to 1-D COARD lon

                string templondimname;
                map<string,string>::iterator tempmapit;

                // Find the new name of LONFIELDNAME
                tempmapit = (*i)->ncvarnamelist.find(LONFIELDNAME);
                if(tempmapit != (*i)->ncvarnamelist.end()) 
                    templondimname= tempmapit->second;
                else 
                    throw2("cannot find the corrected field of Longitude", (*i)->getName());

                correcteddim = new Dimension(templondimname,((*j)->getDimensions())[0]->getSize());
                correcteddims.push_back(correcteddim);
                (*j)->setCorrectedDimensions(correcteddims);
                (*j)->iscoard = true;
                (*i)->iscoard = true;
                if(this->onelatlon) 
                    this->iscoard = true; 
                correcteddims.clear();

                if(((((*j)->getDimensions())[0]->getName()!=DIMXNAME))
                    &&((((*j)->getDimensions())[0]->getName())!=DIMYNAME)){
                    throw3("the dimension name of longitude should not be ",
                           ((*j)->getDimensions())[0]->getName(),(*i)->getName()); 
                }
                if((((*j)->getDimensions())[0]->getName())==DIMXNAME) {
                    HDFCFUtil::insert_map(tempnewxdimnamelist, (*i)->getName(), templondimname);
                }
                else {
                    HDFCFUtil::insert_map(tempnewydimnamelist, (*i)->getName(), templondimname);
                }
            }
            else if(((*j)->getRank()==1) &&((*j)->getName()==LATFIELDNAME) ) {// 1-D lon to 1-D  COARD lon

                string templatdimname;
                map<string,string>::iterator tempmapit;

                // Find the new name of LATFIELDNAME
                tempmapit = (*i)->ncvarnamelist.find(LATFIELDNAME);
                if(tempmapit != (*i)->ncvarnamelist.end()) 
                    templatdimname= tempmapit->second;
                else 
                    throw2("cannot find the corrected field of Latitude", (*i)->getName());

                correcteddim = new Dimension(templatdimname,((*j)->getDimensions())[0]->getSize());
                correcteddims.push_back(correcteddim);
                (*j)->setCorrectedDimensions(correcteddims);
              
                (*j)->iscoard = true;
                (*i)->iscoard = true;
                if(this->onelatlon) 
                    this->iscoard = true;
                correcteddims.clear();

                if(((((*j)->getDimensions())[0]->getName())!=DIMXNAME)
                    &&((((*j)->getDimensions())[0]->getName())!=DIMYNAME))
                    throw3("the dimension name of latitude should not be ",
                           ((*j)->getDimensions())[0]->getName(),(*i)->getName());
                if((((*j)->getDimensions())[0]->getName())==DIMXNAME){
                    HDFCFUtil::insert_map(tempnewxdimnamelist, (*i)->getName(), templatdimname);
                }
                else {
                    HDFCFUtil::insert_map(tempnewydimnamelist, (*i)->getName(), templatdimname);
                }
            }
            else {
                ;
            }
        }
    }
      
    // G7. If COARDS follows, apply the new DIMXNAME and DIMYNAME name to the  ndimnamelist 
    if(this->onelatlon){ //One lat/lon for all grids .
        if(this->iscoard){ // COARDS is followed.
            // For this case, only one pair of corrected XDim and YDim for all grids.
            string tempcorrectedxdimname;
            string tempcorrectedydimname;

            if((int)(tempnewxdimnamelist.size())!= 1) 
                throw1("the corrected dimension name should have only one pair");
            if((int)(tempnewydimnamelist.size())!= 1) 
                throw1("the corrected dimension name should have only one pair");

            map<string,string>::iterator tempdimmapit = tempnewxdimnamelist.begin();
            tempcorrectedxdimname = tempdimmapit->second;      
            tempdimmapit = tempnewydimnamelist.begin();
            tempcorrectedydimname = tempdimmapit->second;
       
            for (vector<GridDataset *>::const_iterator i = this->grids.begin();
                i != this->grids.end(); ++i){

                // Find the DIMXNAME and DIMYNAME in the dimension name list.  
                map<string,string>::iterator tempmapit;
                tempmapit = (*i)->ndimnamelist.find(DIMXNAME);
                if(tempmapit != (*i)->ndimnamelist.end()) {
                    HDFCFUtil::insert_map((*i)->ndimnamelist, DIMXNAME, tempcorrectedxdimname);
                }
                else 
                    throw2("cannot find the corrected dimension name", (*i)->getName());
                tempmapit = (*i)->ndimnamelist.find(DIMYNAME);
                if(tempmapit != (*i)->ndimnamelist.end()) {
                    HDFCFUtil::insert_map((*i)->ndimnamelist, DIMYNAME, tempcorrectedydimname);
                }
                else 
                    throw2("cannot find the corrected dimension name", (*i)->getName());
            }
        }
    }
    else {// We have to search each grid
        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i){
            if((*i)->iscoard){

                string tempcorrectedxdimname;
                string tempcorrectedydimname;

                // Find the DIMXNAME and DIMYNAME in the dimension name list.
                map<string,string>::iterator tempdimmapit;
                map<string,string>::iterator tempmapit;
                tempdimmapit = tempnewxdimnamelist.find((*i)->getName());
                if(tempdimmapit != tempnewxdimnamelist.end()) 
                    tempcorrectedxdimname = tempdimmapit->second;
                else 
                    throw2("cannot find the corrected COARD XDim dimension name", (*i)->getName());
                tempmapit = (*i)->ndimnamelist.find(DIMXNAME);
                if(tempmapit != (*i)->ndimnamelist.end()) {
                    HDFCFUtil::insert_map((*i)->ndimnamelist, DIMXNAME, tempcorrectedxdimname);
                }
                else 
                    throw2("cannot find the corrected dimension name", (*i)->getName());

                tempdimmapit = tempnewydimnamelist.find((*i)->getName());
                if(tempdimmapit != tempnewydimnamelist.end()) 
                    tempcorrectedydimname = tempdimmapit->second;
                else 
                    throw2("cannot find the corrected COARD YDim dimension name", (*i)->getName());

                tempmapit = (*i)->ndimnamelist.find(DIMYNAME);
                if(tempmapit != (*i)->ndimnamelist.end()) {
                    HDFCFUtil::insert_map((*i)->ndimnamelist, DIMYNAME, tempcorrectedydimname);
                }
                else 
                    throw2("cannot find the corrected dimension name", (*i)->getName());
            }
        }
    }

      
    //G8. For 1-D lat/lon cases, Make the third (other than lat/lon coordinate variable) dimension to follow COARD conventions. 

    for (vector<GridDataset *>::const_iterator i = this->grids.begin();
        i != this->grids.end(); ++i){
        for (map<string,string>::const_iterator j =
            (*i)->dimcvarlist.begin(); j!= (*i)->dimcvarlist.end();++j){

            // It seems that the condition for onelatlon case is if(this->iscoard) is true instead if
            // this->onelatlon is true.So change it. KY 2010-7-4
            //if((this->onelatlon||(*i)->iscoard) && (*j).first !=DIMXNAME && (*j).first !=DIMYNAME) 
            if((this->iscoard||(*i)->iscoard) && (*j).first !=DIMXNAME && (*j).first !=DIMYNAME) {
                string tempnewdimname;
                map<string,string>::iterator tempmapit;

                // Find the new field name of the corresponding dimennsion name 
                tempmapit = (*i)->ncvarnamelist.find((*j).second);
                if(tempmapit != (*i)->ncvarnamelist.end()) 
                    tempnewdimname= tempmapit->second;
                else 
                    throw3("cannot find the corrected field of ", (*j).second,(*i)->getName());

                // Make the new field name to the correponding dimension name 
                tempmapit =(*i)->ndimnamelist.find((*j).first);
                if(tempmapit != (*i)->ndimnamelist.end()) 
                    HDFCFUtil::insert_map((*i)->ndimnamelist, (*j).first, tempnewdimname);
                else 
                    throw3("cannot find the corrected dimension name of ", (*j).first,(*i)->getName());

            }
        }
    }
    // The following code may be usedful for future more robust third-dimension support
    //  G9. Create the corrected dimension vectors.
    for (vector<GridDataset *>::const_iterator i = this->grids.begin();
        i != this->grids.end(); ++i){ 

        for (vector<Field *>::const_iterator j =
            (*i)->getDataFields().begin();
            j != (*i)->getDataFields().end(); ++j) {

            if((*j)->iscoard == false) {// the corrected dimension name of lat/lon have been updated.

                // Just obtain the corrected dim names  and save the corrected dimensions for each field.
                for(vector<Dimension *>::const_iterator k=(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){

                    //tempcorrecteddimname =(*i)->ndimnamelist((*k)->getName());
                    map<string,string>::iterator tempmapit;

                    // Find the new name of this field
                    tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                    if(tempmapit != (*i)->ndimnamelist.end()) 
                        tempcorrecteddimname= tempmapit->second;
                    else 
                        throw4("cannot find the corrected dimension name", (*i)->getName(),(*j)->getName(),(*k)->getName());
                    correcteddim = new Dimension(tempcorrecteddimname,(*k)->getSize());
                    correcteddims.push_back(correcteddim);
                }
                (*j)->setCorrectedDimensions(correcteddims);
                correcteddims.clear();
            }
        }
    }
}

//STOP
void File::handle_grid_cf_attrs() throw(Exception) {

        // G10. Create "coordinates" ,"units"  attributes. The "units" attributes only apply to latitude and longitude.
        // This is the last round of looping through everything, 
        // we will match dimension name list to the corresponding dimension field name 
        // list for every field. 
        // Since we find some swath files don't specify fillvalue when -9999.0 is found in the real data,
        // we specify fillvalue for those fields. This is not in this routine. This is entirely artifical and we will evaluate this approach. KY 2010-3-3
           
        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i){
            for (vector<Field *>::const_iterator j =
                (*i)->getDataFields().begin();
                j != (*i)->getDataFields().end(); ++j) {
                 
                // Real fields: adding coordinate attributesinate attributes
                if((*j)->fieldtype == 0)  {
                    string tempcoordinates="";
                    string tempfieldname="";
                    string tempcorrectedfieldname="";
                    int tempcount = 0;
                    for(vector<Dimension *>::const_iterator k=(*j)->getDimensions().begin();
                        k!=(*j)->getDimensions().end();++k){

                        // handle coordinates attributes
                        map<string,string>::iterator tempmapit;
                        map<string,string>::iterator tempmapit2;
              
                        // Find the dimension field name
                        tempmapit = ((*i)->dimcvarlist).find((*k)->getName());
                        if(tempmapit != ((*i)->dimcvarlist).end()) 
                            tempfieldname = tempmapit->second;
                        else 
                            throw4("cannot find the dimension field name",
                                   (*i)->getName(),(*j)->getName(),(*k)->getName());

                        // Find the corrected dimension field name
                        tempmapit2 = ((*i)->ncvarnamelist).find(tempfieldname);
                        if(tempmapit2 != ((*i)->ncvarnamelist).end()) 
                            tempcorrectedfieldname = tempmapit2->second;
                        else 
                            throw4("cannot find the corrected dimension field name",
                                   (*i)->getName(),(*j)->getName(),(*k)->getName());

                        if(tempcount == 0) 
                            tempcoordinates= tempcorrectedfieldname;
                        else 
                            tempcoordinates = tempcoordinates +" "+tempcorrectedfieldname;
                        tempcount++;
                    }
                    (*j)->setCoordinates(tempcoordinates);
                }

                // Add units for latitude and longitude
                if((*j)->fieldtype == 1) {// latitude,adding the "units" attribute  degrees_east.
                    string tempunits = "degrees_north";
                    (*j)->setUnits(tempunits);
                }
                if((*j)->fieldtype == 2) { // longitude, adding the units of 
                    string tempunits = "degrees_east";
                    (*j)->setUnits(tempunits);
                }

                // Add units for Z-dimension, now it is always "level"
                // This also needs to be corrected since the Z-dimension may not always be "level".
                // KY 2012-6-13
                // We decide not to touch "units" when the Z-dimension is an existing field(fieldtype =3).
  
                if(((*j)->fieldtype == 4)) {
                    string tempunits ="level";
                    (*j)->setUnits(tempunits);
                }

            
                // The units of the time may not be right. KY 2012-6-13
                if(((*j)->fieldtype == 5)) {
                    string tempunits ="days since 1900-01-01 00:00:00";
                    (*j)->setUnits(tempunits);
                }

                // We meet a really special case for CERES TRMM data. We attribute it to the specialformat 2 case
                // since the corner coordinate is set to default in HDF-EOS2 structmetadata. We also find that there are
                // values such as 3.4028235E38 that is the maximum single precision floating point value. This value
                // is a fill value but the fillvalue attribute is not set. So we add the fillvalue attribute for this case.
                // We may find such cases for other products and will tackle them also.
                if (true == (*i)->addfvalueattr) {
                    if((((*j)->getFillValue()).empty()) && ((*j)->getType()==DFNT_FLOAT32 )) {
                        float tempfillvalue = HUGE;
                        (*j)->addFillValue(tempfillvalue);
                        (*j)->setAddedFillValue(true);
                    }
                }
            }
        }




}

void File::handle_grid_SOM_projection() throw(Exception) {

        // G11: Special handling SOM projection
        // Since the latitude and longitude of the SOM projection are 3-D. 
        // Based on our current understanding, the third dimension size is always 180. 
        // If the size is not 180, the latitude and longitude will not be calculated correctly.
        // This is according to the MISR Lat/lon calculation document 
        // at http://eosweb.larc.nasa.gov/PRODOCS/misr/DPS/DPS_v50_RevS.pdf
        // KY 2012-6-12

        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i){
            if (GCTP_SOM == (*i)->getProjection().getCode()) {
                
                // 0. Getting the SOM dimension for latitude and longitude.
                string som_dimname;
                for(vector<Dimension *>::const_iterator j=(*i)->getDimensions().begin();
                    j!=(*i)->getDimensions().end();++j){
                    // NBLOCK is from misrproj.h. It is the number of block that MISR team support for the SOM projection.
                    if(NBLOCK == (*j)->getSize()) {
                        // To make sure we catch the right dimension, check the first three characters of the dim. name
                        // It should be SOM
                        
                        if ((*j)->getName().compare(0,3,"SOM") == 0) {
                            som_dimname = (*j)->getName();
                            break;
                        }
                    }
                }

                if(""== som_dimname) 
                    throw4("Wrong number of block: The number of block of MISR SOM Grid ",
                            (*i)->getName()," is not ",NBLOCK);

                map<string,string>::iterator tempmapit;

                // Find the new name of this field
                string cor_som_dimname;
                string cor_som_cvname;
                tempmapit = (*i)->ndimnamelist.find(som_dimname);
                if(tempmapit != (*i)->ndimnamelist.end()) 
                    cor_som_dimname = tempmapit->second;
                else 
                    throw2("cannot find the corrected dimension name for ", som_dimname);


                // Here we cannot use getDataFields() since the returned elements cannot be modified. KY 2012-6-12
                for (vector<Field *>::iterator j = (*i)->datafields.begin();
                    j != (*i)->datafields.end(); ++j) {
                    
                    // Only 6-7 fields, so just loop through 
                    // 1. Set the SOM dimension for latitude and longitude
                    if (1 == (*j)->fieldtype || 2 == (*j)->fieldtype) {
                        
                        Dimension *newdim = new Dimension(som_dimname,NBLOCK);
                        Dimension *newcor_dim = new Dimension(cor_som_dimname,NBLOCK);
                        vector<Dimension *>::iterator it_d;

                        it_d = (*j)->dims.begin();
                        (*j)->dims.insert(it_d,newdim);

                        it_d = (*j)->correcteddims.begin();
                        (*j)->correcteddims.insert(it_d,newcor_dim);

                    } 

                    // 2. Remove the added coordinate variable for the SOM dimension
                    // The added variable is a variable with the nature number
                    if ( 4 == (*j)->fieldtype) {
                        cor_som_cvname = (*j)->newname;
                        delete (*j);
                        (*i)->datafields.erase(j);
                        // When erasing the iterator, the iterator will automatically go to the next element, so we need to go back 1 in order not to miss the next element.
                        j--;
                    }
                }

                // 3. Fix the "coordinates" attribute: remove the SOM CV name from the coordinate attribute. 
                // Notice this is a little inefficient. Since we only have a few fields and non-SOM projection products
                // won't be affected, and more importantly, to keep the SOM projection handling in a central place,
                // I handle the adjustment of "coordinates" attribute here. KY 2012-6-12

                // MISR data cannot be visualized by Panoply and IDV. So the coordinates attribute
                // here reflects the coordinates of this variable more accurately. KY 2012-6-13 

                for (vector<Field *>::const_iterator j = (*i)->getDataFields().begin();
                    j != (*i)->getDataFields().end(); ++j) {

                    if ( 0 == (*j)->fieldtype) {

                        string temp_coordinates = (*j)->coordinates; 
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

                        (*j)->setCoordinates(temp_coordinates);   

                    }
                }
            }
        }
}

int File::obtain_dimmap_num(int numswath) throw(Exception) {


            // S(wath)0. Check if there are dimension maps in this this.
            int tempnumdm = 0;
            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i){
                tempnumdm += (*i)->get_num_map();
                if (tempnumdm >0) 
                    break;
            }

            // MODATML2 and MYDATML2 in year 2010 include dimension maps. But the dimension map
            // is not used. Furthermore, they provide additional latitude/longtiude 
            // for 10 KM under the data field. So we have to handle this differently.
            // MODATML2 in year 2000 version doesn't include dimension map, so we 
            // have to consider both with dimension map and without dimension map cases.
            // The swath name is atml2.

            bool fakedimmap = false;

            if(numswath == 1) {// Start special atml2-like handling
                if((this->swaths[0]->getName()).find("atml2")!=string::npos){
                    if(tempnumdm >0) fakedimmap = true;
                    int templlflag = 0;
                    for (vector<Field *>::const_iterator j =
                             this->swaths[0]->getGeoFields().begin();
                         j != this->swaths[0]->getGeoFields().end(); ++j) {
                        if((*j)->getName() == "Latitude" || (*j)->getName() == "Longitude") {
                            if ((*j)->getType() == DFNT_UINT16 ||
                                (*j)->getType() == DFNT_INT16)
                                (*j)->type = DFNT_FLOAT32;
                            templlflag ++;
                            if(templlflag == 2) 
                                break;
                        }
                    }

                    templlflag = 0;

                    for (vector<Field *>::const_iterator j =
                        this->swaths[0]->getDataFields().begin();
                        j != this->swaths[0]->getDataFields().end(); ++j) {

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

                        if(((*j)->getName()).find("Latitude") != string::npos){
                            if ((*j)->getType() == DFNT_UINT16 ||
                                (*j)->getType() == DFNT_INT16)
                                (*j)->type = DFNT_FLOAT32;
                            (*j)->fieldtype = 1;
                            // Also need to link the dimension to the coordinate variable list
                            if((*j)->getRank() != 2) 
                                throw2("The lat/lon rank must be  2 for Java clients to work",
                                       (*j)->getRank());
                            HDFCFUtil::insert_map(this->swaths[0]->dimcvarlist, 
                                       (((*j)->getDimensions())[0])->getName(),(*j)->getName());
                            templlflag ++;
                        }
                        if(((*j)->getName()).find("Longitude")!= string::npos) {
                            if((*j)->getType() == DFNT_UINT16 ||
                                (*j)->getType() == DFNT_INT16)
                                (*j)->type = DFNT_FLOAT32;

                            (*j)->fieldtype = 2;
                            if((*j)->getRank() != 2) 
                                throw2("The lat/lon rank must be  2 for Java clients to work",
                                       (*j)->getRank());
                            HDFCFUtil::insert_map(this->swaths[0]->dimcvarlist, 
                                       (((*j)->getDimensions())[1])->getName(), (*j)->getName());
                            templlflag ++;
                        }
                        if(templlflag == 2) 
                            break;
                    }
                }
            }// End of special atml2 handling

            // Although this file includes dimension map, it doesn't use it at all. So change
            // tempnumdm to 0.
            if(fakedimmap) 
                tempnumdm = 0;
 
    return tempnumdm;

}

void File::create_swath_latlon_dim_cvar_map(int numdm) throw(Exception){

            // S1. Prepare the right dimension name and the dimension field list for each swath. 
            // The assumption is that within a swath, the dimension name is unique.
            //  The dimension field name(even with the added Z-like field) is unique. 
            // A map <dimension name, dimension field name> will be created.
            // The name clashing handling for multiple swaths will not be done in this step. 

            // S1.1 Obtain the dimension names corresponding to the latitude and longitude,save them to the <dimname, dimfield> map.

            // We found a special MODAPS product: the Latitude and Longitude are put under the Data fields rather than GeoLocation fields.
            // So we need to go to the "Data Fields" to grab the "Latitude and Longitude ".

            bool lat_in_geofields = false;
            bool lon_in_geofields = false;

            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i){

                int tempgeocount = 0;
                for (vector<Field *>::const_iterator j =
                    (*i)->getGeoFields().begin();
                    j != (*i)->getGeoFields().end(); ++j) {

                    // Here we assume it is always lat[f0][f1] and lon [f0][f1]. No lat[f0][f1] and lon[f1][f0] occur.
                    // So far only "Latitude" and "Longitude" are used as standard names of Lat and lon for swath.
                    if((*j)->getName()=="Latitude" ){
                        if((*j)->getRank() > 2) 
                            throw2("Currently the lat/lon rank must be 1 or 2 for Java clients to work",
                                   (*j)->getRank());

                        lat_in_geofields = true;
                        // Since under our assumption, lat/lon are always 2-D for a swath and 
                        // dimension order doesn't matter for Java clients,
                        // we always map Latitude the first dimension and longitude the second dimension.
                        // Save this information in the coordinate variable name and field map.
                        // For rank =1 case, we only handle the cross-section along the same 
                        // longitude line. So Latitude should be the dimension name.

                        HDFCFUtil::insert_map((*i)->dimcvarlist, (((*j)->getDimensions())[0])->getName(), "Latitude");

                        // Have dimension map, we want to remember the dimension and remove it from the list.
                        if(numdm >0) {                    

                            // We have to loop through the dimension map
                            for(vector<SwathDataset::DimensionMap *>::const_iterator 
                                l=(*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){

                                // This dimension name will be replaced by the mapped dimension name, 
                                // the mapped dimension name can be obtained from the getDataDimension() method.
                                if(((*j)->getDimensions()[0])->getName() == (*l)->getGeoDimension()) {
                                    HDFCFUtil::insert_map((*i)->dimcvarlist, (*l)->getDataDimension(), "Latitude");
                                    break;
                                }
                            }
                        }
                        (*j)->fieldtype = 1;
                        tempgeocount ++;
                    }
                    if((*j)->getName()=="Longitude"){
                        if((*j)->getRank() > 2) 
                            throw2("Currently the lat/lon rank must be  1 or 2 for Java clients to work",
                                    (*j)->getRank());
                        // Only lat-level cross-section(for Panoply)is supported 
                        // when longitude/latitude is 1-D, so ignore the longitude as the dimension field.

                        lon_in_geofields = true;
                        if((*j)->getRank() == 1) {
                            tempgeocount++;
                            continue;
                        }
                        // Since under our assumption, lat/lon are almost always 2-D for 
                        // a swath and dimension order doesn't matter for Java clients,
                        // we always map Latitude the first dimension and longitude the second dimension.
                        // Save this information in the dimensiion name and coordinate variable map.
                        HDFCFUtil::insert_map((*i)->dimcvarlist, 
                                              (((*j)->getDimensions())[1])->getName(), "Longitude");
                        if(numdm >0) {

                            // We have to loop through the dimension map
                            for(vector<SwathDataset::DimensionMap *>::const_iterator 
                                l=(*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){

                                // This dimension name will be replaced by the mapped dimension name,
                                // This name can be obtained by getDataDimension() fuction of 
                                // dimension map class. 
                                if(((*j)->getDimensions()[1])->getName() == 
                                    (*l)->getGeoDimension()) {
                                    HDFCFUtil::insert_map((*i)->dimcvarlist, 
                                               (*l)->getDataDimension(), "Longitude");
                                    break;
                                }
                            }
                        }
                        (*j)->fieldtype = 2;
                        tempgeocount++;
                    }
                    if(tempgeocount == 2) 
                        break;
                }
            }// end of creating the <dimname,dimfield> map.

            // If lat and lon are not together, throw an error.
            if (lat_in_geofields ^ lon_in_geofields) 
                throw1("Latitude and longitude must be both under Geolocation fields or Data fields");
            
            if (!lat_in_geofields && !lon_in_geofields) {// Check if they are under data fields

                bool lat_in_datafields = false;
                bool lon_in_datafields = false;

                for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                    i != this->swaths.end(); ++i){

                    int tempgeocount = 0;
                    for (vector<Field *>::const_iterator j =
                         (*i)->getDataFields().begin();
                         j != (*i)->getDataFields().end(); ++j) {

                        // Here we assume it is always lat[f0][f1] and lon [f0][f1]. 
                        // No lat[f0][f1] and lon[f1][f0] occur.
                        // So far only "Latitude" and "Longitude" are used as 
                        // standard names of Lat and lon for swath.
                        if((*j)->getName()=="Latitude" ){
                            if((*j)->getRank() > 2) { 
                                throw2("Currently the lat/lon rank must be 1 or 2 for Java clients to work",
                                       (*j)->getRank());
                            }
                            lat_in_datafields = true;
                            // Since under our assumption, lat/lon are always 2-D 
                            // for a swath and dimension order doesn't matter for Java clients,
                            // we always map Latitude the first dimension and longitude the second dimension.
                            // Save this information in the coordinate variable name and field map.
                            // For rank =1 case, we only handle the cross-section along the same longitude line. 
                            // So Latitude should be the dimension name.

                            HDFCFUtil::insert_map((*i)->dimcvarlist, 
                                              (((*j)->getDimensions())[0])->getName(), "Latitude");

                            // Have dimension map, we want to remember the dimension and remove it from the list.
                            if(numdm >0) {                    

                                // We have to loop through the dimension map
                                for(vector<SwathDataset::DimensionMap *>::const_iterator 
                                    l=(*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){

                                    // This dimension name will be replaced by the mapped dimension name, 
                                    // the mapped dimension name can be obtained from the getDataDimension() method.
                                    if(((*j)->getDimensions()[0])->getName() == (*l)->getGeoDimension()) {
                                        HDFCFUtil::insert_map((*i)->dimcvarlist, (*l)->getDataDimension(), "Latitude");
                                        break;
                                    }
                                }
                            }
                            (*j)->fieldtype = 1;
                            tempgeocount ++;
                        }
                        if((*j)->getName()=="Longitude"){
                            if((*j)->getRank() > 2) { 
                                throw2("Currently the lat/lon rank must be  1 or 2 for Java clients to work",
                                        (*j)->getRank());
                            }
                            // Only lat-level cross-section(for Panoply)is supported when 
                            // longitude/latitude is 1-D, so ignore the longitude as the dimension field.

                            lon_in_datafields = true;
                            if((*j)->getRank() == 1) {
                                tempgeocount++;
                                continue;
                            }
                            // Since under our assumption, 
                            // lat/lon are almost always 2-D for a swath and dimension order doesn't matter for Java clients,
                            // we always map Latitude the first dimension and longitude the second dimension.
                            // Save this information in the dimensiion name and coordinate variable map.
                            HDFCFUtil::insert_map((*i)->dimcvarlist, 
                                                  (((*j)->getDimensions())[1])->getName(), "Longitude");
                            if(numdm >0) {
                                // We have to loop through the dimension map
                                for(vector<SwathDataset::DimensionMap *>::const_iterator 
                                    l=(*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){

                                    // This dimension name will be replaced by the mapped dimension name,
                                    // This name can be obtained by getDataDimension() fuction of dimension map class. 
                                    if(((*j)->getDimensions()[1])->getName() == (*l)->getGeoDimension()) {
                                        HDFCFUtil::insert_map((*i)->dimcvarlist, 
                                                              (*l)->getDataDimension(), "Longitude");
                                        break;
                                    }
                                }
                            }
                            (*j)->fieldtype = 2;
                            tempgeocount++;
                        }
                        if(tempgeocount == 2) 
                            break;
                    }
                }// end of creating the <dimname,dimfield> map.

                // If lat and lon are not together, throw an error.
                if (lat_in_datafields ^ lon_in_datafields) 
                    throw1("Latitude and longitude must be both under Geolocation fields or Data fields");

                // If lat,lon are not found under either "Data fields" or "Geolocation fields", we should not generate "coordiantes"
                // However, this case should be handled in the future release. KY 2012-09-24
                //**************** INVESTIGATE in the NEXT RELEASE ******************************
                //if (!lat_in_datafields && !lon_in_datafields)
                //  throw1("Latitude and longitude don't exist");
                //*********************************************************************************/
               
            }
 

}

void File:: create_swath_nonll_dim_cvar_map() throw(Exception)

{
            // S1.3 Handle existing and missing fields 
            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i){
                
                // Since we find multiple 1-D fields with the same dimension names for some Swath files(AIRS level 1B),
                // we currently always treat the third dimension field as a missing field, this may be corrected later.
                // Corrections for the above: MODIS data do include the unique existing Z fields, so we have to search
                // the existing Z field. KY 2010-8-11
                // One possible correction is to search all 1-D fields with the same dimension name within one swath,
                // if only one field is found, we use this field  as the third dimension.
                // 1.1 Add the missing Z-dimension field.
                // Some dimension name's corresponding fields are missing, 
                // so add the missing Z-dimension fields based on the dimension name. When the real
                // data is read, nature number 1,2,3,.... will be filled!
                // NOTE: The latitude and longitude dim names are not handled yet.  
               
                // S1.2.1 Build a unique 1-D dimension name list.Now the list only includes dimension names of "latitude" and "longitude".
    
                pair<set<string>::iterator,bool> tempdimret;
                for(map<string,string>::const_iterator j = (*i)->dimcvarlist.begin(); 
                    j!= (*i)->dimcvarlist.end();++j){ 
                    tempdimret = (*i)->nonmisscvdimlist.insert((*j).first);
                }

                // S1.2.2 Search the geofield group and see if there are any existing 1-D Z dimension data.
                //  If 1-D field data with the same dimension name is found under GeoField, we still search if that 1-D field  is the dimension
                // field of a dimension name.
                for (vector<Field *>::const_iterator j =
                    (*i)->getGeoFields().begin();
                    j != (*i)->getGeoFields().end(); ++j) {
             
                    if((*j)->getRank()==1) {
                        if((*i)->nonmisscvdimlist.find((((*j)->getDimensions())[0])->getName()) == (*i)->nonmisscvdimlist.end()){
                            tempdimret = (*i)->nonmisscvdimlist.insert((((*j)->getDimensions())[0])->getName());
                            if((*j)->getName() =="Time") (*j)->fieldtype = 5;// This is for IDV.
                            // This is for temporarily COARD fix. 
                            // For 2-D lat/lon, the third dimension should NOT follow
                            // COARD conventions. It will cause Panoply and IDV failed.
                            // KY 2010-7-21
                            // It turns out that we need to keep the original field name of the third dimension.
                            // So assign the flag and save the original name.
                            // KY 2010-9-9
#if 0
                            if(((((*j)->getDimensions())[0])->getName())==(*j)->getName()){
                                (*j)->oriname = (*j)->getName();
                                // netCDF-Java fixes the problem, now goes back to COARDS.
                                //(*j)->name = (*j)->getName() +"_d";
                                (*j)->specialcoard = true;
                            }
#endif
                            HDFCFUtil::insert_map((*i)->dimcvarlist, (((*j)->getDimensions())[0])->getName(), (*j)->getName());
                            (*j)->fieldtype = 3;

                        }
                    }
                }

                // We will also check the third dimension inside DataFields
                // This may cause potential problems for AIRS data
                // We will double CHECK KY 2010-6-26
                // So far the tests seem okay. KY 2010-8-11
                for (vector<Field *>::const_iterator j =
                    (*i)->getDataFields().begin();
                    j != (*i)->getDataFields().end(); ++j) {

                    if((*j)->getRank()==1) {
                        if((*i)->nonmisscvdimlist.find((((*j)->getDimensions())[0])->getName()) == (*i)->nonmisscvdimlist.end()){
                            tempdimret = (*i)->nonmisscvdimlist.insert((((*j)->getDimensions())[0])->getName());
                            if((*j)->getName() =="Time") (*j)->fieldtype = 5;// This is for IDV.
                            // This is for temporarily COARD fix. 
                            // For 2-D lat/lon, the third dimension should NOT follow
                            // COARD conventions. It will cause Panoply and IDV failed.
                            // KY 2010-7-21
#if 0
                            if(((((*j)->getDimensions())[0])->getName())==(*j)->getName()){
                                (*j)->oriname = (*j)->getName();
                                //(*j)->name = (*j)->getName() +"_d";
                                (*j)->specialcoard = true;
                            }
#endif
                            HDFCFUtil::insert_map((*i)->dimcvarlist, (((*j)->getDimensions())[0])->getName(), (*j)->getName());
                            (*j)->fieldtype = 3;

                        }
                    }
                }


                // S1.2.3 Handle the missing fields 
                // Loop through all dimensions of this swath to search the missing fields
                for (vector<Dimension *>::const_iterator j =
                    (*i)->getDimensions().begin(); j!= (*i)->getDimensions().end();++j){

                    if(((*i)->nonmisscvdimlist.find((*j)->getName())) == (*i)->nonmisscvdimlist.end()){// This dimension needs a field
                      
                        // Need to create a new data field vector element with the name and dimension as above.
                        Field *missingfield = new Field();
                        // This is for temporarily COARD fix. 
                        // For 2-D lat/lon, the third dimension should NOT follow
                        // COARD conventions. It will cause Panoply and IDV failed.
                        // Since Swath is always 2-D lat/lon, so we are okay here. Add a "_d" for each field name.
                        // KY 2010-7-21
                        // netCDF-Java now first follows COARDS, change back
                        // missingfield->name = (*j)->getName()+"_d";
                        missingfield->name = (*j)->getName();
                        missingfield->rank = 1;
                        missingfield->type = DFNT_INT32;//This is an HDF constant.the data type is always integer.
                        Dimension *dim = new Dimension((*j)->getName(),(*j)->getSize());

                        // only 1 dimension
                        missingfield->dims.push_back(dim);

                        // Provide information for the missing data, since we need to calculate the data, so
                        // the information is different than a normal field.
                        int missingdatarank =1;
                        int missingdatatypesize = 4;
                        int missingdimsize[1];
                        missingdimsize[0]= (*j)->getSize();
                        missingfield->fieldtype = 4; //added Z-dimension coordinate variable with nature number

                        // input data should be empty now. We will add the support later.
                        LightVector<char>inputdata;
                        missingfield->data = new 
                                             MissingFieldData(missingdatarank,missingdatatypesize,missingdimsize,inputdata);

                        (*i)->geofields.push_back(missingfield);
                        HDFCFUtil::insert_map((*i)->dimcvarlist, 
                                              (missingfield->getDimensions())[0]->getName(), missingfield->name);
                    }
                }
                (*i)->nonmisscvdimlist.clear();// clear this set.

            }// End of dealing with missing fields
 

}

void File::handle_swath_dim_cvar_maps(int tempnumdm) throw(Exception) {

            // Start handling name clashing
            vector <string> tempfieldnamelist;
            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i) {
                 
                // First handle geofield, all dimension fields are under the geofield group.
                for (vector<Field *>::const_iterator j =
                    (*i)->getGeoFields().begin();
                    j != (*i)->getGeoFields().end(); ++j) {
                    tempfieldnamelist.push_back(HDFCFUtil::get_CF_string((*j)->name));   
                }

                for (vector<Field *>::const_iterator j = (*i)->getDataFields().begin();
                    j!= (*i)->getDataFields().end(); ++j) {
                    tempfieldnamelist.push_back(HDFCFUtil::get_CF_string((*j)->name));
                }
            }

            HDFCFUtil::Handle_NameClashing(tempfieldnamelist);

            int total_fcounter = 0;
      
            // S4. Create a map for dimension field name <original field name, corrected field name>
            // Also assure the uniqueness of all field names,save the new field names.
            //the original dimension field name to the corrected dimension field name
            map<string,string>tempncvarnamelist;
            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i){

                // First handle geofield, all dimension fields are under the geofield group.
                for (vector<Field *>::const_iterator j = 
                    (*i)->getGeoFields().begin();
                    j != (*i)->getGeoFields().end(); ++j) 
                {
               
                    (*j)->newname = tempfieldnamelist[total_fcounter];
                    total_fcounter++;

                    if((*j)->fieldtype!=0) {// If this field is a dimension field, save the name/new name pair. 
                        HDFCFUtil::insert_map((*i)->ncvarnamelist, (*j)->getName(), (*j)->newname);
                    }
                }
 
                for (vector<Field *>::const_iterator j = 
                    (*i)->getDataFields().begin();
                    j != (*i)->getDataFields().end(); ++j) 
                {
               
                    (*j)->newname = tempfieldnamelist[total_fcounter];
                    total_fcounter++;

                    if((*j)->fieldtype!=0) {// If this field is a dimension field, save the name/new name pair. 
                        HDFCFUtil::insert_map((*i)->ncvarnamelist, (*j)->getName(), (*j)->newname);
                    }
                }
            } // end of creating a map for dimension field name <original field name, corrected field name>

            // S5. Create a map for dimension name < original dimension name, corrected dimension name>

            vector <string>tempalldimnamelist;

            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i)
                for (map<string,string>::const_iterator j =
                    (*i)->dimcvarlist.begin(); j!= (*i)->dimcvarlist.end();++j)
                    tempalldimnamelist.push_back(HDFCFUtil::get_CF_string((*j).first));

            HDFCFUtil::Handle_NameClashing(tempalldimnamelist);

            int total_dcounter = 0;

            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i){
                for (map<string,string>::const_iterator j =
                    (*i)->dimcvarlist.begin(); j!= (*i)->dimcvarlist.end();++j){
                    HDFCFUtil::insert_map((*i)->ndimnamelist, (*j).first, tempalldimnamelist[total_dcounter]);
                    total_dcounter++;
                }
            }

            //  S6. Create corrected dimension vectors.
            vector<Dimension*> correcteddims;
            string tempcorrecteddimname;
            Dimension *correcteddim;

            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i){ 

                // First the geofield. 
                for (vector<Field *>::const_iterator j =
                    (*i)->getGeoFields().begin();
                    j != (*i)->getGeoFields().end(); ++j) {

                    for(vector<Dimension *>::const_iterator k=(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){

                        map<string,string>::iterator tempmapit;

                        if(tempnumdm == 0) { // No dimension map, just obtain the new dimension name.

                            // Find the new name of this field
                            tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                            if(tempmapit != (*i)->ndimnamelist.end()) 
                                tempcorrecteddimname= tempmapit->second;
                            else 
                                throw4("cannot find the corrected dimension name", 
                                        (*i)->getName(),(*j)->getName(),(*k)->getName());

                            correcteddim = new Dimension(tempcorrecteddimname,(*k)->getSize());
                        }
                        else { 
                            // have dimension map, use the datadim and datadim size to replace the geodim and geodim size. 
                            bool isdimmapname = false;

                            // We have to loop through the dimension map
                            for(vector<SwathDataset::DimensionMap *>::const_iterator 
                                l=(*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){
                                // This dimension name is the geo dimension name in the dimension map, 
                                // replace the name with data dimension name.
                                if((*k)->getName() == (*l)->getGeoDimension()) {

                                    isdimmapname = true;
                                    (*j)->dmap = true;
                                    string temprepdimname = (*l)->getDataDimension();

                                    // Find the new name of this data dimension name
                                    tempmapit = (*i)->ndimnamelist.find(temprepdimname);
                                    if(tempmapit != (*i)->ndimnamelist.end()) 
                                        tempcorrecteddimname= tempmapit->second;
                                    else 
                                        throw4("cannot find the corrected dimension name", (*i)->getName(),
                                                (*j)->getName(),(*k)->getName());
                                    
                                    // Find the size of this data dimension name
                                    // We have to loop through the Dimensions of this swath
                                    bool ddimsflag = false;
                                    for(vector<Dimension *>::const_iterator m=(*i)->getDimensions().begin();
                                        m!=(*i)->getDimensions().end();++m) {
                                        if((*m)->getName() == temprepdimname) { 
                                        // Find the dimension size, create the correcteddim
                                            correcteddim = new Dimension(tempcorrecteddimname,(*m)->getSize());
                                            ddimsflag = true;
                                            break;
                                        }
                                    }
                                    if(!ddimsflag) 
                                        throw4("cannot find the corrected dimension size", (*i)->getName(),
                                                (*j)->getName(),(*k)->getName());
                                    break;
                                }
                            }
                            if(!isdimmapname) { // Still need to assign the corrected dimensions.
                                // Find the new name of this field
                                tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                                if(tempmapit != (*i)->ndimnamelist.end()) 
                                    tempcorrecteddimname= tempmapit->second;
                                else 
                                    throw4("cannot find the corrected dimension name", 
                                            (*i)->getName(),(*j)->getName(),(*k)->getName());

                                correcteddim = new Dimension(tempcorrecteddimname,(*k)->getSize());

                            }
                        }         

                        correcteddims.push_back(correcteddim);
                    }
                    (*j)->setCorrectedDimensions(correcteddims);
                    correcteddims.clear();
                }// End of creating the corrected dimension vectors for GeoFields.
 
                // Then the data field.
                for (vector<Field *>::const_iterator j =
                    (*i)->getDataFields().begin();
                    j != (*i)->getDataFields().end(); ++j) {

                    for(vector<Dimension *>::const_iterator k=
                        (*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){

                        if(tempnumdm == 0) {

                            map<string,string>::iterator tempmapit;

                            // Find the new name of this field
                            tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                            if(tempmapit != (*i)->ndimnamelist.end()) 
                                tempcorrecteddimname= tempmapit->second;
                            else 
                                throw4("cannot find the corrected dimension name", (*i)->getName(),
                                        (*j)->getName(),(*k)->getName());

                            correcteddim = new Dimension(tempcorrecteddimname,(*k)->getSize());
                        }
                        else {
                            map<string,string>::iterator tempmapit;
           
                            bool isdimmapname = false;
                            // We have to loop through dimension map
                            for(vector<SwathDataset::DimensionMap *>::const_iterator l=
                                (*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){
                                // This dimension name is the geo dimension name in the dimension map, 
                                // replace the name with data dimension name.
                                if((*k)->getName() == (*l)->getGeoDimension()) {
                                    isdimmapname = true;
                                    (*j)->dmap = true;
                                    string temprepdimname = (*l)->getDataDimension();
                   
                                    // Find the new name of this data dimension name
                                    tempmapit = (*i)->ndimnamelist.find(temprepdimname);
                                    if(tempmapit != (*i)->ndimnamelist.end()) 
                                        tempcorrecteddimname= tempmapit->second;
                                    else 
                                        throw4("cannot find the corrected dimension name", 
                                                (*i)->getName(),(*j)->getName(),(*k)->getName());
                                    
                                    // Find the size of this data dimension name
                                    // We have to loop through the Dimensions of this swath
                                    bool ddimsflag = false;
                                    for(vector<Dimension *>::const_iterator m=
                                        (*i)->getDimensions().begin();m!=(*i)->getDimensions().end();++m) {

                                        // Find the dimension size, create the correcteddim
                                        if((*m)->getName() == temprepdimname) { 
                                            correcteddim = new Dimension(tempcorrecteddimname,(*m)->getSize());
                                            ddimsflag = true;
                                            break;
                                        }
                                    }
                                    if(!ddimsflag) 
                                        throw4("cannot find the corrected dimension size", 
                                                (*i)->getName(),(*j)->getName(),(*k)->getName());
                                    break;
                                }
 
                            }
                            // Not a dimension with dimension map; Still need to assign the corrected dimensions.
                            if(!isdimmapname) { 

                                // Find the new name of this field
                                tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                                if(tempmapit != (*i)->ndimnamelist.end()) 
                                    tempcorrecteddimname= tempmapit->second;
                                else 
                                    throw4("cannot find the corrected dimension name", 
                                            (*i)->getName(),(*j)->getName(),(*k)->getName());

                                correcteddim = new Dimension(tempcorrecteddimname,(*k)->getSize());

                            }

                        }
                        correcteddims.push_back(correcteddim);
                    }
                    (*j)->setCorrectedDimensions(correcteddims);
                    correcteddims.clear();
                }// End of creating the dimensions for data fields.
            }



}
 
void File::handle_swath_cf_attrs() throw(Exception) {

            // S7. Create "coordinates" ,"units"  attributes. The "units" attributes only apply to latitude and longitude.
            // This is the last round of looping through everything, 
            // we will match dimension name list to the corresponding dimension field name 
            // list for every field. 
            // Since we find some swath files don't specify fillvalue when -9999.0 is found in the real data,
            // we specify fillvalue for those fields. This is entirely 
            // artifical and we will evaluate this approach. KY 2010-3-3
           
            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                 i != this->swaths.end(); ++i){

                // Handle GeoField first.
                for (vector<Field *>::const_iterator j =
                         (*i)->getGeoFields().begin();
                     j != (*i)->getGeoFields().end(); ++j) {
                 
                    // Real fields: adding the coordinate attribute
                    if((*j)->fieldtype == 0)  {// currently it is always true.
                        string tempcoordinates="";
                        string tempfieldname="";
                        string tempcorrectedfieldname="";
                        int tempcount = 0;
                        for(vector<Dimension *>::const_iterator 
                            k=(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){
                            // handle coordinates attributes
               
                            map<string,string>::iterator tempmapit;
                            map<string,string>::iterator tempmapit2;
              
                            // Find the dimension field name
                            tempmapit = ((*i)->dimcvarlist).find((*k)->getName());
                            if(tempmapit != ((*i)->dimcvarlist).end()) 
                                tempfieldname = tempmapit->second;
                            else 
                                throw4("cannot find the dimension field name",(*i)->getName(),
                                        (*j)->getName(),(*k)->getName());

                            // Find the corrected dimension field name
                            tempmapit2 = ((*i)->ncvarnamelist).find(tempfieldname);
                            if(tempmapit2 != ((*i)->ncvarnamelist).end()) 
                                tempcorrectedfieldname = tempmapit2->second;
                            else 
                                throw4("cannot find the corrected dimension field name",
                                        (*i)->getName(),(*j)->getName(),(*k)->getName());

                            if(tempcount == 0) 
                                tempcoordinates= tempcorrectedfieldname;
                            else 
                                tempcoordinates = tempcoordinates +" "+tempcorrectedfieldname;
                            tempcount++;
                        }
                        (*j)->setCoordinates(tempcoordinates);
                    }


                    // Add units for latitude and longitude
                    // latitude,adding the "units" attribute  degrees_east.
                    if((*j)->fieldtype == 1) {
                        string tempunits = "degrees_north";
                        (*j)->setUnits(tempunits);
                    }

                    // longitude, adding the units of
                    if((*j)->fieldtype == 2) {  
                        string tempunits = "degrees_east";
                        (*j)->setUnits(tempunits);
                    }

                    // Add units for Z-dimension, now it is always "level"
                    // We decide not touch the units if the third-dimension CV exists(fieldtype =3)
                    // KY 2013-02-15
                    //if(((*j)->fieldtype == 3)||((*j)->fieldtype == 4)) 
                    if(((*j)->fieldtype == 4)) {
                        string tempunits ="level";
                        (*j)->setUnits(tempunits);
                    }

                    // Add units for "Time", 
                    // Be aware that it is always "days since 1900-01-01 00:00:00"
                    if(((*j)->fieldtype == 5)) {
                        string tempunits = "days since 1900-01-01 00:00:00";
                        (*j)->setUnits(tempunits);
                    }
                    // Set the fill value for floating type data that doesn't have the fill value.
                    // We found _FillValue attribute is missing from some swath data.
                    // To cover the most cases, an attribute called _FillValue(the value is -9999.0)
                    // is added to the data whose type is float32 or float64.
                    if((((*j)->getFillValue()).empty()) && 
                        ((*j)->getType()==DFNT_FLOAT32 || (*j)->getType()==DFNT_FLOAT64)) { 
                        float tempfillvalue = -9999.0;
                        (*j)->addFillValue(tempfillvalue);
                        (*j)->setAddedFillValue(true);
                    }
                }
 
                // Data fields
                for (vector<Field *>::const_iterator j =
                    (*i)->getDataFields().begin();
                    j != (*i)->getDataFields().end(); ++j) {
                 
                    // Real fields: adding coordinate attributesinate attributes
                    if((*j)->fieldtype == 0)  {// currently it is always true.
                        string tempcoordinates="";
                        string tempfieldname="";
                        string tempcorrectedfieldname="";
                        int tempcount = 0;
                        for(vector<Dimension *>::const_iterator k
                            =(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){
                            // handle coordinates attributes
               
                            map<string,string>::iterator tempmapit;
                            map<string,string>::iterator tempmapit2;
              
                            // Find the dimension field name
                            tempmapit = ((*i)->dimcvarlist).find((*k)->getName());
                            if(tempmapit != ((*i)->dimcvarlist).end()) 
                                tempfieldname = tempmapit->second;
                            else 
                                throw4("cannot find the dimension field name",(*i)->getName(),
                                        (*j)->getName(),(*k)->getName());

                            // Find the corrected dimension field name
                            tempmapit2 = ((*i)->ncvarnamelist).find(tempfieldname);
                            if(tempmapit2 != ((*i)->ncvarnamelist).end()) 
                                tempcorrectedfieldname = tempmapit2->second;
                            else 
                                throw4("cannot find the corrected dimension field name",
                                        (*i)->getName(),(*j)->getName(),(*k)->getName());

                            if(tempcount == 0) 
                                tempcoordinates= tempcorrectedfieldname;
                            else 
                                tempcoordinates = tempcoordinates +" "+tempcorrectedfieldname;
                            tempcount++;
                        }
                        (*j)->setCoordinates(tempcoordinates);
                    }
                    // Add units for Z-dimension, now it is always "level"
                    if(((*j)->fieldtype == 3)||((*j)->fieldtype == 4)) {
                        string tempunits ="level";
                        (*j)->setUnits(tempunits);
                    }

                    // Add units for "Time", Be aware that it is always "days since 1900-01-01 00:00:00"
                    if(((*j)->fieldtype == 5)) {
                        string tempunits = "days since 1900-01-01 00:00:00";
                        (*j)->setUnits(tempunits);
                    }

                    // Set the fill value for floating type data that doesn't have the fill value.
                    // We found _FillValue attribute is missing from some swath data.
                    // To cover the most cases, an attribute called _FillValue(the value is -9999.0)
                    // is added to the data whose type is float32 or float64.
                    if((((*j)->getFillValue()).empty()) && 
                        ((*j)->getType()==DFNT_FLOAT32 || (*j)->getType()==DFNT_FLOAT64)) { 
                        float tempfillvalue = -9999.0;
                        (*j)->addFillValue(tempfillvalue);
                        (*j)->setAddedFillValue(true);
                    }

                }
            }

}
void File::Prepare(const char *path) throw(Exception)
{

    // The current HDF-EOS2 module doesn't  cause performance penalty to
    // obtain all the information of the file. So we just retrieve all.
//    File *this = this;

    // Obtain the number of swaths and the number of grids
    int numgrid = this->grids.size();
    int numswath = this->swaths.size(); 
    
    if(numgrid < 0) 
        throw2("the number of grid is less than 0", path);
    
    if (numgrid > 0) {

        // 0. obtain "XDim","YDim","Latitude","Longitude" and "location" set.
        string DIMXNAME = this->get_geodim_x_name();       
      
        string DIMYNAME = this->get_geodim_y_name();       

        string LATFIELDNAME = this->get_latfield_name();       

        string LONFIELDNAME = this->get_lonfield_name();       

        // Now only there is only one geo grid name "location", so don't call it know.
        // string GEOGRIDNAME = this->get_geogrid_name();
        string GEOGRIDNAME = "location";

        // First handle Grid.
        // G1. Check global lat/lon for multiple grids.
        // We want to check if there is a global lat/lon for multiple grids.
        // AIRS level 3 data provides lat/lon under the GEOGRIDNAME grid.
        check_onelatlon_grids();
//#if 0
        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
                i != this->grids.end(); ++i) 
                handle_one_grid_zdim(*i);
        
        if (true == this->onelatlon) 
            handle_onelatlon_grids();
        else  {
            for (vector<GridDataset *>::const_iterator i = this->grids.begin();
                i != this->grids.end(); ++i) {

                // G2.0. Set the horizontal dimension name "dimxname" and "dimyname"
                // G2.0. This will be used to detect the dimension major order.
                (*i)->setDimxName(DIMXNAME);
                (*i)->setDimyName(DIMYNAME);

                handle_one_grid_latlon(*i);
            }
        }

        handle_grid_dim_cvar_maps();
        handle_grid_coards();
        handle_grid_cf_attrs();
        handle_grid_SOM_projection();

//#endif

#if 0
        if(numgrid > 0) {
            // This is not needed for one grid case. Will revise later KY 2010-6-29
            int onellcount = 0; //only one lat/lon and it is under GEOGRIDNAME
            int morellcount = 0; // Check if lat/lon is found under other grids.

            // Loop through all grids
            for(vector<GridDataset *>::const_iterator i = this->grids.begin();
                i != this->grids.end(); ++i){

                // Loop through all fields
                for(vector<Field *>::const_iterator j =
                        (*i)->getDataFields().begin();
                    j != (*i)->getDataFields().end(); ++j) {
                    if((*i)->getName()==GEOGRIDNAME){
                        if((*j)->getName()==LATFIELDNAME){
                            onellcount++;
                            (*i)->latfield = *j;
                        }
                        if((*j)->getName()==LONFIELDNAME){ 
                            onellcount++;
                            (*i)->lonfield = *j;
                        }
                        if(onellcount == 2) 
                           break;//Finish this grid
                    }
                    else {// Here we assume that lat and lon are always in pairs.
                        if(((*j)->getName()==LATFIELDNAME)||((*j)->getName()==LONFIELDNAME)){ 
                            (*i)->ownllflag = true;
                            morellcount++;
                            break;
                        }
                    }
                }
            }
            if(morellcount ==0 && onellcount ==2) 
                this->onelatlon = true; 
        }
#endif
#if 0

        // G2 Prepare the right dimension name and the dimension field list for each grid. 
        // The assumption is that within a grid, the dimension name is unique. The dimension field name(even with the added Z-like field) is unique. 
        // A map <dimension name, dimension field name> will be created.
        // The name clashing handling for multiple grids will not be done in this step. 

        //Dimension name and the corresponding field name when only one lat/lon is used for all grids.
        map<string,string>temponelatlondimcvarlist;
          
        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i){

            // G2.0. Set the horizontal dimension name "dimxname" and "dimyname"
            // G2.0. This will be used to detect the dimension major order.
            (*i)->setDimxName(DIMXNAME);
            (*i)->setDimyName(DIMYNAME);

            // G2.1. Find the existing Z-dimension field and save the list

            // This is a big assumption, it may be wrong since not every 1-D field with the third dimension(name and size) is a coordinate
            // variable. We have to watch the products we've supported. KY 2012-6-13
            set<string> tempdimlist; // Unique 1-D field's dimension name list.
            pair<set<string>::iterator,bool> tempdimret;
            map<string,string>tempdimcvarlist;//dimension name and the corresponding field name. 

            for (vector<Field *>::const_iterator j =
                (*i)->getDataFields().begin();
                j != (*i)->getDataFields().end(); ++j) {
                if ((*j)->getRank()==1){//We only need to search those 1-D fields

                    // DIMXNAME and DIMYNAME correspond to latitude and longitude.
                    // They should NOT be treated as dimension names missing fields. It will be handled differently.
                    // Kent: The following implementation may not be always right. This essentially is the flaw of the 
                    // data product if a file encounters this case.
                    if(((*j)->getDimensions())[0]->getName()!=DIMXNAME && ((*j)->getDimensions())[0]->getName()!=DIMYNAME){
                        tempdimret = tempdimlist.insert(((*j)->getDimensions())[0]->getName());

                        // Only pick up the first 1-D field that the third-dimension 
                        if(tempdimret.second == true) {
                            HDFCFUtil::insert_map(tempdimcvarlist, ((*j)->getDimensions())[0]->getName(),
                            (*j)->getName());
                            (*j)->fieldtype = 3;
                            if((*j)->getName() == "Time") 
                                (*j)->fieldtype = 5;// IDV can handle 4-D fields when the 4th dim is Time.
                        }
                    }
                }
            } 

            // G2.2 Add the missing Z-dimension field.
            // Some dimension name's corresponding fields are missing, 
            // so add the missing Z-dimension fields based on the dimension names. When the real
            // data is read, nature number 1,2,3,.... will be filled!
            // NOTE: The latitude and longitude dim names are not handled yet.  

            // The above handling is also a big assumption. KY 2012-6-12
            // Loop through all dimensions of this grid.
            for (vector<Dimension *>::const_iterator j =
                (*i)->getDimensions().begin(); j!= (*i)->getDimensions().end();++j){

                if((*j)->getName()!=DIMXNAME && (*j)->getName()!=DIMYNAME){// Don't handle DIMXNAME and DIMYNAME yet.
                    if((tempdimlist.find((*j)->getName())) == tempdimlist.end()){// This dimension needs a field
                      
                        // Need to create a new data field vector element with the name and dimension as above.
                        Field *missingfield = new Field();
                        missingfield->name = (*j)->getName();
                        missingfield->rank = 1;
                        missingfield->type = DFNT_INT32;//This is an HDF constant.the data type is always integer.
                        Dimension *dim = new Dimension((*j)->getName(),(*j)->getSize());

                        // only 1 dimension
                        missingfield->dims.push_back(dim);

                        // Provide information for the missing data, since we need to calculate the data, so
                        // the information is different than a normal field.
                        int missingdatarank =1;
                        int missingdatatypesize = 4;
                        int missingdimsize[1];
                        missingdimsize[0]= (*j)->getSize();
                        missingfield->fieldtype = 4; //added Z-dimension coordinate variable with nature number

                        // input data should be empty now. We will add the support later.
                        LightVector<char>inputdata;
                        missingfield->data = new MissingFieldData(missingdatarank,missingdatatypesize,missingdimsize,inputdata);
                        (*i)->datafields.push_back(missingfield);
                        HDFCFUtil::insert_map(tempdimcvarlist, (missingfield->getDimensions())[0]->getName(), 
                                   missingfield->name);
                    }
                }
            }
            tempdimlist.clear();// clear this temp. set.

            // G2.3 Handle latitude and longitude for dim. list and coordinate variable list
            if(this->onelatlon) {// One global lat/lon for all grids.
                if((*i)->getName()==GEOGRIDNAME) {// Handle lat/lon. Note that other grids need to point to this lat/lon.

                    // Figure out dimension order,2D or 1D for lat/lon
                    // if lat/lon field's pointed value is changed, the value of the lat/lon field is also changed.

                    // set the flag to tell if this is lat or lon. The unit will be different for lat and lon.
                    (*i)->lonfield->fieldtype = 2;
                    (*i)->latfield->fieldtype = 1;

                    // latitude and longitude rank must be equal and should not be greater than 2.
                    if((*i)->lonfield->rank >2 || (*i)->latfield->rank >2) 
                        throw2("Either the rank of lat or the lon is greater than 2",(*i)->getName());
                    if((*i)->lonfield->rank !=(*i)->latfield->rank) 
                        throw2("The rank of the latitude is not the same as the rank of the longitude",(*i)->getName());

                    if((*i)->lonfield->rank !=1) {// 2-D lat/lon arrays

                        // obtain the major dim. For most cases, it is YDim Major. 
                        //But for some cases it is not. So still need to check.
                        (*i)->lonfield->ydimmajor = (*i)->getCalculated().isYDimMajor();
                        (*i)->latfield->ydimmajor = (*i)->lonfield->ydimmajor;

                        // Check if the 2-D lat/lon can be condensed to 1-D. 
                        //For current HDF-EOS2 files, only GEO and CEA can be condensed. To gain performance,
                        // we just check the projection code, don't check with real values.
                        int32 projectioncode = (*i)->getProjection().getCode();
                        if(projectioncode == GCTP_GEO || projectioncode ==GCTP_CEA) {
                            (*i)->lonfield->condenseddim = true;
                            (*i)->latfield->condenseddim = true;
                     
                        }
                        // Now we want to handle the dim and the var lists.  
                        // If the lat/lon can be condensed to 1-D array, COARD convention needs to be followed. 
                        // Since COARD requires that the dimension name of lat/lon is the same as the field name of lat/lon,
                        // we still need to handle this case later. Now we do the first step.  

                        if((*i)->lonfield->condenseddim) {// can be condensed to 1-D array.
                            //regardless of YDim major or XDim major,,always lat->YDim, lon->XDim; 
                            // We still need to remember the dimension major when we calculate the data.
                            // We don't need to adjust the dimension rank.
                            for (vector<Dimension *>::const_iterator j =
                                (*i)->lonfield->getDimensions().begin(); j!= (*i)->lonfield->getDimensions().end();++j){
                                if((*j)->getName() == DIMXNAME) {
                                    HDFCFUtil::insert_map(tempdimcvarlist, (*j)->getName(), 
                                               (*i)->lonfield->getName());

                                }
                                if((*j)->getName() == DIMYNAME) {
                                    HDFCFUtil::insert_map(tempdimcvarlist, (*j)->getName(), 
                                               (*i)->latfield->getName());
                                }
                            }
                        }
                        else {// 2-D lat/lon case. Since dimension order doesn't matter, so we always assume lon->XDim, lat->YDim.
                            for (vector<Dimension *>::const_iterator j =
                                (*i)->lonfield->getDimensions().begin(); j!= (*i)->lonfield->getDimensions().end();++j){
                                if((*j)->getName() == DIMXNAME) {
                                    HDFCFUtil::insert_map(tempdimcvarlist, (*j)->getName(), 
                                               (*i)->lonfield->getName());
                                }
                                if((*j)->getName() == DIMYNAME) {
                                    HDFCFUtil::insert_map(tempdimcvarlist, (*j)->getName(), 
                                               (*i)->latfield->getName());
                                }
                            }
                        }
                    }
                    else { // This is the 1-D case, just inserting the dimension, field pair.
                        HDFCFUtil::insert_map(tempdimcvarlist, ((*i)->lonfield->getDimensions())[0]->getName(),
                                   (*i)->lonfield->getName());
                        HDFCFUtil::insert_map(tempdimcvarlist, ((*i)->latfield->getDimensions())[0]->getName(),
                                   (*i)->latfield->getName());
                    }
                    temponelatlondimcvarlist = tempdimcvarlist;
                }
                
            }
            else {// Has its own latitude/longitude or lat/lon needs to be calculated.
                if((*i)->ownllflag) {// this grid has its own latitude/longitude
                    // Searching the lat/lon field from the grid. 
                    for (vector<Field *>::const_iterator j =
                        (*i)->getDataFields().begin();
                        j != (*i)->getDataFields().end(); ++j) {
 
                        if((*j)->getName() == LATFIELDNAME) {

                            // set the flag to tell if this is lat or lon. The unit will be different for lat and lon.
                            (*j)->fieldtype = 1;

                            // Latitude rank should not be greater than 2.
                            // Here I don't check if the rank of latitude and longitude is the same. Hopefully it never happens for HDF-EOS2 cases.
                            // We are still investigating if Java clients work when the rank of latitude and longitude is greater than 2.
                            if((*j)->getRank() > 2) throw3("The rank of latitude is greater than 2",(*i)->getName(),(*j)->getName());

                            if((*j)->getRank() != 1) {

                                // Obtain the major dim. For most cases, it is YDim Major. But some cases may be not. Still need to check.
                                (*j)->ydimmajor = (*i)->getCalculated().isYDimMajor();

                                // If the 2-D lat/lon can be condensed to 1-D.
                                //For current HDF-EOS2 files, only GEO and CEA can be condensed. To gain performance,
                                // we don't check with real values.
                                int32 projectioncode = (*i)->getProjection().getCode();
                                if(projectioncode == GCTP_GEO || projectioncode ==GCTP_CEA) {
                                    (*j)->condenseddim = true;
                                }

                                // Now we want to handle the dim and the var lists.
                                // If the lat/lon can be condensed to 1-D array, COARD convention needs to be followed.
                                // Since COARD requires that the dimension name of lat/lon is the same as the field name of lat/lon,
                                // we still need to handle this case at last.
                                if((*j)->condenseddim) {// can be condensed to 1-D array.
                                    // If it is YDim major, lat->YDim, lon->XDim;
                                    // We don't need to adjust the dimension rank.
                                    for (vector<Dimension *>::const_iterator k =
                                        (*j)->getDimensions().begin(); k!= (*j)->getDimensions().end();++k){
                                        if((*k)->getName() == DIMYNAME) {
                                            HDFCFUtil::insert_map(tempdimcvarlist, (*k)->getName(), (*j)->getName());
                                        }
                                    }
                                }
                                else {// 2-D lat/lon case. Since dimension order doesn't matter, so we always assume lon->XDim, lat->YDim.
                                    for (vector<Dimension *>::const_iterator k =
                                        (*j)->getDimensions().begin(); k!= (*j)->getDimensions().end();++k){
                                        if((*k)->getName() == DIMYNAME) {
                                            HDFCFUtil::insert_map(tempdimcvarlist, (*k)->getName(), (*j)->getName());
                                        }
                                    }
                                }
                            }
                            else { // This is the 1-D case, just inserting  the dimension, field pair.
                                HDFCFUtil::insert_map(tempdimcvarlist, (((*j)->getDimensions())[0])->getName(), 
                                           (*j)->getName());
                            }
                        } 
                        else if ((*j)->getName() == LONFIELDNAME) {

                            // set the flag to tell if this is lat or lon. The unit will be different for lat and lon.
                            (*j)->fieldtype = 2;

                            // longitude rank should not be greater than 2.
                            // Here I don't check if the rank of latitude and longitude is the same. Hopefully it never happens for HDF-EOS2 cases.
                            // We are still investigating if Java clients work when the rank of latitude and longitude is greater than 2.
                            if((*j)->getRank() >2) 
                                throw3("The rank of Longitude is greater than 2",(*i)->getName(),(*j)->getName());

                            if((*j)->getRank() != 1) {

                                // Obtain the major dim. For most cases, it is YDim Major. But some cases may be not. Still need to check.
                                (*j)->ydimmajor = (*i)->getCalculated().isYDimMajor();

                                // If the 2-D lat/lon can be condensed to 1-D.
                                //For current HDF-EOS2 files, only GEO and CEA can be condensed. To gain performance,
                                // we don't check with real values.
                                int32 projectioncode = (*i)->getProjection().getCode();
                                if(projectioncode == GCTP_GEO || projectioncode ==GCTP_CEA) {
                                    (*j)->condenseddim = true;
                                }

                                // Now we want to handle the dim and the var lists.
                                // If the lat/lon can be condensed to 1-D array, COARD convention needs to be followed.
                                // Since COARD requires that the dimension name of lat/lon is the same as the field name of lat/lon,
                                // we still need to handle this case at last.
                                if((*j)->condenseddim) {// can be condensed to 1-D array.

                                    //  lat->YDim, lon->XDim;
                                    // We don't need to adjust the dimension rank.
                                    for (vector<Dimension *>::const_iterator k =
                                        (*j)->getDimensions().begin(); k!= (*j)->getDimensions().end();++k){
                                        if((*k)->getName() == DIMXNAME) {
                                            HDFCFUtil::insert_map(tempdimcvarlist, (*k)->getName(), (*j)->getName());
                                        }
                                    }
                                }
                                else {// 2-D lat/lon case. Since dimension order doesn't matter, so we always assume lon->XDim, lat->YDim.
                                    for (vector<Dimension *>::const_iterator k =
                                        (*j)->getDimensions().begin(); k!= (*j)->getDimensions().end();++k){
                                        if((*k)->getName() == DIMXNAME) {
                                            HDFCFUtil::insert_map(tempdimcvarlist, (*k)->getName(), (*j)->getName());
                                        }
                                    }
                                }
                            }
                            else { // This is the 1-D case, just inserting  the dimension, field pair.
                                HDFCFUtil::insert_map(tempdimcvarlist, (((*j)->getDimensions())[0])->getName(), 
                                           (*j)->getName());
                            }
 
                        }
                    } 
                }
                else { // this grid's lat/lon has to be calculated.
                    // Latitude and Longitude 
                    Field *latfield = new Field();
                    Field *lonfield = new Field();

                    latfield->name = LATFIELDNAME;
                    lonfield->name = LONFIELDNAME;

                    latfield->rank = 2;// Assume it is a 2-D array
                    lonfield->rank = 2;// Assume it is a 2-D array

                    latfield->type = DFNT_FLOAT64;//This is an HDF constant.the data type is always float64.
                    lonfield->type = DFNT_FLOAT64;//This is an HDF constant.the data type is always float64.

                    latfield->fieldtype = 1;
                    lonfield->fieldtype = 2;

                    // Check if YDim is the major order. 
                    // Obtain the major dim. For most cases, it is YDim Major. But some cases may be not. Still need to check.
                    latfield->ydimmajor = (*i)->getCalculated().isYDimMajor();
                    lonfield->ydimmajor = latfield->ydimmajor;

                    // Obtain XDim and YDim size.
                    int xdimsize = (*i)->getInfo().getX();
                    int ydimsize = (*i)->getInfo().getY();

                    // Add dimensions. If it is YDim major,the dimension name list is "YDim XDim", otherwise, it is "XDim YDim". 
                    // For LAMAZ projection, Y dimension is always supposed to be major for calculating lat/lon, but for dimension order, it should be consistent with data fields. (LD -2012/01/16)
                    bool dmajor=((*i)->getProjection().getCode()==GCTP_LAMAZ)? (*i)->getCalculated().DetectFieldMajorDimension(): latfield->ydimmajor;
                    // latfield->ydimmajor(just keep the comment to remember the old code. 
                    if(dmajor) { 
                        Dimension *dimlaty = new Dimension(DIMYNAME,ydimsize);
                        latfield->dims.push_back(dimlaty);
                        Dimension *dimlony = new Dimension(DIMYNAME,ydimsize);
                        lonfield->dims.push_back(dimlony);
                        Dimension *dimlatx = new Dimension(DIMXNAME,xdimsize);
                        latfield->dims.push_back(dimlatx);
                        Dimension *dimlonx = new Dimension(DIMXNAME,xdimsize);
                        lonfield->dims.push_back(dimlonx);
                    }
                    else {
                        Dimension *dimlatx = new Dimension(DIMXNAME,xdimsize);
                        latfield->dims.push_back(dimlatx);
                        Dimension *dimlonx = new Dimension(DIMXNAME,xdimsize);
                        lonfield->dims.push_back(dimlonx);
                        Dimension *dimlaty = new Dimension(DIMYNAME,ydimsize);
                        latfield->dims.push_back(dimlaty);
                        Dimension *dimlony = new Dimension(DIMYNAME,ydimsize);
                        lonfield->dims.push_back(dimlony);
                    }

                    latfield->data = NULL; 
                    lonfield->data = NULL;

                    // Obtain info upleft and lower right for special longitude.
                    float64* upleft;
                    float64* lowright;
                    upleft = const_cast<float64 *>((*i)->getInfo().getUpLeft());
                    lowright = const_cast<float64 *>((*i)->getInfo().getLowRight());

                    // SOme special longitude is from 0 to 360.We need to check this case.
                    int32 projectioncode = (*i)->getProjection().getCode();
                    if(((int)lowright[0]>180000000) && ((int)upleft[0]>-1)) {
                        // We can only handle geographic projection now.
                        // This is the only case we can handle.
                        if(projectioncode == GCTP_GEO) // Will handle when data is read.
                            lonfield->speciallon = true;
                    }

                    // Some MODIS MCD files don't follow standard format for lat/lon (DDDMMMSSS);
                    // they simply represent lat/lon as -180.0000000 or -90.000000.
                    // HDF-EOS2 library won't give the correct value based on these value.
                    // These need to be remembered and resumed to the correct format when retrieving the data.
                    // Since so far we haven't found region of satellite files is within 0.1666 degree(1 minute)
                    // so, we divide the corner coordinate by 1000 and see if the integral part is 0.
                    // If it is 0, we know this file uses special lat/lon coordinate.

                    if(((int)(lowright[0]/1000)==0) &&((int)(upleft[0]/1000)==0) 
                        && ((int)(upleft[1]/1000)==0) && ((int)(lowright[1]/1000)==0)) {
                        if(projectioncode == GCTP_GEO){
                            lonfield->specialformat = 1;
                            latfield->specialformat = 1;
                        }             
                    }

                    // Some TRMM CERES Grid Data have "default" to be set for the corner coordinate,
                    // which they really mean for the whole globe(-180 - 180 lon and -90 - 90 lat). 
                    // We will remember the information and change
                    // those values when we read the lat and lon.

                    if(((int)(lowright[0])==0) &&((int)(upleft[0])==0)
                        && ((int)(upleft[1])==0) && ((int)(lowright[1])==0)) {
                        if(projectioncode == GCTP_GEO){
                            lonfield->specialformat = 2;
                            latfield->specialformat = 2;
                            (*i)->addfvalueattr = true;
                        }
                    }
                
                    //One MOD13C2 file doesn't provide projection code
                    // The upperleft and lowerright coordinates are all -1
                    // We have to calculate lat/lon by ourselves.
                    // Since it doesn't provide the project code, we double check their information
                    // and find that it covers the whole globe with 0.05 degree resolution.
                    // Lat. is from 90 to -90 and Lon is from -180 to 180.

                    if(((int)(lowright[0])==-1) &&((int)(upleft[0])==-1)
                        && ((int)(upleft[1])==-1) && ((int)(lowright[1])==-1)) {
                        lonfield->specialformat = 3;
                        latfield->specialformat = 3;
                        lonfield->condenseddim = true;
                        latfield->condenseddim = true;
                    }


                    if(GCTP_SOM == projectioncode) {
                        lonfield->specialformat = 4;
                        latfield->specialformat = 4;
                    }

                    // Check if the 2-D lat/lon can be condensed to 1-D.
                    //For current HDF-EOS2 files, only GEO and CEA can be condensed. To gain performance,
                    // we just check the projection code, don't check with real values.
                    if(projectioncode == GCTP_GEO || projectioncode ==GCTP_CEA) {
                        lonfield->condenseddim = true;
                        latfield->condenseddim = true;
                    }
                    // Add latitude and longitude fields to the field list.
                    (*i)->datafields.push_back(latfield);
                    (*i)->datafields.push_back(lonfield);

                    // Now we want to handle the dim and the var lists.
                    // If the lat/lon can be condensed to 1-D array, COARD convention needs to be followed.
                    // Since COARD requires that the dimension name of lat/lon is the same as the field name of lat/lon,
                    // we still need to handle this case later.

                    if(lonfield->condenseddim) {// can be condensed to 1-D array.
                        //  lat->YDim, lon->XDim;
                        // We don't need to adjust the dimension rank.
                        for (vector<Dimension *>::const_iterator j =
                            lonfield->getDimensions().begin(); j!= lonfield->getDimensions().end();++j){
                            if((*j)->getName() == DIMXNAME) {
                                HDFCFUtil::insert_map(tempdimcvarlist, (*j)->getName(), lonfield->getName());
                            }

                            if((*j)->getName() == DIMYNAME) {
                                HDFCFUtil::insert_map(tempdimcvarlist, (*j)->getName(), latfield->getName());
                            }
                        }
                    }
                    else {// 2-D lat/lon case. Since dimension order doesn't matter, so we always assume lon->XDim, lat->YDim.
                        for (vector<Dimension *>::const_iterator j =
                            lonfield->getDimensions().begin(); j!= lonfield->getDimensions().end();++j){

                            if((*j)->getName() == DIMXNAME){ 
                                HDFCFUtil::insert_map(tempdimcvarlist, (*j)->getName(), lonfield->getName());
                            }

                            if((*j)->getName() == DIMYNAME){
                                HDFCFUtil::insert_map(tempdimcvarlist, (*j)->getName(), latfield->getName());
                            }
                        }
                    }
                }
            }
            (*i)->dimcvarlist = tempdimcvarlist;
            tempdimcvarlist.clear();
        }

        // G2.4 When only one lat/lon is used for all grids, grids other than "locations" need to add dim lat/lon pairs.
        if(this->onelatlon) {
            for (vector<GridDataset *>::const_iterator i = this->grids.begin();
                i != this->grids.end(); ++i){

                string templatlonname1;
                string templatlonname2;

                if((*i)->getName() != GEOGRIDNAME) {
                    map<string,string>::iterator tempmapit;

                    // Find DIMXNAME field
                    tempmapit = temponelatlondimcvarlist.find(DIMXNAME);
                    if(tempmapit != temponelatlondimcvarlist.end()) 
                        templatlonname1= tempmapit->second;
                    else 
                        throw2("cannot find the dimension field of XDim", (*i)->getName());
                    HDFCFUtil::insert_map((*i)->dimcvarlist, DIMXNAME, templatlonname1);

                    // Find DIMYNAME field
                    tempmapit = temponelatlondimcvarlist.find(DIMYNAME);
                    if(tempmapit != temponelatlondimcvarlist.end()) 
                        templatlonname2= tempmapit->second;
                    else
                        throw2("cannot find the dimension field of YDim", (*i)->getName());
                    HDFCFUtil::insert_map((*i)->dimcvarlist, DIMYNAME, templatlonname2);
                }
            }
        }

//#endif 
        //// ************* START HANDLING the name clashings for the field names. ******************
        //// Using a string vector for new field names.
        //// ******************

        // G3.5 Handle name clashings
        // G3.5.1 build up a temp. name list
        // Note here: we don't include grid and swath names(simply (*j)->name) due to the products we observe
        // Adding the grid/swath names makes the names artificially long. Will check user's feedback
        // and may change them later. KY 2012-6-25
        vector <string> tempfieldnamelist;
        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i) {
            for (vector<Field *>::const_iterator j = (*i)->getDataFields().begin();
                j!= (*i)->getDataFields().end(); ++j) { 
                tempfieldnamelist.push_back(HDFCFUtil::get_CF_string((*j)->name));
            }
        }
        
        HDFCFUtil::Handle_NameClashing(tempfieldnamelist);

                //// ***** Handling the dimension name clashing ******************

        vector <string>tempalldimnamelist;
        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i)
            for (map<string,string>::const_iterator j =
                (*i)->dimcvarlist.begin(); j!= (*i)->dimcvarlist.end();++j)
                tempalldimnamelist.push_back(HDFCFUtil::get_CF_string((*j).first));

        HDFCFUtil::Handle_NameClashing(tempalldimnamelist);

        // G4. Create a map for dimension field name <original field name, corrected field name>
        // Also assure the uniqueness of all field names,save the new field names.
        map<string,string>tempncvarnamelist;//the original dimension field name to the corrected dimension field name
         string tempcorrectedlatname, tempcorrectedlonname;
      
        int total_fcounter = 0;

        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i){

            // Here we can't use getDataFields call since for no lat/lon fields 
            // are created for one global lat/lon case. We have to use the dimcvarnamelist 
            // map we just created.
 
            for (vector<Field *>::const_iterator j = 
                (*i)->getDataFields().begin();
                j != (*i)->getDataFields().end(); ++j) 
            {
                (*j)->newname = tempfieldnamelist[total_fcounter];
                total_fcounter++;  
               
                if((*j)->fieldtype!=0) {// If this field is a dimension field, save the name/new name pair. 
                    tempncvarnamelist.insert(make_pair((*j)->getName(), (*j)->newname));
                    // For one latlon case, remember the corrected latitude and longitude field names.
                    if((this->onelatlon)&&(((*i)->getName())==GEOGRIDNAME)) {
                        if((*j)->getName()==LATFIELDNAME) tempcorrectedlatname = (*j)->newname;
                        if((*j)->getName()==LONFIELDNAME) tempcorrectedlonname = (*j)->newname;
                    }
                }
            }
            (*i)->ncvarnamelist = tempncvarnamelist;
            tempncvarnamelist.clear();
        }

        // for one lat/lon case, we have to add the lat/lon field name to other grids.
        // We know the original lat and lon names. So just retrieve the corrected lat/lon names from
        // the geo grid(GEOGRIDNAME).

        if(this->onelatlon) {
            for(vector<GridDataset *>::const_iterator i = this->grids.begin();
                i != this->grids.end(); ++i){
                if((*i)->getName()!=GEOGRIDNAME){// Lat/lon names must be in this group.
                    HDFCFUtil::insert_map((*i)->ncvarnamelist, LATFIELDNAME, tempcorrectedlatname);
                    HDFCFUtil::insert_map((*i)->ncvarnamelist, LONFIELDNAME, tempcorrectedlonname);
                }
            }
        }

        // G5. Create a map for dimension name < original dimension name, corrected dimension name>
        map<string,string>tempndimnamelist;//the original dimension name to the corrected dimension name

#if 0
        //// ***** Handling the dimension name clashing ******************

        vector <string>tempalldimnamelist;
        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i)
            for (map<string,string>::const_iterator j =
                (*i)->dimcvarlist.begin(); j!= (*i)->dimcvarlist.end();++j)
                tempalldimnamelist.push_back(HDFCFUtil::get_CF_string((*j).first));

        HDFCFUtil::Handle_NameClashing(tempalldimnamelist);
#endif
      
        // Since DIMXNAME and DIMYNAME are not in the original dimension name list, we use the dimension name,field map 
        // we just formed. 

        int total_dcounter = 0;
        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i){

            for (map<string,string>::const_iterator j =
                (*i)->dimcvarlist.begin(); j!= (*i)->dimcvarlist.end();++j){

                // We have to handle DIMXNAME and DIMYNAME separately.
                if((DIMXNAME == (*j).first || DIMYNAME == (*j).first) && (true==(this->onelatlon))) 
                    HDFCFUtil::insert_map(tempndimnamelist, (*j).first,(*j).first);
                else
                    HDFCFUtil::insert_map(tempndimnamelist, (*j).first, tempalldimnamelist[total_dcounter]);
                total_dcounter++;
            }

            (*i)->ndimnamelist = tempndimnamelist;
            tempndimnamelist.clear();   
        }

#endif
#if 0
        // G6. Revisit the lat/lon fields to check if 1-D COARD convention needs to be followed.
        vector<Dimension*> correcteddims;
        string tempcorrecteddimname;
        map<string,string> tempnewxdimnamelist;
        map<string,string> tempnewydimnamelist;
     
        Dimension *correcteddim;
     
        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i){
            for (vector<Field *>::const_iterator j =
                (*i)->getDataFields().begin();
                j != (*i)->getDataFields().end(); ++j) {
                // Now handling COARD cases, since latitude/longitude can be either 1-D or 2-D array. 
                // So we need to correct both cases.
                if((*j)->getName()==LATFIELDNAME && (*j)->getRank()==2 &&(*j)->condenseddim) {// 2-D lat to 1-D COARD lat

                    string templatdimname;
                    map<string,string>::iterator tempmapit;

                    // Find the new name of LATFIELDNAME
                    tempmapit = (*i)->ncvarnamelist.find(LATFIELDNAME);
                    if(tempmapit != (*i)->ncvarnamelist.end()) 
                        templatdimname= tempmapit->second;
                    else 
                        throw2("cannot find the corrected field of Latitude", (*i)->getName());

                    for(vector<Dimension *>::const_iterator k =(*j)->getDimensions().begin();
                        k!=(*j)->getDimensions().end();++k){
                        if((*k)->getName()==DIMYNAME) {// This needs to be latitude
                            correcteddim = new Dimension(templatdimname,(*k)->getSize());
                            correcteddims.push_back(correcteddim);
                            (*j)->setCorrectedDimensions(correcteddims);
                            HDFCFUtil::insert_map(tempnewydimnamelist, (*i)->getName(), templatdimname);
                        }
                    }
                    (*j)->iscoard = true;
                    (*i)->iscoard = true;
                    if(this->onelatlon) 
                        this->iscoard = true;

                    correcteddims.clear();//clear the local temporary vector
                }
                else if((*j)->getName()==LONFIELDNAME && (*j)->getRank()==2 &&(*j)->condenseddim){// 2-D lon to 1-D COARD lat
               
                    string templondimname;
                    map<string,string>::iterator tempmapit;

                    // Find the new name of LONFIELDNAME
                    tempmapit = (*i)->ncvarnamelist.find(LONFIELDNAME);
                    if(tempmapit != (*i)->ncvarnamelist.end()) 
                        templondimname= tempmapit->second;
                    else 
                        throw2("cannot find the corrected field of Longitude", (*i)->getName());

                    for(vector<Dimension *>::const_iterator k =(*j)->getDimensions().begin();
                        k!=(*j)->getDimensions().end();++k){

                        if((*k)->getName()==DIMXNAME) {// This needs to be longitude
                            correcteddim = new Dimension(templondimname,(*k)->getSize());
                            correcteddims.push_back(correcteddim);
                            (*j)->setCorrectedDimensions(correcteddims);
                            HDFCFUtil::insert_map(tempnewxdimnamelist, (*i)->getName(), templondimname);
                        }
                    }

                    (*j)->iscoard = true;
                    (*i)->iscoard = true;
                    if(this->onelatlon) 
                        this->iscoard = true;
                    correcteddims.clear();
                }
                else if(((*j)->getRank()==1) &&((*j)->getName()==LONFIELDNAME) ) {// 1-D lon to 1-D COARD lon

                    string templondimname;
                    map<string,string>::iterator tempmapit;

                    // Find the new name of LONFIELDNAME
                    tempmapit = (*i)->ncvarnamelist.find(LONFIELDNAME);
                    if(tempmapit != (*i)->ncvarnamelist.end()) 
                        templondimname= tempmapit->second;
                    else 
                        throw2("cannot find the corrected field of Longitude", (*i)->getName());

                    correcteddim = new Dimension(templondimname,((*j)->getDimensions())[0]->getSize());
                    correcteddims.push_back(correcteddim);
                    (*j)->setCorrectedDimensions(correcteddims);
                    (*j)->iscoard = true;
                    (*i)->iscoard = true;
                    if(this->onelatlon) 
                        this->iscoard = true; 
                    correcteddims.clear();

                    if(((((*j)->getDimensions())[0]->getName()!=DIMXNAME))
                        &&((((*j)->getDimensions())[0]->getName())!=DIMYNAME)){
                        throw3("the dimension name of longitude should not be ",
                              ((*j)->getDimensions())[0]->getName(),(*i)->getName()); 
                    }
                    if((((*j)->getDimensions())[0]->getName())==DIMXNAME) {
                        HDFCFUtil::insert_map(tempnewxdimnamelist, (*i)->getName(), templondimname);
                    }
                    else {
                        HDFCFUtil::insert_map(tempnewydimnamelist, (*i)->getName(), templondimname);
                    }
                }
                else if(((*j)->getRank()==1) &&((*j)->getName()==LATFIELDNAME) ) {// 1-D lon to 1-D  COARD lon

                    string templatdimname;
                    map<string,string>::iterator tempmapit;

                    // Find the new name of LATFIELDNAME
                    tempmapit = (*i)->ncvarnamelist.find(LATFIELDNAME);
                    if(tempmapit != (*i)->ncvarnamelist.end()) 
                        templatdimname= tempmapit->second;
                    else 
                        throw2("cannot find the corrected field of Latitude", (*i)->getName());

                    correcteddim = new Dimension(templatdimname,((*j)->getDimensions())[0]->getSize());
                    correcteddims.push_back(correcteddim);
                    (*j)->setCorrectedDimensions(correcteddims);
              
                    (*j)->iscoard = true;
                    (*i)->iscoard = true;
                    if(this->onelatlon) 
                        this->iscoard = true;
                    correcteddims.clear();

                    if(((((*j)->getDimensions())[0]->getName())!=DIMXNAME)
                        &&((((*j)->getDimensions())[0]->getName())!=DIMYNAME))
                        throw3("the dimension name of latitude should not be ",
                              ((*j)->getDimensions())[0]->getName(),(*i)->getName());
                    if((((*j)->getDimensions())[0]->getName())==DIMXNAME){
                        HDFCFUtil::insert_map(tempnewxdimnamelist, (*i)->getName(), templatdimname);
                    }
                    else {
                        HDFCFUtil::insert_map(tempnewydimnamelist, (*i)->getName(), templatdimname);
                    }
                }
                else {
                    ;
                }
            }
        }
      
        // G7. If COARDS follows, apply the new DIMXNAME and DIMYNAME name to the  ndimnamelist 
        if(this->onelatlon){ //One lat/lon for all grids .
            if(this->iscoard){ // COARDS is followed.
                // For this case, only one pair of corrected XDim and YDim for all grids.
                string tempcorrectedxdimname;
                string tempcorrectedydimname;

                if((int)(tempnewxdimnamelist.size())!= 1) 
                    throw1("the corrected dimension name should have only one pair");
                if((int)(tempnewydimnamelist.size())!= 1) 
                    throw1("the corrected dimension name should have only one pair");

                map<string,string>::iterator tempdimmapit = tempnewxdimnamelist.begin();
                tempcorrectedxdimname = tempdimmapit->second;      
                tempdimmapit = tempnewydimnamelist.begin();
                tempcorrectedydimname = tempdimmapit->second;
       
                for (vector<GridDataset *>::const_iterator i = this->grids.begin();
                     i != this->grids.end(); ++i){

                    // Find the DIMXNAME and DIMYNAME in the dimension name list.  
                    map<string,string>::iterator tempmapit;
                    tempmapit = (*i)->ndimnamelist.find(DIMXNAME);
                    if(tempmapit != (*i)->ndimnamelist.end()) {
                        HDFCFUtil::insert_map((*i)->ndimnamelist, DIMXNAME, tempcorrectedxdimname);
                    }
                    else 
                        throw2("cannot find the corrected dimension name", (*i)->getName());
                    tempmapit = (*i)->ndimnamelist.find(DIMYNAME);
                    if(tempmapit != (*i)->ndimnamelist.end()) {
                        HDFCFUtil::insert_map((*i)->ndimnamelist, DIMYNAME, tempcorrectedydimname);
                    }
                    else 
                        throw2("cannot find the corrected dimension name", (*i)->getName());
                }
            }
        }
        else {// We have to search each grid
            for (vector<GridDataset *>::const_iterator i = this->grids.begin();
                i != this->grids.end(); ++i){
                if((*i)->iscoard){
                    string tempcorrectedxdimname;
                    string tempcorrectedydimname;

                    // Find the DIMXNAME and DIMYNAME in the dimension name list.
                    map<string,string>::iterator tempdimmapit;
                    map<string,string>::iterator tempmapit;
                    tempdimmapit = tempnewxdimnamelist.find((*i)->getName());
                    if(tempdimmapit != tempnewxdimnamelist.end()) 
                        tempcorrectedxdimname = tempdimmapit->second;
                    else 
                        throw2("cannot find the corrected COARD XDim dimension name", (*i)->getName());
                    tempmapit = (*i)->ndimnamelist.find(DIMXNAME);
                    if(tempmapit != (*i)->ndimnamelist.end()) {
                        HDFCFUtil::insert_map((*i)->ndimnamelist, DIMXNAME, tempcorrectedxdimname);
                    }
                    else 
                        throw2("cannot find the corrected dimension name", (*i)->getName());

                    tempdimmapit = tempnewydimnamelist.find((*i)->getName());
                    if(tempdimmapit != tempnewydimnamelist.end()) 
                        tempcorrectedydimname = tempdimmapit->second;
                    else 
                        throw2("cannot find the corrected COARD YDim dimension name", (*i)->getName());

                    tempmapit = (*i)->ndimnamelist.find(DIMYNAME);
                    if(tempmapit != (*i)->ndimnamelist.end()) {
                        HDFCFUtil::insert_map((*i)->ndimnamelist, DIMYNAME, tempcorrectedydimname);
                    }
                    else 
                        throw2("cannot find the corrected dimension name", (*i)->getName());
                }
            }
        }

      
        //G8. For 1-D lat/lon cases, Make the third (other than lat/lon coordinate variable) dimension to follow COARD conventions. 

        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i){
            for (map<string,string>::const_iterator j =
                (*i)->dimcvarlist.begin(); j!= (*i)->dimcvarlist.end();++j){

                // It seems that the condition for onelatlon case is if(this->iscoard) is true instead if
                // this->onelatlon is true.So change it. KY 2010-7-4
                //if((this->onelatlon||(*i)->iscoard) && (*j).first !=DIMXNAME && (*j).first !=DIMYNAME) 
                if((this->iscoard||(*i)->iscoard) && (*j).first !=DIMXNAME && (*j).first !=DIMYNAME) {
                     string tempnewdimname;
                    map<string,string>::iterator tempmapit;

                    // Find the new field name of the corresponding dimennsion name 
                    tempmapit = (*i)->ncvarnamelist.find((*j).second);
                    if(tempmapit != (*i)->ncvarnamelist.end()) 
                        tempnewdimname= tempmapit->second;
                    else 
                        throw3("cannot find the corrected field of ", (*j).second,(*i)->getName());

                    // Make the new field name to the correponding dimension name 
                    tempmapit =(*i)->ndimnamelist.find((*j).first);
                    if(tempmapit != (*i)->ndimnamelist.end()) 
                        HDFCFUtil::insert_map((*i)->ndimnamelist, (*j).first, tempnewdimname);
                    else 
                        throw3("cannot find the corrected dimension name of ", (*j).first,(*i)->getName());

                }
            }
        }
        // The following code may be usedful for future more robust third-dimension support
 
        //  G9. Create the corrected dimension vectors.
        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i){ 

            for (vector<Field *>::const_iterator j =
                (*i)->getDataFields().begin();
                j != (*i)->getDataFields().end(); ++j) {

                if((*j)->iscoard == false) {// the corrected dimension name of lat/lon have been updated.

                    // Just obtain the corrected dim names  and save the corrected dimensions for each field.
                    for(vector<Dimension *>::const_iterator k=(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){

                        //tempcorrecteddimname =(*i)->ndimnamelist((*k)->getName());
                        map<string,string>::iterator tempmapit;

                        // Find the new name of this field
                        tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                        if(tempmapit != (*i)->ndimnamelist.end()) 
                            tempcorrecteddimname= tempmapit->second;
                        else 
                            throw4("cannot find the corrected dimension name", (*i)->getName(),(*j)->getName(),(*k)->getName());
                        correcteddim = new Dimension(tempcorrecteddimname,(*k)->getSize());
                        correcteddims.push_back(correcteddim);
                    }
                    (*j)->setCorrectedDimensions(correcteddims);
                    correcteddims.clear();
                }
            }
        }
        // G10. Create "coordinates" ,"units"  attributes. The "units" attributes only apply to latitude and longitude.
        // This is the last round of looping through everything, 
        // we will match dimension name list to the corresponding dimension field name 
        // list for every field. 
        // Since we find some swath files don't specify fillvalue when -9999.0 is found in the real data,
        // we specify fillvalue for those fields. This is not in this routine. This is entirely artifical and we will evaluate this approach. KY 2010-3-3
           
        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i){
            for (vector<Field *>::const_iterator j =
                (*i)->getDataFields().begin();
                j != (*i)->getDataFields().end(); ++j) {
                 
                // Real fields: adding coordinate attributesinate attributes
                if((*j)->fieldtype == 0)  {
                    string tempcoordinates="";
                    string tempfieldname="";
                    string tempcorrectedfieldname="";
                    int tempcount = 0;
                    for(vector<Dimension *>::const_iterator k=(*j)->getDimensions().begin();
                        k!=(*j)->getDimensions().end();++k){

                        // handle coordinates attributes
                        map<string,string>::iterator tempmapit;
                        map<string,string>::iterator tempmapit2;
              
                        // Find the dimension field name
                        tempmapit = ((*i)->dimcvarlist).find((*k)->getName());
                        if(tempmapit != ((*i)->dimcvarlist).end()) 
                            tempfieldname = tempmapit->second;
                        else 
                            throw4("cannot find the dimension field name",
                                   (*i)->getName(),(*j)->getName(),(*k)->getName());

                        // Find the corrected dimension field name
                        tempmapit2 = ((*i)->ncvarnamelist).find(tempfieldname);
                        if(tempmapit2 != ((*i)->ncvarnamelist).end()) 
                            tempcorrectedfieldname = tempmapit2->second;
                        else 
                            throw4("cannot find the corrected dimension field name",
                                   (*i)->getName(),(*j)->getName(),(*k)->getName());

                        if(tempcount == 0) 
                            tempcoordinates= tempcorrectedfieldname;
                        else 
                            tempcoordinates = tempcoordinates +" "+tempcorrectedfieldname;
                        tempcount++;
                    }
                    (*j)->setCoordinates(tempcoordinates);
                }

                // Add units for latitude and longitude
                if((*j)->fieldtype == 1) {// latitude,adding the "units" attribute  degrees_east.
                    string tempunits = "degrees_north";
                    (*j)->setUnits(tempunits);
                }
                if((*j)->fieldtype == 2) { // longitude, adding the units of 
                    string tempunits = "degrees_east";
                    (*j)->setUnits(tempunits);
                }

                // Add units for Z-dimension, now it is always "level"
                // This also needs to be corrected since the Z-dimension may not always be "level".
                // KY 2012-6-13
                // We decide not to touch "units" when the Z-dimension is an existing field(fieldtype =3).
  
                if(((*j)->fieldtype == 4)) {
                    string tempunits ="level";
                    (*j)->setUnits(tempunits);
                }

            
                // The units of the time may not be right. KY 2012-6-13
                if(((*j)->fieldtype == 5)) {
                    string tempunits ="days since 1900-01-01 00:00:00";
                    (*j)->setUnits(tempunits);
                }

                // We meet a really special case for CERES TRMM data. We attribute it to the specialformat 2 case
                // since the corner coordinate is set to default in HDF-EOS2 structmetadata. We also find that there are
                // values such as 3.4028235E38 that is the maximum single precision floating point value. This value
                // is a fill value but the fillvalue attribute is not set. So we add the fillvalue attribute for this case.
                // We may find such cases for other products and will tackle them also.
                if (true == (*i)->addfvalueattr) {
                    if((((*j)->getFillValue()).empty()) && ((*j)->getType()==DFNT_FLOAT32 )) {
                        float tempfillvalue = HUGE;
                        (*j)->addFillValue(tempfillvalue);
                        (*j)->setAddedFillValue(true);
                    }
                }
            }
        }

        // G11: Special handling SOM projection
        // Since the latitude and longitude of the SOM projection are 3-D. 
        // Based on our current understanding, the third dimension size is always 180. 
        // If the size is not 180, the latitude and longitude will not be calculated correctly.
        // This is according to the MISR Lat/lon calculation document 
        // at http://eosweb.larc.nasa.gov/PRODOCS/misr/DPS/DPS_v50_RevS.pdf
        // KY 2012-6-12

        for (vector<GridDataset *>::const_iterator i = this->grids.begin();
            i != this->grids.end(); ++i){
            if (GCTP_SOM == (*i)->getProjection().getCode()) {
                
                // 0. Getting the SOM dimension for latitude and longitude.
                string som_dimname;
                for(vector<Dimension *>::const_iterator j=(*i)->getDimensions().begin();
                    j!=(*i)->getDimensions().end();++j){
                    // NBLOCK is from misrproj.h. It is the number of block that MISR team support for the SOM projection.
                    if(NBLOCK == (*j)->getSize()) {
                        // To make sure we catch the right dimension, check the first three characters of the dim. name
                        // It should be SOM
                        
                        if ((*j)->getName().compare(0,3,"SOM") == 0) {
                            som_dimname = (*j)->getName();
                            break;
                        }
                    }
                }

                if(""== som_dimname) 
                    throw4("Wrong number of block: The number of block of MISR SOM Grid ",
                            (*i)->getName()," is not ",NBLOCK);

                map<string,string>::iterator tempmapit;

                // Find the new name of this field
                string cor_som_dimname;
                string cor_som_cvname;
                tempmapit = (*i)->ndimnamelist.find(som_dimname);
                if(tempmapit != (*i)->ndimnamelist.end()) 
                    cor_som_dimname = tempmapit->second;
                else 
                    throw2("cannot find the corrected dimension name for ", som_dimname);


                // Here we cannot use getDataFields() since the returned elements cannot be modified. KY 2012-6-12
                for (vector<Field *>::iterator j = (*i)->datafields.begin();
                    j != (*i)->datafields.end(); ++j) {
                    
                    // Only 6-7 fields, so just loop through 
                    // 1. Set the SOM dimension for latitude and longitude
                    if (1 == (*j)->fieldtype || 2 == (*j)->fieldtype) {
                        
                        Dimension *newdim = new Dimension(som_dimname,NBLOCK);
                        Dimension *newcor_dim = new Dimension(cor_som_dimname,NBLOCK);
                        vector<Dimension *>::iterator it_d;

                        it_d = (*j)->dims.begin();
                        (*j)->dims.insert(it_d,newdim);

                        it_d = (*j)->correcteddims.begin();
                        (*j)->correcteddims.insert(it_d,newcor_dim);

                    } 

                    // 2. Remove the added coordinate variable for the SOM dimension
                    // The added variable is a variable with the nature number
                    if ( 4 == (*j)->fieldtype) {
                        cor_som_cvname = (*j)->newname;
                        delete (*j);
                        (*i)->datafields.erase(j);
                        // When erasing the iterator, the iterator will automatically go to the next element, so we need to go back 1 in order not to miss the next element.
                        j--;
                    }
                }

                // 3. Fix the "coordinates" attribute: remove the SOM CV name from the coordinate attribute. 
                // Notice this is a little inefficient. Since we only have a few fields and non-SOM projection products
                // won't be affected, and more importantly, to keep the SOM projection handling in a central place,
                // I handle the adjustment of "coordinates" attribute here. KY 2012-6-12

                // MISR data cannot be visualized by Panoply and IDV. So the coordinates attribute
                // here reflects the coordinates of this variable more accurately. KY 2012-6-13 

                for (vector<Field *>::const_iterator j = (*i)->getDataFields().begin();
                    j != (*i)->getDataFields().end(); ++j) {

                    if ( 0 == (*j)->fieldtype) {

                        string temp_coordinates = (*j)->coordinates; 
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

                        (*j)->setCoordinates(temp_coordinates);   

                    }
                }
            }
        }
#endif
    }// End of handling grid

    // Check and set the scale type
    for(vector<GridDataset *>::const_iterator i = this->grids.begin();
        i != this->grids.end(); ++i){
        (*i)->SetScaleType((*i)->name);
    }
    
    if(numgrid==0) {
  
        // Now we handle swath case. 
        if (numswath > 0) {

           int tempnumdm = obtain_dimmap_num(numswath);
           create_swath_latlon_dim_cvar_map(tempnumdm);
           create_swath_nonll_dim_cvar_map();
           handle_swath_dim_cvar_maps(tempnumdm);
           handle_swath_cf_attrs();
 
#if 0
            // S(wath)0. Check if there are dimension maps in this this.
            int tempnumdm = 0;
            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i){
                tempnumdm += (*i)->get_num_map();
                if (tempnumdm >0) 
                    break;
            }

            // MODATML2 and MYDATML2 in year 2010 include dimension maps. But the dimension map
            // is not used. Furthermore, they provide additional latitude/longtiude 
            // for 10 KM under the data field. So we have to handle this differently.
            // MODATML2 in year 2000 version doesn't include dimension map, so we 
            // have to consider both with dimension map and without dimension map cases.
            // The swath name is atml2.

            bool fakedimmap = false;

            if(numswath == 1) {// Start special atml2-like handling
                if((this->swaths[0]->getName()).find("atml2")!=string::npos){
                    if(tempnumdm >0) fakedimmap = true;
                    int templlflag = 0;
                    for (vector<Field *>::const_iterator j =
                             this->swaths[0]->getGeoFields().begin();
                         j != this->swaths[0]->getGeoFields().end(); ++j) {
                        if((*j)->getName() == "Latitude" || (*j)->getName() == "Longitude") {
                            if ((*j)->getType() == DFNT_UINT16 ||
                                (*j)->getType() == DFNT_INT16)
                                (*j)->type = DFNT_FLOAT32;
                            templlflag ++;
                            if(templlflag == 2) 
                                break;
                        }
                    }

                    templlflag = 0;

                    for (vector<Field *>::const_iterator j =
                        this->swaths[0]->getDataFields().begin();
                        j != this->swaths[0]->getDataFields().end(); ++j) {

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

                        if(((*j)->getName()).find("Latitude") != string::npos){
                            if ((*j)->getType() == DFNT_UINT16 ||
                                (*j)->getType() == DFNT_INT16)
                                (*j)->type = DFNT_FLOAT32;
                            (*j)->fieldtype = 1;
                            // Also need to link the dimension to the coordinate variable list
                            if((*j)->getRank() != 2) 
                                throw2("The lat/lon rank must be  2 for Java clients to work",
                                       (*j)->getRank());
                            HDFCFUtil::insert_map(this->swaths[0]->dimcvarlist, 
                                       (((*j)->getDimensions())[0])->getName(),(*j)->getName());
                            templlflag ++;
                        }
                        if(((*j)->getName()).find("Longitude")!= string::npos) {
                            if((*j)->getType() == DFNT_UINT16 ||
                                (*j)->getType() == DFNT_INT16)
                                (*j)->type = DFNT_FLOAT32;

                            (*j)->fieldtype = 2;
                            if((*j)->getRank() != 2) 
                                throw2("The lat/lon rank must be  2 for Java clients to work",
                                       (*j)->getRank());
                            HDFCFUtil::insert_map(this->swaths[0]->dimcvarlist, 
                                       (((*j)->getDimensions())[1])->getName(), (*j)->getName());
                            templlflag ++;
                        }
                        if(templlflag == 2) 
                            break;
                    }
                }
            }// End of special atml2 handling

            // Although this file includes dimension map, it doesn't use it at all. So change
            // tempnumdm to 0.
            if(fakedimmap) 
                tempnumdm = 0;


            // S1. Prepare the right dimension name and the dimension field list for each swath. 
            // The assumption is that within a swath, the dimension name is unique.
            //  The dimension field name(even with the added Z-like field) is unique. 
            // A map <dimension name, dimension field name> will be created.
            // The name clashing handling for multiple swaths will not be done in this step. 

            // S1.1 Obtain the dimension names corresponding to the latitude and longitude,save them to the <dimname, dimfield> map.

            // We found a special MODAPS product: the Latitude and Longitude are put under the Data fields rather than GeoLocation fields.
            // So we need to go to the "Data Fields" to grab the "Latitude and Longitude ".

            bool lat_in_geofields = false;
            bool lon_in_geofields = false;

            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i){

                int tempgeocount = 0;
                for (vector<Field *>::const_iterator j =
                    (*i)->getGeoFields().begin();
                    j != (*i)->getGeoFields().end(); ++j) {

                    // Here we assume it is always lat[f0][f1] and lon [f0][f1]. No lat[f0][f1] and lon[f1][f0] occur.
                    // So far only "Latitude" and "Longitude" are used as standard names of Lat and lon for swath.
                    if((*j)->getName()=="Latitude" ){
                        if((*j)->getRank() > 2) 
                            throw2("Currently the lat/lon rank must be 1 or 2 for Java clients to work",
                                   (*j)->getRank());

                        lat_in_geofields = true;
                        // Since under our assumption, lat/lon are always 2-D for a swath and 
                        // dimension order doesn't matter for Java clients,
                        // we always map Latitude the first dimension and longitude the second dimension.
                        // Save this information in the coordinate variable name and field map.
                        // For rank =1 case, we only handle the cross-section along the same 
                        // longitude line. So Latitude should be the dimension name.

                        HDFCFUtil::insert_map((*i)->dimcvarlist, (((*j)->getDimensions())[0])->getName(), "Latitude");

                        // Have dimension map, we want to remember the dimension and remove it from the list.
                        if(tempnumdm >0) {                    

                            // We have to loop through the dimension map
                            for(vector<SwathDataset::DimensionMap *>::const_iterator 
                                l=(*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){

                                // This dimension name will be replaced by the mapped dimension name, 
                                // the mapped dimension name can be obtained from the getDataDimension() method.
                                if(((*j)->getDimensions()[0])->getName() == (*l)->getGeoDimension()) {
                                    HDFCFUtil::insert_map((*i)->dimcvarlist, (*l)->getDataDimension(), "Latitude");
                                    break;
                                }
                            }
                        }
                        (*j)->fieldtype = 1;
                        tempgeocount ++;
                    }
                    if((*j)->getName()=="Longitude"){
                        if((*j)->getRank() > 2) 
                            throw2("Currently the lat/lon rank must be  1 or 2 for Java clients to work",
                                    (*j)->getRank());
                        // Only lat-level cross-section(for Panoply)is supported 
                        // when longitude/latitude is 1-D, so ignore the longitude as the dimension field.

                        lon_in_geofields = true;
                        if((*j)->getRank() == 1) {
                            tempgeocount++;
                            continue;
                        }
                        // Since under our assumption, lat/lon are almost always 2-D for 
                        // a swath and dimension order doesn't matter for Java clients,
                        // we always map Latitude the first dimension and longitude the second dimension.
                        // Save this information in the dimensiion name and coordinate variable map.
                        HDFCFUtil::insert_map((*i)->dimcvarlist, 
                                              (((*j)->getDimensions())[1])->getName(), "Longitude");
                        if(tempnumdm >0) {

                            // We have to loop through the dimension map
                            for(vector<SwathDataset::DimensionMap *>::const_iterator 
                                l=(*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){

                                // This dimension name will be replaced by the mapped dimension name,
                                // This name can be obtained by getDataDimension() fuction of 
                                // dimension map class. 
                                if(((*j)->getDimensions()[1])->getName() == 
                                    (*l)->getGeoDimension()) {
                                    HDFCFUtil::insert_map((*i)->dimcvarlist, 
                                               (*l)->getDataDimension(), "Longitude");
                                    break;
                                }
                            }
                        }
                        (*j)->fieldtype = 2;
                        tempgeocount++;
                    }
                    if(tempgeocount == 2) 
                        break;
                }
            }// end of creating the <dimname,dimfield> map.

            // If lat and lon are not together, throw an error.
            if (lat_in_geofields ^ lon_in_geofields) 
                throw1("Latitude and longitude must be both under Geolocation fields or Data fields");
            
            if (!lat_in_geofields && !lon_in_geofields) {// Check if they are under data fields

                bool lat_in_datafields = false;
                bool lon_in_datafields = false;

                for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                    i != this->swaths.end(); ++i){

                    int tempgeocount = 0;
                    for (vector<Field *>::const_iterator j =
                         (*i)->getDataFields().begin();
                         j != (*i)->getDataFields().end(); ++j) {

                        // Here we assume it is always lat[f0][f1] and lon [f0][f1]. 
                        // No lat[f0][f1] and lon[f1][f0] occur.
                        // So far only "Latitude" and "Longitude" are used as 
                        // standard names of Lat and lon for swath.
                        if((*j)->getName()=="Latitude" ){
                            if((*j)->getRank() > 2) { 
                                throw2("Currently the lat/lon rank must be 1 or 2 for Java clients to work",
                                       (*j)->getRank());
                            }
                            lat_in_datafields = true;
                            // Since under our assumption, lat/lon are always 2-D 
                            // for a swath and dimension order doesn't matter for Java clients,
                            // we always map Latitude the first dimension and longitude the second dimension.
                            // Save this information in the coordinate variable name and field map.
                            // For rank =1 case, we only handle the cross-section along the same longitude line. 
                            // So Latitude should be the dimension name.

                            HDFCFUtil::insert_map((*i)->dimcvarlist, 
                                              (((*j)->getDimensions())[0])->getName(), "Latitude");

                            // Have dimension map, we want to remember the dimension and remove it from the list.
                            if(tempnumdm >0) {                    

                                // We have to loop through the dimension map
                                for(vector<SwathDataset::DimensionMap *>::const_iterator 
                                    l=(*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){

                                    // This dimension name will be replaced by the mapped dimension name, 
                                    // the mapped dimension name can be obtained from the getDataDimension() method.
                                    if(((*j)->getDimensions()[0])->getName() == (*l)->getGeoDimension()) {
                                        HDFCFUtil::insert_map((*i)->dimcvarlist, (*l)->getDataDimension(), "Latitude");
                                        break;
                                    }
                                }
                            }
                            (*j)->fieldtype = 1;
                            tempgeocount ++;
                        }
                        if((*j)->getName()=="Longitude"){
                            if((*j)->getRank() > 2) { 
                                throw2("Currently the lat/lon rank must be  1 or 2 for Java clients to work",
                                        (*j)->getRank());
                            }
                            // Only lat-level cross-section(for Panoply)is supported when 
                            // longitude/latitude is 1-D, so ignore the longitude as the dimension field.

                            lon_in_datafields = true;
                            if((*j)->getRank() == 1) {
                                tempgeocount++;
                                continue;
                            }
                            // Since under our assumption, 
                            // lat/lon are almost always 2-D for a swath and dimension order doesn't matter for Java clients,
                            // we always map Latitude the first dimension and longitude the second dimension.
                            // Save this information in the dimensiion name and coordinate variable map.
                            HDFCFUtil::insert_map((*i)->dimcvarlist, 
                                                  (((*j)->getDimensions())[1])->getName(), "Longitude");
                            if(tempnumdm >0) {
                                // We have to loop through the dimension map
                                for(vector<SwathDataset::DimensionMap *>::const_iterator 
                                    l=(*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){

                                    // This dimension name will be replaced by the mapped dimension name,
                                    // This name can be obtained by getDataDimension() fuction of dimension map class. 
                                    if(((*j)->getDimensions()[1])->getName() == (*l)->getGeoDimension()) {
                                        HDFCFUtil::insert_map((*i)->dimcvarlist, 
                                                              (*l)->getDataDimension(), "Longitude");
                                        break;
                                    }
                                }
                            }
                            (*j)->fieldtype = 2;
                            tempgeocount++;
                        }
                        if(tempgeocount == 2) 
                            break;
                    }
                }// end of creating the <dimname,dimfield> map.

                // If lat and lon are not together, throw an error.
                if (lat_in_datafields ^ lon_in_datafields) 
                    throw1("Latitude and longitude must be both under Geolocation fields or Data fields");

                // If lat,lon are not found under either "Data fields" or "Geolocation fields", we should not generate "coordiantes"
                // However, this case should be handled in the future release. KY 2012-09-24
                //**************** INVESTIGATE in the NEXT RELEASE ******************************
                //if (!lat_in_datafields && !lon_in_datafields)
                //  throw1("Latitude and longitude don't exist");
                //*********************************************************************************/
               
            }
          
            // S1.3 Handle existing and missing fields 
            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i){
                
                // Since we find multiple 1-D fields with the same dimension names for some Swath files(AIRS level 1B),
                // we currently always treat the third dimension field as a missing field, this may be corrected later.
                // Corrections for the above: MODIS data do include the unique existing Z fields, so we have to search
                // the existing Z field. KY 2010-8-11
                // One possible correction is to search all 1-D fields with the same dimension name within one swath,
                // if only one field is found, we use this field  as the third dimension.
                // 1.1 Add the missing Z-dimension field.
                // Some dimension name's corresponding fields are missing, 
                // so add the missing Z-dimension fields based on the dimension name. When the real
                // data is read, nature number 1,2,3,.... will be filled!
                // NOTE: The latitude and longitude dim names are not handled yet.  
               
                // S1.2.1 Build a unique 1-D dimension name list.Now the list only includes dimension names of "latitude" and "longitude".
    
                pair<set<string>::iterator,bool> tempdimret;
                for(map<string,string>::const_iterator j = (*i)->dimcvarlist.begin(); 
                    j!= (*i)->dimcvarlist.end();++j){ 
                    tempdimret = (*i)->nonmisscvdimlist.insert((*j).first);
                }

                // S1.2.2 Search the geofield group and see if there are any existing 1-D Z dimension data.
                //  If 1-D field data with the same dimension name is found under GeoField, we still search if that 1-D field  is the dimension
                // field of a dimension name.
                for (vector<Field *>::const_iterator j =
                    (*i)->getGeoFields().begin();
                    j != (*i)->getGeoFields().end(); ++j) {
             
                    if((*j)->getRank()==1) {
                        if((*i)->nonmisscvdimlist.find((((*j)->getDimensions())[0])->getName()) == (*i)->nonmisscvdimlist.end()){
                            tempdimret = (*i)->nonmisscvdimlist.insert((((*j)->getDimensions())[0])->getName());
                            if((*j)->getName() =="Time") (*j)->fieldtype = 5;// This is for IDV.
                            // This is for temporarily COARD fix. 
                            // For 2-D lat/lon, the third dimension should NOT follow
                            // COARD conventions. It will cause Panoply and IDV failed.
                            // KY 2010-7-21
                            // It turns out that we need to keep the original field name of the third dimension.
                            // So assign the flag and save the original name.
                            // KY 2010-9-9
#if 0
                            if(((((*j)->getDimensions())[0])->getName())==(*j)->getName()){
                                (*j)->oriname = (*j)->getName();
                                // netCDF-Java fixes the problem, now goes back to COARDS.
                                //(*j)->name = (*j)->getName() +"_d";
                                (*j)->specialcoard = true;
                            }
#endif
                            HDFCFUtil::insert_map((*i)->dimcvarlist, (((*j)->getDimensions())[0])->getName(), (*j)->getName());
                            (*j)->fieldtype = 3;

                        }
                    }
                }

                // We will also check the third dimension inside DataFields
                // This may cause potential problems for AIRS data
                // We will double CHECK KY 2010-6-26
                // So far the tests seem okay. KY 2010-8-11
                for (vector<Field *>::const_iterator j =
                    (*i)->getDataFields().begin();
                    j != (*i)->getDataFields().end(); ++j) {

                    if((*j)->getRank()==1) {
                        if((*i)->nonmisscvdimlist.find((((*j)->getDimensions())[0])->getName()) == (*i)->nonmisscvdimlist.end()){
                            tempdimret = (*i)->nonmisscvdimlist.insert((((*j)->getDimensions())[0])->getName());
                            if((*j)->getName() =="Time") (*j)->fieldtype = 5;// This is for IDV.
                            // This is for temporarily COARD fix. 
                            // For 2-D lat/lon, the third dimension should NOT follow
                            // COARD conventions. It will cause Panoply and IDV failed.
                            // KY 2010-7-21
#if 0
                            if(((((*j)->getDimensions())[0])->getName())==(*j)->getName()){
                                (*j)->oriname = (*j)->getName();
                                //(*j)->name = (*j)->getName() +"_d";
                                (*j)->specialcoard = true;
                            }
#endif
                            HDFCFUtil::insert_map((*i)->dimcvarlist, (((*j)->getDimensions())[0])->getName(), (*j)->getName());
                            (*j)->fieldtype = 3;

                        }
                    }
                }


                // S1.2.3 Handle the missing fields 
                // Loop through all dimensions of this swath to search the missing fields
                for (vector<Dimension *>::const_iterator j =
                    (*i)->getDimensions().begin(); j!= (*i)->getDimensions().end();++j){

                    if(((*i)->nonmisscvdimlist.find((*j)->getName())) == (*i)->nonmisscvdimlist.end()){// This dimension needs a field
                      
                        // Need to create a new data field vector element with the name and dimension as above.
                        Field *missingfield = new Field();
                        // This is for temporarily COARD fix. 
                        // For 2-D lat/lon, the third dimension should NOT follow
                        // COARD conventions. It will cause Panoply and IDV failed.
                        // Since Swath is always 2-D lat/lon, so we are okay here. Add a "_d" for each field name.
                        // KY 2010-7-21
                        // netCDF-Java now first follows COARDS, change back
                        // missingfield->name = (*j)->getName()+"_d";
                        missingfield->name = (*j)->getName();
                        missingfield->rank = 1;
                        missingfield->type = DFNT_INT32;//This is an HDF constant.the data type is always integer.
                        Dimension *dim = new Dimension((*j)->getName(),(*j)->getSize());

                        // only 1 dimension
                        missingfield->dims.push_back(dim);

                        // Provide information for the missing data, since we need to calculate the data, so
                        // the information is different than a normal field.
                        int missingdatarank =1;
                        int missingdatatypesize = 4;
                        int missingdimsize[1];
                        missingdimsize[0]= (*j)->getSize();
                        missingfield->fieldtype = 4; //added Z-dimension coordinate variable with nature number

                        // input data should be empty now. We will add the support later.
                        LightVector<char>inputdata;
                        missingfield->data = new 
                                             MissingFieldData(missingdatarank,missingdatatypesize,missingdimsize,inputdata);

                        (*i)->geofields.push_back(missingfield);
                        HDFCFUtil::insert_map((*i)->dimcvarlist, 
                                              (missingfield->getDimensions())[0]->getName(), missingfield->name);
                    }
                }
                (*i)->nonmisscvdimlist.clear();// clear this set.

            }// End of dealing with missing fields
                  
            // Start handling name clashing
            vector <string> tempfieldnamelist;
            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i) {
                 
                // First handle geofield, all dimension fields are under the geofield group.
                for (vector<Field *>::const_iterator j =
                    (*i)->getGeoFields().begin();
                    j != (*i)->getGeoFields().end(); ++j) {
                    tempfieldnamelist.push_back(HDFCFUtil::get_CF_string((*j)->name));   
                }

                for (vector<Field *>::const_iterator j = (*i)->getDataFields().begin();
                    j!= (*i)->getDataFields().end(); ++j) {
                    tempfieldnamelist.push_back(HDFCFUtil::get_CF_string((*j)->name));
                }
            }

            HDFCFUtil::Handle_NameClashing(tempfieldnamelist);

            int total_fcounter = 0;
      
            // S4. Create a map for dimension field name <original field name, corrected field name>
            // Also assure the uniqueness of all field names,save the new field names.
            //the original dimension field name to the corrected dimension field name
            map<string,string>tempncvarnamelist;
            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i){

                // First handle geofield, all dimension fields are under the geofield group.
                for (vector<Field *>::const_iterator j = 
                    (*i)->getGeoFields().begin();
                    j != (*i)->getGeoFields().end(); ++j) 
                {
               
                    (*j)->newname = tempfieldnamelist[total_fcounter];
                    total_fcounter++;

                    if((*j)->fieldtype!=0) {// If this field is a dimension field, save the name/new name pair. 
                        HDFCFUtil::insert_map((*i)->ncvarnamelist, (*j)->getName(), (*j)->newname);
                    }
                }
 
                for (vector<Field *>::const_iterator j = 
                    (*i)->getDataFields().begin();
                    j != (*i)->getDataFields().end(); ++j) 
                {
               
                    (*j)->newname = tempfieldnamelist[total_fcounter];
                    total_fcounter++;

                    if((*j)->fieldtype!=0) {// If this field is a dimension field, save the name/new name pair. 
                        HDFCFUtil::insert_map((*i)->ncvarnamelist, (*j)->getName(), (*j)->newname);
                    }
                }
            } // end of creating a map for dimension field name <original field name, corrected field name>

            // S5. Create a map for dimension name < original dimension name, corrected dimension name>

            vector <string>tempalldimnamelist;

            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i)
                for (map<string,string>::const_iterator j =
                    (*i)->dimcvarlist.begin(); j!= (*i)->dimcvarlist.end();++j)
                    tempalldimnamelist.push_back(HDFCFUtil::get_CF_string((*j).first));

            HDFCFUtil::Handle_NameClashing(tempalldimnamelist);

            int total_dcounter = 0;

            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i){
                for (map<string,string>::const_iterator j =
                    (*i)->dimcvarlist.begin(); j!= (*i)->dimcvarlist.end();++j){
                    HDFCFUtil::insert_map((*i)->ndimnamelist, (*j).first, tempalldimnamelist[total_dcounter]);
                    total_dcounter++;
                }
            }

            //  S6. Create corrected dimension vectors.
            vector<Dimension*> correcteddims;
            string tempcorrecteddimname;
            Dimension *correcteddim;

            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i){ 

                // First the geofield. 
                for (vector<Field *>::const_iterator j =
                    (*i)->getGeoFields().begin();
                    j != (*i)->getGeoFields().end(); ++j) {

                    for(vector<Dimension *>::const_iterator k=(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){

                        map<string,string>::iterator tempmapit;

                        if(tempnumdm == 0) { // No dimension map, just obtain the new dimension name.

                            // Find the new name of this field
                            tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                            if(tempmapit != (*i)->ndimnamelist.end()) 
                                tempcorrecteddimname= tempmapit->second;
                            else 
                                throw4("cannot find the corrected dimension name", 
                                        (*i)->getName(),(*j)->getName(),(*k)->getName());

                            correcteddim = new Dimension(tempcorrecteddimname,(*k)->getSize());
                        }
                        else { 
                            // have dimension map, use the datadim and datadim size to replace the geodim and geodim size. 
                            bool isdimmapname = false;

                            // We have to loop through the dimension map
                            for(vector<SwathDataset::DimensionMap *>::const_iterator 
                                l=(*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){
                                // This dimension name is the geo dimension name in the dimension map, 
                                // replace the name with data dimension name.
                                if((*k)->getName() == (*l)->getGeoDimension()) {

                                    isdimmapname = true;
                                    (*j)->dmap = true;
                                    string temprepdimname = (*l)->getDataDimension();

                                    // Find the new name of this data dimension name
                                    tempmapit = (*i)->ndimnamelist.find(temprepdimname);
                                    if(tempmapit != (*i)->ndimnamelist.end()) 
                                        tempcorrecteddimname= tempmapit->second;
                                    else 
                                        throw4("cannot find the corrected dimension name", (*i)->getName(),
                                                (*j)->getName(),(*k)->getName());
                                    
                                    // Find the size of this data dimension name
                                    // We have to loop through the Dimensions of this swath
                                    bool ddimsflag = false;
                                    for(vector<Dimension *>::const_iterator m=(*i)->getDimensions().begin();
                                        m!=(*i)->getDimensions().end();++m) {
                                        if((*m)->getName() == temprepdimname) { 
                                        // Find the dimension size, create the correcteddim
                                            correcteddim = new Dimension(tempcorrecteddimname,(*m)->getSize());
                                            ddimsflag = true;
                                            break;
                                        }
                                    }
                                    if(!ddimsflag) 
                                        throw4("cannot find the corrected dimension size", (*i)->getName(),
                                                (*j)->getName(),(*k)->getName());
                                    break;
                                }
                            }
                            if(!isdimmapname) { // Still need to assign the corrected dimensions.
                                // Find the new name of this field
                                tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                                if(tempmapit != (*i)->ndimnamelist.end()) 
                                    tempcorrecteddimname= tempmapit->second;
                                else 
                                    throw4("cannot find the corrected dimension name", 
                                            (*i)->getName(),(*j)->getName(),(*k)->getName());

                                correcteddim = new Dimension(tempcorrecteddimname,(*k)->getSize());

                            }
                        }         

                        correcteddims.push_back(correcteddim);
                    }
                    (*j)->setCorrectedDimensions(correcteddims);
                    correcteddims.clear();
                }// End of creating the corrected dimension vectors for GeoFields.
 
                // Then the data field.
                for (vector<Field *>::const_iterator j =
                    (*i)->getDataFields().begin();
                    j != (*i)->getDataFields().end(); ++j) {

                    for(vector<Dimension *>::const_iterator k=
                        (*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){

                        if(tempnumdm == 0) {

                            map<string,string>::iterator tempmapit;

                            // Find the new name of this field
                            tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                            if(tempmapit != (*i)->ndimnamelist.end()) 
                                tempcorrecteddimname= tempmapit->second;
                            else 
                                throw4("cannot find the corrected dimension name", (*i)->getName(),
                                        (*j)->getName(),(*k)->getName());

                            correcteddim = new Dimension(tempcorrecteddimname,(*k)->getSize());
                        }
                        else {
                            map<string,string>::iterator tempmapit;
           
                            bool isdimmapname = false;
                            // We have to loop through dimension map
                            for(vector<SwathDataset::DimensionMap *>::const_iterator l=
                                (*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){
                                // This dimension name is the geo dimension name in the dimension map, 
                                // replace the name with data dimension name.
                                if((*k)->getName() == (*l)->getGeoDimension()) {
                                    isdimmapname = true;
                                    (*j)->dmap = true;
                                    string temprepdimname = (*l)->getDataDimension();
                   
                                    // Find the new name of this data dimension name
                                    tempmapit = (*i)->ndimnamelist.find(temprepdimname);
                                    if(tempmapit != (*i)->ndimnamelist.end()) 
                                        tempcorrecteddimname= tempmapit->second;
                                    else 
                                        throw4("cannot find the corrected dimension name", 
                                                (*i)->getName(),(*j)->getName(),(*k)->getName());
                                    
                                    // Find the size of this data dimension name
                                    // We have to loop through the Dimensions of this swath
                                    bool ddimsflag = false;
                                    for(vector<Dimension *>::const_iterator m=
                                        (*i)->getDimensions().begin();m!=(*i)->getDimensions().end();++m) {

                                        // Find the dimension size, create the correcteddim
                                        if((*m)->getName() == temprepdimname) { 
                                            correcteddim = new Dimension(tempcorrecteddimname,(*m)->getSize());
                                            ddimsflag = true;
                                            break;
                                        }
                                    }
                                    if(!ddimsflag) 
                                        throw4("cannot find the corrected dimension size", 
                                                (*i)->getName(),(*j)->getName(),(*k)->getName());
                                    break;
                                }
 
                            }
                            // Not a dimension with dimension map; Still need to assign the corrected dimensions.
                            if(!isdimmapname) { 

                                // Find the new name of this field
                                tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                                if(tempmapit != (*i)->ndimnamelist.end()) 
                                    tempcorrecteddimname= tempmapit->second;
                                else 
                                    throw4("cannot find the corrected dimension name", 
                                            (*i)->getName(),(*j)->getName(),(*k)->getName());

                                correcteddim = new Dimension(tempcorrecteddimname,(*k)->getSize());

                            }

                        }
                        correcteddims.push_back(correcteddim);
                    }
                    (*j)->setCorrectedDimensions(correcteddims);
                    correcteddims.clear();
                }// End of creating the dimensions for data fields.
            }

            // S7. Create "coordinates" ,"units"  attributes. The "units" attributes only apply to latitude and longitude.
            // This is the last round of looping through everything, 
            // we will match dimension name list to the corresponding dimension field name 
            // list for every field. 
            // Since we find some swath files don't specify fillvalue when -9999.0 is found in the real data,
            // we specify fillvalue for those fields. This is entirely 
            // artifical and we will evaluate this approach. KY 2010-3-3
           
            for (vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                 i != this->swaths.end(); ++i){

                // Handle GeoField first.
                for (vector<Field *>::const_iterator j =
                         (*i)->getGeoFields().begin();
                     j != (*i)->getGeoFields().end(); ++j) {
                 
                    // Real fields: adding the coordinate attribute
                    if((*j)->fieldtype == 0)  {// currently it is always true.
                        string tempcoordinates="";
                        string tempfieldname="";
                        string tempcorrectedfieldname="";
                        int tempcount = 0;
                        for(vector<Dimension *>::const_iterator 
                            k=(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){
                            // handle coordinates attributes
               
                            map<string,string>::iterator tempmapit;
                            map<string,string>::iterator tempmapit2;
              
                            // Find the dimension field name
                            tempmapit = ((*i)->dimcvarlist).find((*k)->getName());
                            if(tempmapit != ((*i)->dimcvarlist).end()) 
                                tempfieldname = tempmapit->second;
                            else 
                                throw4("cannot find the dimension field name",(*i)->getName(),
                                        (*j)->getName(),(*k)->getName());

                            // Find the corrected dimension field name
                            tempmapit2 = ((*i)->ncvarnamelist).find(tempfieldname);
                            if(tempmapit2 != ((*i)->ncvarnamelist).end()) 
                                tempcorrectedfieldname = tempmapit2->second;
                            else 
                                throw4("cannot find the corrected dimension field name",
                                        (*i)->getName(),(*j)->getName(),(*k)->getName());

                            if(tempcount == 0) 
                                tempcoordinates= tempcorrectedfieldname;
                            else 
                                tempcoordinates = tempcoordinates +" "+tempcorrectedfieldname;
                            tempcount++;
                        }
                        (*j)->setCoordinates(tempcoordinates);
                    }


                    // Add units for latitude and longitude
                    // latitude,adding the "units" attribute  degrees_east.
                    if((*j)->fieldtype == 1) {
                        string tempunits = "degrees_north";
                        (*j)->setUnits(tempunits);
                    }

                    // longitude, adding the units of
                    if((*j)->fieldtype == 2) {  
                        string tempunits = "degrees_east";
                        (*j)->setUnits(tempunits);
                    }

                    // Add units for Z-dimension, now it is always "level"
                    // We decide not touch the units if the third-dimension CV exists(fieldtype =3)
                    // KY 2013-02-15
                    //if(((*j)->fieldtype == 3)||((*j)->fieldtype == 4)) {
                    if(((*j)->fieldtype == 4)) {
                        string tempunits ="level";
                        (*j)->setUnits(tempunits);
                    }

                    // Add units for "Time", 
                    // Be aware that it is always "days since 1900-01-01 00:00:00"
                    if(((*j)->fieldtype == 5)) {
                        string tempunits = "days since 1900-01-01 00:00:00";
                        (*j)->setUnits(tempunits);
                    }
                    // Set the fill value for floating type data that doesn't have the fill value.
                    // We found _FillValue attribute is missing from some swath data.
                    // To cover the most cases, an attribute called _FillValue(the value is -9999.0)
                    // is added to the data whose type is float32 or float64.
                    if((((*j)->getFillValue()).empty()) && 
                        ((*j)->getType()==DFNT_FLOAT32 || (*j)->getType()==DFNT_FLOAT64)) { 
                        float tempfillvalue = -9999.0;
                        (*j)->addFillValue(tempfillvalue);
                        (*j)->setAddedFillValue(true);
                    }
                }
 
                // Data fields
                for (vector<Field *>::const_iterator j =
                    (*i)->getDataFields().begin();
                    j != (*i)->getDataFields().end(); ++j) {
                 
                    // Real fields: adding coordinate attributesinate attributes
                    if((*j)->fieldtype == 0)  {// currently it is always true.
                        string tempcoordinates="";
                        string tempfieldname="";
                        string tempcorrectedfieldname="";
                        int tempcount = 0;
                        for(vector<Dimension *>::const_iterator k
                            =(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){
                            // handle coordinates attributes
               
                            map<string,string>::iterator tempmapit;
                            map<string,string>::iterator tempmapit2;
              
                            // Find the dimension field name
                            tempmapit = ((*i)->dimcvarlist).find((*k)->getName());
                            if(tempmapit != ((*i)->dimcvarlist).end()) 
                                tempfieldname = tempmapit->second;
                            else 
                                throw4("cannot find the dimension field name",(*i)->getName(),
                                        (*j)->getName(),(*k)->getName());

                            // Find the corrected dimension field name
                            tempmapit2 = ((*i)->ncvarnamelist).find(tempfieldname);
                            if(tempmapit2 != ((*i)->ncvarnamelist).end()) 
                                tempcorrectedfieldname = tempmapit2->second;
                            else 
                                throw4("cannot find the corrected dimension field name",
                                        (*i)->getName(),(*j)->getName(),(*k)->getName());

                            if(tempcount == 0) 
                                tempcoordinates= tempcorrectedfieldname;
                            else 
                                tempcoordinates = tempcoordinates +" "+tempcorrectedfieldname;
                            tempcount++;
                        }
                        (*j)->setCoordinates(tempcoordinates);
                    }
                    // Add units for Z-dimension, now it is always "level"
                    if(((*j)->fieldtype == 3)||((*j)->fieldtype == 4)) {
                        string tempunits ="level";
                        (*j)->setUnits(tempunits);
                    }

                    // Add units for "Time", Be aware that it is always "days since 1900-01-01 00:00:00"
                    if(((*j)->fieldtype == 5)) {
                        string tempunits = "days since 1900-01-01 00:00:00";
                        (*j)->setUnits(tempunits);
                    }

                    // Set the fill value for floating type data that doesn't have the fill value.
                    // We found _FillValue attribute is missing from some swath data.
                    // To cover the most cases, an attribute called _FillValue(the value is -9999.0)
                    // is added to the data whose type is float32 or float64.
                    if((((*j)->getFillValue()).empty()) && 
                        ((*j)->getType()==DFNT_FLOAT32 || (*j)->getType()==DFNT_FLOAT64)) { 
                        float tempfillvalue = -9999.0;
                        (*j)->addFillValue(tempfillvalue);
                        (*j)->setAddedFillValue(true);
                    }

                }
            }
#endif

            // Check and set the scale type
            for(vector<SwathDataset *>::const_iterator i = this->swaths.begin();
                i != this->swaths.end(); ++i)
                (*i)->SetScaleType((*i)->name);
        }

    }
   
}


void Dataset::SetScaleType(const string EOS2ObjName) throw(Exception) {


    // Group features of MODIS products.
    // Using vector of strings instead of the following. 
    // C++11 may allow the vector of string to be assigned as follows
    // string modis_type1[] = {"L1B", "GEO", "BRDF", "0.05Deg", "Reflectance", "MOD17A2", "North","MOD_Grid_MOD15A2","MODIS_NACP_LAI"};

    vector<string> modis_multi_scale_type;
    modis_multi_scale_type.push_back("L1B");
    modis_multi_scale_type.push_back("GEO");
    modis_multi_scale_type.push_back("BRDF");
    modis_multi_scale_type.push_back("0.05Deg");
    modis_multi_scale_type.push_back("Reflectance");
    modis_multi_scale_type.push_back("MOD17A2");
    modis_multi_scale_type.push_back("North");
    modis_multi_scale_type.push_back("South");
    modis_multi_scale_type.push_back("MOD_Grid_MOD15A2");
    modis_multi_scale_type.push_back("MODIS_NACP_LAI");
        
    vector<string> modis_div_scale_type;
    // Always start with MOD or mod.
    modis_div_scale_type.push_back("VI");
    modis_div_scale_type.push_back("1km_2D");
    modis_div_scale_type.push_back("L2g_2d");
    modis_div_scale_type.push_back("CMG");
    modis_div_scale_type.push_back("MODIS SWATH TYPE L2");

    // This one doesn't start with "MOD" or "mod".
    //modis_div_scale_type.push_back("VIP_CMG_GRID");

    string modis_eq_scale_type   = "LST";
    string modis_divequ_scale_group = "MODIS_Grid";
    string modis_div_scale_group = "MOD_Grid";
    // The scale-offset rule for the following group may be MULTI but since add_offset is 0. So
    // the MULTI rule is equal to the EQU rule. KY 2013-01-25
    string modis_equ_scale_group  = "MODIS_Grid_1km_2D";

    if(EOS2ObjName=="mod05" || EOS2ObjName=="mod06" || EOS2ObjName=="mod07" 
                            || EOS2ObjName=="mod08" || EOS2ObjName=="atml2")
    {
        scaletype = MODIS_MUL_SCALE;
        return;
    }

    // Find one MYD09GA2012.version005 file that 
    // the grid names change to MODIS_Grid_500m_2D. 
    // So add this one. KY 2012-11-20
 
    // Find the grid name in one MCD43C1 file starts with "MCD_CMG_BRDF", however, the offset of the data is 0.
    // So we may just leave this file since it follows the CF conventions. May need to double check them later. KY 2013-01-24 

    //if(EOS2ObjName.find("MOD")==0 || EOS2ObjName.find("mod")==0 || EOS2ObjName.find("MCD") == 0)

    if(EOS2ObjName.find("MOD")==0 || EOS2ObjName.find("mod")==0) 
    {
        size_t pos = EOS2ObjName.rfind(modis_eq_scale_type);
        if(pos != string::npos && 
          (pos== (EOS2ObjName.length()-modis_eq_scale_type.length())))
        {
            scaletype = MODIS_EQ_SCALE;
            return;
        }

        for(unsigned int k=0; k<modis_multi_scale_type.size(); k++)
        {
            pos = EOS2ObjName.rfind(modis_multi_scale_type[k]);
            if (pos !=string::npos && 
               (pos== (EOS2ObjName.length()-modis_multi_scale_type[k].length())))
            {
//cerr<<"EOS2 object name: "<<EOS2ObjName <<endl;
                scaletype = MODIS_MUL_SCALE;
                return;
            }
        }

        for(unsigned int k=0; k<modis_div_scale_type.size(); k++)
        {
            pos = EOS2ObjName.rfind(modis_div_scale_type[k]);
            if (pos != string::npos && 
                (pos==(EOS2ObjName.length()-modis_div_scale_type[k].length()))){
                scaletype = MODIS_DIV_SCALE;
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
        // We have to separate MODIS_Grid_1km_2D(EQ) from other grids(DIV). 
        if (0 == pos) { 
            size_t eq_scale_pos = EOS2ObjName.find(modis_equ_scale_group);
            if (0 == eq_scale_pos) 
                scaletype = MODIS_EQ_SCALE;
            else 
                scaletype = MODIS_DIV_SCALE;
        }
        else {
            size_t div_scale_pos = EOS2ObjName.find(modis_div_scale_group);
            // Find the "MOD_Grid???" group. 
            if ( 0 == div_scale_pos) 
                scaletype = MODIS_DIV_SCALE;
        }
    }

    //  MEASuRES VIP files have the grid name VIP_CMG_GRID. This applies to all VIP version 2 files. KY 2013-01-24
    if (EOS2ObjName =="VIP_CMG_GRID")
        scaletype = MODIS_DIV_SCALE;
}



Field::~Field()
{
    for_each(this->dims.begin(), this->dims.end(), delete_elem());
    for_each(this->correcteddims.begin(), this->correcteddims.end(), delete_elem());
    if (this->data) {
        delete this->data;
    }
}

Dataset::~Dataset()
{
    for_each(this->dims.begin(), this->dims.end(), delete_elem());
    for_each(this->datafields.begin(), this->datafields.end(),
                  delete_elem());
    for_each(this->attrs.begin(), this->attrs.end(), delete_elem());
}

void Dataset::ReadDimensions(int32 (*entries)(int32, int32, int32 *),
                             int32 (*inq)(int32, char *, int32 *),
                             vector<Dimension *> &dims) throw(Exception)
{
    int32 numdims, bufsize;

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
        if (inq(this->datasetid, &namelist[0], &dimsize[0]) == -1)
            throw2("inquire dimension", this->name);

        vector<string> dimnames;

        // Make the "," separated name string to a string list without ",".
        // This split is for global dimension of a Swath or a Grid object.
        HDFCFUtil::Split(&namelist[0], bufsize, ',', dimnames);
        int count = 0;
        for (vector<string>::const_iterator i = dimnames.begin();
            i != dimnames.end(); ++i) {
            Dimension *dim = new Dimension(*i, dimsize[count]);
            dims.push_back(dim);
            ++count;
        }
    }
}

void Dataset::ReadFields(int32 (*entries)(int32, int32, int32 *),
                         int32 (*inq)(int32, char *, int32 *, int32 *),
                         intn (*fldinfo)
                         (int32, char *, int32 *, int32 *, int32 *, char *),
                         intn (*readfld)
                         (int32, char *, int32 *, int32 *, int32 *, VOIDP),
                         intn (*getfill)(int32, char *, VOIDP),
                         bool geofield,
                         vector<Field *> &fields) throw(Exception)
{
    int32 numfields, bufsize;

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
        if (inq(this->datasetid, &namelist[0], NULL, NULL) == -1)
            throw2("inquire field", this->name);

        vector<string> fieldnames;

        // Split the field namelist, make the "," separated name string to a
        // string list without ",".
        HDFCFUtil::Split(&namelist[0], bufsize, ',', fieldnames);
        for (vector<string>::const_iterator i = fieldnames.begin();
            i != fieldnames.end(); ++i) {
            Field *field = new Field();
            field->name = *i;
            // XXX: We assume the maximum number of dimension for an EOS field
            // is 16.
            int32 dimsize[16];
            char dimlist[512]; // XXX: what an HDF-EOS2 developer recommended

            // Obtain most information of a field such as rank, dimension etc.
            if ((fldinfo(this->datasetid,
                         const_cast<char *>(field->name.c_str()),
                         &field->rank, dimsize, &field->type, dimlist)) == -1)
                throw3("field info", this->name, field->name);
            {
                vector<string> dimnames;

                // Split the dimension name list for a field
                HDFCFUtil::Split(dimlist, ',', dimnames);
                if ((int)dimnames.size() != field->rank)
                    throw4("field rank", dimnames.size(), field->rank,
                           this->name);
                for (int k = 0; k < field->rank; ++k) {
                    Dimension *dim = new Dimension(dimnames[k], dimsize[k]);
                    field->dims.push_back(dim);
                }
            }

            // prepare a way to retrieve actual data later. The actual data
            // is not read in this stage.
            {
                int numelem = field->rank == 0 ? 0 : 1;
                for (int k = 0; k < field->rank; ++k)
                    numelem *= dimsize[k];
                field->data = new
                    UnadjustedFieldData(this->datasetid,
                                        field->name,
                                        numelem * DFKNTsize(field->type),
                                        DFKNTsize(field->type),
                                        readfld);
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
}

void Dataset::ReadAttributes(int32 (*inq)(int32, char *, int32 *),
                             intn (*attrinfo)(int32, char *, int32 *, int32 *),
                             intn (*readattr)(int32, char *, VOIDP),
                             vector<Attribute *> &attrs) throw(Exception)
{
    int32 numattrs, bufsize;

    // Obtain the number of attributes in a Grid or Swath
    if ((numattrs = inq(this->datasetid, NULL, &bufsize)) == -1)
        throw2("inquire attribute", this->name);

    // Obtain the list of  "name, type, value" tuple
    if (numattrs > 0) {
        vector<char> namelist;

        namelist.resize(bufsize + 1);
        // inquiry namelist and buffer size
        if (inq(this->datasetid, &namelist[0], &bufsize) == -1)
            throw2("inquire attribute", this->name);

        vector<string> attrnames;

        // Split the attribute namelist, make the "," separated name string to
        // a string list without ",".
        HDFCFUtil::Split(&namelist[0], bufsize, ',', attrnames);
        for (vector<string>::const_iterator i = attrnames.begin();
            i != attrnames.end(); ++i) {
            Attribute *attr = new Attribute();
            attr->name = *i;
            attr->newname = HDFCFUtil::get_CF_string(attr->name);

            int32 count;
            // Obtain the datatype and byte count of this attribute
            if (attrinfo(this->datasetid, const_cast<char *>
                        (attr->name.c_str()), &attr->type, &count) == -1)
                throw3("attribute info", this->name, attr->name);

            attr->value.resize(count);
                        
            // Obtain the attribute value. Note that this function just
            // provides a copy of all attribute values. 
            //The client should properly interpret to obtain individual
            // attribute value.
            if (readattr(this->datasetid,
                         const_cast<char *>(attr->name.c_str()),
                         &attr->value[0]) == -1)
                throw3("read attribute", this->name, attr->name);

            // Append this attribute to attrs list.
            attrs.push_back(attr);
        }
    }
}

GridDataset::~GridDataset()
{
    if (this->datasetid != -1){
        GDdetach(this->datasetid);
    }
}

GridDataset * GridDataset::Read(int32 fd, const string &gridname)
    throw(Exception)
{
    GridDataset *grid = new GridDataset(gridname);

    // Open this Grid object 
    if ((grid->datasetid = GDattach(fd, const_cast<char *>(gridname.c_str())))
        == -1)
        throw2("attach grid", gridname);

    // Obtain the size of XDim and YDim as well as latitude and longitude of
    // the upleft corner and the low right corner. 
    {
        Info &info = grid->info;
        if (GDgridinfo(grid->datasetid, &info.xdim, &info.ydim, info.upleft,
                       info.lowright) == -1) throw2("grid info", gridname);
    }

    // Obtain projection information.
    {
        Projection &proj = grid->proj;
        if (GDprojinfo(grid->datasetid, &proj.code, &proj.zone, &proj.sphere,
                       proj.param) == -1) 
            throw2("projection info", gridname);
        if (GDpixreginfo(grid->datasetid, &proj.pix) == -1)
            throw2("pixreg info", gridname);
        if (GDorigininfo(grid->datasetid, &proj.origin) == -1)
            throw2("origin info", gridname);
    }

    // Read dimensions
    grid->ReadDimensions(GDnentries, GDinqdims, grid->dims);

    // Read all fields of this Grid.
    grid->ReadFields(GDnentries, GDinqfields, GDfieldinfo, GDreadfield,
                     GDgetfillvalue, false, grid->datafields);

    // Read all attributes of this Grid.
    grid->ReadAttributes(GDinqattrs, GDattrinfo, GDreadattr, grid->attrs);

    return grid;
}

GridDataset::Calculated & GridDataset::getCalculated() const
{
    if (this->calculated.grid != this)
        this->calculated.grid = this;
    return this->calculated;
}

const float64 * GridDataset::Calculated::peekLongitude() const
{
    if (this->valid)
        return &this->lons[0];
    return 0;
}

const float64 * GridDataset::Calculated::getLongitude() throw(Exception)
{
    if (!this->valid)
        this->ReadLongitudeLatitude();
    return this->peekLongitude();
}

const float64 * GridDataset::Calculated::peekLatitude() const
{
    if (this->valid)
        return &this->lats[0];
    return 0;
}

const float64 * GridDataset::Calculated::getLatitude() throw(Exception)
{
    if (!this->valid)
        this->ReadLongitudeLatitude();
    return this->peekLatitude();
}

// This method release the buffer by calling resize().
void GridDataset::Calculated::dropLongitudeLatitude()
{
    if (this->valid) {
        this->valid = false;
        this->lons.resize(0);
        this->lats.resize(0);
    }
}

bool GridDataset::Calculated::isYDimMajor() throw(Exception)
{
    if (!this->valid) 
        this->DetectMajorDimension();
    //Kent: this is too costly. KY 2012-09-19
    //this->ReadLongitudeLatitude();
    return this->ydimmajor;
}

bool GridDataset::Calculated::isOrthogonal() throw(Exception)
{
    if (!this->valid)
        this->ReadLongitudeLatitude();
    return this->orthogonal;
}

int GridDataset::Calculated::DetectFieldMajorDimension() throw(Exception)
{
    int ym = -1;
	
    // Traverse all data fields

    for (vector<Field *>::const_iterator i =
        this->grid->getDataFields().begin();
        i != this->grid->getDataFields().end(); ++i) {

        int xdimindex = -1, ydimindex = -1, index = 0;

        // Traverse all dimensions in each data field
        for (vector<Dimension *>::const_iterator j =
            (*i)->getDimensions().begin();
            j != (*i)->getDimensions().end(); ++j) {
            if ((*j)->getName() == this->grid->dimxname) 
                xdimindex = index;
            else if ((*j)->getName() == this->grid->dimyname) 
                ydimindex = index;
            ++index;
        }
        if (xdimindex == -1 || ydimindex == -1) 
            continue;

        int major = ydimindex < xdimindex ? 1 : 0;

        if (ym == -1)
            ym = major;
        break; // TO gain performance, just check one field.
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

void GridDataset::Calculated::DetectMajorDimension() throw(Exception)
{
    int ym = -1;
    // ydimmajor := true if (YDim, XDim)
    // ydimmajor := false if (XDim, YDim)

    // Traverse all data fields
    
    for (vector<Field *>::const_iterator i =
        this->grid->getDataFields().begin();
        i != this->grid->getDataFields().end(); ++i) {

        int xdimindex = -1, ydimindex = -1, index = 0;

        // Traverse all dimensions in each data field
        for (vector<Dimension *>::const_iterator j =
            (*i)->getDimensions().begin();
            j != (*i)->getDimensions().end(); ++j) {
            if ((*j)->getName() == this->grid->dimxname) 
                xdimindex = index;
            else if ((*j)->getName() == this->grid->dimyname) 
                ydimindex = index;
            ++index;
        }
        if (xdimindex == -1 || ydimindex == -1) 
            continue;

        // Change the way of deciding the major dimesion (LD -2012/01/10).
        int major;
        if(this->grid->getProjection().getCode() == GCTP_LAMAZ)
            major = 1;
        else
            major = ydimindex < xdimindex ? 1 : 0;

        if (ym == -1)
            ym = major;
        break; // TO gain performance, just check one field.
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


void GridDataset::Calculated::DetectOrthogonality() throw(Exception)
{
    int32 xdim = this->grid->getInfo().getX();
    int32 ydim = this->grid->getInfo().getY();
    const float64 * lon = &this->lons[0];
    const float64 * lat = &this->lats[0];

    int j;
    this->orthogonal = false;

    /// Ideally we should check both latitude and longitude for each case.
    /// Here we assume that latitude and longitude have the same orthogonality
    // for any EOS files.
    /// We just check longitude and latitude alternatively for each case.

    if (this->ydimmajor) {
        // check if longitude is like the following:
        //   /- xdim  -/
        //   1 2 3 ... x
        //   1 2 3 ... x
        //   1 2 3 ... x
        //      ...
        //   1 2 3 ... x
        for (int i = 0; i < xdim; ++i) {
            double v = lon[i];
            for (j = 1; j < ydim; ++j) {
                if (lon[j * xdim + i] != v)
                    break;
            }
            if (j != ydim)
                return;
        }

        // check if latitude is like the following:
        //   /- xdim  -/
        //   1 1 1 ... 1
        //   2 2 2 ... 2
        //   3 3 3 ... 3
        //      ...
        //   y y y ... y
        for (int i = 0; i < ydim; ++i) {
            double v = lat[i * xdim];
            for (j = 1; j < xdim; ++j) {
                if (lat[i * xdim + j] != v)
                    break;
            }
            if (j != xdim)
                return;
        }
    }
    else {
        // check if longitude is like the following:
        //   /- ydim  -/
        //   1 1 1 ... 1
        //   2 2 2 ... 2
        //   3 3 3 ... 3
        //      ...
        //   x x x ... x
        for (int i = 0; i < xdim; ++i) {
            double v = lon[i * ydim];
            for (j = 1; j < ydim; ++j) {
                if (lon[i * ydim + j] != v)
                    break;
            }
            if (j != ydim)
                return;
        }

        // check if latitude is like the following:
        //   /- ydim  -/
        //   1 2 3 ... y
        //   1 2 3 ... y
        //   1 2 3 ... y
        //      ...
        //   1 2 3 ... y

        for (int i = 0; i < ydim; ++i) {
            double v = lat[i];
            for (j = 1; j < xdim; ++j) {
                if (lat[j * ydim + i] != v)
                    break;
            }
            if (j != xdim)
                return;
        }
    }
    this->orthogonal = true;
}

void GridDataset::Calculated::ReadLongitudeLatitude() throw(Exception)
{
    this->DetectMajorDimension();

    const GridDataset::Info &info = this->grid->getInfo();
    const GridDataset::Projection &proj = this->grid->getProjection();

    int32 numpoints = info.getX() * info.getY();

    vector<int32> rows, cols;
    rows.reserve(numpoints);
    cols.reserve(numpoints);

    if (this->ydimmajor) {
        // rows             cols
        //   /- xdim  -/      /- xdim  -/
        //   0 0 0 ... 0      0 1 2 ... x
        //   1 1 1 ... 1      0 1 2 ... x
        //       ...              ...
        //   y y y ... y      0 1 2 ... x
        for (int j = 0; j < info.getY(); ++j) {
            for (int i = 0; i < info.getX(); ++i) {
                rows.push_back(j);
                cols.push_back(i);
            }
        }
    }
    else {
        // rows             cols
        //   /- ydim  -/      /- ydim  -/
        //   0 1 2 ... y      0 0 0 ... y
        //   0 1 2 ... y      1 1 1 ... y
        //       ...              ...
        //   0 1 2 ... y      2 2 2 ... y
        for (int j = 0; j < info.getX(); ++j) {
            for (int i = 0; i < info.getY(); ++i) {
                rows.push_back(i);
                cols.push_back(j);
            }
        }
    }

    this->lons.resize(numpoints);
    this->lats.resize(numpoints);

    // Call EOS GDij2ll function to obtain latitude and longitude
    if (GDij2ll(proj.getCode(), proj.getZone(),
        const_cast<float64 *>(proj.getParam()), proj.getSphere(),
        info.getX(), info.getY(),
        const_cast<float64 *>(info.getUpLeft()),
        const_cast<float64 *>(info.getLowRight()), numpoints,
                              &rows[0], &cols[0],
                              &this->lons[0], &this->lats[0],
                              proj.getPix(), proj.getOrigin()) == -1)
        throw2("ij2ll", this->grid->getName());

    // This will detect the orthogonality of latitude and longitude and
    // provide the application either 1-D or 2-D lat/lon in the future.
    this->DetectOrthogonality();

    this->valid = true;
}

// The following internal utilities are not used currently, will see if
// they are necessary in the future. KY 2012-09-19
// The internal utility method to check if two vectors have overlapped.
// If not, return true.
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

SwathDataset * SwathDataset::Read(int32 fd, const string &swathname)
    throw(Exception)
{
    SwathDataset *swath = new SwathDataset(swathname);

    // Open this Swath object
    if ((swath->datasetid = SWattach(fd,
        const_cast<char *>(swathname.c_str())))
        == -1)
        throw2("attach swath", swathname);

    // Read dimensions of this Swath
    swath->ReadDimensions(SWnentries, SWinqdims, swath->dims);
        
    // Read all information related to data fields of this Swath
    swath->ReadFields(SWnentries, SWinqdatafields, SWfieldinfo, SWreadfield,
                      SWgetfillvalue, false, swath->datafields);
        
    // Read all information related to geo-location fields of this Swath
    swath->ReadFields(SWnentries, SWinqgeofields, SWfieldinfo, SWreadfield,
                      SWgetfillvalue, true, swath->geofields);
        
    // Read all attributes of this Swath
    swath->ReadAttributes(SWinqattrs, SWattrinfo, SWreadattr, swath->attrs);
        
    // Read dimension map and save the number of dimension map for dim. subsetting

    swath->set_num_map(swath->ReadDimensionMaps(swath->dimmaps));

    // Read index maps, we haven't found any files with the Index Maps.
    swath->ReadIndexMaps(swath->indexmaps);

    return swath;
}

int SwathDataset::ReadDimensionMaps(vector<DimensionMap *> &dimmaps)
    throw(Exception)
{
    int32 nummaps, bufsize;

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
        vector<int32> offset, increment;

        namelist.resize(bufsize + 1);
        offset.resize(nummaps);
        increment.resize(nummaps);
        if (SWinqmaps(this->datasetid, &namelist[0], &offset[0], &increment[0])
            == -1)
            throw2("inquire dimmap", this->name);

        vector<string> mapnames;
        HDFCFUtil::Split(&namelist[0], bufsize, ',', mapnames);
        int count = 0;
        for (vector<string>::const_iterator i = mapnames.begin();
            i != mapnames.end(); ++i) {
            vector<string> parts;
            HDFCFUtil::Split(i->c_str(), '/', parts);
            if (parts.size() != 2) 
                throw3("dimmap part", parts.size(),
                        this->name);

            DimensionMap *dimmap = new DimensionMap(parts[0], parts[1],
                                                    offset[count],
                                                    increment[count]);
            dimmaps.push_back(dimmap);
            ++count;
        }
    }
    return nummaps;
}

// The following function is nevered tested and probably will never be used.
void SwathDataset::ReadIndexMaps(vector<IndexMap *> &indexmaps)
    throw(Exception)
{
    int32 numindices, bufsize;

    if ((numindices = SWnentries(this->datasetid, HDFE_NENTIMAP, &bufsize)) ==
        -1)
        throw2("indexmap entry", this->name);
    if (numindices > 0) {
        // TODO: I have never seen any EOS2 files that have index map.
        vector<char> namelist;

        namelist.resize(bufsize + 1);
        if (SWinqidxmaps(this->datasetid, &namelist[0], NULL) == -1)
            throw2("inquire indexmap", this->name);

        vector<string> mapnames;
        HDFCFUtil::Split(&namelist[0], bufsize, ',', mapnames);
        for (vector<string>::const_iterator i = mapnames.begin();
             i != mapnames.end(); ++i) {
            IndexMap *indexmap = new IndexMap();
            vector<string> parts;
            HDFCFUtil::Split(i->c_str(), '/', parts);
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

            indexmaps.push_back(indexmap);
        }
    }
}


PointDataset::~PointDataset()
{
}

PointDataset * PointDataset::Read(int32 /*fd*/, const string &pointname)
    throw(Exception)
{
    PointDataset *point = new PointDataset(pointname);
    return point;
}

const char * FieldData::peek() const
{
    if (this->valid)
        return &this->data[0];
    return 0;
}



const char * MissingFieldData::get(int*offset,int*step,int*count,int nelms)
{
    // The first time to read the data(valid is false).
    // The buffer(datalen) is allocated at this stage.
    int datalen = length();
    unsigned int databufsize=nelms*this->datatypesize;
    this->data.resize(databufsize);
    if(datalen == (int32)databufsize) {
        // filling the whole array with nature number.
        for(int i = 0; i < nelms; i++) 
            *(int*)&data[i*datatypesize] = i;
    }
    else {//subsetting case
        // int startingvalue;
        if(this->rank == 1) {// this is the current case.
            for (int i =0; i <count[0];i++) 
                *(int*)&data[i*datatypesize] = offset[0]+step[0]*i; 
        }
        else {// handle later, the code is almost ready.
            ;
        }
    }
    
    return &this->data[0];
}

void MissingFieldData::drop()
{
    if (this->valid) {
        this->valid = false;
        this->data.resize(0);
    }
}

int MissingFieldData::length() const
{
    return this->datalen;
}

int MissingFieldData::dtypesize() const
{
    return this->datatypesize;
}

const char * UnadjustedFieldData::get(int*offset,int*step,int*count,int nelms)
{

    if (!this->valid) {
        unsigned int databufsize=nelms*this->datatypesize;
        this->data.resize(databufsize);
        if(this->datalen == (int32)databufsize) {
            if (this->reader(this->datasetid,
                             const_cast<char *>(this->fieldname.c_str()),
                             NULL, NULL, NULL, &this->data[0]) == -1)
                return 0;
        }
        else {
            if (this->reader(this->datasetid,
                             const_cast<char *>(this->fieldname.c_str()),
                             (int32*)offset, (int32*)step, (int32*)count, &this->data[0]) == -1)
                return 0;
        }
        this->valid = true;
    }
    return &this->data[0];
}

void UnadjustedFieldData::drop()
{
    if (this->valid) {
        this->valid = false;
        this->data.resize(0);
    }
}

int UnadjustedFieldData::length() const
{
    return this->datalen;
}

int UnadjustedFieldData::dtypesize() const
{
    return this->datatypesize;
}



bool Utility::ReadNamelist(const char *path,
                           int32 (*inq)(char *, char *, int32 *),
                           vector<string> &names)
{
    char *fname = const_cast<char *>(path);
    int32 bufsize;
    int numobjs;

    if ((numobjs = inq(fname, NULL, &bufsize)) == -1) return false;
    if (numobjs > 0) {
        vector<char> buffer;
        buffer.resize(bufsize + 1);
        if (inq(fname, &buffer[0], &bufsize) == -1) return false;
        HDFCFUtil::Split(&buffer[0], bufsize, ',', names);
    }
    return true;
}
#endif

// vim:ts=4:sw=4:sts=4

