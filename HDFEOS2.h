#ifdef USE_HDFEOS2_LIB
#ifndef _HDFEOS2_H
#define _HDFEOS2_H
#ifdef _WIN32
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#endif


#include <string>
#include <vector>
#include <map>
#include "mfhdf.h"
#include "hdf.h"
#include "HdfEosDef.h"

#ifdef _WIN32
#ifdef _MSC_VER
#pragma warning(disable:4290)
#endif
#endif

///   HDFEOS2 is the library used by the handler to obtain information of
/// EOS objects.
///
///   It currently handles EOS Swath and Grid, which covers 99%
/// of all EOS2 files. It not only retrieves the general information of an
/// EOS physical data field but also geo-location fields of the data field.
/// For Grid, latitude and longitude fields will be retrieved for all types
/// of projections supported by EOS2. For Swath, latitude and longitude will
/// also be retrieved when dimension map is involved.
///
///   Please distinguish this HDFEOS2 library(inside HDF4 handler) from
/// the HDFEOS2 library implemented by Raytheon. This HDFEOS2 library uses
/// the HDFEOS2 library implemented by Raytheon and create calls convenient
/// for the handler to pass the geolocation and metadata information to DAP.
/// It can be viewed as an high-level convenient C++ EOS2 library
/// specifically designed for HDF4 handler. Users need to link with HDF-EOS2
/// C library provided by Raytheon when configuring the handler.
/// Please refer to the installation file for details.
///
/// @author Choonghwan Lee <clee83@hdfgroup.org>
/// @note  reviewed and edited by Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2009 The HDF Group
///
/// All rights reserved.
namespace HDFEOS2
{
    /// The Exception class for handling exceptions caused by using this
    /// HDFEOS2 library 
    class Exception : public std::exception
    {
    public:
        /// Constructor
        Exception(const std::string &msg)
            : message(msg)
        {
        }

        virtual ~Exception() throw()
        {
        }

        virtual const char * what() const throw()
        {
            return this->message.c_str();
        }

    protected:
        const std::string message;
    };

    /// This class is similar to a standard vector class but with only limited
    /// features.
    ///
    /// The main unique feature of this class is that it doesn't call default
    /// constructor to initialize the value of each element when resizing
    /// this class. It may reduce some overheads. However, it only provides
    /// push_back,reserve and resize functions.  FieldData class and its
    /// subclasses use this class to hold elements in the field.
    template<typename T> class LightVector
    {
    public:
        LightVector()
            : data(0), length(0), capacity(0)
        {
        }

        LightVector(const LightVector<T> &that)
        {
            this->data = new T[that.length];
            for (unsigned int i = 0; i < that.length; ++i)
                this->data[i] = that[i];
            this->length = that.length;
            this->capacity = that.length;
        }

        ~LightVector()
        {
            if (this->data) delete [] data;
        }

        void push_back(const T &d)
        {
            this->reserve(this->length + 1);
            this->data[this->length] = d;
            ++this->length;
        }

        void reserve(unsigned int len)
        {
            if (this->capacity >= len)
                return;

            this->capacity = len;
            T *old = this->data;
            this->data = new T[len];
            if (old) {
                for (unsigned int i = 0; i < this->length; ++i)
                    this->data[i] = old[i];
                delete [] old;
            }
        }

        void resize(unsigned int len)
        {
            /// do not call constructor for each element
            if (this->length == len)
                return;
            else if (this->length < len) {
                if (this->capacity < len) {
                    this->capacity = len;
                    T *old = this->data;
                    this->data = new T[len];
                    if (old) {
                        for (unsigned int i = 0; i < this->length; ++i)
                            this->data[i] = old[i];
                        delete [] old;
                    }
                }
            }
            else {
                this->capacity = len;
                T *old = this->data;
                this->data = new T[len];
                for (unsigned int i = 0; i < len; ++i)
                    this->data[i] = old[i];
                if (old)
                    delete [] old;
            }
            this->length = len;
        }

        unsigned int size() const
        {
            return this->length;
        }

        T & operator[](unsigned int i)
        {
            return this->data[i];
        }
        const T & operator[](unsigned int i) const
        {
            return this->data[i];
        }

        LightVector<T> & operator=(const LightVector<T> &that)
        {
            if (this != &that) {
                this->data = new T[that.length];
                for (unsigned int i = 0; i < that.length; ++i)
                    this->data[i] = that[i];
                this->length = that.length;
                this->capacity = that.length;
            }
            return *this;
        }

    private:
        T *data;
        unsigned int length;
        unsigned int capacity;
    };

