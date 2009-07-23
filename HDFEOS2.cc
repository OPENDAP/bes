/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
//
// Author:   Choonghwan Lee <clee83@hdfgroup.org>
// Copyright (c) 2009 The HDF Group
/////////////////////////////////////////////////////////////////////////////
#include "HDFEOS2.h"
#ifdef USE_HDFEOS2_LIB
#include <sstream>
#include <algorithm>
#include <functional>
#include <vector>
#include <map>


using namespace HDFEOS2;

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

/// The followings are convenient fuctions to throw exceptions with different
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
}

File * File::Read(const char *path) throw(Exception)
{
    File *file = new File(path);

    // Read information of all Grid objects in this file.
    if ((file->gridfd = GDopen(const_cast<char *>(file->path.c_str()),
                               DFACC_READ)) == -1) throw2("grid open", path);
    std::vector<std::string> gridlist;
    if (!Utility::ReadNamelist(file->path.c_str(), GDinqgrid, gridlist))
        throw2("grid namelist", path);
    for (std::vector<std::string>::const_iterator i = gridlist.begin();
         i != gridlist.end(); ++i)
        file->grids.push_back(GridDataset::Read(file->gridfd, *i));

    // Read information of all Swath objects in this file
    if ((file->swathfd = SWopen(const_cast<char *>(file->path.c_str()),
                                DFACC_READ)) == -1)
        throw2("swath open", path);
    std::vector<std::string> swathlist;
    if (!Utility::ReadNamelist(file->path.c_str(), SWinqswath, swathlist))
        throw2("swath namelist", path);
    for (std::vector<std::string>::const_iterator i = swathlist.begin();
         i != swathlist.end(); ++i)
        file->swaths.push_back(SwathDataset::Read(file->swathfd, *i));

    // We only obtain the name list of point objects but not don't provide
    // other information of these objects.
    // The client will only get the name list of point objects. 
    std::vector<std::string> pointlist;
    if (!Utility::ReadNamelist(file->path.c_str(), PTinqpoint, pointlist))
        throw2("point namelist", path);
    for (std::vector<std::string>::const_iterator i = pointlist.begin();
         i != pointlist.end(); ++i)
        file->points.push_back(PointDataset::Read(-1, *i));

    return file;
}

File * File::ReadAndAdjust(const char *path) throw(Exception)
{
    File *file = File::Read(path);

    for (std::vector<SwathDataset *>::const_iterator i =
             file->getSwaths().begin(); i != file->getSwaths().end(); ++i)
        (*i)->OverrideGeoFields();

    return file;
}

Field::~Field()
{
    std::for_each(this->dims.begin(), this->dims.end(), delete_elem());
    if (this->data) delete this->data;
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
            char dimlist[512]; // XXX: what an HDF-EOS2 developer recommeded
                        
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
    if (this->datasetid != -1)
        GDdetach(this->datasetid);
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
    if (!this->valid)
        this->ReadLongitudeLatitude();
    return this->ydimmajor;
}

bool GridDataset::Calculated::isOrthogonal() throw(Exception)
{
    if (!this->valid)
        this->ReadLongitudeLatitude();
    return this->orthogonal;
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
            if ((*j)->getName() == "XDim") xdimindex = index;
            else if ((*j)->getName() == "YDim") ydimindex = index;
            ++index;
        }
        if (xdimindex == -1 || ydimindex == -1) continue;
        int major = ydimindex < xdimindex ? 1 : 0;
        if (ym == -1)
            ym = major;
        // The dimension order for all data fields in a grid should be
        // consistent.      
        else if (ym != major)
            throw2("inconsistent XDim, YDim order", this->grid->getName());
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
static bool IsDisjoint(std::vector<std::pair<Field *, std::string> > &l,
                       const std::vector<Field *> &r)
{
    for (std::vector<std::pair<Field *, std::string> >::const_iterator i =
             l.begin(); i != l.end(); ++i) {
        if (std::find(r.begin(), r.end(), i->first) != r.end())
            return false;
    }
    return true;
}

