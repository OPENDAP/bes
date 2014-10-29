/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf4 data handler for the OPeNDAP data server.
//
// Author:   Kent Yang,Choonghwan Lee <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////

//#include "InternalErr.h"
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


#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include "mfhdf.h"
#include "hdf.h"
#include "HdfEosDef.h"


// Add MISR SOM projection header
#include "misrproj.h"

#include "HDFEOS2EnumType.h"

#ifdef _WIN32
#ifdef _MSC_VER
#pragma warning(disable:4290)
#endif
#endif

/// HDFEOS2.h and HDFEOS2.cc include the core part of retrieving HDF-EOS2 Grid and Swath 
/// metadata info and translate them into DAP DDS and DAS.
///
///   It currently handles EOS Swath and Grid, which covers 99%
/// of all EOS2 files. It not only retrieves the general information of an
/// EOS physical data field but also geo-location fields of the data field.
/// For Grid, latitude and longitude fields will be retrieved for all types
/// of projections supported by EOS2. For Swath, latitude and longitude will
/// also be retrieved when dimension map is involved.
/// The translation of Grid and Swath to DAP DDS and DAS is not a pure syntax
/// translation. It addes sematic meaning by following the CF conventions.
/// The core part of the translation is to create coordinate variable list and dimension 
/// list for each field(variable). The variable/attribute/dimension names are 
/// also made to follow CF.
///
/// @author Kent Yang, Choonghwan Lee <myang6@hdfgroup.org>
///
/// Copyright (C) 2010-2012 The HDF Group
///
/// All rights reserved.
namespace HDFEOS2
{
    /// The Exception class for handling exceptions caused by using this
    /// HDFEOS2 library 
    class Exception:public std::exception
    {
        public:
            /// Constructor
            Exception (const std::string & msg)
                : message (msg), isHDFEOS2 (true)
            {
            }

            /// Destructor
            virtual ~ Exception () throw ()
            {
            }

            /// Exception message
            virtual const char *what () const throw ()
            {
                return this->message.c_str ();
            }

            /// check if this file is an HDFEOS2 file, this is necessary
            /// since the handler also supports non-HDFEOS2 HDF4 products.
            virtual bool getFileType ()
            {
                return this->isHDFEOS2;
            }

            /// set file type be either HDF-EOS2 or HDF4(non-HDFEOS2)
            virtual void setFileType (bool isHDFEOS2)
            {
                this->isHDFEOS2 = isHDFEOS2;
            }

            /// set exception message
            virtual void setException (std::string message)
            {
                this->message = message;
            }

        protected:
            std::string message;
            bool isHDFEOS2;
    };