    /// The base class of unadjusted FieldData and adjusted FieldData.
    class FieldData
    {
    protected:
        FieldData()
            : valid(false)
        {
        }

    public:
        virtual ~FieldData()
        {
        }

        /// Peek the value, it will return NULL if this class doesn't hold
        /// the data.
        const char * peek() const;
        
        /// It will get the value even if the value is not in the FieldData
        /// buffer(LightVector). 
        virtual const char * get() = 0;
        
        /// Release the buffer by resizing the LightVector. 
        virtual void drop() = 0;
        virtual int length() const = 0;

    protected:
        bool valid;
        LightVector<char> data;
    };

    class SwathDimensionAdjustment;

    /// It repersents one dimension of an EOS object including fields,
    /// geo-location fields and others.
    /// It holds the dimension name and the size of that dimension.
    class Dimension
    {
    public:
        const std::string & getName() const { return this->name; }
        int32 getSize() const { return this->dimsize; }

    protected:
        Dimension(const std::string &name, int32 dimsize)
            : name(name), dimsize(dimsize)
        {
        }

    protected:
        std::string name;
        int32 dimsize;

        friend class Dataset;
        friend class SwathDimensionAdjustment;
    };

    /// One instance of this class represents one field or one geo-location
    /// field
    class Field
    {
    public:
        virtual ~Field();

    public:

        /// Get the name of this field
        const std::string & getName() const { return this->name; }
        
        /// Get the dimension rank of this field
        int32 getRank() const { return this->rank; }
        
        /// Get the data type of this field
        int32 getType() const { return this->type; }
        
        /// Get the list of dimensions
        const std::vector<Dimension *> & getDimensions() const
        { return this->dims; }
        
        /// Return an instance of FieldData, which refers to this field.
        /// Then one can obtain the data array from this instance.
        FieldData & getData() const { return *this->data; }
        
        /// Obtain fill value of this field.
        const std::vector<char> & getFillValue() const { return this->filler; }

    protected:
        std::string name;
        int32 rank;
        int32 type;
        std::vector<Dimension *> dims;
        FieldData *data;
        std::vector<char> filler;

        friend class Dataset;
        friend class SwathDimensionAdjustment;
        friend class SwathDataset;
    };

    /// Representing one attribute in grid or swath
    class Attribute
    {
    public:
        const std::string & getName() const { return this->name; }
        int32 getType() const { return this->type; }
        const std::vector<char> & getValue() const { return this->value; }

    protected:
        std::string name;
        int32 type;
        std::vector<char> value;

        friend class Dataset;
    };

    /// Base class of Grid and Swath Dataset. It provides public methods
    /// to obtain name, number of dimensions, number of data fields and
    /// attributes.
    class Dataset
    {
    public:
        const std::string & getName() const
        { return this->name; }
        const std::vector<Dimension *> & getDimensions() const
        { return this->dims; }
        const std::vector<Field *> & getDataFields() const
        { return this->datafields; }
        const std::vector<Attribute *> & getAttributes() const
        { return this->attrs; }

    protected:
        Dataset(const std::string &n)
            : datasetid(-1), name(n)
        {
        }

        virtual ~Dataset();

        /// Obtain dimensions from Swath or Grid by calling EOS2 APIs such as
        /// GDnentries and GDinqdims.
        void ReadDimensions(int32 (*entries)(int32, int32, int32 *),
                            int32 (*inq)(int32, char *, int32 *),
                            std::vector<Dimension *> &dims) throw(Exception);
        