// The internal utility method to check if vector s is a subset of vector b. 
static bool IsSubset(const std::vector<Field *> &s,
                     const std::vector<Field *> &b)
{
    for (std::vector<Field *>::const_iterator i = s.begin(); i != s.end(); ++i)
        {
            if (std::find(b.begin(), b.end(), *i) == b.end())
                return false;
        }
    return true;
}

// The internal utility method to check if vector s is a subset of vector b. 
static bool IsSubset(std::vector<std::pair<Field *, std::string> > &s,
                     const std::vector<Field *> &b)
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
    if (this->datasetid != -1)
        SWdetach(this->datasetid);

    std::for_each(this->dimmaps.begin(), this->dimmaps.end(), delete_elem());
    std::for_each(this->indexmaps.begin(), this->indexmaps.end(),
                  delete_elem());

    // There are four fields regarding geo-location fields: allgeofields,
    // geofields, adjustedgeofields and replacedgeofields.
    // 1. allgeofields should be ( geofields U adjustedgeofields U
    // replacedgeofields )
    if (!IsSubset(this->geofields, this->allgeofields))
        throw1("an element of geofields does not belong to allgeofields");
    if (!IsSubset(this->adjustedgeofields, this->allgeofields))
        throw1("an element of adjustedgeofields does not belong to allgeofields");
    if (!IsSubset(this->replacedgeofields, this->allgeofields))
        throw1("an element of replacedgeofields does not belong to allgeofields");
    // 2. Those three sets should be disjoint.
    if (!IsDisjoint(this->adjustedgeofields, this->geofields))
        throw1("geofields and adjustedgeofields are not disjoint");
    if (!IsDisjoint(this->adjustedgeofields, this->replacedgeofields))
        throw1("adjustedgeofields and replacedgeofields are not disjoint");
    if (!IsDisjoint(this->replacedgeofields, this->geofields))
        throw1("replacedgeofields and geofields are not disjoint");
    std::for_each(this->allgeofields.begin(), this->allgeofields.end(),
                  delete_elem());
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
        
    // Read dimension map 
    swath->ReadDimensionMaps(swath->dimmaps);
        
    // Read index maps, we haven't found any files with the Index Maps.
    swath->ReadIndexMaps(swath->indexmaps);

    // allgeofields owns all geo-location fields no matter how each element is
    // created.
    swath->allgeofields = swath->geofields;

    return swath;
}