    /// This class is similar to a standard vector class but with only limited
    /// features.
    ///
    /// The main unique feature of this class is that it doesn't call default
    /// constructor to initialize the value of each element when resizing
    /// this class. It may reduce some overheads. However, it only provides
    /// push_back,reserve and resize functions.  FieldData class and its
    /// subclasses use this class to hold elements in the field.
    template < typename T > class LightVector {
        public:
            LightVector ()
                : data (0), length (0), capacity (0) {
            }

            LightVector (const LightVector < T > &that)
            {
                this->data = new T[that.length];
                for (unsigned int i = 0; i < that.length; ++i)
                    this->data[i] = that[i];
                this->length = that.length;
                this->capacity = that.length;
            }

            ~LightVector () {
                if (this->data)
                    delete[]data;
            }

            void push_back (const T & d)
            {
                this->reserve (this->length + 1);
                this->data[this->length] = d;
                ++this->length;
            }

            void reserve (unsigned int len)
            {
                if (this->capacity >= len)
                    return;

                this->capacity = len;
                T *old = this->data;

                this->data = new T[len];
                if (old) {
                    for (unsigned int i = 0; i < this->length; ++i)
                        this->data[i] = old[i];
                    delete[]old;
                }
            }

            void resize (unsigned int len)
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
                            delete[]old;
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
                        delete[]old;
                }
                this->length = len;
            }

            unsigned int size () const
            {
                return this->length;
            }

            T & operator[] (unsigned int i)
            {
                return this->data[i];
            }
            const T & operator[] (unsigned int i) const
            {
                return this->data[i];
            }

            LightVector < T > &operator= (const LightVector < T > &that)
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
            T * data;
            unsigned int length;
            unsigned int capacity;
    };

    class SwathDimensionAdjustment;

    /// It repersents one dimension of an EOS object including fields,
    /// geo-location fields and others.
    /// It holds the dimension name and the size of that dimension.
    class Dimension
    {
        public:
            const std::string & getName () const
            {
                return this->name;
            }
            int32 getSize () const
            {
                return this->dimsize;
            }

        protected:
            Dimension (const std::string & name, int32 dimsize)
                : name (name), dimsize (dimsize)
            {
            }

        protected:
            std::string name;
            int32 dimsize;

        friend class File;
        friend class Dataset;
        friend class SwathDimensionAdjustment;
    };

    /// One instance of this class represents one field or one geo-location
    /// field
    class Field
    {
        public:
            Field ()
                :fieldtype (0), condenseddim (false), iscoard (false), ydimmajor (true), speciallon (false), specialformat (0), haveaddedfv (false), addedfv (-9999.0), dmap (false)/*, field_cache(0)*/

            {
            }
            virtual ~ Field ();

        public:

            /// Get the name of this field
            const std::string & getName () const
            {
                return this->name;
            }

            /// Get the CF name of this field
            const std::string & getNewName () const
            {
                return this->newname;
            }

            /// Get the dimension rank of this field
            int32 getRank () const 
            {	
                return this->rank;
            }

            /// Get the data type of this field
            int32 getType () const
            {
                return this->type;
            }

            /// Get the list of the corrected dimensions 
            const std::vector < Dimension * >&getCorrectedDimensions () const
            {
                return this->correcteddims;
            }

            /// Get the pointer of the corrected dimensions
            std::vector < Dimension * >*getCorrectedDimensionsPtr ()
            {
                return &(this->correcteddims);
            }

            /// Set the list of the corrected dimensions
            void setCorrectedDimensions (std::vector < Dimension * >dims)
            {
                correcteddims = dims;
            }

            /// Get the "coordinates" attribute value
            const std::string getCoordinate () const
            {
                return this->coordinates;
            }

            /// Set the "coordinates" attribute value
            void setCoordinates (std::string coor)
            {
                coordinates = coor;
            }

            /// Get the "units" attribute value 
            const std::string getUnits () const
            {
                return this->units;
            }

            /// Set the "units" attribute value
            void setUnits (std::string uni)
            {
                units = uni;
            }

            /// Get the _Fillvalue" attribute value
            const float getAddedFillValue () const
            {
                return this->addedfv;
            }

            // Add the "_FillValue" attribute
            // This is for supporting the old versions of HDF-EOS2 data(AIRS) because
            // some data products have fillvalue(-9999.0) but don't specify the fillvalue.
            // We add the fillvalue to ensure the netCDF client can successfully display the data.
            // KY 2013-06-30
            void addFillValue (float fv)
            {
                addedfv = fv;
            }

            /// Have the added fillvaue?
            const bool haveAddedFillValue () const
            {
                return this->haveaddedfv;
            }

            /// set the flag for the added FillValue
            void setAddedFillValue (bool havefv)
            {
                haveaddedfv = havefv;
            }

            
            // Obtain fieldtype values
            // For fieldtype values:
            // 0 is general fields
            // 1 is latitude.
            // 2 is longtitude.    
            // 3 is defined level.
            // 4 is an inserted natural number.
            // 5 is time.

            const int getFieldType () const
            {
                return this->fieldtype;
            }

            /// Get the list of dimensions
            const std::vector < Dimension * >&getDimensions () const
            {
                return this->dims;
            }

            /// Obtain fill value of this field.
            const std::vector < char >&getFillValue () const
            {
                return this->filler;
            }

            /// Obtain the ydimmajor info.
            const bool getYDimMajor () const
            {
                return this->ydimmajor;
            }

            /// Obtain the speciallon info.
            const bool getSpecialLon () const
            {
                return this->speciallon;
            }

            /// Obtain the special lat/lon format info.
            const int getSpecialLLFormat () const
            {
                return this->specialformat;
            }

            /// Obtain if the dimension can be condensed.  
            const bool getCondensedDim () const
            {
                return this->condenseddim;
            }

            /// Have dimension map or not
            const bool UseDimMap () const
            {
                return this->dmap;
            }

//No need to check field_cache in the DDS level. 
//May remove the debugging info. totally in the next release.
//KY 2014-10-23
#if 0
            /// Have field cache or not
            // 2 exactly cached 
            // 1 maybe cached, need to check the 
            // file size when accessing the data
            const short UseFieldCache () const
            {
                return this->field_cache;
            }
#endif
        protected:

            // field name
            std::string name;

            // field dimension rank
            int32 rank;

            // field  datatype
            int32 type;

            // field dimensions with original dimension names
            std::vector < Dimension * >dims;

            // field dimensions with the corrected(CF) dimension names
            std::vector < Dimension * >correcteddims;
            
            // This is for reading the fillvalue. 
            // HDF-EOS2 provides a special routine to read fillvalue.
            // Up to HDF-EOS2 version 2.18, this is the only field attribute
            // that HDF-EOS2 APIs provide. KY 2013-07-01
            
            std::vector < char >filler;

            // Coordinate attributes that includes coordinate variable list.
            std::string coordinates;

            // newname is to record CF Grid/Swath name + "_"+ CF field name(special characters replaced by underscores).
            std::string newname;


            // This flag will specify the fieldtype.
            // 0 means this field is general field.
            // 1 means this field is lat.
            // 2 means this field is lon.
            // 3 means this field is other dimension variable.
            // 4 means this field is added other dimension variable with nature number.
            // 5 means time, but currently the units is not correct.
            int fieldtype;

            //  Latitude and longitude retrieved by HDF-EOS2 are always
            //  2-D arrays(XDim * YDim). However, for some projections
            // (geographic etc.), latiude and longitude can be condensed to
            //  1-D arrays. The handler will track such projections and condense
            //  the latitude and longitude to 1-D arrays. This can reduce       
            //  the disk storage and can greatly improve the performance for 
            //  the visualization tool to access the latitude and longitde.
            //  condenseddim is the flag internally used by the handler to track this.
            bool condenseddim;

            //   This flag is to mark if the data should follow COARDS.
            bool iscoard;

            //   This flag is to check if the field is YDim major(temp(YDim,XDim). This
            //   flag is necessary when calling GDij2ll to retrieve latitude and longitude.
            bool ydimmajor;

            // SOme special longitude is from 0 to 360.We need to check this case with this flag.
            bool speciallon;

            // This flag specifies the special latitude/longitude coordinate format
            // The latiude and longitude should represent as DDDMMMSSS format
            // However, we found some files simply represent lat/lon as -180.0000000 or -90.000000. 
            // Some other files use default. So we this flag to record this and correctly retrieve
            // the latitude and longitude values in the DAP output.
            // 0 means normal 
            // 1 means the coordinate is -180 to 180
            // 2 means the coordinate is default(0)
            int specialformat;

            // CF units attribute(mostly to add latitude and longitude CF units).
            std::string units;

            // Some data products have fillvalue(-9999.0) but don't specify the fillvalue.
            // We add the fillvalue to ensure the netCDF client can successfully display the data.
            // haveaddedfv and addedfv are to check if having added fillvalues.
            bool haveaddedfv;
            float addedfv;

            // Check if this swath uses the dimension map. 
            bool dmap;

            // Flag to indicate the field cache. Now this only applies to some HDF-EOS2 grid lat/lon fields.
            //short field_cache;

            /// Add a special_flag to indicate if the field data needs to be adjusted(dimension map case)
            /// KY 2009-12-3
            // This may not necessary. Check in the next release. KY 2012-09-20
            // bool need_adjustment;

        friend class Dataset;
        friend class SwathDimensionAdjustment;
        friend class GridDataset;
        friend class SwathDataset;
        friend class File;
    };