        /// Obtain field data information from Swath or Gird by calling EOS2
        /// APIs such as GDnentries, GDinqfields, GDfieldinfo, GDreadfield and
        /// GDgetfillvalue.
        void ReadFields(int32 (*entries)(int32, int32, int32 *),
                        int32 (*inq)(int32, char *, int32 *, int32 *),
                        intn (*fldinfo)(int32, char *, int32 *, int32 *,
                                        int32 *, char *),
                        intn (*readfld)(int32, char *, int32 *, int32 *,
                                        int32 *, VOIDP),
                        intn (*getfill)(int32, char *, VOIDP),
                        bool geofield,
                        std::vector<Field *> &fields)
            throw(Exception);
        
        /// Obtain Grid or Swath attributes by calling EOS2 APIs such as
        /// GDinqattrs, GDattrinfo and GDreadattr.
        void ReadAttributes(int32 (*inq)(int32, char *, int32 *),
                            intn (*attrinfo)(int32, char *, int32 *, int32 *),
                            intn (*readattr)(int32, char *, VOIDP),
                            std::vector<Attribute *> &attrs)
            throw(Exception);

    protected:
        int32 datasetid;
        std::string name;
        std::vector<Dimension *> dims;
        std::vector<Field *> datafields;
        std::vector<Attribute *> attrs;
    };

    /// This class mainly handles the calculation of longitude and latitude of
    /// an EOS Grid.
    class GridDataset : public Dataset
    {
    public:
        class Info
        {
        public:
        
            /// dimension size of XDim.
            int32 getX() const { return this->xdim; }
                
            /// dimension size of YDim.
            int32 getY() const { return this->ydim; }
                
            /// Geo-location value(latitude and longtitude for geographic
            /// projection) at upper-left corner of the grid.
            /// It consists of two values. The information was obtained from
            /// EOS2 API GDgridinfo. These values are used by
            /// EOS2 GDij2ll function to calculate latitude and longitude.
            const float64 * getUpLeft() const { return this->upleft; }
                
            /// Geo-location value(latitude and longtitude for geographic
            /// projection) at lower-right corner of the grid.
            /// It consists of two values. The information was obtained from
            /// EOS2 API GDgridinfo. These values are used by
            /// EOS2 GDij2ll function to calculate latitude and longitude.
            const float64 * getLowRight() const { return this->lowright; }

        protected:
            int32 xdim;
            int32 ydim;
            float64 upleft[2];
            float64 lowright[2];

            friend class GridDataset;
        };

        class Projection
        {
        public:
            /// These methods are for obtaining projection information from
            /// EOS2 APIs.
        
            /// Obtain projection code such as geographic projection or
            /// sinusoidal projection.
            int32 getCode() const { return this->code; }
                
            /// Obtain GCTP zone code used by UTM projection
            int32 getZone() const { return this->zone; }
                
            /// Obtain GCTP spheriod code
            int32 getSphere() const { return this->sphere; }
                
            /// Obtain GCTP projection parameter array from EOS API GDprojinfo
            const float64 * getParam() const { return this->param; }
                
            /// Obtain pix registration code from EOS API GDpixreginfo
            int32 getPix() const { return this->pix; }
                
            /// Obtain origin code from EOS API GDorigininfo
            int32 getOrigin() const { return this->origin; }

        protected:
            int32 code;
            int32 zone;
            int32 sphere;
            float64 param[16];
            int32 pix;
            int32 origin;

            friend class GridDataset;
        };

        /// This class holds corresponding calculated latitude and longitude
        /// values of a grid field.
        class Calculated
        {
        public:
            const float64 * peekLongitude() const;
            const float64 * getLongitude() throw(Exception);
            const float64 * peekLatitude() const;
            const float64 * getLatitude() throw(Exception);
            void dropLongitudeLatitude();

            /// We follow C-major convention. Normally YDim is major.
            /// Sometimes it is not. .
            bool isYDimMajor() throw(Exception);
            /// The projection can be either 1-D or 2-D. For 1-D, the method
            /// returns true. Otherwise, return false.
            bool isOrthogonal() throw(Exception);

        protected:
        
                
            Calculated(const GridDataset *grid)
                : grid(grid), valid(false), ydimmajor(false), orthogonal(false)
            {
            }

