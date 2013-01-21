/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf4 data handler for the OPeNDAP data server.
/// HDFSP.h and HDFSP.cc include the core part of retrieving HDF-SP Grid and Swath
/// metadata info and translate them into DAP DDS and DAS.
///
/// It currently provides the CF-compliant support for the following NASA HDF4 products.
/// Other HDF4 products can still be mapped to DAP but they are not CF-compliant.
///   TRMM Level2 1B21,2A12,2B31,2A25
///   TRMM Level3 3B42,3B43
/// CERES  CER_AVG_Aqua-FM3-MODIS,CER_AVG_Terra-FM1-MODIS
/// CERES CER_ES4_Aqua-FM3_Edition1-CV
/// CERES CER_ISCCP-D2like-Day_Aqua-FM3-MODIS
/// CERES CER_ISCCP-D2like-GEO_
/// CERES CER_SRBAVG3_Aqua-
/// CERES CER_SYN_Aqua-
/// CERES CER_ZAVG_
/// OBPG   SeaWIFS,OCTS,CZCS,MODISA,MODIST Level2
/// OBPG   SeaWIFS,OCTS,CZCS,MODISA,MODIST Level3 m 

/// KY 2010-8-12
///
///
/// @author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2010-2012 The HDF Group
///
/// All rights reserved.


//#include "InternalErr.h"
#ifndef _HDFSP_H
#define _HDFSP_H
/*
#ifdef USE_HDFEOS2_LIB
#ifdef _WIN32
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#endif
*/

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include "mfhdf.h"
#include "hdf.h"

#include "HDFSPEnumType.h"

/*
#ifdef _WIN32
#ifdef _MSC_VER
#pragma warning(disable:4290)
#endif
#endif
*/

#define MAX_FULL_PATH_LEN 1024
//#define EOS_DATA_PATH "Data Fields"
//#define EOS_GEO_PATH "Geolocation Fields"

#define _HDF_CHK_TBL_CLASS "_HDF_CHK_TBL_"
#define CERE_META_NAME "CERES_metadata"
#define CERE_META_FIELD_NAME "LOCALGRANULEID"
#define SENSOR_NAME "Sensor Name"
#define PRO_NAME "Product Name"
#define CER_AVG_NAME "CER_AVG"
#define CER_ES4_NAME "CER_ES4"
#define CER_CDAY_NAME "CER_ISCCP-D2like-Day"
#define CER_CGEO_NAME "CER_ISCCP-D2like-GEO"
#define CER_SRB_NAME "CER_SRBAVG3"
#define CER_SYN_NAME "CER_SYN"
#define CER_ZAVG_NAME "CER_ZAVG"


///TRMML2: TRMM Level2 1B21,2A12,2B31,2A25
///TRMML3: TRMM Level3 3B42,3B43
///CERES
/// CER_AVG: CER_AVG_Aqua-FM3-MODIS,CER_AVG_Terra-FM1-MODIS
/// CER_ES4: CER_ES4_Aqua-FM3_Edition1-CV
/// CER_CDAY: CER_ISCCP-D2like-Day_Aqua-FM3-MODIS
/// CER_CGEO: CER_ISCCP-D2like-GEO_
/// CER_SRB: CER_SRBAVG3_Aqua-
/// CER_SYN: CER_SYN_Aqua-
/// CER_ZAVG: CER_ZAVG_
/// OBPGL2: SeaWIFS,OCTS,CZCS,MODISA,MODIST Level2
/// OBPGL3: SeaWIFS,OCTS,CZCS,MODISA,MODIST Level3 m 



namespace HDFSP
{
    /// The Exception class for handling exceptions caused by using this
    /// HDFEOS2 library 
    class File;
    class SD;
    class VDATA;

    class Exception:public std::exception
    {
         public:
            /// Constructor
            Exception (const std::string & msg)
                : message (msg)
            {
            }

            virtual ~ Exception () throw ()
            {
            }

            virtual const char *what () const throw ()
            {
                return this->message.c_str ();
            }


            virtual void setException (std::string message)
            {
                this->message = message;
            }


        protected:
            std::string message;
    };


    /// It repersents one dimension of an SDS or a VDATA.
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
            int32 getType () const
            {
                return this->dimtype;
            }

        protected:
            Dimension (const std::string & name, int32 dimsize, int32 dimtype)
                : name (name), dimsize (dimsize), dimtype (dimtype)
            {
            }

        protected:
            std::string name;
            int32 dimsize;
            int32 dimtype;