void SwathDataset::OverrideGeoFields()
{
    // Create all necessary adjusted geo-location fields for all data fields
    // Usedgeofields is a global map of <unadjusted geo-location field name,
    // adjusted geo-location field name>(e.g.,lat, lat_1) for this swath. 
    std::map<std::string, std::string> usedgeofields;
        
    // Traverse all data fields of this swath
    for (std::vector<Field *>::const_iterator j = this->datafields.begin();
         j != this->datafields.end(); ++j) {
        
        // Associated is the name pair of <unadjusted geo-location field name,
        // adjusted geo-location field name>
        std::vector<std::pair<std::string, std::string> > associated;
                
        // This action essentially called methods to generate adjusted
        // geo-location fields by interpolation or subsampling.
        this->GetAssociatedGeoFields(**j, associated);

        // Check if there are multiple adjusted geo-location fields for one
        // geo-location field.
        // HDF-EOS2 can represent this relation, but OpenDAP does not want this. 
        // OPeNDAP currently only handles one dimension map for a swath.
        // This may get changed later. KY-2009-07-02
        for (std::vector<std::pair<std::string, std::string> >::const_iterator
                 k = associated.begin(); k != associated.end(); ++k) {
            std::map<std::string, std::string>::const_iterator l =
                usedgeofields.find(k->first);
            if (l == usedgeofields.end()){
                // This essentially build up usedgeofields
                usedgeofields[k->first] = k->second;
            }
            else if (l->second != k->second) {
                //cannot be handled by OPeNDAP.
                throw5("a geo-location field", k->first,
                       "is adjusted twice",
                       l->second, k->second);

            }
        }
    }

    // Replace unadjusted geo-location fields
    // Traverse the usedgeofields table.
    for (std::map<std::string, std::string>::const_iterator
             j = usedgeofields.begin(); j != usedgeofields.end(); ++j) {
        
        // If no dimension map(the name will not be mingled), no need to
        // adjust the names and do interpolations.
        if (j->first == j->second) continue;  

        std::vector<Field *>::iterator victim;
        for (victim = this->geofields.begin(); victim !=
                 this->geofields.end(); ++victim) {
            if ((*victim)->getName() == j->first)
                break;
        }
        if (victim == this->geofields.end())
            throw2("cannot find replaced geo-location fields", j->first);

        // The pair is adjusted field and the original name of the field
        std::vector<std::pair<Field *, std::string> >::iterator replacing;
        for (replacing = this->adjustedgeofields.begin();
             replacing != this->adjustedgeofields.end(); ++replacing) {
            if (replacing->first->getName() == j->second) {
                // If finding that the adjusted geo-field is one of the
                // usedgeofields, replacing.
                break;
            }
        }
        if (replacing == this->adjustedgeofields.end())
            throw2("cannot find replacing geo-location fields", j->second);

        // This is the final wrap-up of the work: 
                
        //1. Append the replaced geofield into a list,removing it from
        // geofields list.
        this->replacedgeofields.push_back(*victim);
        this->geofields.erase(victim);

        //2. Make the adjusted geofield name be the original one. (e.g.,
        // lat_1->lat)
        replacing->first->name = j->first;
                
        //3. Append the adjusted geofield to the geofields list. 
        //By doing this, geofields will cleanly represent the final version of
        // adjusted geo-fields applications want to use.
        this->geofields.push_back(replacing->first);
                
        //4. After replacing, removed the replaced geofield from adjusted
        // geofield list.
        this->adjustedgeofields.erase(replacing);
    }
}

void SwathDataset::ReadDimensionMaps(std::vector<DimensionMap *> &dimmaps)
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

// This class will do the work of obtaining the adjusted geo-location
// fields:including 1)handling the mangled name, 2)interpolation and
// subsampling
class HDFEOS2::SwathDimensionAdjustment
{
private:
    struct SwathFieldDimensionTuple
    {
        const Field *geofield;
        std::vector<Dimension *>::size_type datadimindex;
        std::vector<Dimension *>::size_type geodimindex;
        const SwathDataset::DimensionMap *dimmap;

        SwathFieldDimensionTuple(const Field *geo,
                                 std::vector<Dimension *>::size_type datadim,
                                 std::vector<Dimension *>::size_type geodim,
                                 const SwathDataset::DimensionMap *map)
            : geofield(geo), datadimindex(datadim), geodimindex(geodim),
              dimmap(map)
        {
        }
    };

public:
    SwathDimensionAdjustment(const SwathDataset &swath, const Field &datafield,
                             std::vector<std::pair<std::string, std::string> >
                             &associated)
        : swath(swath), datafield(datafield), associatedgeofields(associated)
    {
    }

    // This method executes 
    void Run()
    {
        this->PrepareDimensionMaps();
        this->CollectDimensionTuples();

        std::vector<std::vector<SwathFieldDimensionTuple>::size_type>
            queueindices;
        std::vector<unsigned int> datadimindices;

        this->TraverseCartesianProduct(0, queueindices, datadimindices);
    }

private:
    void PrepareDimensionMaps()
    {
        for (std::vector<SwathDataset::DimensionMap *>::const_iterator
                 i = this->swath.getDimensionMaps().begin();
             i != this->swath.getDimensionMaps().end(); ++i) {
            this->dimmaps.push_back(**i);
        }

        this->AddDefaultDimensionMaps();
    }