            Calculated & operator=(const Calculated &victim)
            {
                if (this != &victim) {
                    this->grid = victim.grid;
                    this->valid = victim.valid;
                    this->lons = victim.lons;
                    this->lats = victim.lats;
                    this->ydimmajor = victim.ydimmajor;
                    this->orthogonal = victim.orthogonal;
                }
                return *this;
            }

            /// We follow C-major convention. Normally YDim is major. But
            /// sometimes it is not. We have to check.
            void DetectMajorDimension() throw(Exception);
                
            ///  This method will detect if this projection is 1-D or 2-D.
            /// For 1-D, it is treated as "orthogonal".
            void DetectOrthogonality() throw(Exception);
            void ReadLongitudeLatitude() throw(Exception);

        protected:
            const GridDataset *grid;

            bool valid;
            LightVector<float64> lons;
            LightVector<float64> lats;
            bool ydimmajor;
            bool orthogonal;

            friend class GridDataset;
        };

    public:
        /// Read all information regarding this Grid.
        static GridDataset * Read(int32 fd, const std::string &gridname)
            throw(Exception);
        virtual ~GridDataset();

        /// Return all information of Info class.
        const Info & getInfo() const { return this->info; }
        
        /// Return all information of Projection class.
        const Projection & getProjection() const { return this->proj; }
        
        /// Return all information of Calculated class.
        Calculated & getCalculated() const;

    private:
        GridDataset(const std::string &name)
            : Dataset(name), calculated(0)
        {
        }

    protected:
        Info info;
        Projection proj;
        mutable Calculated calculated;
    };

    class SwathDimensionAdjustment;
    class File;

    /// This class retrieves and holds all information of an EOS swath.
    /// In addition to retrieve the general data field information.
    /// This class especially handles the calculation of longitude and
    /// latitude of EOS Swath. 
    /// Unlike retrieving latitude and longitude from EOS grid, extra efforts
    /// need to be made to retrieve geo-location fields when dimension map is
    /// used. 

    class SwathDataset : public Dataset
    {
    public:
        /// Dimension map indeed exists in some HDF-EOS2 files. We have to
        /// interpolate or subsample the existing geo-location fields so that 
        /// the number of element(extent) for each dimension of an adjusted
        /// geo-location field can be the same as that of the corresponding
        /// data field.
        /// Please refer to HDF-EOS user's guide to understand the meaning of
        /// offset and increment.
        ///
        /// each dimension map tuple includes four elements: 
        /// [dimension name of this geo-location field(e.g., coarse-cross),
        /// dimension name of this data field(e.g., cross), offset(e.g., 0),
        /// increment(e.g., 2)]
        /// Dimension map information described by this class is used to link
        /// between a data field and a geo-location field. 
        
        class DimensionMap
        {
        public:
            const std::string & getGeoDimension() const
            { return this->geodim; }
            const std::string & getDataDimension() const
            { return this->datadim; }
            int32 getOffset() const { return this->offset; }
            int32 getIncrement() const { return this->increment; }

        protected:
            DimensionMap(const std::string &geodim, const std::string &datadim,
                         int32 offset, int32 increment)
                : geodim(geodim), datadim(datadim), offset(offset),
                  increment(increment)
            {
            }

        protected:
            std::string geodim;
            std::string datadim;
            int32 offset;
            int32 increment;

            friend class SwathDataset;
            friend class SwathDimensionAdjustment;
        };

        /// Index map is another way to "compress" geo-location fields in
        /// swath. However, we haven't found any examples in real HDF-EOS2
        /// swath files.
        class IndexMap
        {
        public:
            const std::string & getGeoDimension() const { return this->geo; }
            const std::string & getDataDimension() const { return this->data; }
            const LightVector<int32> & getIndices() const
            { return this->indices; }

        protected:
            std::string geo;
            std::string data;
            LightVector<int32> indices;

            friend class SwathDataset;
        };

    public:
        /// This method reads the information of all fields in a swath
        static SwathDataset * Read(int32 fd, const std::string &swathname)
            throw(Exception);
        virtual ~SwathDataset();

        /// Obtain all information regarding dimension maps of this swath
        const std::vector<DimensionMap *> & getDimensionMaps() const
        { return this->dimmaps; }
        const std::vector<IndexMap *> & getIndexMaps() const
        { return this->indexmaps; }
        