        friend class SD;
        friend class File;
        friend class SDField;
    };

    /// Representing one attribute in grid or swath
    class Attribute
    {
        public:
            const std::string & getName () const
            {
                return this->name;
            }
            const std::string & getNewName () const
            {
                return this->newname;
            }
            int32 getType () const
            {
                return this->type;
            }
            int32 getCount () const
            {
                return this->count;
            }
            const std::vector < char >&getValue () const
            {
                return this->value;
            }

        protected:
            std::string name;
            std::string newname;
            int32 type;
            int32 count;
            std::vector < char >value;

        friend class SD;
        friend class VDATA;
        friend class File;
        friend class Field;
        friend class VDField;
    };

    class AttrContainer {
        public:
            AttrContainer()
            {
            }
            ~AttrContainer();

        public:

            /// Get the name of this field
            const std::string & getName () const
            {
                return this->name;
            }

            /// Get the new name of this field
            // const std::string & getNewName () const
            //{
            //       return this->newname;
            //}

            const std::vector < Attribute * >&getAttributes () const
            {
                return this->attrs;
            }

        protected:
            //  std:: string newname;
            std:: string name;
            std::vector < Attribute * >attrs;
        friend class SD;
        friend class File;

    };


    class Field
    {
        public:
            Field ():type (-1), rank (-1)
            {
            }
            ~Field ();

        public:

            /// Get the name of this field
            const std::string & getName () const
            {
                return this->name;
            }

            /// Get the new name of this field
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

            const std::vector < Attribute * >&getAttributes () const
            {
                return this->attrs;
            }


        protected:

            std::string newname;

            std::string name;
            int32 type;
            int32 rank;

            std::vector < Attribute * >attrs;

        friend class SD;
        friend class VDATA;
        friend class File;
    };

    /// One instance of this class represents one SDS object

    class SDField:public Field
    {
         public:
            SDField ()
                :fieldtype (0), sdsref (-1), condenseddim (false),is_dim_noscale(false),is_dim_scale(false)
            {
            }
            ~SDField ();

        public:


            /// Get the list of the corrected dimensions 
            const std::vector < Dimension * >&getCorrectedDimensions () const
            {
                return this->correcteddims;
            }

            std::vector < Dimension * >*getCorrectedDimensionsPtr ()
            {
                return &(this->correcteddims);
            }

            /// Set the list of the corrected dimensions
            void setCorrectedDimensions (std::vector < Dimension * >dims)
            {
                correcteddims = dims;
            }

            const std::string getCoordinate () const
            {
                return this->coordinates;
            }

            /// Set the coordinate attribute
            void setCoordinates (std::string coor)
            {
                coordinates = coor;
            }

            const std::string getUnits () const
            {
                return this->units;
            }

            // Set units
            void setUnits (std::string uni)
            {
                units = uni;
            }

            const int getFieldType () const
            {
                return this->fieldtype;
            }

            int32 getSDSref () const
            {
                return this->sdsref;
            }

            /// Get the list of dimensions
            const std::vector < Dimension * >&getDimensions () const
            {
                return this->dims;
            }

            /// Get the list of dimension information
            const std::vector < AttrContainer * >&getDimInfo () const
            {
                return this->dims_info;
            }

            bool IsDimNoScale() const
            {
                return is_dim_noscale;
            }

            bool IsDimScale() const
            {
                return is_dim_scale;
            }


            const string getSpecFullPath() const 
            {
                return special_product_fullpath;
            }
        protected:

            std::vector < Dimension * >dims;
            std::vector < Dimension * >correcteddims;

            vector<AttrContainer *>dims_info;
            std::string coordinates;

            // This flag will specify the fieldtype.
            // 0 means this field is general field.
            // 1 means this field is lat.
            // 2 means this field is lon.
            // 3 means this field is other dimension variable.
            // 4 means this field is added other dimension variable with nature number.
            int fieldtype;

            std::string units;
            std::string special_product_fullpath;

            int32 sdsref;
            bool condenseddim;
            bool is_dim_noscale;
            bool is_dim_scale;

            std::string rootfieldname;

        friend class File;
        friend class SD;
    };

    /// One instance of this class represents one Vdata field.
    class VDField:public Field
    {
        public:
            VDField ():order (-1), numrec (-1), size (-1)
            {
            }
            ~VDField ();

        public:

            /// Get the order of this field
            int32 getFieldOrder ()const
            {
                return this->order;
            }

            /// Get the field size
            int32 getFieldsize () const
            {
                return this->size;
            }

            /// Get the number of record
            int32 getNumRec () const
            {
                return this->numrec;
            }

            const std::vector < char >&getValue () const
            {
                return this->value;
            }
            void ReadAttributes (int32 vdata_id, int32 fieldindex) throw (Exception);

        protected:
            int32 order;
            int32 numrec;
            int32 size;
            std::vector < char >value;

        friend class File;
        friend class VDATA;
    };

    /// This class retrieves all SDS objects and SD file attributes.
    class SD
    {
        public:

            /// Read the information of all SDS objects from the HDF4 file.
            static SD *Read (int32 sdfileid, int32 hfileid) throw (Exception);

            /// Read the information of all hybrid SDS objects from the HDF4 file.
            static SD *Read_Hybrid (int32 sdfileid, int32 hfileid) throw (Exception);

            /// Retrieve the absolute path of the file(full file name).
            const std::string & getPath () const
            {
                return this->path;
            }

            /// Public interface to obtain information of all SDS vectors(objects).
            const std::vector < SDField * >&getFields () const
            {
                return this->sdfields;
            }

            /// Public interface to obtain the SD(file) attributes.
            const std::vector < Attribute * >&getAttributes () const
            {
                return this->attrs;
            }

            /// Obtain SDS path, this is like a clone of obtain_path in File class, except the Vdata and some minor parts.
            void obtain_sds_path(int32,int32,char*,int32) throw(Exception);


            ~SD ();

        protected:
            SD (int32 sdfileid, int32 hfileid)
                : sdfd (sdfileid), fileid (hfileid)
            {
            }

        protected:

            /// The full path of the file(file name). 
            std::string path;

            /// SDS objects stored in vectors
            std::vector < SDField * >sdfields;

            /// SD attributes stored in vectors
            std::vector < Attribute * >attrs;

            /// SDS reference number list
            std::list <int32> sds_ref_list;

            ///SDS reference number to index map, use to quickly obtain the SDS id.
            std::map < int32, int >refindexlist;

            ///Unique dimension name to its size map, may be replaced in the current implementation.   
            /// Still leave it here for potential fakeDim handling in the future.
            std::map < std::string, int32 > n1dimnamelist;

            /// Original dimension name to corrected dimension name map.
            std::map < std::string, std::string > n2dimnamelist;

            /// Full dimension name list set.
            std::set < std::string > fulldimnamelist;

            /// This set stores non-missing coordinate variable dimension names. Many third dimensions of HDF4 files have to be
            /// treated as missing coordinate variables. But latitude and longitude's corresponding dimensions are normally 
            /// provided in the file. So this set is used to exclude these dimensions when creating the corresponding missing fields.
            std::set < std::string > nonmisscvdimnamelist;

            /// dimension name to coordinate variable name list: the key list to generate CF "coordinates" attributes.
            std::map < std::string, std::string > dimcvarlist;

        private:
            int32 sdfd, fileid;
        friend class File;
    };

    /// This class retrieves all information of one Vdata. 
    class VDATA
    {
        public:

            ~VDATA ();

            /// Retrieve all information of this Vdata.
            static VDATA *Read (int32 vdata_id, int32 obj_ref) throw (Exception);

            /// Retrieve all attributes of this Vdata.
            void ReadAttributes (int32 vdata_id) throw (Exception);

            /// Obtain new names(with the path and special characters and name clashing handlings)
            const std::string & getNewName () const
            {
                return this->newname;
            }

            /// Obtain the original vdata name
            const std::string & getName () const
            {
                return this->name;
            }

            /// Obtain Vdata fields.
            const std::vector < VDField * >&getFields () const
            {
                return this->vdfields;
            }

            /// Obtain Vdata attributes.
            const std::vector < Attribute * >&getAttributes () const
            {
                return this->attrs;
            }

            /// Some Vdata fields are very large in size. Some Vdata fields are very small. 
            /// So we map smaller Vdata fields to DAP attributes and map bigger Vdata fields to DAP variables.
            /// This flag is used for that.
            bool getTreatAsAttrFlag () const
            {
                return TreatAsAttrFlag;
            }

            /// Obtain Vdata reference number, this is necessary for retrieving Vdata information from HDF4.
            int32 getObjRef () const
            {
                return vdref;
            }

        protected:
            VDATA (int32 vdata_myid, int32 obj_ref)
                :vdref(obj_ref),TreatAsAttrFlag (true),vdata_id (vdata_myid) {
            }

        protected:

            /// New name with path and CF compliant(no special characters and name clashing).
            std::string newname;
            std::string name;

            /// Vdata field vectors.
            std::vector < VDField * >vdfields;

            /// Vdata attribute vectors.
            std::vector < Attribute * >attrs;

            /// Vdata reference number
            int32 vdref;

            /// Flag to map vdata fields to DAP variables or DAP attributes.
            bool TreatAsAttrFlag;

        private:
            int32 vdata_id;


        friend class File;
    };

    /// This class retrieves all information from an HDF4 file. It is a
    /// container for SDS and Vdata.
    class File
    {
        public:

            /// Retrieve SDS and Vdata information from the HDF4 file.
            static File *Read (const char *path, int32 myfileid) throw (Exception);

            /// Retrieve SDS and Vdata information from the hybrid HDF-EOS file.
            /// Currently we only support the access of additional SDS objects.
            static File *Read_Hybrid (const char *path,
                                                   int32 myfileid) throw (Exception);

            /// The main step to make HDF4 SDS objects CF-complaint. 
            /// All dimension(coordinate variables) information need to be ready.
            /// All special arrangements need to be done in this step.

            void Prepare() throw(Exception);

            ///  This method will check if the HDF4 file is one of TRMM or OBPG products we supported. 
            void CheckSDType () throw (Exception);

            /// Latitude and longitude are stored in one array(geolocation). Need to separate.
            void PrepareTRMML2 () throw (Exception);

            /// Special method to prepare TRMM Level 3 latitude and longitude information.
            void PrepareTRMML3 () throw (Exception);

            /// Special method to prepare CERES AVG (CER_AVG_???) and CERES SYN(CER_SYN_???) latitude and longitude information.
            /// Latitude and longitude are provided; some redundant CO-Latitude and longitude are removed from the final DDS.
            void PrepareCERAVGSYN () throw (Exception);

            /// Special method to prepare CERES ES4 (CER_ES4_???) and CERES ISCCP GEO(CER_ISCCP__???GEO) latitude and longitude information.
            /// Essentially the lat/lon need to be condensed to 1-D for the geographic projection.
            void PrepareCERES4IG () throw (Exception);

            /// Special method to prepare CERES SAVG (CER_SAVG_???) and CERES ISCCP DAYLIKE(CER_ISCCP__???DAYLIKE) latitude and longitude information.
            /// Essentially nested CERES 2-d  lat/lon need to be provided.
            void PrepareCERSAVGID () throw (Exception);

            /// Special method to prepare CERES Zonal Average latitude and longitude information.
            void PrepareCERZAVG () throw (Exception);

            /// Special method to prepare OBPG Level 2 latitude and longitude information. The latitude and longitude need to be interpolated.
            void PrepareOBPGL2 () throw (Exception);

            /// Special method to prepare OBPG Level 3 latitude and longitude information. The latitude and longitude are calculated by using the attributes.
            void PrepareOBPGL3 () throw (Exception);

            /// MODISARNSS is a special MODIS product saved as pure HDF4 files. Dimension names of different fields need to be 
            /// changed to be consistent with lat/lon.
            void PrepareMODISARNSS () throw (Exception);

            /// We still provide a hook for other HDF data product although no CF compliant is followed.
            void PrepareOTHERHDF () throw (Exception);

            /// Handle non-attribute vdatas.
            void ReadVdatas(File*) throw(Exception);

            /// The full path of SDS and Vdata will be obtained.
            void InsertOrigFieldPath_ReadVgVdata () throw (Exception);

            /// The internal function used by InsertOrigFieldPath_ReadVgVdata.
            void obtain_path (int32 file_id, int32 sd_id, char *full_path, int32 pobj_ref) throw (Exception);

            /// Obtain special HDF4 product type
            SPType getSPType () const
            {
                return this->sptype;
            }

            bool Has_Dim_NoScale_Field() const
            {
                return this->OTHERHDF_Has_Dim_NoScale_Field;
            }

            ~File ();

            /// Obtain the path of the file
            const std::string & getPath () const
            {
                return this->path;
            }

            /// Public interface to Obtain SD
            SD *getSD () const
            {
                return this->sd;
            }

            /// Public interface to Obtain Vdata
            const std::vector < VDATA * >&getVDATAs () const
            {
                return this->vds;
            }


        protected:
            File (const char *path)
                : path (path), sdfd (-1), fileid (-1), sptype (OTHERHDF),OTHERHDF_Has_Dim_NoScale_Field(false)
            {
            }

        protected:
            std::string path;
            SD *sd;

            std::vector < VDATA * >vds;

            // Check name clashing for fields. Borrowed from HDFEOS.cc 
            bool check_field_name_clashing (bool bUseDimNameMatching) const;

        private:
            int32 sdfd;	// SD interface ID
            int32 fileid;// H interface ID
            SPType sptype;// Special HDF4 file type
            bool OTHERHDF_Has_Dim_NoScale_Field;
    };

}


#endif