    // For the case that doesn't have dimension map, create a default
    // dimension map with offset 0, increment 1 to make it consistent.
    void AddDefaultDimensionMaps()
    {
        for (std::vector<Dimension *>::const_iterator
                 i = this->datafield.getDimensions().begin();
             i != this->datafield.getDimensions().end(); ++i) {
            SwathDataset::DimensionMap dimmap((*i)->getName(), (*i)->getName(),
                                              0, 1);
            this->dimmaps.push_back(dimmap);
        }
    }

    // Will create a list of tuples with the following elements:
    //(geofield name, dim. index in data field, dim. index in geofield,
    // dimension map)
    void CollectDimensionTuples()
    {
        // for each dimension associated with this field
        for (std::vector<Dimension *>::size_type i = 0;
             i < this->datafield.getDimensions().size(); ++i) {
            const Dimension *datadim = this->datafield.getDimensions()[i];
            // for each dimension maps relevant to this data field
            for (std::vector<SwathDataset::DimensionMap>::const_iterator
                     j = this->dimmaps.begin(); j != this->dimmaps.end(); ++j)
                {
                    if (j->getDataDimension() != datadim->getName()) continue;
                    // for each geo-location field whose dimensions contain
                    // the enclosing dimension map
                    for (std::vector<Field *>::const_iterator
                             k = this->swath.getGeoFields().begin();
                         k != this->swath.getGeoFields().end(); ++k) {
                        // for each dimension in enclosing geo-location field
                        for (std::vector<Dimension>::size_type l = 0;
                             l < (*k)->getDimensions().size(); ++l) {
                            const Dimension *geodim = (*k)->getDimensions()[l];
                            if (j->getGeoDimension() != geodim->getName())
                                continue;
                            // Append the list
                            // k:(address of) Geofield name, i: data field
                            // dimension index, l: geo-location field
                            // dimension index, 
                            // j:address of the current dimension map tuple
                            this->queue.push_back(SwathFieldDimensionTuple(*k,
                                                                           i,
                                                                           l,
                                                                           &*j
                                                                           ));
                    }
                }
            }
        }
    }

    // Traverse dimension tuple queue to form a complete information of an
    // adjusted geo-location field(e.g.:lat,lon)
    // All the information includes: all data indices that is associated with
    // this geo-field(?), the dimension map information
    void TraverseCartesianProduct(unsigned int geodimindex,
                                  std::vector<std::vector
                                  <SwathFieldDimensionTuple>::size_type>
                                  &queueindices,
                                  std::vector<unsigned int> &datadimindices)
    {
        for (std::vector<SwathFieldDimensionTuple>::size_type i = 0;
             i < this->queue.size(); ++i) {
            const SwathFieldDimensionTuple &tuple = this->queue[i];
            if (tuple.geodimindex != geodimindex) continue;
            if (geodimindex > 0) {
                // We only need to compare with the previous geo-location
                // name in order to gather the same geo-location field
                if (tuple.geofield->getName() !=
                    this->queue[queueindices[geodimindex - 1]].geofield->getName())
                    continue;
            }

            queueindices.push_back(i);
            datadimindices.push_back(tuple.datadimindex);

            if (geodimindex == tuple.geofield->getDimensions().size() - 1)
                // Go to the last index, it is the time to assemble the whole
                // list for this adjusted geo-location field
                HandleOneCartesianProduct(*tuple.geofield, queueindices,
                                          datadimindices);
            else
                // recursive traversing all dimension index for this
                // geo-location field
                TraverseCartesianProduct(geodimindex + 1, queueindices,
                                         datadimindices);

            queueindices.pop_back();
            datadimindices.pop_back();
        }
    }