        /// Obtain all information regarding geo-fields of this swath
        const std::vector<Field *> & getGeoFields() const
        { return this->geofields; }
        
        /// For dimension map case, geo-location fields need to be resized
        /// (either interpolated or subsampled), this method will provide the
        /// adjusted(resized) geo-location fields.
        const std::vector<std::pair<Field *, std::string> > &
        getAdjustedGeoFields() const { return this->adjustedgeofields; }

        ///For a dimension map case, geo-location fields need to be resized
        /// (either interpolated or subsampled). Given a data field, this
        /// method will provide an associated geo-location field. 
        /// "associated" is a list of "unadjusted" and "adjusted"
        /// geo-location field of this data field.
        void GetAssociatedGeoFields(const Field &datafield,
                                    std::vector<std::pair<std::string,
                                    std::string> > &associated) const;

    private:
        SwathDataset(const std::string &name)
            : Dataset(name)
        {
        }

        /// Replace unadjusted geo-location fields by adjusted geo-location
        /// fields in this swath
        void OverrideGeoFields();
        
        /// get all information of dimension maps in this swath
        void ReadDimensionMaps(std::vector<DimensionMap *> &dimmaps)
            throw(Exception);
        void ReadIndexMaps(std::vector<IndexMap *> &indexmaps)
            throw(Exception);

    protected:
        std::vector<DimensionMap *> dimmaps;
        std::vector<IndexMap *> indexmaps;
        
        /// This vector may need to be updated by GetAssociatedGeoFields.
        mutable std::vector<Field *> allgeofields;

        /// The geo-location fields that serves both as "input" and "output".
        /// Applications will obtain geo-location fields stored by this vector.
        /// Elements of this vector should be adjusted geo-location fields if
        /// OverrideGeoFields is called and a dimension map exists and is used.
        /// Otherwise, elements of this vector are unadjusted geo-location
        /// fields.
        std::vector<Field *> geofields;
        
        /// This vector stores adjusted geo-location fields based on dimension
        /// map. 
        /// This vector may be updated by GetAssociatedGeoFields.
        mutable std::vector<std::pair<Field *, std::string> >
        adjustedgeofields;
        
        /// This vector stores the replaced geo-location fields for future
        /// interopolation or subsampling. 
        std::vector<Field *> replacedgeofields;

        friend class SwathDimensionAdjustment;
        friend class File;
    };

    /// We currently cannot handle point data. So far we only find two EOS
    /// files that use EOS2 point. We will delay the implementation until we
    /// find more point EOS files.
    class PointDataset : public Dataset
    {
    public:
        static PointDataset * Read(int32 fd, const std::string &pointname)
            throw(Exception);
        virtual ~PointDataset();

    private:
        PointDataset(const std::string &name)
            : Dataset(name)
        {
        }
    };

    /// Interface for users to access elements of any original field data
    /// (including both data fields and geolocation fields)
    /// Please note that for Grid, there are no geo-location fields, so this
    /// class won't retrieve geo-location fields for Grid. 
    class UnadjustedFieldData : public FieldData
    {
    private:
        UnadjustedFieldData(int32 id, const std::string &name, int32 len,
                            intn (*readfld)(int32, char *, int32 *, int32 *,
                                            int32 *, VOIDP))
            : datasetid(id), fieldname(name), datalen(len), reader(readfld)
        {
        }

    public:
        virtual const char * get();
        virtual void drop();
        virtual int length() const;

    protected:
        int32 datasetid;
        std::string fieldname;
        int32 datalen;
        intn (*reader)(int32, char *, int32 *, int32 *, int32 *, VOIDP);

        friend class Dataset;
    };

    /// Interface for users to access elements of any adjusted swath
    /// geo-location field data
    /// Please note that this class won't retrieve latitude and longtiude
    /// of data fields in a Grid object
    /// For obtaining latitude and longitude of data fields in a Grid object, 
    /// please refer to Class GridDataset. 
    template<typename T>
    class AdjustedFieldData : public FieldData
    {
    public:
        struct Map
        {
            int32 offset;
            int32 increment;

