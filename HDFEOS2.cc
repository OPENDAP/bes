/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
//
//  Authors:   MuQun Yang <myang6@hdfgroup.org> Choonghwan Lee 
// Copyright (c) 2009 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include "HDFEOS2.h"
#ifdef USE_HDFEOS2_LIB
//#include "InternalErr.h"
//#include <BESDEBUG.h> // Should provide BESDebug info. later
#include <sstream>
#include <algorithm>
#include <functional>
#include <vector>
#include <map>
#include <set>

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
//const char *HDFEOS2::File::_swathlatfield_names[] = {"Latitude"};
//const char *HDFEOS2::File::_swathlonfield_names[] = {"Longitude"};


using namespace HDFEOS2;
using namespace std;

template<typename T, typename U, typename V, typename W, typename X>
static void _throw5(const char *fname, int line, int numarg,
                    const T &a1, const U &a2, const V &a3, const W &a4,
                    const X &a5)
{
    std::ostringstream ss;
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

// This is a safer way to insert and update map item than map[key]=val style string assignment.
// Otherwise, the local testsuite at The HDF Group will fail for HDF-EOS2 data
//  under iMac machine platform.
bool insert_map(std::map<std::string,std::string>& m, string key, string val)
{
    pair<map<string,string>::iterator, bool> ret;
    ret = m.insert(make_pair(key, val));
    if(ret.second == false){
        m.erase(key);
        ret = m.insert(make_pair(key, val));
        if(ret.second == false){
            cerr << "insert_map():insertion failed on Key=" << key << " Val=" << val << endl;
        }
    }    
    return ret.second;
}

File::~File()
{
    if(gridfd !=-1) {
        for (std::vector<GridDataset *>::const_iterator i = grids.begin();
             i != grids.end(); ++i){
            delete *i;
        }
        if((GDclose(gridfd))==-1) throw2("grid close",path);
    }

    if(swathfd !=-1) {
        for (std::vector<SwathDataset *>::const_iterator i = swaths.begin();
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

    std::vector<std::string> gridlist;
    if (!Utility::ReadNamelist(file->path.c_str(), GDinqgrid, gridlist))
        throw2("grid namelist", path);
    for (std::vector<std::string>::const_iterator i = gridlist.begin();
         i != gridlist.end(); ++i)
        file->grids.push_back(GridDataset::Read(file->gridfd, *i));

    // Read information of all Swath objects in this file
    if ((file->swathfd = SWopen(const_cast<char *>(file->path.c_str()),
                                DFACC_READ)) == -1){
        delete file;
        throw2("swath open", path);
    }

    std::vector<std::string> swathlist;
    if (!Utility::ReadNamelist(file->path.c_str(), SWinqswath, swathlist)){
        delete file;
        throw2("swath namelist", path);
    }
    for (std::vector<std::string>::const_iterator i = swathlist.begin();
         i != swathlist.end(); ++i)
        file->swaths.push_back(SwathDataset::Read(file->swathfd, *i));

    // We only obtain the name list of point objects but not don't provide
    // other information of these objects.
    // The client will only get the name list of point objects.
    std::vector<std::string> pointlist;
    if (!Utility::ReadNamelist(file->path.c_str(), PTinqpoint, pointlist)){
        delete file;
        throw2("point namelist", path);
    }
    for (std::vector<std::string>::const_iterator i = pointlist.begin();
         i != pointlist.end(); ++i)
        file->points.push_back(PointDataset::Read(-1, *i));

    if(file->grids.size() == 0 && file->swaths.size() == 0 
       && file->points.size() == 0) {
        Exception e("Not an HDF-EOS2 file");
        e.setFileType(false);
        delete file;
        throw e;  
    }

    return file;
}


bool File::check_dim_name_clashing(bool llclash) const
{
    // one grid/swath case. quick return
    if(grids.size()==1)
        return false;
    if(swaths.size()==1)
        return false;

    std::set<std::string> names;
    for(size_t i=0; i<grids.size() + swaths.size(); i++)
	{
            Dataset *ds = NULL;
            SwathDataset *sw = NULL;
            if(i < grids.size())
                ds = static_cast<Dataset*>(grids[i]);
            else 
		{
                    sw = swaths[i-grids.size()];
                    ds = static_cast<Dataset*>(sw);
		}

            std::set<std::string> no_checking_names;
            if(sw)
		{
                    // Geo fields
                    const std::vector<Field *> & geofields= (sw)->getGeoFields();
                    for(std::vector<Field*>::const_iterator it2=geofields.begin();
                        it2!=geofields.end(); ++it2)
			{
                            if((*it2)->getName() == "Latitude" ||
                               (*it2)->getName() == "Longitude")
				{
                                    const std::vector<Dimension *>& dims = (*it2)->getDimensions();
                                    for(std::vector<Dimension*>::const_iterator it3 = dims.begin();
                                        it3 != dims.end(); ++it3)
					{
                                            no_checking_names.insert((*it3)->getName());
					}

				}
			}

                    // Dimension maps
                    const std::vector<HDFEOS2::SwathDataset::DimensionMap *> & dimmap = sw->getDimensionMaps();
                    for(std::vector<HDFEOS2::SwathDataset::DimensionMap*>::const_iterator it2=dimmap.begin();
                        it2!=dimmap.end(); ++it2)
			{
                            no_checking_names.insert((*it2)->getGeoDimension());
                            no_checking_names.insert((*it2)->getDataDimension());
			}

		}

            const std::vector<Dimension *> & dims = (ds)->getDimensions();
            for(std::vector<Dimension *>::const_iterator it2=dims.begin();
                it2!=dims.end(); ++it2)
		{
                    const std::string& dimname = (*it2)->getName();
                    // When one lat/lon is available for all grids, we  
                    // don't have to check the name clashing for "XDim" and "YDim".
                    // So far we only find this happen with the name "XDim" and "YDim".
                    // We will test more files and add other names.
                    if(llclash && (dimname=="XDim" || dimname=="YDim"))
                        continue;
                    else if(no_checking_names.find(dimname) != no_checking_names.end())
                        continue;
                    else
			{
                            if(names.find(dimname)==names.end())
				{
                                    names.insert(dimname);
				}
                            else
				{
                                    return true;
				}
			}
		}
	}
    return false;
}

bool File::check_field_name_clashing(bool bUseDimNameMatching) const
{

    std::set<std::string> names;

    for(size_t i=0; i<grids.size() + swaths.size(); i++)
	{
            Dataset *ds = NULL;
            SwathDataset *sw = NULL;
            if(i < grids.size())
                ds = static_cast<Dataset*>(grids[i]);
            else 
		{
                    sw = swaths[i-grids.size()];
                    ds = static_cast<Dataset*>(sw);
		}
            std::set<std::string> no_checking_names;
            if(sw)
		{
                    const std::vector<Field *> & geofields= (sw)->getGeoFields();
                    for(std::vector<Field*>::const_iterator it2=geofields.begin();
                        it2!=geofields.end(); ++it2)
			{
                            // So far we only find "Latitude" and "Longitude" names to be used for swath.
                            // The dimension names of "Latitude" and "Longitude" won't be used as the 
                            // third dimension "field" name, so we don't need to consider this name when 
                            // checking the name clashing. KY 2010-8-11
                            if((*it2)->getName() == "Latitude" ||
                               (*it2)->getName() == "Longitude")
				{
                                    const std::vector<Dimension *>& dims = (*it2)->getDimensions();
                                    for(std::vector<Dimension*>::const_iterator it3 = dims.begin();
                                        it3 != dims.end(); ++it3)
					{
                                            no_checking_names.insert((*it3)->getName());
					}

				}
			}
		}

            std::set<std::string> fieldnames;
            std::set<std::string> dims_of_1d_datafields;

            const std::vector<Field *> & datafields= (ds)->getDataFields();
            for(std::vector<Field*>::const_iterator it2=datafields.begin();
                it2!=datafields.end(); ++it2)
		{
                    fieldnames.insert((*it2)->getName());
                    if((*it2)->getRank()==1)
			{
                            const std::string& dimname=(((*it2)->getDimensions())[0])->getName();
                            dims_of_1d_datafields.insert(dimname);
			}
		}

            if(sw)
		{
                    const std::vector<Field *> & geofields= (sw)->getGeoFields();
                    for(std::vector<Field*>::const_iterator it2=datafields.begin();
                        it2!=datafields.end(); ++it2)
			{
                            fieldnames.insert((*it2)->getName());
                            if((*it2)->getRank()==1)
				{
                                    const std::string& dimname=(((*it2)->getDimensions())[0])->getName();
                                    dims_of_1d_datafields.insert(dimname);
				}
			}
		}

            std::set<std::string> dimnames;
            const std::vector<Dimension *> & dims = (ds)->getDimensions();
            for(std::vector<Dimension*>::const_iterator it3=dims.begin();
                it3!=dims.end(); ++it3)
		{
                    dimnames.insert((*it3)->getName());
		}

            // Getting NF-dims
            std::set<std::string> nf_dimnames;
            if(bUseDimNameMatching)
		{
                    for(std::set<std::string>::const_iterator it=dimnames.begin();
                        it != dimnames.end(); ++it)
			{
                            if(dims_of_1d_datafields.find(*it)==dims_of_1d_datafields.end())
                                nf_dimnames.insert(*it);
			}
		} else
		{
                    nf_dimnames = dimnames;
		}

            // Inserting data fields
            for(std::set<std::string>::const_iterator it=fieldnames.begin();
                it != fieldnames.end(); ++it)
		{
                    if(names.find(*it)==names.end())
			{
                            names.insert(*it);
			}
                    else
			{
                            return true;
			}
		}

            // Inserting NF_dimnames
            for(std::set<std::string>::const_iterator it=nf_dimnames.begin();
                it != nf_dimnames.end(); ++it)
		{
                    if(no_checking_names.find(*it) != no_checking_names.end())
                        continue;
                    else if(names.find(*it)==names.end())
			{
                            names.insert(*it);
			}
                    else
			{
                            return true;
			}
		}
	}
    return false;
}


std::string File::get_geodim_x_name()
{
    if(!_geodim_x_name.empty())
        return _geodim_x_name;
    _find_geodim_names();
    return _geodim_x_name;
}

std::string File::get_geodim_y_name()
{
    if(!_geodim_y_name.empty())
        return _geodim_y_name;
    _find_geodim_names();
    return _geodim_y_name;
}

std::string File::get_latfield_name()
{
    if(!_latfield_name.empty())
        return _latfield_name;
    _find_latlonfield_names();
    return _latfield_name;
}

std::string File::get_lonfield_name()
{
    if(!_lonfield_name.empty())
        return _lonfield_name;
    _find_latlonfield_names();
    return _lonfield_name;
}

std::string File::get_geogrid_name()
{
    if(!_geogrid_name.empty())
        return _geogrid_name;
    _find_geogrid_name();
    return _geogrid_name;
}

void File::_find_geodim_names()
{
    std::set<std::string> geodim_x_name_set;
    for(size_t i = 0; i<sizeof(_geodim_x_names) / sizeof(const char *); i++)
        geodim_x_name_set.insert(_geodim_x_names[i]);

    std::set<std::string> geodim_y_name_set;
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

            const std::vector<Dimension *>& dims = dataset->getDimensions();
            for(std::vector<Dimension*>::const_iterator it = dims.begin();
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
    std::set<std::string> latfield_name_set;
    for(size_t i = 0; i<sizeof(_latfield_names) / sizeof(const char *); i++)
        latfield_name_set.insert(_latfield_names[i]);

    std::set<std::string> lonfield_name_set;
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

            const std::vector<Field *>& fields = dataset->getDataFields();
            for(std::vector<Field*>::const_iterator it = fields.begin();
                it != fields.end(); ++it)
		{
                    if(latfield_name_set.find((*it)->getName()) != latfield_name_set.end())
                        _latfield_name = (*it)->getName();
                    else if(lonfield_name_set.find((*it)->getName()) != lonfield_name_set.end())
                        _lonfield_name = (*it)->getName();
		}

            if(sw)
		{
                    const std::vector<Field *>& geofields = dataset->getDataFields();
                    // I changed the test for this loop from
                    // "it != fields.end()" to "it != geofields.end()".
                    // jhrg 3/16/11
                    for(std::vector<Field*>::const_iterator it = geofields.begin();
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
    std::set<std::string> geogrid_name_set;
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

void File::Prepare(const char *path, HE2CFShortName *sn, HE2CFShortName* sn_dim,HE2CFUniqName *un, HE2CFUniqName *un_dim) throw(Exception)
{

    // The current HDF-EOS2 module doesn't  cause performance penalty to
    // obtain all the information of the file. So we just retrieve all.
    File *file = this;

    // Obtain the vector for swath and grid

    int numgrid = file->grids.size();// Retrieve the number of grids
    int numswath = file->swaths.size(); // Retrieve the number of swaths
    
    if(numgrid < 0) throw2("the number of grid is less than 0", path);
    
    if (numgrid > 0) {

        // 0. obtain "XDim","YDim","Latitude","Longitude" and "location" set.
        std::string DIMXNAME = file->get_geodim_x_name();       
      
        std::string DIMYNAME = file->get_geodim_y_name();       

        std::string LATFIELDNAME = file->get_latfield_name();       

        std::string LONFIELDNAME = file->get_lonfield_name();       

        // Now only there is only one geo grid name "location", so don't call it know.
        //       std::string GEOGRIDNAME = file->get_geogrid_name();
        std::string GEOGRIDNAME = "location";

        // First handle Grid.
        // G1. Check global lat/lon for multiple grids.
        // We want to check if there is a global lat/lon for multiple grids.
        // AIRS level 3 data provides lat/lon under the GEOGRIDNAME grid.

        if(numgrid > 0) {// This is not needed for one grid case. Will revise later KY 2010-6-29

            int onellcount = 0; //only one lat/lon and it is under GEOGRIDNAME
            int morellcount = 0; // Check if lat/lon is found under other grids.

            // Loop through all grids
            for(std::vector<GridDataset *>::const_iterator i = file->grids.begin();
                i != file->grids.end(); ++i){
                // Loop through all fields
                for(std::vector<Field *>::const_iterator j =
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
                        if(onellcount == 2) break;//Finish this grid
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
            if(morellcount ==0 && onellcount ==2) file->onelatlon = true; 
        }

        // G2 Prepare the right dimension name and the dimension field list for each grid. 
        // The assumption is that within a grid, the dimension name is unique. The dimension field name(even with the added Z-like field) is unique. 
        // A map <dimension name, dimension field name> will be created.
        // The name clashing handling for multiple grids will not be done in this step. 

        //Dimension name and the corresponding field name when only one lat/lon is used for all grids.
        std::map<std::string,std::string>temponelatlondimcvarlist;
          
        for (std::vector<GridDataset *>::const_iterator i = file->grids.begin();
             i != file->grids.end(); ++i){

            // G2.0. Set the horizontal dimension name "dimxname" and "dimyname"
            // G2.0. This will be used to detect the dimension major order.
            (*i)->setDimxName(DIMXNAME);
            (*i)->setDimyName(DIMYNAME);

            // G2.1. Find the existing Z-dimension field and save the list
            std::set<std::string> tempdimlist; // Unique 1-D field's dimension name list.
            std::pair<set<std::string>::iterator,bool> tempdimret;
            std::map<std::string,std::string>tempdimcvarlist;//dimension name and the corresponding field name. 

            for (std::vector<Field *>::const_iterator j =
                     (*i)->getDataFields().begin();
                 j != (*i)->getDataFields().end(); ++j) {
                if ((*j)->getRank()==1){//We only need to search those 1-D fields

                    // DIMXNAME and DIMYNAME correspond to latitude and longitude.
                    // They should NOT be treated as dimension names missing fields. It will be handled differently.
                    if(((*j)->getDimensions())[0]->getName()!=DIMXNAME && ((*j)->getDimensions())[0]->getName()!=DIMYNAME){
                        tempdimret = tempdimlist.insert(((*j)->getDimensions())[0]->getName());
                        if(tempdimret.second == true) {
                            insert_map(tempdimcvarlist, ((*j)->getDimensions())[0]->getName(),
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

            // Loop through all dimensions of this grid.
            for (std::vector<Dimension *>::const_iterator j =
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
                        insert_map(tempdimcvarlist, (missingfield->getDimensions())[0]->getName(), 
                                   missingfield->name);
                    }
                }
            }
            tempdimlist.clear();// clear this temp. set.

            // G2.3 Handle latitude and longitude for dim. list and coordinate variable list
            if(file->onelatlon) {// One global lat/lon for all grids.
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
                            for (std::vector<Dimension *>::const_iterator j =
                                     (*i)->lonfield->getDimensions().begin(); j!= (*i)->lonfield->getDimensions().end();++j){
                                if((*j)->getName() == DIMXNAME) {
                                    insert_map(tempdimcvarlist, (*j)->getName(), 
                                               (*i)->lonfield->getName());

                                }
                                if((*j)->getName() == DIMYNAME) {
                                    insert_map(tempdimcvarlist, (*j)->getName(), 
                                               (*i)->latfield->getName());
                                }
                            }

                        }
                        else {// 2-D lat/lon case. Since dimension order doesn't matter, so we always assume lon->XDim, lat->YDim.
                            for (std::vector<Dimension *>::const_iterator j =
                                     (*i)->lonfield->getDimensions().begin(); j!= (*i)->lonfield->getDimensions().end();++j){
                                if((*j)->getName() == DIMXNAME) {
                                    insert_map(tempdimcvarlist, (*j)->getName(), 
                                               (*i)->lonfield->getName());
                                }
                                if((*j)->getName() == DIMYNAME) {
                                    insert_map(tempdimcvarlist, (*j)->getName(), 
                                               (*i)->latfield->getName());
                                }
                            }
                        }
                    }
                    else { // This is the 1-D case, just inserting the dimension, field pair.
                        insert_map(tempdimcvarlist, ((*i)->lonfield->getDimensions())[0]->getName(),
                                   (*i)->lonfield->getName());
                        insert_map(tempdimcvarlist, ((*i)->latfield->getDimensions())[0]->getName(),
                                   (*i)->latfield->getName());
                    }
                    temponelatlondimcvarlist = tempdimcvarlist;
                }
                
            }
            else {// Has its own latitude/longitude or lat/lon needs to be calculated.
                if((*i)->ownllflag) {// this grid has its own latitude/longitude
                    // Searching the lat/lon field from the grid. 
                    for (std::vector<Field *>::const_iterator j =
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
                                    for (std::vector<Dimension *>::const_iterator k =
                                             (*j)->getDimensions().begin(); k!= (*j)->getDimensions().end();++k){
                                        if((*k)->getName() == DIMYNAME) {
                                            insert_map(tempdimcvarlist, (*k)->getName(), (*j)->getName());
                                        }
                                    }
                     
                                }
                                else {// 2-D lat/lon case. Since dimension order doesn't matter, so we always assume lon->XDim, lat->YDim.
                                    for (std::vector<Dimension *>::const_iterator k =
                                             (*j)->getDimensions().begin(); k!= (*j)->getDimensions().end();++k){
                                        if((*k)->getName() == DIMYNAME) {
                                            insert_map(tempdimcvarlist, (*k)->getName(), (*j)->getName());
                                        }
                                    }
                                }
                            }
                            else { // This is the 1-D case, just inserting  the dimension, field pair.
                                insert_map(tempdimcvarlist, (((*j)->getDimensions())[0])->getName(), 
                                           (*j)->getName());
                            }
                        } 
                        else if ((*j)->getName() == LONFIELDNAME) {

                            // set the flag to tell if this is lat or lon. The unit will be different for lat and lon.
                            (*j)->fieldtype = 2;

                            // longitude rank should not be greater than 2.
                            // Here I don't check if the rank of latitude and longitude is the same. Hopefully it never happens for HDF-EOS2 cases.
                            // We are still investigating if Java clients work when the rank of latitude and longitude is greater than 2.
                            if((*j)->getRank() >2) throw3("The rank of Longitude is greater than 2",(*i)->getName(),(*j)->getName());

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
                                    for (std::vector<Dimension *>::const_iterator k =
                                             (*j)->getDimensions().begin(); k!= (*j)->getDimensions().end();++k){
                                        if((*k)->getName() == DIMXNAME) {
                                            insert_map(tempdimcvarlist, (*k)->getName(), (*j)->getName());
                                        }
                                    }
                                }
                                else {// 2-D lat/lon case. Since dimension order doesn't matter, so we always assume lon->XDim, lat->YDim.
                                    for (std::vector<Dimension *>::const_iterator k =
                                             (*j)->getDimensions().begin(); k!= (*j)->getDimensions().end();++k){
                                        if((*k)->getName() == DIMXNAME) {
                                            insert_map(tempdimcvarlist, (*k)->getName(), (*j)->getName());
                                        }
                                    }
                                }
                            }
                            else { // This is the 1-D case, just inserting  the dimension, field pair.
                                insert_map(tempdimcvarlist, (((*j)->getDimensions())[0])->getName(), 
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

                    if(dmajor) { //latfield->ydimmajor) {
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
                        for (std::vector<Dimension *>::const_iterator j =
                                 lonfield->getDimensions().begin(); j!= lonfield->getDimensions().end();++j){
                            if((*j)->getName() == DIMXNAME) {
                                insert_map(tempdimcvarlist, (*j)->getName(), lonfield->getName());
                            }

                            if((*j)->getName() == DIMYNAME) {
                                insert_map(tempdimcvarlist, (*j)->getName(), latfield->getName());
                            }
                        }
                    }
                    else {// 2-D lat/lon case. Since dimension order doesn't matter, so we always assume lon->XDim, lat->YDim.
                        for (std::vector<Dimension *>::const_iterator j =
                                 lonfield->getDimensions().begin(); j!= lonfield->getDimensions().end();++j){

                            if((*j)->getName() == DIMXNAME){ 
                                insert_map(tempdimcvarlist, (*j)->getName(), lonfield->getName());
                            }

                            if((*j)->getName() == DIMYNAME){
                                insert_map(tempdimcvarlist, (*j)->getName(), latfield->getName());
                            }
                        }
                    }
                }
                
            }
            (*i)->dimcvarlist = tempdimcvarlist;
            tempdimcvarlist.clear();
        }

        // G2.4 When only one lat/lon is used for all grids, grids other than "locations" need to add dim lat/lon pairs.
        if(file->onelatlon) {
            for (std::vector<GridDataset *>::const_iterator i = file->grids.begin();
                 i != file->grids.end(); ++i){

                std::string templatlonname1;
                std::string templatlonname2;

                if((*i)->getName() != GEOGRIDNAME) {
                    map<std::string,std::string>::iterator tempmapit;

                    // Find DIMXNAME field
              
                    tempmapit = temponelatlondimcvarlist.find(DIMXNAME);
                    if(tempmapit != temponelatlondimcvarlist.end()) 
                        templatlonname1= tempmapit->second;
                    else 
                        throw2("cannot find the dimension field of XDim", (*i)->getName());
                    insert_map((*i)->dimcvarlist, DIMXNAME, templatlonname1);

                    // Find DIMYNAME field
                    tempmapit = temponelatlondimcvarlist.find(DIMYNAME);
                    if(tempmapit != temponelatlondimcvarlist.end()) 
                        templatlonname2= tempmapit->second;
                    else
                        throw2("cannot find the dimension field of YDim", (*i)->getName());
                    insert_map((*i)->dimcvarlist, DIMYNAME, templatlonname2);
                }
            }
        }
        // G3. check global name clashing
        // Two flags are supposed to return, one for field name clashing, one for dimension name clashing. 
        bool dimnameclash = file->check_dim_name_clashing(file->onelatlon);

        // We probably don't have to set  the input parameter as true 
        // since we do consider the potential missing dimension names before. We may double check this in the future version 
        // KY 2010-6-29
        bool fieldnameclash = file->check_field_name_clashing(true);
      
        // G4. Create a map for dimension field name <original field name, corrected field name>
        // Also assure the uniqueness of all field names,save the new field names.
        std::string temp1name,temp2name;
        bool shorteningname=false;// This evaluates a shorter name when short name option is specified.
        std::map<std::string,std::string>tempncvarnamelist;//the original dimension field name to the corrected dimension field name
        std:: string tempcorrectedlatname, tempcorrectedlonname;
        for (std::vector<GridDataset *>::const_iterator i = file->grids.begin();
             i != file->grids.end(); ++i){
            // Here we can't use getDataFields call since for no lat/lon fields 
            // are created for one global lat/lon case. We have to use the dimcvarnamelist 
            // map we just created.
 
            for (std::vector<Field *>::const_iterator j = 
                     (*i)->getDataFields().begin();
                 j != (*i)->getDataFields().end(); ++j) 
                    
                {
               
                    // sn is a pointer of an instance of the HE2CFShortName object, it should be defined globally.
                    // un is a pointer of an instance of the HE2CFUniqName object.
                    temp1name = (*j)->getName();
                    temp1name = sn->get_short_string(temp1name,&shorteningname);  
                    if(!shorteningname && fieldnameclash && (numgrid > 1))  // this will apply to shortername and non-shortname-option long-string
                        temp2name = un->get_uniq_string(temp1name);// Notice this object is cfun instead of cfsn 
                    else 
                        temp2name = temp1name;
                    (*j)->newname = temp2name;//remember the newname. 
                    if((*j)->fieldtype!=0) {// If this field is a dimension field, save the name/new name pair. 
                        tempncvarnamelist.insert(make_pair((*j)->getName(), temp2name));
                        // For one latlon case, remember the corrected latitude and longitude field names.
                        if((file->onelatlon)&&(((*i)->getName())==GEOGRIDNAME)) {
                            if((*j)->getName()==LATFIELDNAME) tempcorrectedlatname = temp2name;
                            if((*j)->getName()==LONFIELDNAME) tempcorrectedlonname = temp2name;
                        }
                    }
                }
            (*i)->ncvarnamelist = tempncvarnamelist;
            tempncvarnamelist.clear();
        }

        // for one lat/lon case, we have to add the lat/lon field name to other grids.
        // We know the original lat and lon names. So just retrieve the corrected lat/lon names from
        // the geo grid(GEOGRIDNAME).

        if(file->onelatlon) {
            for(std::vector<GridDataset *>::const_iterator i = file->grids.begin();
                i != file->grids.end(); ++i){
                if((*i)->getName()!=GEOGRIDNAME){// Lat/lon names must be in this group.
                    insert_map((*i)->ncvarnamelist, LATFIELDNAME, tempcorrectedlatname);
                    insert_map((*i)->ncvarnamelist, LONFIELDNAME, tempcorrectedlonname);
                }
            }
        }

        // G5. Create a map for dimension name < original dimension name, corrected dimension name>
        std::map<std::string,std::string>tempndimnamelist;//the original dimension name to the corrected dimension name
        shorteningname=false;

        // Since DIMXNAME and DIMYNAME are not in the original dimension name list, we use the dimension name,field map 
        // we just formed. 

        for (std::vector<GridDataset *>::const_iterator i = file->grids.begin();
             i != file->grids.end(); ++i){
            for (std::map<std::string,std::string>::const_iterator j =
                     (*i)->dimcvarlist.begin(); j!= (*i)->dimcvarlist.end();++j){

                // We have to handle DIMXNAME and DIMYNAME separately.
                if((*j).first==DIMXNAME || (*j).first==DIMYNAME ){// The dimension name is so short, we don't need to check the short name option.
                    if(!(file->onelatlon) && (numgrid > 1)){ // For one latitude/longitude and one grid, DIMXNAME and DIMYNAME are unique
                        temp1name = (*j).first;
                        temp1name = un_dim->get_uniq_string(temp1name);
                    }
                    else
                        temp1name = (*j).first;
                    insert_map(tempndimnamelist, (*j).first, temp1name);
                    continue;
                }
                temp1name = (*j).first;
                temp1name = sn_dim->get_short_string(temp1name,&shorteningname);
                if(!shorteningname && dimnameclash && (numgrid > 1)) // this will apply to shortername and non-shortname-option long-string
                    temp2name = un_dim->get_uniq_string(temp1name);
                else
                    temp2name = temp1name;
                insert_map(tempndimnamelist, (*j).first, temp2name);
            }
            (*i)->ndimnamelist = tempndimnamelist;
            tempndimnamelist.clear();   
        }

        // G6. Revisit the lat/lon fields to check if 1-D COARD convention needs to be followed.
        std::vector<Dimension*> correcteddims;
        std::string tempcorrecteddimname;
        std::map<std::string,std::string> tempnewxdimnamelist;
        std::map<std::string,std::string> tempnewydimnamelist;
     
        Dimension *correcteddim;
        // int dimsize; jhrg 3/16/11
     
        for (std::vector<GridDataset *>::const_iterator i = file->grids.begin();
             i != file->grids.end(); ++i){
            for (std::vector<Field *>::const_iterator j =
                     (*i)->getDataFields().begin();
                 j != (*i)->getDataFields().end(); ++j) {
                // Now handling COARD cases, since latitude/longitude can be either 1-D or 2-D array. 
                // So we need to correct both cases.
                if((*j)->getName()==LATFIELDNAME && (*j)->getRank()==2 &&(*j)->condenseddim) {// 2-D lat to 1-D COARD lat

                    std::string templatdimname;
                    std::map<std::string,std::string>::iterator tempmapit;

                    // Find the new name of LATFIELDNAME
                    tempmapit = (*i)->ncvarnamelist.find(LATFIELDNAME);
                    if(tempmapit != (*i)->ncvarnamelist.end()) templatdimname= tempmapit->second;
                    else throw2("cannot find the corrected field of Latitude", (*i)->getName());


                    for(std::vector<Dimension *>::const_iterator k =(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){
                        if((*k)->getName()==DIMYNAME) {// This needs to be latitude
                            correcteddim = new Dimension(templatdimname,(*k)->getSize());
                            correcteddims.push_back(correcteddim);
                            (*j)->setCorrectedDimensions(correcteddims);
                            insert_map(tempnewydimnamelist, (*i)->getName(), templatdimname);
                        }
                    }
                    (*j)->iscoard = true;
                    (*i)->iscoard = true;
                    if(file->onelatlon) file->iscoard = true;
                    correcteddims.clear();//clear the local temporary vector
                }
                else if((*j)->getName()==LONFIELDNAME && (*j)->getRank()==2 &&(*j)->condenseddim){// 2-D lon to 1-D COARD lat
               
                    std::string templondimname;
                    map<std::string,std::string>::iterator tempmapit;

                    // Find the new name of LONFIELDNAME
                    tempmapit = (*i)->ncvarnamelist.find(LONFIELDNAME);
                    if(tempmapit != (*i)->ncvarnamelist.end()) templondimname= tempmapit->second;
                    else throw2("cannot find the corrected field of Longitude", (*i)->getName());

                    for(std::vector<Dimension *>::const_iterator k =(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){
                        if((*k)->getName()==DIMXNAME) {// This needs to be longitude
                            correcteddim = new Dimension(templondimname,(*k)->getSize());
                            correcteddims.push_back(correcteddim);
                            (*j)->setCorrectedDimensions(correcteddims);
                            insert_map(tempnewxdimnamelist, (*i)->getName(), templondimname);
                        }

                    }

                    (*j)->iscoard = true;
                    (*i)->iscoard = true;
                    if(file->onelatlon) file->iscoard = true;
                    correcteddims.clear();
                }
                else if(((*j)->getRank()==1) &&((*j)->getName()==LONFIELDNAME) ) {// 1-D lon to 1-D COARD lon

                    std::string templondimname;
                    map<std::string,std::string>::iterator tempmapit;

                    // Find the new name of LONFIELDNAME
                    tempmapit = (*i)->ncvarnamelist.find(LONFIELDNAME);
                    if(tempmapit != (*i)->ncvarnamelist.end()) templondimname= tempmapit->second;
                    else throw2("cannot find the corrected field of Longitude", (*i)->getName());

                    correcteddim = new Dimension(templondimname,((*j)->getDimensions())[0]->getSize());
                    correcteddims.push_back(correcteddim);
                    (*j)->setCorrectedDimensions(correcteddims);
                    (*j)->iscoard = true;
                    (*i)->iscoard = true;
                    if(file->onelatlon) file->iscoard = true; 
                    correcteddims.clear();

                    if(((((*j)->getDimensions())[0]->getName()!=DIMXNAME))&&((((*j)->getDimensions())[0]->getName())!=DIMYNAME)){
                        throw3("the dimension name of longitude should not be ",((*j)->getDimensions())[0]->getName(),(*i)->getName()); 
                    }
                    if((((*j)->getDimensions())[0]->getName())==DIMXNAME) {
                        insert_map(tempnewxdimnamelist, (*i)->getName(), templondimname);
                    }
                    else {
                        insert_map(tempnewydimnamelist, (*i)->getName(), templondimname);
                    }
                }
                else if(((*j)->getRank()==1) &&((*j)->getName()==LATFIELDNAME) ) {// 1-D lon to 1-D  COARD lon

                    std::string templatdimname;
                    map<std::string,std::string>::iterator tempmapit;

                    // Find the new name of LATFIELDNAME
                    tempmapit = (*i)->ncvarnamelist.find(LATFIELDNAME);
                    if(tempmapit != (*i)->ncvarnamelist.end()) templatdimname= tempmapit->second;
                    else throw2("cannot find the corrected field of Latitude", (*i)->getName());

                    correcteddim = new Dimension(templatdimname,((*j)->getDimensions())[0]->getSize());
                    correcteddims.push_back(correcteddim);
                    (*j)->setCorrectedDimensions(correcteddims);
              
                    (*j)->iscoard = true;
                    (*i)->iscoard = true;
                    if(file->onelatlon) file->iscoard = true;
                    correcteddims.clear();

                    if(((((*j)->getDimensions())[0]->getName())!=DIMXNAME)&&((((*j)->getDimensions())[0]->getName())!=DIMYNAME))
                        throw3("the dimension name of latitude should not be ",((*j)->getDimensions())[0]->getName(),(*i)->getName());
                    if((((*j)->getDimensions())[0]->getName())==DIMXNAME){
                        insert_map(tempnewxdimnamelist, (*i)->getName(), templatdimname);
                    }
                    else {
                       insert_map(tempnewydimnamelist, (*i)->getName(), templatdimname);
                    }
                }
                else {
                    ;
                }
            }
        
        }
      
        // G7. If COARD follows, apply the new DIMXNAME and DIMYNAME name to the  ndimnamelist 
        if(file->onelatlon){ //One lat/lon for all grids .
            if(file->iscoard){ // COARD is followed.
                // For this case, only one pair of corrected XDim and YDim for all grids.
                std::string tempcorrectedxdimname;
                std::string tempcorrectedydimname;

                if((int)(tempnewxdimnamelist.size())!= 1) throw1("the corrected dimension name should have only one pair");
                if((int)(tempnewydimnamelist.size())!= 1) throw1("the corrected dimension name should have only one pair");

                map<std::string,std::string>::iterator tempdimmapit = tempnewxdimnamelist.begin();
                tempcorrectedxdimname = tempdimmapit->second;      
                tempdimmapit = tempnewydimnamelist.begin();
                tempcorrectedydimname = tempdimmapit->second;
       
                for (std::vector<GridDataset *>::const_iterator i = file->grids.begin();
                     i != file->grids.end(); ++i){

                    // Find the DIMXNAME and DIMYNAME in the dimension name list.  
                    map<std::string,std::string>::iterator tempmapit;
                    tempmapit = (*i)->ndimnamelist.find(DIMXNAME);
                    if(tempmapit != (*i)->ndimnamelist.end()) {
                        insert_map((*i)->ndimnamelist, DIMXNAME, tempcorrectedxdimname);
                    }
                    else 
                        throw2("cannot find the corrected dimension name", (*i)->getName());
                    tempmapit = (*i)->ndimnamelist.find(DIMYNAME);
                    if(tempmapit != (*i)->ndimnamelist.end()) {
                        insert_map((*i)->ndimnamelist, DIMYNAME, tempcorrectedydimname);
                    }
                    else 
                        throw2("cannot find the corrected dimension name", (*i)->getName());
                }
            }
        }
        else {// We have to search each grid
            for (std::vector<GridDataset *>::const_iterator i = file->grids.begin();
                 i != file->grids.end(); ++i){
                if((*i)->iscoard){
     
                    std::string tempcorrectedxdimname;
                    std::string tempcorrectedydimname;

                    // Find the DIMXNAME and DIMYNAME in the dimension name list.
                    map<std::string,std::string>::iterator tempdimmapit;
                    map<std::string,std::string>::iterator tempmapit;
                    tempdimmapit = tempnewxdimnamelist.find((*i)->getName());
                    if(tempdimmapit != tempnewxdimnamelist.end()) tempcorrectedxdimname = tempdimmapit->second;
                    else throw2("cannot find the corrected COARD XDim dimension name", (*i)->getName());
                    tempmapit = (*i)->ndimnamelist.find(DIMXNAME);
                    if(tempmapit != (*i)->ndimnamelist.end()) {
                        insert_map((*i)->ndimnamelist, DIMXNAME, tempcorrectedxdimname);
                    }
                    else throw2("cannot find the corrected dimension name", (*i)->getName());

                    tempdimmapit = tempnewydimnamelist.find((*i)->getName());
                    if(tempdimmapit != tempnewydimnamelist.end()) 
                        tempcorrectedydimname = tempdimmapit->second;
                    else throw2("cannot find the corrected COARD YDim dimension name", (*i)->getName());
                    tempmapit = (*i)->ndimnamelist.find(DIMYNAME);
                    if(tempmapit != (*i)->ndimnamelist.end()) {
                        insert_map((*i)->ndimnamelist, DIMYNAME, tempcorrectedydimname);
                    }
                    else throw2("cannot find the corrected dimension name", (*i)->getName());
                }
            }
        }

      
        //G8. For 1-D lat/lon cases, Make the third (other than lat/lon coordinate variable) dimension to follow COARD conventions. 

        for (std::vector<GridDataset *>::const_iterator i = file->grids.begin();
             i != file->grids.end(); ++i){
            for (std::map<std::string,std::string>::const_iterator j =
                     (*i)->dimcvarlist.begin(); j!= (*i)->dimcvarlist.end();++j){

                // It seems that the condition for onelatlon case is if(file->iscoard) is true instead if
                // file->onelatlon is true.So change it. KY 2010-7-4
                //if((file->onelatlon||(*i)->iscoard) && (*j).first !=DIMXNAME && (*j).first !=DIMYNAME) 
                if((file->iscoard||(*i)->iscoard) && (*j).first !=DIMXNAME && (*j).first !=DIMYNAME) {
                    std:: string tempnewdimname;
                    map<std::string,std::string>::iterator tempmapit;

                    // Find the new field name of the corresponding dimennsion name 
                    tempmapit = (*i)->ncvarnamelist.find((*j).second);
                    if(tempmapit != (*i)->ncvarnamelist.end()) tempnewdimname= tempmapit->second;
                    else throw3("cannot find the corrected field of ", (*j).second,(*i)->getName());

                    // Make the new field name to the correponding dimension name 
                    tempmapit =(*i)->ndimnamelist.find((*j).first);
                    if(tempmapit != (*i)->ndimnamelist.end()) 
                        insert_map((*i)->ndimnamelist, (*j).first, tempnewdimname);
                    else throw3("cannot find the corrected dimension name of ", (*j).first,(*i)->getName());

                }
            }
        }
        // The following code may be usedful for future more robust third-dimension support
#if 0
        // 8 GrADS now still follows COARD conventions. It requires 
        // the dimension name and the field name to be the same. So we try to make it happen.
        // 
      

        // Need to remember the original time dimension name for each grid
        std::map<std::string,std::string> temptdimnamelist;

        // Need to remember the corrected time dimension name for each grid
        std::map<std::string,std::string> tempnewtdimnamelist;

        for (std::vector<GridDataset *>::const_iterator i = file->grids.begin();
             i != file->grids.end(); ++i){
            for (std::vector<Field *>::const_iterator j =
                     (*i)->getDataFields().begin();
                 j != (*i)->getDataFields().end(); ++j) {

                // If the field is "Time",obtain the original "dimension name",the original "field name"
                // also obtain the corrected dimension name, the corrected field name
                // Assign the corrected field name to be the new dimension name
                // Also remember the new dimension name in a special "time" map<original dimension name: new dimension name>
                if(((*j)->fieldtype == 5) && ((*j)->getRank() == 1) && (*i)->iscoard){

                    //  std::string templondimname = (*i)->ncvarnamelist(LONFIELDNAME);
                    std::string temptimedimname;
                    map<std::string,std::string>::iterator tempmapit;

                    // Find the new name of "Time" field
                    tempmapit = (*i)->ncvarnamelist.find((*j)->getName());
                    if(tempmapit != (*i)->ncvarnamelist.end()) temptimedimname= tempmapit->second;
                    else throw3("cannot find the corrected field of ", (*j)->getName(),(*i)->getName());

                    (*i)->coardtime = true;
                    (*j)->iscoard = true;
                    correcteddim = new Dimension(temptimedimname,((*j)->getDimensions())[0]->getSize());
                    correcteddims.push_back(correcteddim);
                    (*j)->setCorrectedDimensions(correcteddims);
                    correcteddims.clear();
                    tempnewtdimnamelist[(*i)->getName()] = temptimedimname;
                    temptdimnamelist[(*i)->getName()] = ((*j)->getDimensions())[0]->getName();
                }
            }
        }

        //  Change the dimension name of the "Time" for dimension names of those fields which have the dimension name of the "Time"

        for (std::vector<GridDataset *>::const_iterator i = file->grids.begin();
             i != file->grids.end(); ++i){
            if((*i)->iscoard && (*i)->coardtime){

                std::string temporiginaltdimname;
                std::string tempcorrectedtdimname;

                // Find the TIME DIMENSION NAME in the dimension name list.
                map<std::string,std::string>::iterator tempdimmapit;
                map<std::string,std::string>::iterator tempmapit;

                tempdimmapit = temptdimnamelist.find((*i)->getName());
                if(tempdimmapit != temptdimnamelist.end()) temporiginaltdimname = tempdimmapit->second;
                else throw2("cannot find the original COARD Time dimension name", (*i)->getName());

                tempdimmapit = tempnewtdimnamelist.find((*i)->getName());
                if(tempdimmapit != tempnewtdimnamelist.end()) tempcorrectedtdimname = tempdimmapit->second;
                else throw2("cannot find the corrected COARD Time dimension name", (*i)->getName());

                tempmapit = (*i)->ndimnamelist.find(temporiginaltdimname);
                if(tempmapit != (*i)->ndimnamelist.end()) (*i)->ndimnamelist[temporiginaltdimname]=tempcorrectedtdimname;
                else throw2("cannot find the orignal dimension name in the dimension name map", (*i)->getName());

            }
        }
#endif
 
        //  G9. Create the corrected dimension vectors.
        for (std::vector<GridDataset *>::const_iterator i = file->grids.begin();
             i != file->grids.end(); ++i){ 

            for (std::vector<Field *>::const_iterator j =
                     (*i)->getDataFields().begin();
                 j != (*i)->getDataFields().end(); ++j) {

                if((*j)->iscoard == false) {// the corrected dimension name of lat/lon have been updated.

                    // Just obtain the corrected dim names  and save the corrected dimensions for each field.
                    for(std::vector<Dimension *>::const_iterator k=(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){

                        //tempcorrecteddimname =(*i)->ndimnamelist((*k)->getName());
                        map<std::string,std::string>::iterator tempmapit;

                        // Find the new name of this field
                        tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                        if(tempmapit != (*i)->ndimnamelist.end()) tempcorrecteddimname= tempmapit->second;
                        else throw4("cannot find the corrected dimension name", (*i)->getName(),(*j)->getName(),(*k)->getName());
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
           
        for (std::vector<GridDataset *>::const_iterator i = file->grids.begin();
             i != file->grids.end(); ++i){
            for (std::vector<Field *>::const_iterator j =
                     (*i)->getDataFields().begin();
                 j != (*i)->getDataFields().end(); ++j) {
                 
                // Real fields: adding coordinate attributesinate attributes
                if((*j)->fieldtype == 0)  {
                    std::string tempcoordinates="";
                    std::string tempfieldname="";
                    std::string tempcorrectedfieldname="";
                    int tempcount = 0;
                    for(std::vector<Dimension *>::const_iterator k=(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){
                        // handle coordinates attributes
               
                        map<std::string,std::string>::iterator tempmapit;
                        map<std::string,std::string>::iterator tempmapit2;
              
                        // Find the dimension field name
                        tempmapit = ((*i)->dimcvarlist).find((*k)->getName());
                        if(tempmapit != ((*i)->dimcvarlist).end()) tempfieldname = tempmapit->second;
                        else throw4("cannot find the dimension field name",(*i)->getName(),(*j)->getName(),(*k)->getName());

                        // Find the corrected dimension field name
                        tempmapit2 = ((*i)->ncvarnamelist).find(tempfieldname);
                        if(tempmapit2 != ((*i)->ncvarnamelist).end()) tempcorrectedfieldname = tempmapit2->second;
                        else throw4("cannot find the corrected dimension field name",(*i)->getName(),(*j)->getName(),(*k)->getName());

                        if(tempcount == 0) tempcoordinates= tempcorrectedfieldname;
                        else tempcoordinates = tempcoordinates +" "+tempcorrectedfieldname;
                        tempcount++;
                    }
                    (*j)->setCoordinates(tempcoordinates);
                }

                // Add units for latitude and longitude
                if((*j)->fieldtype == 1) {// latitude,adding the "units" attribute  degrees_east.
                    std::string tempunits = "degrees_north";
                    (*j)->setUnits(tempunits);
                }
                if((*j)->fieldtype == 2) { // longitude, adding the units of 
                    std::string tempunits = "degrees_east";
                    (*j)->setUnits(tempunits);
                }

                // Add units for Z-dimension, now it is always "level"
                if(((*j)->fieldtype == 3)||((*j)->fieldtype == 4)) {
                    std::string tempunits ="level";
                    (*j)->setUnits(tempunits);
                }

                if(((*j)->fieldtype == 5)) {
                    std::string tempunits ="days since 1900-01-01 00:00:00";
                    (*j)->setUnits(tempunits);
                }
                    
            }
        }
    }
    if(numgrid==0) {
  
        // Now we handle swath case. 
        if (numswath > 0) {
 
            // S(wath)0. Check if there are dimension maps in this file.
            int tempnumdm = 0;
            for (std::vector<SwathDataset *>::const_iterator i = file->swaths.begin();
                 i != file->swaths.end(); ++i){
                tempnumdm += (*i)->get_num_map();
                if (tempnumdm >0) break;
            }

            // MODATML2 and MYDATML2 in year 2010 include dimension maps. But the dimension map
            // is not used. Furthermore, they provide additional latitude/longtiude 
            // for 10 KM under the data field. So we have to handle this differently.
            // MODATML2 in year 2000 version doesn't include dimension map, so we 
            // have to consider both with dimension map and without dimension map cases.
            // The swath name is atml2.

            bool fakedimmap = false;

            if(numswath == 1) {
                if((file->swaths[0]->getName()).find("atml2")!=std::string::npos){
                    if(tempnumdm >0) fakedimmap = true;
                    int templlflag = 0;
                    for (std::vector<Field *>::const_iterator j =
                             file->swaths[0]->getGeoFields().begin();
                         j != file->swaths[0]->getGeoFields().end(); ++j) {
                        if((*j)->getName() == "Latitude" || (*j)->getName() == "Longitude") {
                            if((*j)->getType() == DFNT_UINT16 ||
                               (*j)->getType() == DFNT_INT16)
                                (*j)->type = DFNT_FLOAT32;
                            templlflag ++;
                            if(templlflag == 2) break;
                        }
                    }

                    templlflag = 0;

                    for (std::vector<Field *>::const_iterator j =
                             file->swaths[0]->getDataFields().begin();
                         j != file->swaths[0]->getDataFields().end(); ++j) {

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

                        if(((*j)->getName()).find("Latitude") != std::string::npos){
                            if((*j)->getType() == DFNT_UINT16 ||
                               (*j)->getType() == DFNT_INT16)
                                (*j)->type = DFNT_FLOAT32;
                            (*j)->fieldtype = 1;
                            // Also need to link the dimension to the coordinate variable list
                            if((*j)->getRank() != 2) 
                                throw2("The lat/lon rank must be  2 for Java clients to work",(*j)->getRank());
                            insert_map(file->swaths[0]->dimcvarlist, (((*j)->getDimensions())[0])->getName(),(*j)->getName());
                            templlflag ++;
                        }
                        if(((*j)->getName()).find("Longitude")!= std::string::npos) {
                            if((*j)->getType() == DFNT_UINT16 ||
                               (*j)->getType() == DFNT_INT16)
                                (*j)->type = DFNT_FLOAT32;
                            (*j)->fieldtype = 2;
                            if((*j)->getRank() != 2) 
                                throw2("The lat/lon rank must be  2 for Java clients to work",(*j)->getRank());
                            insert_map(file->swaths[0]->dimcvarlist, (((*j)->getDimensions())[1])->getName(), (*j)->getName());
                            templlflag ++;
                        }
                        if(templlflag == 2) break;
                    }
                }
            }

            // Although this file includes dimension map, it doesn't use it at all. So change
            // tempnumdm to 0.
            if(fakedimmap) tempnumdm = 0;
 

            // S1. Prepare the right dimension name and the dimension field list for each swath. 
            // The assumption is that within a swath, the dimension name is unique.
            //  The dimension field name(even with the added Z-like field) is unique. 
            // A map <dimension name, dimension field name> will be created.
            // The name clashing handling for multiple swaths will not be done in this step. 

            // S1.1 Obtain the dimension names corresponding to the latitude and longitude,save them to the <dimname, dimfield> map.
            for (std::vector<SwathDataset *>::const_iterator i = file->swaths.begin();
                 i != file->swaths.end(); ++i){

                int tempgeocount = 0;
                for (std::vector<Field *>::const_iterator j =
                         (*i)->getGeoFields().begin();
                     j != (*i)->getGeoFields().end(); ++j) {

                    // Here we assume it is always lat[f0][f1] and lon [f0][f1]. No lat[f0][f1] and lon[f1][f0] occur.
                    // So far only "Latitude" and "Longitude" are used as standard names of Lat and lon for swath.
                    if((*j)->getName()=="Latitude" ){
                        if((*j)->getRank() > 2) throw2("Currently the lat/lon rank must be 1 or 2 for Java clients to work",(*j)->getRank());

                        // Since under our assumption, lat/lon are always 2-D for a swath and dimension order doesn't matter for Java clients,
                        // we always map Latitude the first dimension and longitude the second dimension.
                        // Save this information in the coordinate variable name and field map.
                        // For rank =1 case, we only handle the cross-section along the same longitude line. So Latitude should be the dimension name.

                        insert_map((*i)->dimcvarlist, (((*j)->getDimensions())[0])->getName(), "Latitude");

                        // Have dimension map, we want to remember the dimension and remove it from the list.
                        if(tempnumdm >0) {                    

                            // We have to loop through the dimension map
                            for(std::vector<SwathDataset::DimensionMap *>::const_iterator l=(*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){

                                // This dimension name will be replaced by the mapped dimension name, 
                                // the mapped dimension name can be obtained from the getDataDimension() method.
                                if(((*j)->getDimensions()[0])->getName() == (*l)->getGeoDimension()) {
                                    insert_map((*i)->dimcvarlist, (*l)->getDataDimension(), "Latitude");
                                    break;
                                }
                            }
                        }
                        (*j)->fieldtype = 1;
                        tempgeocount ++;
                    }
                    if((*j)->getName()=="Longitude"){
                        if((*j)->getRank() > 2) throw2("Currently the lat/lon rank must be  1 or 2 for Java clients to work",(*j)->getRank());
                        // Only lat-level cross-section(for Panoply)is supported when longitude/latitude is 1-D, so ignore the longitude as the dimension field.
                        if((*j)->getRank() == 1) {
                            tempgeocount++;
                            continue;
                        }
                        // Since under our assumption, lat/lon are almost always 2-D for a swath and dimension order doesn't matter for Java clients,
                        // we always map Latitude the first dimension and longitude the second dimension.
                        // Save this information in the dimensiion name and coordinate variable map.
                        insert_map((*i)->dimcvarlist, (((*j)->getDimensions())[1])->getName(), "Longitude");
                        if(tempnumdm >0) {
                            // We have to loop through the dimension map
                            for(std::vector<SwathDataset::DimensionMap *>::const_iterator l=(*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){

                                // This dimension name will be replaced by the mapped dimension name,
                                // This name can be obtained by getDataDimension() fuction of dimension map class. 
                                if(((*j)->getDimensions()[1])->getName() == (*l)->getGeoDimension()) {
                                    insert_map((*i)->dimcvarlist, (*l)->getDataDimension(), "Longitude");
                                    break;
                                }
                            }
                        }
                        (*j)->fieldtype = 2;
                        tempgeocount++;
                    }
                    if(tempgeocount == 2) break;
                }
            }
          
            // S1.3 Handle existing and missing fields 
            for (std::vector<SwathDataset *>::const_iterator i = file->swaths.begin();
                 i != file->swaths.end(); ++i){
                
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
    
                //std::set<std::string> (*i)->nonmisscvdimlist; // Unique 1-D field's dimension name list.
                std::pair<set<std::string>::iterator,bool> tempdimret;
                //          if(tempnumdm == 0) {
                for(std::map<std::string,std::string>::const_iterator j = (*i)->dimcvarlist.begin(); 
                    j!= (*i)->dimcvarlist.end();++j){ 
                    tempdimret = (*i)->nonmisscvdimlist.insert((*j).first);
                }
                //         }

                // S1.2.2 Search the geofield group and see if there are any existing 1-D Z dimension data.
                //  If 1-D field data with the same dimension name is found under GeoField, we still search if that 1-D field  is the dimension
                // field of a dimension name.
                for (std::vector<Field *>::const_iterator j =
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
                            if(((((*j)->getDimensions())[0])->getName())==(*j)->getName()){
                                (*j)->oriname = (*j)->getName();
                                (*j)->name = (*j)->getName() +"_d";
                                (*j)->specialcoard = true;
                            }
                            insert_map((*i)->dimcvarlist, (((*j)->getDimensions())[0])->getName(), (*j)->getName());
                            (*j)->fieldtype = 3;

                        }
                    }
                }

                // We will also check the third dimension inside DataFields
                // This may cause potential problems for AIRS data
                // We will double CHECK KY 2010-6-26
                // So far the tests seem okay. KY 2010-8-11
                for (std::vector<Field *>::const_iterator j =
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
                            if(((((*j)->getDimensions())[0])->getName())==(*j)->getName()){
                                (*j)->oriname = (*j)->getName();
                                (*j)->name = (*j)->getName() +"_d";
                                (*j)->specialcoard = true;
                            }
                            insert_map((*i)->dimcvarlist, (((*j)->getDimensions())[0])->getName(), (*j)->getName());
                            (*j)->fieldtype = 3;

                        }
                    }
                }


                // S1.2.3 Handle the missing fields 
                // Loop through all dimensions of this swath to search the missing fields
                for (std::vector<Dimension *>::const_iterator j =
                         (*i)->getDimensions().begin(); j!= (*i)->getDimensions().end();++j){

                    if(((*i)->nonmisscvdimlist.find((*j)->getName())) == (*i)->nonmisscvdimlist.end()){// This dimension needs a field
                      
                        // Need to create a new data field vector element with the name and dimension as above.
                        Field *missingfield = new Field();
                        // This is for temporarily COARD fix. 
                        // For 2-D lat/lon, the third dimension should NOT follow
                        // COARD conventions. It will cause Panoply and IDV failed.
                        // Since Swath is always 2-D lat/lon, so we are okay here. Add a "_d" for each field name.
                        // KY 2010-7-21
                        missingfield->name = (*j)->getName()+"_d";
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

                        (*i)->geofields.push_back(missingfield);
                        insert_map((*i)->dimcvarlist, (missingfield->getDimensions())[0]->getName(), missingfield->name);

                    }
                }
                (*i)->nonmisscvdimlist.clear();// clear this set.

            }
                  
                 
            // S3. check global name clashing
            // Two flags are supposed to return, one for field name clashing, one for dimension name clashing. 
            // We assume no name clashing between the dimension and field names.
            bool dimnameclash = file->check_dim_name_clashing(false);
            bool fieldnameclash = file->check_field_name_clashing(false);
      
            // S4. Create a map for dimension field name <original field name, corrected field name>
            // Also assure the uniqueness of all field names,save the new field names.
            std::string temp1name,temp2name;
            bool shorteningname=false;// This evaluates a shorter name when short name option is specified.
            std::map<std::string,std::string>tempncvarnamelist;//the original dimension field name to the corrected dimension field name
            for (std::vector<SwathDataset *>::const_iterator i = file->swaths.begin();
                 i != file->swaths.end(); ++i){

                // First handle geofield, all dimension fields are under the geofield group.
                for (std::vector<Field *>::const_iterator j = 
                         (*i)->getGeoFields().begin();
                     j != (*i)->getGeoFields().end(); ++j) 
                    {
               
                        // sn is a pointer of an instance of the HE2CFShortName object, it should be defined globally.
                        // un is a pointer of an instance of the HE2CFUniqName object.
                        temp1name = (*j)->getName();
                        temp1name = sn->get_short_string(temp1name,&shorteningname);  

                        // numswath >1 is necessary for handling MODIS files.
                        if(!shorteningname && fieldnameclash && (numswath > 1))  // this will apply to shortername and non-shortname-option long-string
                            temp2name = un->get_uniq_string(temp1name);// Notice this object is cfun instead of cfsn 
                        else 
                            temp2name = temp1name;
                        (*j)->newname = temp2name;//remember the newname. 
                        if((*j)->fieldtype!=0) {// If this field is a dimension field, save the name/new name pair. 
                            insert_map((*i)->ncvarnamelist, (*j)->getName(), temp2name);
                        }
                    }
 
                for (std::vector<Field *>::const_iterator j = 
                         (*i)->getDataFields().begin();
                     j != (*i)->getDataFields().end(); ++j) 
                    {
               
                        // sn is a pointer of an instance of the HE2CFShortName object, it should be defined globally.
                        // un is a pointer of an instance of the HE2CFUniqName object.
                        temp1name = (*j)->getName();
                        temp1name = sn->get_short_string(temp1name,&shorteningname);  
                        if(!shorteningname && fieldnameclash && (numswath > 1))  // this will apply to shortername and non-shortname-option long-string
                            temp2name = un->get_uniq_string(temp1name);// Notice this object is cfun instead of cfsn 
                        else 
                            temp2name = temp1name;
                        (*j)->newname = temp2name;//remember the newname. 
                        if((*j)->fieldtype!=0) {// If this field is a dimension field, save the name/new name pair. 
                            insert_map((*i)->ncvarnamelist, (*j)->getName(), temp2name);
                        }
                    }
            }

            // S5. Create a map for dimension name < original dimension name, corrected dimension name>
            shorteningname=false;

            for (std::vector<SwathDataset *>::const_iterator i = file->swaths.begin();
                 i != file->swaths.end(); ++i){
                for (std::map<std::string,std::string>::const_iterator j =
                         (*i)->dimcvarlist.begin(); j!= (*i)->dimcvarlist.end();++j){

                    temp1name = (*j).first;
                    temp1name = sn_dim->get_short_string(temp1name,&shorteningname);
                    if(!shorteningname && dimnameclash && (numswath > 1)) // this will apply to shortername and non-shortname-option long-string
                        temp2name = un_dim->get_uniq_string(temp1name);
                    else
                        temp2name = temp1name;
                    insert_map((*i)->ndimnamelist, (*j).first, temp2name);
                }
            }

            //  S6. Create corrected dimension vectors.
            std::vector<Dimension*> correcteddims;
            std::string tempcorrecteddimname;
            Dimension *correcteddim;

            for (std::vector<SwathDataset *>::const_iterator i = file->swaths.begin();
                 i != file->swaths.end(); ++i){ 

                // First the geofield. 
                for (std::vector<Field *>::const_iterator j =
                         (*i)->getGeoFields().begin();
                     j != (*i)->getGeoFields().end(); ++j) {

                    for(std::vector<Dimension *>::const_iterator k=(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){

                        //tempcorrecteddimname =(*i)->ndimnamelist((*k)->getName());
                        map<std::string,std::string>::iterator tempmapit;

                        if(tempnumdm == 0) { // No dimension map, just obtain the new dimension name.
                            // Find the new name of this field
                            tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                            if(tempmapit != (*i)->ndimnamelist.end()) tempcorrecteddimname= tempmapit->second;
                            else throw4("cannot find the corrected dimension name", (*i)->getName(),(*j)->getName(),(*k)->getName());

                            correcteddim = new Dimension(tempcorrecteddimname,(*k)->getSize());
                        }
                        else { // have dimension map, use the datadim and datadim size to replace the geodim and geodim size. 
                            bool isdimmapname = false;

                            // We have to loop through the dimension map
                            for(std::vector<SwathDataset::DimensionMap *>::const_iterator l=(*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){
                                // This dimension name is the geo dimension name in the dimension map, replace the name with data dimension name.
                                if((*k)->getName() == (*l)->getGeoDimension()) {
                                    isdimmapname = true;
                                    (*j)->dmap = true;
                                    std::string temprepdimname = (*l)->getDataDimension();

                                    // Find the new name of this data dimension name
                                    tempmapit = (*i)->ndimnamelist.find(temprepdimname);
                                    if(tempmapit != (*i)->ndimnamelist.end()) tempcorrecteddimname= tempmapit->second;
                                    else throw4("cannot find the corrected dimension name", (*i)->getName(),(*j)->getName(),(*k)->getName());
                                    
                                    // Find the size of this data dimension name
                                    // We have to loop through the Dimensions of this swath
                                    bool ddimsflag = false;
                                    for(std::vector<Dimension *>::const_iterator m=(*i)->getDimensions().begin();m!=(*i)->getDimensions().end();++m) {
                                        if((*m)->getName() == temprepdimname) { // Find the dimension size, create the correcteddim
                                            correcteddim = new Dimension(tempcorrecteddimname,(*m)->getSize());
                                            ddimsflag = true;
                                            break;
                                        }
                                    }
                                    if(!ddimsflag) throw4("cannot find the corrected dimension size", (*i)->getName(),(*j)->getName(),(*k)->getName());
                                    break;
                                }
                            }
                            if(!isdimmapname) { // Still need to assign the corrected dimensions.
                                // Find the new name of this field
                                tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                                if(tempmapit != (*i)->ndimnamelist.end()) tempcorrecteddimname= tempmapit->second;
                                else throw4("cannot find the corrected dimension name", (*i)->getName(),(*j)->getName(),(*k)->getName());

                                correcteddim = new Dimension(tempcorrecteddimname,(*k)->getSize());

                            }
                        }         

                        correcteddims.push_back(correcteddim);
                    }
                    (*j)->setCorrectedDimensions(correcteddims);
                    correcteddims.clear();
                }
 
                // Then the data field.
                for (std::vector<Field *>::const_iterator j =
                         (*i)->getDataFields().begin();
                     j != (*i)->getDataFields().end(); ++j) {

                    for(std::vector<Dimension *>::const_iterator k=(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){

                        if(tempnumdm == 0) {

                            map<std::string,std::string>::iterator tempmapit;

                            // Find the new name of this field
                            tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                            if(tempmapit != (*i)->ndimnamelist.end()) tempcorrecteddimname= tempmapit->second;
                            else throw4("cannot find the corrected dimension name", (*i)->getName(),(*j)->getName(),(*k)->getName());

                            correcteddim = new Dimension(tempcorrecteddimname,(*k)->getSize());
                        }
                        else {
                            map<std::string,std::string>::iterator tempmapit;
           
                            bool isdimmapname = false;
                            // We have to loop through dimension map
                            for(std::vector<SwathDataset::DimensionMap *>::const_iterator l=(*i)->getDimensionMaps().begin(); l!=(*i)->getDimensionMaps().end();++l){
                                // This dimension name is the geo dimension name in the dimension map, replace the name with data dimension name.
                                if((*k)->getName() == (*l)->getGeoDimension()) {
                                    isdimmapname = true;
                                    (*j)->dmap = true;
                                    std::string temprepdimname = (*l)->getDataDimension();
                   
                                    // Find the new name of this data dimension name
                                    tempmapit = (*i)->ndimnamelist.find(temprepdimname);
                                    if(tempmapit != (*i)->ndimnamelist.end()) tempcorrecteddimname= tempmapit->second;
                                    else throw4("cannot find the corrected dimension name", (*i)->getName(),(*j)->getName(),(*k)->getName());
                                    
                                    // Find the size of this data dimension name
                                    // We have to loop through the Dimensions of this swath
                                    bool ddimsflag = false;
                                    for(std::vector<Dimension *>::const_iterator m=(*i)->getDimensions().begin();m!=(*i)->getDimensions().end();++m) {
                                        if((*m)->getName() == temprepdimname) { // Find the dimension size, create the correcteddim
                                            correcteddim = new Dimension(tempcorrecteddimname,(*m)->getSize());
                                            ddimsflag = true;
                                            break;
                                        }
                                    }
                                    if(!ddimsflag) throw4("cannot find the corrected dimension size", (*i)->getName(),(*j)->getName(),(*k)->getName());
                                    break;
                                }
 
                            }
                            if(!isdimmapname) { // Not a dimension with dimension map; Still need to assign the corrected dimensions.

                                // Find the new name of this field
                                tempmapit = (*i)->ndimnamelist.find((*k)->getName());
                                if(tempmapit != (*i)->ndimnamelist.end()) tempcorrecteddimname= tempmapit->second;
                                else throw4("cannot find the corrected dimension name", (*i)->getName(),(*j)->getName(),(*k)->getName());

                                correcteddim = new Dimension(tempcorrecteddimname,(*k)->getSize());

                            }

                        }
                        correcteddims.push_back(correcteddim);
                    }
                    (*j)->setCorrectedDimensions(correcteddims);
                    correcteddims.clear();
                }
            }
            // S7. Create "coordinates" ,"units"  attributes. The "units" attributes only apply to latitude and longitude.
            // This is the last round of looping through everything, 
            // we will match dimension name list to the corresponding dimension field name 
            // list for every field. 
            // Since we find some swath files don't specify fillvalue when -9999.0 is found in the real data,
            // we specify fillvalue for those fields. This is entirely artifical and we will evaluate this approach. KY 2010-3-3
           
            for (std::vector<SwathDataset *>::const_iterator i = file->swaths.begin();
                 i != file->swaths.end(); ++i){

                // Handle GeoField first.
                for (std::vector<Field *>::const_iterator j =
                         (*i)->getGeoFields().begin();
                     j != (*i)->getGeoFields().end(); ++j) {
                 
                    // Real fields: adding the coordinate attribute
                    if((*j)->fieldtype == 0)  {// currently it is always true.
                        std::string tempcoordinates="";
                        std::string tempfieldname="";
                        std::string tempcorrectedfieldname="";
                        int tempcount = 0;
                        for(std::vector<Dimension *>::const_iterator k=(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){
                            // handle coordinates attributes
               
                            map<std::string,std::string>::iterator tempmapit;
                            map<std::string,std::string>::iterator tempmapit2;
              
                            // Find the dimension field name
                            tempmapit = ((*i)->dimcvarlist).find((*k)->getName());
                            if(tempmapit != ((*i)->dimcvarlist).end()) tempfieldname = tempmapit->second;
                            else throw4("cannot find the dimension field name",(*i)->getName(),(*j)->getName(),(*k)->getName());

                            // Find the corrected dimension field name
                            tempmapit2 = ((*i)->ncvarnamelist).find(tempfieldname);
                            if(tempmapit2 != ((*i)->ncvarnamelist).end()) tempcorrectedfieldname = tempmapit2->second;
                            else throw4("cannot find the corrected dimension field name",(*i)->getName(),(*j)->getName(),(*k)->getName());

                            if(tempcount == 0) tempcoordinates= tempcorrectedfieldname;
                            else tempcoordinates = tempcoordinates +" "+tempcorrectedfieldname;
                            tempcount++;
                        }
                        (*j)->setCoordinates(tempcoordinates);
                    }


                    // Add units for latitude and longitude
                    if((*j)->fieldtype == 1) {// latitude,adding the "units" attribute  degrees_east.
                        std::string tempunits = "degrees_north";
                        (*j)->setUnits(tempunits);
                    }
                    if((*j)->fieldtype == 2) { // longitude, adding the units of 
                        std::string tempunits = "degrees_east";
                        (*j)->setUnits(tempunits);
                    }

                    // Add units for Z-dimension, now it is always "level"
                    if(((*j)->fieldtype == 3)||((*j)->fieldtype == 4)) {
                        std::string tempunits ="level";
                        (*j)->setUnits(tempunits);
                    }

                    // Add units for "Time", Be aware that it is always "days since 1900-01-01 00:00:00"
                    if(((*j)->fieldtype == 5)) {
                        std::string tempunits = "days since 1900-01-01 00:00:00";
                        (*j)->setUnits(tempunits);
                    }
                    // Set the fill value for floating type data that doesn't have the fill value.
                    // We found _FillValue attribute is missing from some swath data.
                    // To cover the most cases, an attribute called _FillValue(the value is -9999.0)
                    // is added to the data whose type is float32 or float64.
                    if((((*j)->getFillValue()).empty()) && ((*j)->getType()==DFNT_FLOAT32 || (*j)->getType()==DFNT_FLOAT64)) { 
                        float tempfillvalue = -9999.0;
                        (*j)->addFillValue(tempfillvalue);
                        (*j)->setAddedFillValue(true);
                    }

                }
 
                // Data fields
                for (std::vector<Field *>::const_iterator j =
                         (*i)->getDataFields().begin();
                     j != (*i)->getDataFields().end(); ++j) {
                 
                    // Real fields: adding coordinate attributesinate attributes
                    if((*j)->fieldtype == 0)  {// currently it is always true.
                        std::string tempcoordinates="";
                        std::string tempfieldname="";
                        std::string tempcorrectedfieldname="";
                        int tempcount = 0;
                        for(std::vector<Dimension *>::const_iterator k=(*j)->getDimensions().begin();k!=(*j)->getDimensions().end();++k){
                            // handle coordinates attributes
               
                            map<std::string,std::string>::iterator tempmapit;
                            map<std::string,std::string>::iterator tempmapit2;
              
                            // Find the dimension field name
                            tempmapit = ((*i)->dimcvarlist).find((*k)->getName());
                            if(tempmapit != ((*i)->dimcvarlist).end()) tempfieldname = tempmapit->second;
                            else throw4("cannot find the dimension field name",(*i)->getName(),(*j)->getName(),(*k)->getName());

                            // Find the corrected dimension field name
                            tempmapit2 = ((*i)->ncvarnamelist).find(tempfieldname);
                            if(tempmapit2 != ((*i)->ncvarnamelist).end()) tempcorrectedfieldname = tempmapit2->second;
                            else throw4("cannot find the corrected dimension field name",(*i)->getName(),(*j)->getName(),(*k)->getName());

                            if(tempcount == 0) tempcoordinates= tempcorrectedfieldname;
                            else tempcoordinates = tempcoordinates +" "+tempcorrectedfieldname;
                            tempcount++;
                        }
                        (*j)->setCoordinates(tempcoordinates);
                    }
                    // Add units for Z-dimension, now it is always "level"
                    if(((*j)->fieldtype == 3)||((*j)->fieldtype == 4)) {
                        std::string tempunits ="level";
                        (*j)->setUnits(tempunits);
                    }

                    // Add units for "Time", Be aware that it is always "days since 1900-01-01 00:00:00"
                    if(((*j)->fieldtype == 5)) {
                        std::string tempunits = "days since 1900-01-01 00:00:00";
                        (*j)->setUnits(tempunits);
                    }

                    // Set the fill value for floating type data that doesn't have the fill value.
                    // We found _FillValue attribute is missing from some swath data.
                    // To cover the most cases, an attribute called _FillValue(the value is -9999.0)
                    // is added to the data whose type is float32 or float64.
                    if((((*j)->getFillValue()).empty()) && ((*j)->getType()==DFNT_FLOAT32 || (*j)->getType()==DFNT_FLOAT64)) { 
                        float tempfillvalue = -9999.0;
                        (*j)->addFillValue(tempfillvalue);
                        (*j)->setAddedFillValue(true);
                    }


                }
            
            }

        }

    }
   
}


Field::~Field()
{
    std::for_each(this->dims.begin(), this->dims.end(), delete_elem());
    std::for_each(this->correcteddims.begin(), this->correcteddims.end(), delete_elem());
    if (this->data) {
        delete this->data;
    }
}

Dataset::~Dataset()
{
    std::for_each(this->dims.begin(), this->dims.end(), delete_elem());
    std::for_each(this->datafields.begin(), this->datafields.end(),
                  delete_elem());
    std::for_each(this->attrs.begin(), this->attrs.end(), delete_elem());
}

void Dataset::ReadDimensions(int32 (*entries)(int32, int32, int32 *),
                             int32 (*inq)(int32, char *, int32 *),
                             std::vector<Dimension *> &dims) throw(Exception)
{
    int32 numdims, bufsize;

    // Obtain the number of dimensions and buffer size of holding ","
    // separated dimension name list.
    if ((numdims = entries(this->datasetid, HDFE_NENTDIM, &bufsize)) == -1)
        throw2("dimension entry", this->name);

    // Read all dimension information
    if (numdims > 0) {
        std::vector<char> namelist;
        std::vector<int32> dimsize;

        namelist.resize(bufsize + 1);
        dimsize.resize(numdims);
        // Inquiry dimension name lists and sizes for all dimensions
        if (inq(this->datasetid, &namelist[0], &dimsize[0]) == -1)
            throw2("inquire dimension", this->name);

        std::vector<std::string> dimnames;

        // Make the "," separated name string to a string list without ",".
        // This split is for global dimension of a Swath or a Grid object.
        Utility::Split(&namelist[0], bufsize, ',', dimnames);
        int count = 0;
        for (std::vector<std::string>::const_iterator i = dimnames.begin();
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
                         std::vector<Field *> &fields) throw(Exception)
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
        std::vector<char> namelist;

        namelist.resize(bufsize + 1);

        // Inquiry fieldname list of the current object
        if (inq(this->datasetid, &namelist[0], NULL, NULL) == -1)
            throw2("inquire field", this->name);

        std::vector<std::string> fieldnames;

        // Split the field namelist, make the "," separated name string to a
        // string list without ",".
        Utility::Split(&namelist[0], bufsize, ',', fieldnames);
        for (std::vector<std::string>::const_iterator i = fieldnames.begin();
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
                std::vector<std::string> dimnames;

                // Split the dimension name list for a field
                Utility::Split(dimlist, ',', dimnames);
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
                             std::vector<Attribute *> &attrs) throw(Exception)
{
    int32 numattrs, bufsize;

    // Obtain the number of attributes in a Grid or Swath
    if ((numattrs = inq(this->datasetid, NULL, &bufsize)) == -1)
        throw2("inquire attribute", this->name);

    // Obtain the list of  "name, type, value" tuple
    if (numattrs > 0) {
        std::vector<char> namelist;

        namelist.resize(bufsize + 1);
        // inquiry namelist and buffer size
        if (inq(this->datasetid, &namelist[0], &bufsize) == -1)
            throw2("inquire attribute", this->name);

        std::vector<std::string> attrnames;

        // Split the attribute namelist, make the "," separated name string to
        // a string list without ",".
        Utility::Split(&namelist[0], bufsize, ',', attrnames);
        for (std::vector<std::string>::const_iterator i = attrnames.begin();
             i != attrnames.end(); ++i) {
            Attribute *attr = new Attribute();
            attr->name = *i;

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
        //cerr<<"DETACH this GRID" << endl;
        GDdetach(this->datasetid);
    }
}

GridDataset * GridDataset::Read(int32 fd, const std::string &gridname)
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
                       proj.param) == -1) throw2("projection info", gridname);
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
    if (!this->valid) this->DetectMajorDimension();
    //Kent, this is too costly.
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

    for (std::vector<Field *>::const_iterator i =
             this->grid->getDataFields().begin();
         i != this->grid->getDataFields().end(); ++i) {

        int xdimindex = -1, ydimindex = -1, index = 0;

        // Traverse all dimensions in each data field
        for (std::vector<Dimension *>::const_iterator j =
                 (*i)->getDimensions().begin();
             j != (*i)->getDimensions().end(); ++j) {
            if ((*j)->getName() == this->grid->dimxname) xdimindex = index;
            else if ((*j)->getName() == this->grid->dimyname) ydimindex = index;
            ++index;
        }
        if (xdimindex == -1 || ydimindex == -1) continue;

        int major = ydimindex < xdimindex ? 1 : 0;

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
    
    return ym;
}

void GridDataset::Calculated::DetectMajorDimension() throw(Exception)
{
    int ym = -1;
    // ydimmajor := true if (YDim, XDim)
    // ydimmajor := false if (XDim, YDim)

    // Traverse all data fields
    
    for (std::vector<Field *>::const_iterator i =
             this->grid->getDataFields().begin();
         i != this->grid->getDataFields().end(); ++i) {

        int xdimindex = -1, ydimindex = -1, index = 0;

        // Traverse all dimensions in each data field
        for (std::vector<Dimension *>::const_iterator j =
                 (*i)->getDimensions().begin();
             j != (*i)->getDimensions().end(); ++j) {
            if ((*j)->getName() == this->grid->dimxname) xdimindex = index;
            else if ((*j)->getName() == this->grid->dimyname) ydimindex = index;
            ++index;
        }
        if (xdimindex == -1 || ydimindex == -1) continue;

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

    std::vector<int32> rows, cols;
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

// The internal utility method to check if two vectors have overlapped.
// If not, return true.
static bool IsDisjoint(const std::vector<Field *> &l,
                       const std::vector<Field *> &r)
{
    for (std::vector<Field *>::const_iterator i = l.begin(); i != l.end(); ++i)
        {
            if (std::find(r.begin(), r.end(), *i) != r.end())
                return false;
        }
    return true;
}

// The internal utility method to check if two vectors have overlapped.
// If not, return true.
static bool IsDisjoint(std::vector<std::pair<Field *, std::string> > &l, const std::vector<Field *> &r)
{
    for (std::vector<std::pair<Field *, std::string> >::const_iterator i =
             l.begin(); i != l.end(); ++i) {
        if (std::find(r.begin(), r.end(), i->first) != r.end())
            return false;
    }
    return true;
}

// The internal utility method to check if vector s is a subset of vector b.
static bool IsSubset(const std::vector<Field *> &s, const std::vector<Field *> &b)
{
    for (std::vector<Field *>::const_iterator i = s.begin(); i != s.end(); ++i)
        {
            if (std::find(b.begin(), b.end(), *i) == b.end())
                return false;
        }
    return true;
}

// The internal utility method to check if vector s is a subset of vector b.
static bool IsSubset(std::vector<std::pair<Field *, std::string> > &s, const std::vector<Field *> &b)
{
    for (std::vector<std::pair<Field *, std::string> >::const_iterator i
             = s.begin(); i != s.end(); ++i) {
        if (std::find(b.begin(), b.end(), i->first) == b.end())
            return false;
    }
    return true;
}

SwathDataset::~SwathDataset()
{
    if (this->datasetid != -1) {
        //cerr<<"DETACH THIS SWATH" <<endl;
        SWdetach(this->datasetid);
    }

    std::for_each(this->dimmaps.begin(), this->dimmaps.end(), delete_elem());
    std::for_each(this->indexmaps.begin(), this->indexmaps.end(),
                  delete_elem());

    std::for_each(this->geofields.begin(), this->geofields.end(),
                  delete_elem());
    return; 

}

SwathDataset * SwathDataset::Read(int32 fd, const std::string &swathname)
    throw(Exception)
{
    SwathDataset *swath = new SwathDataset(swathname);

    // Openn this Swath object
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

int SwathDataset::ReadDimensionMaps(std::vector<DimensionMap *> &dimmaps)
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
        std::vector<char> namelist;
        std::vector<int32> offset, increment;

        namelist.resize(bufsize + 1);
        offset.resize(nummaps);
        increment.resize(nummaps);
        if (SWinqmaps(this->datasetid, &namelist[0], &offset[0], &increment[0])
            == -1)
            throw2("inquire dimmap", this->name);

        std::vector<std::string> mapnames;
        Utility::Split(&namelist[0], bufsize, ',', mapnames);
        int count = 0;
        for (std::vector<std::string>::const_iterator i = mapnames.begin();
             i != mapnames.end(); ++i) {
            std::vector<std::string> parts;
            Utility::Split(i->c_str(), '/', parts);
            if (parts.size() != 2) throw3("dimmap part", parts.size(),
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
void SwathDataset::ReadIndexMaps(std::vector<IndexMap *> &indexmaps)
    throw(Exception)
{
    int32 numindices, bufsize;

    if ((numindices = SWnentries(this->datasetid, HDFE_NENTIMAP, &bufsize)) ==
        -1)
        throw2("indexmap entry", this->name);
    if (numindices > 0) {
        // TODO: I have never seen any EOS2 files that have index map.
        std::vector<char> namelist;

        namelist.resize(bufsize + 1);
        if (SWinqidxmaps(this->datasetid, &namelist[0], NULL) == -1)
            throw2("inquire indexmap", this->name);

        std::vector<std::string> mapnames;
        Utility::Split(&namelist[0], bufsize, ',', mapnames);
        for (std::vector<std::string>::const_iterator i = mapnames.begin();
             i != mapnames.end(); ++i) {
            IndexMap *indexmap = new IndexMap();
            std::vector<std::string> parts;
            Utility::Split(i->c_str(), '/', parts);
            if (parts.size() != 2) throw3("indexmap part", parts.size(),
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

PointDataset * PointDataset::Read(int32 /*fd*/, const std::string &pointname)
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
    //Kent
    //cerr<<"goinside UnadjustedField"<<endl;
    //cerr<<"nelms"<<nelms<<endl;

    if (!this->valid) {
        //KENT
        unsigned int databufsize=nelms*this->datatypesize;
        this->data.resize(databufsize);
        //cerr<<"databufsize= "<< databufsize<<endl;
        //cerr<<"datelen= "<<this->datalen<<endl;
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
        //END KENT
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


void Utility::Split(const char *s, int len, char sep,
                    std::vector<std::string> &names)
{
    names.clear();
    for (int i = 0, j = 0; j <= len; ++j) {
        if ((j == len && len) || s[j] == sep) {
            std::string elem(s + i, j - i);
            names.push_back(elem);
            i = j + 1;
            continue;
        }
    }
}

void Utility::Split(const char *sz, char sep, std::vector<std::string> &names)
{
    Split(sz, (int)strlen(sz), sep, names);
}

bool Utility::ReadNamelist(const char *path,
                           int32 (*inq)(char *, char *, int32 *),
                           std::vector<std::string> &names)
{
    char *fname = const_cast<char *>(path);
    int32 bufsize;
    int numobjs;

    if ((numobjs = inq(fname, NULL, &bufsize)) == -1) return false;
    if (numobjs > 0) {
        std::vector<char> buffer;
        buffer.resize(bufsize + 1);
        if (inq(fname, &buffer[0], &bufsize) == -1) return false;
        Split(&buffer[0], bufsize, ',', names);
    }
    return true;
}
#endif

// vim:ts=4:sw=4:sts=4