    // Check if this is the case that has no dimension map.
    bool CheckDefaultDimensionMaps(const
                                   std::vector<
                                   std::vector<
                                   SwathFieldDimensionTuple
                                   >::size_type
                                   >
                                   &queueindices) const
    {
        bool puredefault = true;
        for (std::vector<
                 std::vector<SwathFieldDimensionTuple>::size_type
                 >::const_iterator i = queueindices.begin();
             i != queueindices.end(); ++i) {
            const SwathDataset::DimensionMap *dimmap = this->queue[*i].dimmap;
            if (dimmap->getGeoDimension() == dimmap->getDataDimension() &&
                dimmap->offset == 0 &&
                dimmap->increment == 1)
                continue;
            puredefault = false;
            break;
        }
        return puredefault;
    }

    // This method will make a unique name for the same geo_location field
    // with different dimension map 
    // The name will be like latitude_5:10_5:10(5 is the offset and 10 is the
    // increment in a dimension map)
    std::string MangleAdjustedGeoFieldName(const Field &geofield,
                                           const std::vector<
                                           std::vector<
                                           SwathFieldDimensionTuple
                                           >::size_type
                                           > &queueindices)
    {
        std::ostringstream ss;
        ss << geofield.getName();
        for (std::vector<
                 std::vector<SwathFieldDimensionTuple
                 >::size_type>::const_iterator i = queueindices.begin();
             i != queueindices.end(); ++i) {
            const SwathDataset::DimensionMap *dimmap = this->queue[*i].dimmap;
            ss << "_" << dimmap->offset << ":" << dimmap->increment;
        }
        return ss.str();
    }

        
    static bool AlreadyAdjusted(std::pair<Field *, std::string> fieldpair,
                                std::string name)
    {
        return fieldpair.first->getName() == name;
    }

    // Assemble dimension information for a geo-location field
    void HandleOneCartesianProduct(const Field &geofield,
                                   const std::vector<
                                   std::vector<SwathFieldDimensionTuple
                                   >::size_type> &queueindices,
                                   std::vector<unsigned int> &datadimindices)
    {
        // no dimension map case
        if (this->CheckDefaultDimensionMaps(queueindices))
            this->associatedgeofields.push_back(std::make_pair(geofield.getName(), geofield.getName()));
        else {
            // obtain unique mangled name
            std::string adjustedname = this->MangleAdjustedGeoFieldName(geofield, queueindices);
            // check if this geo-location field exists or not, if existing,
            // just add to the list;otherwise, create a new one.
            std::vector<std::pair<Field *, std::string> >::const_iterator i =
                std::find_if(this->swath.adjustedgeofields.begin(),
                             this->swath.adjustedgeofields.end(),
                             std::bind2nd(std::ptr_fun(AlreadyAdjusted),
                                          adjustedname));
            if (i != this->swath.adjustedgeofields.end())
                this->associatedgeofields.push_back(std::make_pair(i->second, i->first->getName()));
            else {
                Field *adjusted = new Field();
                adjusted->name = adjustedname;
                adjusted->rank = geofield.getRank();
                adjusted->type = DFNT_FLOAT64;
                for (std::vector<std::vector<SwathFieldDimensionTuple>::size_type>::size_type i = 0; i < queueindices.size(); ++i) {
                    const Dimension *dim =
                        this->datafield.getDimensions()[datadimindices[i]];
                    Dimension *clone = new Dimension(dim->getName(),
                                                     dim->getSize());
                    adjusted->dims.push_back(clone);
                }
                {
                    std::vector<AdjustedFieldData<float64>::Map> maps;
                    for (std::vector<std::vector<SwathFieldDimensionTuple>::size_type>::const_iterator i = queueindices.begin(); i != queueindices.end(); ++i) {
                        AdjustedFieldData<float64>::Map map;
                        map.increment = this->queue[*i].dimmap->getIncrement();
                        map.offset = this->queue[*i].dimmap->getOffset();
                        maps.push_back(map);
                    }
                    adjusted->data = new AdjustedFieldData<float64>
                        (&this->swath, &geofield, maps, adjusted->dims);
                }
                this->swath.adjustedgeofields.push_back(std::make_pair(adjusted, geofield.getName()));
                this->swath.allgeofields.push_back(adjusted);
                this->associatedgeofields.push_back(std::make_pair(geofield.getName(), adjusted->getName()));
            }
        }
    }

private:
    const SwathDataset &swath;
    const Field &datafield;
    std::vector<SwathDataset::DimensionMap> dimmaps;
    std::vector<SwathFieldDimensionTuple> queue;
    std::vector<std::pair<std::string, std::string> > &associatedgeofields;
};