            int32 getOffset() const { return this->offset; }
            int32 getIncrement() const { return this->increment; }
        };

    private:

        /// SwathDataset: enclosing a swath object information
        /// basegeofield: the original geo-location field 
        /// dimmaps: the dimension map that includes offset and increment
        /// datadims: will give the final size of geo-location field
        AdjustedFieldData(const SwathDataset *swath,
                          const Field *basegeofield,
                          const std::vector<Map> &dimmaps,
                          const std::vector<Dimension *> &datadims);

    public:
        virtual const char * get();
        virtual void drop();
        virtual int length() const;

    protected:
        const SwathDataset *swath;
        const Field *basegeofield;
        std::vector<Map> dimmaps;
        std::vector<Dimension *> datadims;

    protected:
        struct AdjustInfo
        {
            /// The index of the left point for interpolation
            int leftpoint;
            /// The index of the right point for interpolation
            int rightpoint;
            /// The distance from the current location to the left point
            int leftdist;
            /// The distance from the current location to the right point
            int rightdist; 
        };

        /// Traverse each geo-location in a data field and calculate the
        /// geo-location value(e.g., latitude and longitude)
        void Adjust(std::vector<AdjustInfo> &adjustinfo);
        
        /// wrapper of AccumalatePivots(),  used by Adjust()
        void FinishOnePoint(const std::vector<AdjustInfo> &adjustinfo);
        
        /// Collect and calculate the geolocation values of 2^N points
        /// for interpolation(N is the number of dimension of a data field)
        /// This Method does the interpolation from unadjusted geo-location
        /// fields to adjusted geo-location fields
        T AccumulatePivots(const std::vector<AdjustInfo> &adjustinfo,
                           std::vector<int> &pivots);
        
        /// This method is used to obtain index of geolocation from the index
        ///of a datafield when increment is greater than 0(interpolation case).
        int GetGeoIndexFromDataIndexLE(int dataindex, const Map &dimmap);
        
        /// This method is used to obtain index of geolocation from the index
        /// of a datafield when increment is less then 0(sampling case).
        int GetGeoIndexFromDataIndex(int dataindex, const Map &dimmap);
        
        /// This method is used to obtain index of a data field from the index
        /// of a geo-location field for interpolcation.
        /// It is used to get distance between the data point and one of its
        /// 2^N pivots during interpolation. 
        int GetDataIndexFromGeoIndex(int geoindex, const Map &dimmap);

        friend class SwathDimensionAdjustment;
    };

    /// This class retrieves all information from an EOS file. It is a
    /// container for Grid, Swath and Point objects.
    class File
    {
    public:
        static File * Read(const char *path) throw(Exception);
        
        /// It will read all information from an EOS file with the adjusted
        /// geo-location fields of swath objects.
        static File * ReadAndAdjust(const char *path) throw(Exception);

        virtual ~File();

        const std::string & getPath() const { return this->path; }
        const std::vector<GridDataset *> & getGrids() const
        { return this->grids; }
        const std::vector<SwathDataset *> & getSwaths() const
        { return this->swaths; }
        const std::vector<PointDataset *> & getPoints() const
        { return this->points; }

    protected:
        File(const char *path)
            : path(path), gridfd(-1), swathfd(-1)
        {
        }

    protected:
        std::string path;
        std::vector<GridDataset *> grids;
        std::vector<SwathDataset *> swaths;
        std::vector<PointDataset *> points;

    private:
        int32 gridfd;
        int32 swathfd;
    };


    struct Utility
    {

        /// From a string separated by a separator to a list of string,
        /// for example, split "ab,c" to {"ab","c"}
        static void Split(const char *s, int len, char sep,
                          std::vector<std::string> &names);
        
        /// Assume sz is Null terminated string.
        static void Split(const char *sz, char sep,
                          std::vector<std::string> &names);
        
        /// Call inquiry functions twice to get a list of strings. 
        static bool ReadNamelist(const char *path,
                                 int32 (*inq)(char *, char *, int32 *),
                                 std::vector<std::string> &names);
    };

}
#endif


#endif