#if 0
    // For future improvement of the modulization
    class GeoField:public Field
    {

        protected:
            bool condenseddim;
            bool ydimmajor;
            bool speciallon;

    };
#endif

    /// Representing one attribute in grid or swath
    class Attribute
    {
        public:

            /// Obtain the original attribute name
            const std::string & getName () const
            {
                return this->name;
            }

            /// Obtain the CF attribute name(special characters are replaced by underscores)
            const std::string & getNewName () const
            {
                return this->newname;
            }

            /// Obtain the attribute data type
            int32 getType () const
            {
                return this->type;
            }

            /// Get the number of elements of this attribute
            int32 getCount () const
            {
                return this->count;
            }

            /// Obtain the attribute values(in vector <char> form)
            const std::vector < char >&getValue () const
            {
                return this->value;
            }

        protected:

            /// Attribute name
            std::string name;

            /// Attribute CF name(special characters are replaced by underscores)
            std::string newname;

            /// Attribute datatype(in HDF4 representation)
            int32 type;

            /// Attribute number of elements
            int32 count;

            /// Attribute value
            std::vector < char >value;

        friend class Dataset;
    };

    /// Base class of Grid and Swath Dataset. It provides public methods
    /// to obtain name, number of dimensions, number of data fields and
    /// attributes.
    class Dataset
    {
        public:
            /// Get the Dataset name
            const std::string & getName () const
            {
                return this->name;
            }
            /// Get the dimension list of this grid or swath
            const std::vector < Dimension * >&getDimensions () const
            {
                return this->dims;
            }
            /// Get data fields 
            const std::vector < Field * >&getDataFields () const
            {
                return this->datafields;
            }

            /// Get the attributes
            const std::vector < Attribute * >&getAttributes () const
            {
                return this->attrs;
            }

            /// Get the scale and offset type  
            const SOType getScaleType () const
            {
                return this->scaletype;
            }


        protected:
            Dataset (const std::string & n)
                : datasetid (-1), addfvalueattr(false),name (n),scaletype(DEFAULT_CF_EQU)
            {
            }

            virtual ~ Dataset ();

            /// Obtain dimensions from Swath or Grid by calling EOS2 APIs such as
            /// GDnentries and GDinqdims.
            void ReadDimensions (int32 (*entries) (int32, int32, int32 *),
                int32 (*inq) (int32, char *, int32 *),
                std::vector < Dimension * >&dims) throw (Exception);

            /// Obtain field data information from Swath or Gird by calling EOS2
            /// APIs such as GDnentries, GDinqfields, GDfieldinfo, GDreadfield and
            /// GDgetfillvalue.
            void ReadFields (int32 (*entries) (int32, int32, int32 *),
                int32 (*inq) (int32, char *, int32 *, int32 *),
                intn (*fldinfo) (int32, char *, int32 *, int32 *,
                int32 *, char *),
                intn (*readfld) (int32, char *, int32 *, int32 *,
                int32 *, VOIDP),
                intn (*getfill) (int32, char *, VOIDP),
                bool geofield, std::vector < Field * >&fields) 
                throw (Exception);

            /// Obtain Grid or Swath attributes by calling EOS2 APIs such as
            /// GDinqattrs, GDattrinfo and GDreadattr.
            void ReadAttributes (int32 (*inq) (int32, char *, int32 *),
                intn (*attrinfo) (int32, char *, int32 *, int32 *),
                intn (*readattr) (int32, char *, VOIDP),
                std::vector < Attribute * >&attrs)
                throw (Exception);
 
            /// Set scale and offset type
            /// MODIS data has three scale and offset rules. 
            /// They are 
            /// MODIS_EQ_SCALE: raw_data = scale*data + offset
            /// MODIS_MUL_SCALE: raw_data = scale*(data -offset)
            /// MODIS_DIV_SCALE: raw_data = (data-offset)/scale
            void SetScaleType(const std::string EOS2ObjName) throw(Exception);

        protected:
            /// Grid and Swath ID
            int32 datasetid;

            /// This flag is for CERES TRMM data that has fillvalues(huge real number) but doesn't
            /// have the fillvalue attribute. We need to add a fillvalue.
            /// Actually we also need to handle AIRS -9999.0 fillvalue case with this flag.
            bool addfvalueattr;

            /// Dataset name
            std::string name;

            /// Dataset dimension list
            std::vector < Dimension * >dims;

            /// Dataset field list
            std::vector < Field * >datafields;

            /// Dataset attribute list(This is equivalent to vgroup attributes)
            std::vector < Attribute * >attrs;

            /// dimension name to coordinate variable name map list
            /// this is for building the coordinate variables associated with each variables.
            std::map < std::string, std::string > dimcvarlist;

            /// original coordinate variable name to corrected(CF) coordinate variable name map list
            std::map < std::string, std::string > ncvarnamelist;

            /// original dimension name to corrected(CF) dimension name map list
            std::map < std::string, std::string > ndimnamelist;

            // Some MODIS files don't use the CF linear equation y = scale * x + offset,
            // The scaletype distinguishs products following different scale and offset rules.
            // Note the assumption here: we assume that all fields will
            // use one scale and offset function in a file. If
            // multiple scale and offset equations are used in one file, our
            // function will fail. So far only one scale and offset equation is
            //  applied for NASA HDF-EOS2 files we observed. KY 2012-6-13
            // Since I found one MODIS product(MOD09GA) uses different scale offset
            // equations for different grids. I had to move the scaletype to the
            // group level. Hopefully this is the final fix. Truly hope that 
            // this will not happen at the field level since it will be too messy to 
            // check.  KY 2012-11-21
            SOType scaletype;

        friend class File;
    };

    /// This class mainly handles the calculation of longitude and latitude of
    /// an EOS Grid.
    class GridDataset:public Dataset
    {
        public:
            class Info
            {
                public:

                    /// dimension size of XDim.
                    int32 getX ()const
                    {
                        return this->xdim;
                    }

                    /// dimension size of YDim.
                    int32 getY () const
                    {
                        return this->ydim;
                    }

                    /// Geo-location value(latitude and longtitude for geographic
                    /// projection) at upper-left corner of the grid.
                    /// It consists of two values. The information was obtained from
                    /// EOS2 API GDgridinfo. These values are used by
                    /// EOS2 GDij2ll function to calculate latitude and longitude.
                    const float64 *getUpLeft () const
                    {
                        return this->upleft;
                    }

                    /// Geo-location value(latitude and longtitude for geographic
                    /// projection) at lower-right corner of the grid.
                    /// It consists of two values. The information was obtained from
                    /// EOS2 API GDgridinfo. These values are used by
                    /// EOS2 GDij2ll function to calculate latitude and longitude.
                    const float64 *getLowRight () const
                    {
                        return this->lowright;
                    }

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
                    int32 getCode () const
                    {
                        return this->code;
                    }

                    /// Obtain GCTP zone code used by UTM projection
                    int32 getZone () const
                    {
                        return this->zone;
                    }

                    /// Obtain GCTP spheriod code
                    int32 getSphere () const
                    {
                        return this->sphere;
                    }

                    /// Obtain GCTP projection parameter array from EOS API GDprojinfo
                    const float64 *getParam () const
                    {
                        return this->param;
                    }

                    /// Obtain pix registration code from EOS API GDpixreginfo
                    int32 getPix () const
                    {
                        return this->pix;
                    }

                    /// Obtain origin code from EOS API GDorigininfo
                    int32 getOrigin () const
                    {
                        return this->origin;
                    }

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

                    /// We follow C-major convention. Normally YDim is major.
                    /// Sometimes it is not. .
                    bool isYDimMajor () throw (Exception);
                    /// The projection can be either 1-D or 2-D. For 1-D, the method
                    /// returns true. Otherwise, return false.
                    /// bool isOrthogonal () throw (Exception);

                protected:

                    Calculated (const GridDataset * grid)
                        : grid (grid),  ydimmajor (false)
                    {
                    }

                    Calculated & operator= (const Calculated & victim)
                    {
                        if (this != &victim) {
                            this->grid = victim.grid;
                            this->ydimmajor = victim.ydimmajor;
                        }
                        return *this;
                    }

                    /// We follow C-major convention. Normally YDim is major. But
                    /// sometimes it is not. We have to check.
                    void DetectMajorDimension () throw (Exception);

                    /// Find a field and check which dimension is major for this field. If Y dimension is major, return 1; if X dimension is major, return 0, otherwise throw exception. (LD -2012/01/16)
                    int DetectFieldMajorDimension () throw (Exception);


                protected:
                    const GridDataset *grid;
                    bool ydimmajor;

                friend class GridDataset;
                friend class File;
            };

            public:
                /// Read all information regarding this Grid.
                static GridDataset *Read (int32 fd, const std::string & gridname) throw (Exception);

                virtual ~ GridDataset ();

                /// Return all information of Info class.
                const Info & getInfo () const
                {
                    return this->info;
                }

                /// Return all information of Projection class.
                const Projection & getProjection () const
                {
                    return this->proj;
                }

                /// Return all information of Calculated class.
                Calculated & getCalculated () const;

                /// set dimxname, "XDim" may be "LonDim" or "nlon"
                void setDimxName (std::string dxname)
                {
                    dimxname = dxname;
                }
                /// set dimyname, "YDim" may be "LatDim" or "nlat"
                void setDimyName (std::string dyname)
                {
                    dimyname = dyname;
                }

                /// Obtain the ownllflag  info.
                const bool getLatLonFlag () const
                {
                    return this->ownllflag;
                }

            private:
                GridDataset (const std::string & name)
                    : Dataset (name), calculated (0), ownllflag (false), iscoard (false)
                {
                }

            protected:

                /// Info consists of the sizes, lower right and upper left coordinates of the grid.
                Info info;

                /// Projection info. of the grid.
                Projection proj;

                /// A class to detect the dimension major etc. temp(YDim,XDim) or temp(XDim,YDim)
                /// This is used to calculate the latitude and longitude
                mutable Calculated calculated;

                /// This flag is for AIRS level 3 data since the lat/lon for this product is under the geolocation grid.
                bool ownllflag;

                /// If this grid is following COARDS
                bool iscoard;

                /// Each grid has its own lat and lon. So separate them from the rest fields.
                Field *latfield;
                Field *lonfield;

                /// Each grid has its own dimension names for latitude and longtiude. Also separate them from the rest fields.
                std::string dimxname;
                std::string dimyname;

            friend class File;

        };

        class File;

        /// This class retrieves and holds all information of an EOS swath.
        /// The old way is to retrieve dimension map data in this class.
        /// Now we put the way to handle the data in HDFEOS2ArraySwathDimMap.cc.

        class SwathDataset:public Dataset
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
                        const std::string & getGeoDimension () const
                        {
                            return this->geodim;
                        }
                        const std::string & getDataDimension () const
                        {
                            return this->datadim;
                        }
                        int32 getOffset () const
                        {
                            return this->offset;
                        }
                        int32 getIncrement () const
                        {
                            return this->increment;
                        }

                    protected:
                        DimensionMap (const std::string & geodim, const std::string & datadim, int32 offset, int32 increment)
                            : geodim (geodim), datadim (datadim), offset (offset), increment (increment)
                        {
                        }

                    protected:

                        std::string geodim;
                        std::string datadim;
                        int32 offset;
                        int32 increment;

                    friend class SwathDataset;
                    friend class SwathDimensionAdjustment;
                    friend class File;
                };

                /// Index map is another way to "compress" geo-location fields in
                /// swath. However, we haven't found any examples in real HDF-EOS2
                /// swath files. So we don't even comment this.
                class IndexMap
                {
                    public:
                        const std::string & getGeoDimension () const
                        {
                            return this->geo;
                        }
                        const std::string & getDataDimension () const
                        {
                            return this->data;
                        }
                        const LightVector < int32 > &getIndices () const
                        {
                            return this->indices;
                        }

                    protected:
                        std::string geo;
                        std::string data;
                        LightVector < int32 > indices;

                    friend class SwathDataset;
                };

            public:
                /// This method reads the information of all fields in a swath
                static SwathDataset *Read (int32 fd, const std::string & swathname) throw (Exception);

                virtual ~ SwathDataset ();

                /// Obtain all information regarding dimension maps of this swath
                const std::vector < DimensionMap * >&getDimensionMaps () const
                {
                    return this->dimmaps;
                }
                const std::vector < IndexMap * >&getIndexMaps () const
                {
                    return this->indexmaps;
                }

                /// Obtain all information regarding geo-fields of this swath
                const std::vector < Field * >&getGeoFields () const
                {
                    return this->geofields;
                }


                /// Set and get the number of dimension map
                void set_num_map (int this_num_map)
                {
                    num_map = this_num_map;
                }
                int get_num_map () const
                {
                    return num_map;
                };

            private:
                 SwathDataset (const std::string & name)
                    : Dataset (name) {
                 }


                /// get all information of dimension maps in this swath
                /// The number of maps will return for future subsetting
                int ReadDimensionMaps (std::vector < DimensionMap * >&dimmaps) throw (Exception);

                /// Not used.
                void ReadIndexMaps (std::vector < IndexMap * >&indexmaps) throw (Exception);

            protected:

                /// dimension map list.
                std::vector < DimensionMap * >dimmaps;

                /// Not used.
                std::vector < IndexMap * >indexmaps;

                /// This set includes the dimension list of all coordinate variables except the missing coordinate variables.
                std::set < std::string > nonmisscvdimlist;

                /// The geo-location fields.
                std::vector < Field * >geofields;

                /// Return the number of dimension map to correctly handle the subsetting case 
                ///  without dimension map.
                int num_map;

            friend class File;
        };

        /// We currently cannot handle point data. So far we only find two EOS
        /// files that use EOS2 point. We will delay the implementation until we
        /// find more point EOS files.
        class PointDataset:public Dataset
        {
            public:
                static PointDataset *Read (int32 fd, const std::string & pointname) throw (Exception);
                virtual ~ PointDataset ();

            private:
                PointDataset (const std::string & name)
                    : Dataset (name)
                {
                }
        };


        /// This class retrieves all information from an EOS file. It is a
        /// container for Grid, Swath and Point objects.
        class File
        {
            public:

                /// Read all the information in this file from the EOS2 APIs.
                static File *Read (const char *path,int32 gridfd,int32 swathfd) throw (Exception);


                /// Read and prepare. This is the main method to make the DAP output CF-compliant.
                /// All dimension(coordinate variables) information need to be ready.
                /// All special arrangements need to be done in this step.
                void Prepare(const char *path) throw(Exception);

                /// Check if this is a special 1d grid.
                bool check_special_1d_grid() throw(Exception);


                /// This is for the case(AIRS level 3) that the latitude and longitude of all grids are put under 
                /// a special location grid. So all grids share one lat/lon.
                bool getOneLatLon ()
                {
                    return this->onelatlon;
                }

                /// Destructor
                ~File ();

                const std::string & getPath () const
                {
                    return this->path;
                }

                const std::vector < GridDataset * >&getGrids () const
                {
                    return this->grids;
                }

                const std::vector < SwathDataset * >&getSwaths () const
                {
                    return this->swaths;
                }

                const std::vector < PointDataset * >&getPoints () const
                {
                    return this->points;
                }

                const std::string get_first_grid_name() const 
                { return this->grids[0]->getName();}
                /// Get the scale and offset type  
                /// const SOType getScaleType () const
                /// Move to the group level since I found that 
                /// the different Scale-offset fuctions was used.
                /// {
                ///    return this->scaletype;
                /// }


            protected:
                File (const char *path)
                    : path (path), onelatlon (false), iscoard (false), gridfd (-1), swathfd (-1)
                {
                }

            protected:

                /// The absolute path of the file.
                std::string path;

                /// Grids 
                std::vector < GridDataset * >grids;

                /// Swaths
                std::vector < SwathDataset * >swaths;

                /// Points
                std::vector < PointDataset * >points;

                // This is for the case that only one lat/lon
                // set is provided for multiple grid cases.
                //  The current case is that the user provides
                // the latitude and longitude under a special grid.
                // By default, the grid name is "location". This will
                // cover the AIRS grid case.
                bool onelatlon;

                /// If this file should follow COARDS, this is necessary to cover one pair lat/lon grids case.
                bool iscoard;

                /* 
                  A grid's X-dimension can have different names: XDim, LatDim, etc.
                 * Y-dimension also has YDim, LonDim, etc.
                 * This function returns the name of X-dimension which is used in
                 * the given file.
                 * For better performance, we check the first grid or swath only.
                 */
                std::string get_geodim_x_name ();
                std::string get_geodim_y_name ();

                // Internal function used by  
                // get_geodim_x_name and get_geodim_y_name functions.
                // This function is not intended to be used outside the 
                // get_geodim_x_name and get_geodim_y_name functions.
                void _find_geodim_names ();

                std::string _geodim_x_name;
                std::string _geodim_y_name;
                static const char *_geodim_x_names[];
                static const char *_geodim_y_names[];

                /** 
                 * In some cases, values of latitude and longitude are
                 * stored in data fields. Since the latitude field and
                 * longitude field do not have unique names
                 * (e.g., latitude field can be "latitude", "Lat", ...),
                 * we need to retrieve the field name.
                 * The following two functions does this job.
                 * For better performance, we check the first grid or swath only.
                 */
                std::string get_latfield_name ();
                std::string get_lonfield_name ();

                // Internal function used by  
                // get_latfield_name and get_lonfield_name functions.
                // This function is not intended to be used outside 
                // the get_latfield_name and get_lonfield_name functions.
                void _find_latlonfield_names ();

                std::string _latfield_name;
                std::string _lonfield_name;
                static const char *_latfield_names[];
                static const char *_lonfield_names[];

                /** 
                 * In some cases, a dedicated grid is used to store the values of
                 * latitude and longitude. The following function finds the name
                 * of the geo grid.
                 */
                std::string get_geogrid_name ();

                // Internal function used by
                // the get_geogrid_name function.
                // This function is not intended to be used outside the get_geogrid_name function.
                void _find_geogrid_name ();

                std::string _geogrid_name;
                static const char *_geogrid_names[];


                // All the following functions are called by the Prepare() function.

                // Check if we have the dedicated lat/lon grid.
                void check_onelatlon_grids();                

                // For one grid, need to handle the third-dimension(both existing and missing) coordinate variables
                void handle_one_grid_zdim(GridDataset*);

                // For one grid, need to handle lat/lon(both existing lat/lon and calculated lat/lon from EOS2 APIs)
                void handle_one_grid_latlon(GridDataset*) throw(Exception);

                // For the case of which all grids have one dedicated lat/lon grid,
                // this function shows how to handle lat/lon fields.
                void handle_onelatlon_grids() throw (Exception);

                // Handle the dimension name to coordinate variable map for grid. 
                void handle_grid_dim_cvar_maps() throw(Exception);

                // Follow COARDS for grids.
                void handle_grid_coards() throw(Exception);

                // Create the corrected dimension vector for each field when COARDS is not followed.
                void update_grid_field_corrected_dims() throw(Exception);

                // Handle CF attributes for grids. 
                // The CF attributes include "coordinates", "units" for coordinate variables and "_FillValue". 
                void handle_grid_cf_attrs() throw(Exception);

                // Special handling SOM(Space Oblique Mercator) projection files
                void handle_grid_SOM_projection() throw(Exception);

                // Obtain the number of dimension maps in this file. The input parameter is the number of swath.
                int  obtain_dimmap_num(int numswath) throw(Exception);

                // Create the dimension name to coordinate variable name map for lat/lon. 
                // The input parameter is the number of dimension maps in this file.
                void create_swath_latlon_dim_cvar_map(int numdm) throw(Exception);

                // Create the dimension name to coordinate variable name map for non lat/lon coordinate variables.
                void create_swath_nonll_dim_cvar_map() throw(Exception);

                // Handle swath dimension name to coordinate variable name maps. 
                // The input parameter is the number of dimension maps in this file.
                void handle_swath_dim_cvar_maps(int numdm) throw(Exception);

                // Handle CF attributes for swaths. 
                // The CF attributes include "coordinates", "units" for coordinate variables and "_FillValue". 
                void handle_swath_cf_attrs() throw(Exception);

             private:

                // HDF-EOS2 Grid File ID. Notice this ID is not an individual grid ID but the grid file ID returned by 
                // calling the HDF-EOS2 API GDopen. 
                int32 gridfd;

                // HDF-EOS2 Swath File ID. Notice this ID is not an individual swath ID but the swath file ID returned by
                 // calling the HDF-EOS2 API SWopen.
                int32 swathfd;

                /// Some MODIS files don't use the CF linear equation y = scale * x + offset,
                /// the scaletype will describe this case. 
                /// Note the assumption here: we assume that all fields will
                /// use one scale and offset function in a file. If
                /// multiple scale and offset equations are used in one file, our
                /// function will fail. So far only one scale and offset equation is
                ///  applied for NASA HDF-EOS2 files we observed. KY 2012-6-13
                /// Since I found one MODIS product(MOD09GA) uses different scale offset
                /// equations for different grids. I had to move the scaletype to the
                /// group level. Hopefully this is the final fix. Truly hope that 
                /// this will not happen at the field level since it will be too messy to 
                /// check.  KY 2012-11-21

                /// SOType scaletype;
        };


        struct Utility
        {

            /// Call inquiry functions twice to get a list of strings. 
            static bool ReadNamelist (const char *path,
                int32 (*inq) (char *, char *, int32 *),
                std::vector < std::string > &names);


        };

}
#endif

#endif