void SwathDataset::GetAssociatedGeoFields(const Field &datafield,
                                          std::vector<std::pair<std::string,
                                          std::string> > &associated) const
{
    SwathDimensionAdjustment adjust(*this, datafield, associated);
    adjust.Run();
}

PointDataset::~PointDataset()
{
}

PointDataset * PointDataset::Read(int32 fd, const std::string &pointname)
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

const char * UnadjustedFieldData::get()
{
    if (!this->valid) {
        this->data.resize(this->datalen);
        if (this->reader(this->datasetid,
                         const_cast<char *>(this->fieldname.c_str()),
                         NULL, NULL, NULL, &this->data[0]) == -1)
            return 0;
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

template<typename T>
AdjustedFieldData<T>::AdjustedFieldData(const SwathDataset *swath,
                                        const Field *basegeofield,
                                        const std::vector<Map> &dimmaps,
                                        const std::vector<Dimension *>
                                        &datadims)
    : swath(swath), basegeofield(basegeofield), dimmaps(dimmaps),
      datadims(datadims)
{
    for (typename std::vector<Map>::const_iterator i = this->dimmaps.begin();
         i != this->dimmaps.end(); ++i) {
        assert_throw0(i->getIncrement());
    }
}

template<typename T>
const char * AdjustedFieldData<T>::get()
{
    if (!this->valid) {
        this->data.reserve(this->length());

        bool alreadyfetched = this->basegeofield->getData().get() != 0;
        if (!alreadyfetched)
            this->basegeofield->getData().get();
        std::vector<AdjustInfo> info;
        this->Adjust(info);
        if (!alreadyfetched)
            this->basegeofield->getData().drop();

        this->valid = true;
    }
    return &this->data[0];
}

template<typename T>
void AdjustedFieldData<T>::drop()
{
    if (this->valid) {
        this->valid = false;
        this->data.resize(0);
    }
}

template<typename T>
int AdjustedFieldData<T>::length() const
{
    if (this->datadims.empty())
        return 0;

    int numelem = 1;
    for (std::vector<Dimension *>::const_iterator i = this->datadims.begin();
         i != this->datadims.end(); ++i)
        numelem *= (*i)->getSize();
    return numelem * sizeof(T);
}

template<typename T>
int AdjustedFieldData<T>::GetGeoIndexFromDataIndexLE(int dataindex,
                                                     const Map &dimmap)
{
    int r = -1;
    assert_throw0(dimmap.getIncrement() > 0);
    if (dimmap.getOffset() >= 0)
        r = (dataindex - dimmap.getOffset()) / dimmap.getIncrement();
    else
        r = dataindex / dimmap.getIncrement() + -dimmap.getOffset();
    return r;
}

template<typename T>
int AdjustedFieldData<T>::GetGeoIndexFromDataIndex(int dataindex,
                                                   const Map &dimmap)
{
    int r = -1;
    assert_throw0(dimmap.getIncrement() < 0);
    if (dimmap.getOffset() >= 0)
        r = (dataindex - dimmap.getOffset()) * -dimmap.getIncrement();
    else
        r = dataindex * -dimmap.getIncrement() + -dimmap.getOffset();
    return r;
}

template<typename T>
int AdjustedFieldData<T>::GetDataIndexFromGeoIndex(int geoindex,
                                                   const Map &dimmap)
{
    int r = -1;
    assert_throw0(dimmap.getIncrement() > 0);
    if (dimmap.getOffset() >= 0)
        r = geoindex * dimmap.getIncrement() + dimmap.getOffset();
    else
        r = (geoindex + dimmap.getOffset()) * dimmap.getIncrement();
    return r;
}

// Overall we use distance as a weight to do interpolation
template<typename T>
void AdjustedFieldData<T>::Adjust(std::vector<AdjustInfo> &adjustinfo)
{
    std::vector<Dimension>::size_type geodimindex = adjustinfo.size();

    assert_range_throw0(geodimindex, 0,
                        this->basegeofield->getDimensions().size());
    assert_range_throw0(geodimindex, 0, this->dimmaps.size());
    assert_range_throw0(geodimindex, 0, this->datadims.size());

    const Dimension &geodim =
        *this->basegeofield->getDimensions()[geodimindex];
    const typename AdjustedFieldData<T>::Map &dimmap =
        this->dimmaps[geodimindex];
    const Dimension &datadim =
        *this->datadims[geodimindex];

    AdjustInfo info;
    for (int32 i = 0; i < datadim.getSize(); ++i) {
        // if "increment" is positive, dimension of geo field has smaller
        // number of elements
        // Interpolation case
        if (dimmap.increment > 0) {
            int geoindex = this->GetGeoIndexFromDataIndexLE(i, dimmap);
            // beyond the left boundary
            if (geoindex <= 0) {
                info.leftpoint = 0;
                info.rightpoint = 1;
                if (info.rightpoint < geodim.getSize()) {
                    info.leftdist =
                        i - this->GetDataIndexFromGeoIndex(info.leftpoint,
                                                           dimmap);
                    info.rightdist =
                        this->GetDataIndexFromGeoIndex(info.rightpoint,
                                                       dimmap) - i;
                }
                else {
                    info.rightpoint = 0;
                    info.leftdist = -1;
                    info.rightdist = -1;
                }
            }
            // beyound the right boundary
            else if (geoindex >= geodim.getSize() - 1) {
                info.rightpoint = geodim.getSize() - 1;
                info.leftpoint = info.rightpoint - 1;
                if (info.leftpoint >= 0) {
                    info.leftdist =
                        i - this->GetDataIndexFromGeoIndex(info.leftpoint,
                                                           dimmap);
                    info.rightdist =
                        this->GetDataIndexFromGeoIndex(info.rightpoint,
                                                       dimmap) - i;
                }
                else {
                    info.leftpoint = geodim.getSize() - 1;
                    info.leftdist = -1;
                    info.rightdist = -1;
                }
            }
            // middle
            else {
                info.leftpoint = geoindex;
                info.rightpoint = info.leftpoint + 1;
                info.leftdist =
                    i - this->GetDataIndexFromGeoIndex(info.leftpoint, dimmap);
                info.rightdist =
                    this->GetDataIndexFromGeoIndex(info.rightpoint, dimmap) - i;
            }
        }
        // if "increment" is negative, dimension of data field has smaller
        // number of elements
        // subsampling case
        else {
            int geoindex = this->GetGeoIndexFromDataIndex(i, dimmap);
            // beyond the left boundary
            if (geoindex <= 0) {
                info.leftpoint = 0;
                info.rightpoint = -dimmap.getIncrement();
                if (info.rightpoint < geodim.getSize()) {
                    info.leftdist = geoindex - info.leftpoint;
                    info.rightdist = info.rightpoint - geoindex;
                }
                else {
                    info.rightpoint = 0;
                    info.leftdist = -1;
                    info.rightdist = -1;
                }
            }
            // beyond the right boundary
            else if (geoindex >= geodim.getSize() - 1) {
                info.rightpoint = geodim.getSize() - 1;
                info.leftpoint = info.rightpoint - -dimmap.getIncrement();
                if (info.leftpoint >= 0) {
                    info.leftdist = geoindex - info.leftpoint;
                    info.rightdist = info.rightpoint - geoindex;
                }
                else {
                    info.leftpoint = geodim.getSize() - 1;
                    info.leftdist = -1;
                    info.rightdist = -1;
                }
            }
            // middle
            else {
                info.leftpoint = geoindex;
                info.rightpoint = geoindex;
                info.leftdist = -1;
                info.rightdist = -1;
            }
        }

        {
            if (info.leftpoint < 0 || info.rightpoint >= geodim.getSize()) {
                assert_throw0(false);
            }
            if (info.rightpoint > 0 && info.leftpoint < geodim.getSize() - 1) {
                if (info.leftdist == -1 && info.rightdist == -1) {
                    assert_throw0(info.leftpoint == info.rightpoint);
                }
                else if (dimmap.getIncrement() > 0) {
                    assert_throw0(info.leftdist + info.rightdist ==
                                  dimmap.getIncrement());
                    assert_throw0(info.leftpoint + 1 == info.rightpoint);
                }
                else {
                    assert_throw0(info.leftdist + info.rightdist ==
                                  -dimmap.getIncrement());
                    assert_throw0(info.leftpoint + -dimmap.getIncrement() ==
                                  info.rightpoint);
                }
            }
            else {
                assert_throw0(info.leftpoint == info.rightpoint);
                assert_throw0(info.leftdist == -1 && info.rightdist == -1);
            }
        }

        adjustinfo.push_back(info);
        if (geodimindex < this->dimmaps.size() - 1)
            this->Adjust(adjustinfo);
        else
            this->FinishOnePoint(adjustinfo);
        adjustinfo.pop_back();
    }
}

template<typename T>
void AdjustedFieldData<T>::FinishOnePoint(const std::vector<AdjustInfo>
                                          &adjustinfo)
{
    std::vector<int> pivots;
    T value = this->AccumulatePivots(adjustinfo, pivots);

    const char *d = reinterpret_cast<const char *>(&value);
    for (unsigned int i = 0; i < sizeof(T); ++i)
        this->data.push_back(*(d + i));
}

template<typename T>
T AdjustedFieldData<T>::AccumulatePivots(const std::vector<AdjustInfo>
                                         &adjustinfo, std::vector<int> &pivots)
{
    typename std::vector<AdjustInfo>::size_type dimindex = pivots.size();
    T accumulated = 0;

    for (int i = 0; i < 2; ++i) {
        const AdjustInfo &info = adjustinfo[dimindex];
        int point = i == 0 ? info.leftpoint : info.rightpoint;
        pivots.push_back(point);

        int distsum = info.leftdist + info.rightdist;
        float64 factor = i == 1 ? info.leftdist : info.rightdist;
        factor /= distsum;

        if (dimindex < adjustinfo.size() - 1) {
            T f = this->AccumulatePivots(adjustinfo, pivots);
            accumulated += factor * f;
        }
        else {
            long dataindex = 0;
            for (unsigned int j = 0; j < pivots.size() - 1; ++j) {
                dataindex += pivots[j];
                dataindex *=
                    this->basegeofield->getDimensions()[j + 1]->getSize();
            }
            dataindex += *pivots.rbegin();

            T value;
            switch (this->basegeofield->getType()) {
#define HANDLE_TYPE(tid, t) case tid: { const t *v; v = reinterpret_cast<const t *>(&this->basegeofield->getData().peek()[dataindex * sizeof(t)]); value = *v; } break;
                HANDLE_TYPE(DFNT_FLOAT32, float32)
                    HANDLE_TYPE(DFNT_FLOAT64, float64)
                    HANDLE_TYPE(DFNT_INT8, int8)
                    HANDLE_TYPE(DFNT_UINT8, uint16)
                    HANDLE_TYPE(DFNT_INT16, int16)
                    HANDLE_TYPE(DFNT_UINT16, uint16)
                    HANDLE_TYPE(DFNT_INT32, int32)
                    HANDLE_TYPE(DFNT_UINT32, uint32)
                    case DFNT_UCHAR8:
            case DFNT_CHAR8:
            default:
                assert_throw0(false);
                break;
            }
            accumulated += factor * value;
        }

        pivots.pop_back();
    }
    return accumulated;
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

// vim:ts=4:sw=4:sts=4
#endif
