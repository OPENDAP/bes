/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf4 data handler for the OPeNDAP data server.
/// HDFSP.h and HDFSP.cc include the core part of retrieving HDF-SP Grid and Swath
/// metadata info and translate them into DAP DDS and DAS.
///
/// It currently provides the CF-compliant support for the following NASA HDF4 products.
/// Other HDF4 products can still be mapped to DAP but they are not CF-compliant.
/// TRMM version 6 Level2 1B21,2A12,2B31,2A25
/// TRMM version 6 Level3A 3A46 
/// TRMM version 6 Level3B 3B42,3B43
/// TRMM version 6 Level3C CSH
//  TRMM version 7 Level2
//  TRMM version 7 Level3
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
/// Copyright (C) The HDF Group
///
/// All rights reserved.


#ifndef _HDFSP_H
#define _HDFSP_H

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


///TRMML2_V6: TRMM Level2 1B21,2A12,2B31,2A25
///TRMML3A_V6: TRMM Level3 3A46 
///TRMML3C_V6: TRMM Level3 CSH
///TRMML3B_V6: TRMM Level3 3B42,3B43
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
            explicit Exception (const std::string & msg)
                : message (msg)
            {
            }

            /// Destructor
            ~ Exception () throw () override = default;

            /// Return exception message 
            const char *what () const throw () override
            {
                return this->message.c_str ();
            }

            /// Set exception message.
            virtual void setException (const std::string &exception_message)
            {
                this->message = exception_message;
            }


        private:
            /// Exception message
            std::string message;
    };


    /// It repersents one dimension of an SDS or a VDATA.
    /// It holds the dimension name and the size of that dimension.
    class Dimension
    {
        public:

            /// Get dimension name
            const std::string & getName () const
            {
                return this->name;
            }

            /// Get dimension size
            int32 getSize () const
            {
                return this->dimsize;
            }

            /// If the SDS dimension scale is available, the returned value is dim. scale datatype.
            /// If there is no dimension scale, the returned value is 0.
            int32 getType () const
            {
                return this->dimtype;
            }

        protected:
            Dimension (const std::string & dim_name, int32 hdf4_dimsize, int32 hdf4_dimtype)
                : name (dim_name), dimsize (hdf4_dimsize), dimtype (hdf4_dimtype)
            {
            }

        private:
            // dimension name
            std::string name;

            // dimension size
            int32 dimsize;

            /// dimension scale datatype or 0 if no dimension scale.
            int32 dimtype;

        friend class SD;
        friend class File;
        friend class SDField;
    };

    /// Representing one attribute in grid or swath
    class Attribute
    {
        public:

            /// Get the attribute name.
            const std::string & getName () const
            {
                return this->name;
            }

            /// Get the CF attribute name(special characters are replaced by underscores)
            const std::string & getNewName () const
            {
                return this->newname;
            }

            /// Get the attribute datatype
            int32 getType () const
            {
                return this->type;
            }

            /// Get the number of elements of this attribute
            int32 getCount () const
            {
                return this->count;
            }

            /// Get the attribute value
            const std::vector < char >&getValue () const
            {
                return this->value;
            }

        private:

            /// Original attribute name
            std::string name;

            /// CF attribute name(special characters are replaced by underscores)
            std::string newname;

            /// Attribute type
            int32 type;

            /// The number of elements 
            int32 count;

            /// Attribute values
            std::vector < char >value;

        friend class SD;
        friend class VDATA;
        friend class File;
        friend class Field;
        friend class VDField;
    };

    /// This class only applies to the OTHERHDF products when there are dimensions but not dimension scales.
    /// To remember the dimension names, we follow the mapping of the default HDF4 OPeNDAP hander. 
    /// In DAP DAS, AttrContainers are used to create attribute containers for each dimension.
    /// For each dimension, the attribute container is variable_name_dim_0 (string name "longitude")
    class AttrContainer {

        public:
            AttrContainer() = default;
            ~AttrContainer();


            /// Get the name of this attribute container
            const std::string & getName () const
            {
                return this->name;
            }

            /// No need to have the newname since we will make the name follow CF conventions

            const std::vector < Attribute * >&getAttributes () const
            {
                return this->attrs;
            }

        private:

            // The name of this attribute container(an attribute container is a DAP DAS table)
            std:: string name;

            // The attributes in this attribute container
            std::vector < Attribute * >attrs;
        friend class SD;
        friend class File;

    };


    // This field class describes SDS or Vdata fields. 
    class Field
    {
        public:
            Field () = default;
            
            virtual ~ Field ();

            /// Get the name of this field
            const std::string & getName () const
            {
                return this->name;
            }

            /// Get the CF name(special characters replaced by underscores) of this field
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

            /// Get the attributes of this field
            const std::vector < Attribute * >&getAttributes () const
            {
                return this->attrs;
            }


        protected:

            /// The  CF full path(special characters replaced by underscores) of this field
            std::string newname;

            /// The original name of this field
            std::string name;

            /// The datatype of this field
            int32 type = -1;

            /// The rank of this field
            int32 rank = -1;

            /// The attributes of this field
            std::vector < Attribute * >attrs;

        friend class SD;
        friend class VDATA;
        friend class File;
    };

    /// One instance of this class represents one SDS object

    class SDField:public Field
    {
         public:
            SDField () = default;
            ~SDField () override;


            /// Get the list of the corrected dimensions
            const std::vector < Dimension * >&getCorrectedDimensions () const
            {
                return this->correcteddims;
            }

            /// Get the list of the corrected dimension ptrs
            std::vector < Dimension * >*getCorrectedDimensionsPtr ()
            {
                return &(this->correcteddims);
            }

            /// Set the list of the corrected dimensions
            void setCorrectedDimensions (const std::vector < Dimension * > &cor_dims)
            {
                correcteddims = cor_dims;
            }

            /// Get the "coordinates" attribute
            std::string getCoordinate () const
            {
                return this->coordinates;
            }

            /// Set the coordinate attribute
            void setCoordinates (const std::string& coor)
            {
                coordinates = coor;
            }

            /// Get the "units" attribute
            std::string getUnits () const
            {
                return this->units;
            }

            // Set the "units" attribute
            void setUnits (const std::string &uni)
            {
                units = uni;
            }

            // Get the field type
            int getFieldType () const
            {
                return this->fieldtype;
            }
        
            // Get the SDS reference number
            int32 getFieldRef () const
            {
                return this->fieldref;
            }

            /// Get the list of dimensions
            const std::vector < Dimension * >&getDimensions () const
            {
                return this->dims;
            }

            /// Get the list of OHTERHDF dimension attribute container information
            const std::vector < AttrContainer * >&getDimInfo () const
            {
                return this->dims_info;
            }


            /// Is this field a dimension without dimension scale(or empty[no data]dimension variable)
            bool IsDimNoScale() const
            {
                return is_noscale_dim;
            }

            /// Is this field a dimension scale field?
            bool IsDimScale() const
            {
                return is_dim_scale;
            }

            /// This function returns the full path of some special products that have a very long path
            std::string getSpecFullPath() const 
            {
                return special_product_fullpath;
            }
        private:

            /// Dimensions of this field
            std::vector < Dimension * >dims;

            /// Corrected dimensions of this field. The only difference between the correcteddims and dims
            /// is the correcteddims uses the CF form of the dimension names whereas the dims uses the original 
            /// dimension names. 
            std::vector < Dimension * >correcteddims;

            ///  OTHERHDF dimension information. See the description of the class AttrContainer.
            std::vector<AttrContainer *>dims_info;

            std::string coordinates;

            /// This flag will specify the fieldtype.
            /// 0 means this field is general field.
            /// 1 means this field is lat.
            /// 2 means this field is lon.
            /// 3 means this field is other dimension coordinate variable.
            /// 4 means this field is added other dimension coordinate variable with nature number.
            int fieldtype = 0;

            /// The "units" attribute
            std::string units;

            /// This string provides the full path of a field for some products that have long path.
            /// The reasons we provide this string is as follows:
            /// 1) Some products(CERES) contain many variables and some field paths may be very long.
            ///    For example,a path in a CERES file is 
            ///    "/Monthly 3-Hourly Averages/D2-like 9 Cloud Types/Deep Convection     (High, Thick)/Ice Water Path - Deep Convection - MH". 
            ///    There are almost a 100 variables like this long path in this file. This will make DDS and DAS containers very huge and choke netCDF Java clients.
            ///    So to avoid such cases, we provide a BES key so that by default only short names are provided. However, we still want to 
            ///    preserve the original path. So we include this special_product_fullpath to record this and output to DAS.
            /// 2) We decide not to use newname since as shown above, the CF form of the path is very different than the original path.
            /// KY 2013-07-02
            std::string special_product_fullpath;

            /// SDS reference number. This and the object tag are a key to identify a SDS object.
            int32 fieldref = -1;


            /// Some fields have dimensions but don't have dimension scales. In HDF4, such dimension appears as a field but no data. So this kind of field
            /// needs special treatments. This flag is to identify such a field.
            bool is_noscale_dim = false;

            /// This is a SDS dimension scale.
            bool is_dim_scale = false;

            /// In some TRMM versions, latitude and longitude are combined into one field geolocation.
            /// This variable is to remember the root field for latitude and longitude.
            std::string rootfieldname;

        friend class File;
        friend class SD;
    };

    /// One instance of this class represents one Vdata field.
    class VDField:public Field
    {
        public:
            VDField () = default;
            ~VDField () override = default;

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

            /// Get the vdata field values.
            const std::vector < char >&getValue () const
            {
                return this->value;
            }
 
            /// Read vdata field attributes.
            void ReadAttributes (int32 vdata_id, int32 fieldindex);

        private:

            /// Vdata field order
            int32 order = -1;

            /// Number of record of the vdata field
            int32 numrec = -1;

            /// Vdata field size
            int32 size = -1;

            /// Vdata field value
            std::vector < char >value;

        friend class File;
        friend class VDATA;
    };

    /// This class retrieves all SDS objects and SD file attributes.
    class SD
    {
        public:

            /// Read the information of all SDS objects from the HDF4 file.
            static SD *Read (int32 sdfileid, int32 hfileid) ;

            /// Read the information of all hybrid SDS objects from the HDF4 file.
            static SD *Read_Hybrid (int32 sdfileid, int32 hfileid) ;


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
            void obtain_noneos2_sds_path(int32,char*,int32) ;


            /// Destructor
            ~SD ();

            SD() = default;

        private:
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

        friend class File;
    };

    /// This class retrieves all information of one Vdata. 
    class VDATA
    {
        public:

            ~VDATA ();

            /// Retrieve all information of this Vdata.
            static VDATA *Read (int32 vdata_id, int32 obj_ref) ;

            /// Retrieve all attributes of this Vdata.
            void ReadAttributes (int32 vdata_id) ;

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

            VDATA (int32 obj_ref)
                :vdref(obj_ref){
            }

        private:
            /// New name with path and CF compliant(no special characters and name clashing).
            std::string newname;

            /// Original vdata name
            std::string name;

            /// Vdata field vectors.
            std::vector < VDField * >vdfields;

            /// Vdata attribute vectors.
            std::vector < Attribute * >attrs;

            /// Vdata reference number
            int32 vdref;

            /// Flag to map vdata fields to DAP variables or DAP attributes.
            bool TreatAsAttrFlag = true;

        friend class File;
    };

    /// This class retrieves all information from an HDF4 file. It is a
    /// container for SDS and Vdata.
    class File
    {
        public:

            /// Retrieve SDS and Vdata information from the HDF4 file.
            static File *Read (const char *path, int32 sdid,int32 fileid);

            /// Retrieve SDS and Vdata information from the hybrid HDF-EOS file.
            /// Currently we only support the access of additional SDS objects.
            static File *Read_Hybrid (const char *path, int32 sdid,
                                                   int32 fileid);

            /// The main step to make HDF4 SDS objects CF-complaint. 
            /// All dimension(coordinate variables) information need to be ready.
            /// All special arrangements need to be done in this step.
            void Prepare() ;

            bool Check_update_special(const std::string &gridname) const;

            void Handle_AIRS_L23();


            /// Obtain special HDF4 product type
            SPType getSPType () const
            {
                return this->sptype;
            }

            /// This file has a field that is a SDS dimension but no dimension scale

            bool Has_Dim_NoScale_Field() const
            {
                return this->OTHERHDF_Has_Dim_NoScale_Field;
            }

            // Destructor
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

            /// Get attributes for all vgroups
            const std::vector<AttrContainer *> &getVgattrs () const
            {
                return this->vg_attrs;
            }



        protected:
            explicit File (const char *hdf4file_path)
                : path (hdf4file_path)
            {
            }


            /// Handle SDS fakedim names: make the dimensions with the same dimension size 
            /// share the same dimension name. In this way, we can reduce many fakedims.
            void handle_sds_fakedim_names();

            /// Create the new dimension name set and the dimension name to size map.
            void create_sds_dim_name_list();

            /// Add the missing coordinate variables based on the corrected dimension name list
            void handle_sds_missing_fields() const;

            /// Create the final CF-compliant dimension name list for each field
            void handle_sds_final_dim_names();

            /// Create the final CF-compliant field name list
            void handle_sds_names(bool & COARDFLAG , std::string & lldimname1, std::string &lldimname2);

            /// Create "coordinates", "units" CF attributes
            void handle_sds_coords(bool COARDFLAG, const std::string &lldimname1,const std::string &lldimname2);

            /// Handle Vdata
            void handle_vdata() const;

            ///  This method will check if the HDF4 file is one of TRMM or OBPG products we supported. 
            void CheckSDType () ;

            /// Latitude and longitude are stored in one array(geolocation). Need to separate.
            void PrepareTRMML2_V6 () ;

            /// Special method to prepare TRMM Level 3A46 latitude and longitude information.
            void PrepareTRMML3A_V6 () ;

            /// Special method to prepare TRMM Level 3B latitude and longitude information.
            void PrepareTRMML3B_V6 () ;

            /// Special method to prepare TRMM Level 3 CSH latitude,longitude and Height information.
            void PrepareTRMML3C_V6 () ;

            /// void Obtain_TRMML3S_V7_latlon_size(int &latsize, int&lonsize) throw(Exception);
            void Obtain_TRMML3S_V7_latlon_size(int &latsize, int&lonsize);

            bool Obtain_TRMM_V7_latlon_name(const SDField* sdfield, const int latsize, const int lonsize, std::string& latname, std::string& lonname);

            /// Latitude and longitude are stored in different fields. Need to separate.
            void PrepareTRMML2_V7 () ;

            /// Special method to prepare TRMM single grid Level 3 geolocation fields(latitude,longitude,etc) information.
            void PrepareTRMML3S_V7 () ;

            /// Special method to prepare TRMM multiple grid Level 3 geolocation fields(latitude,longitude,etc) information.
            void PrepareTRMML3M_V7 () ;


            /// Special method to prepare CERES AVG (CER_AVG_???) and CERES SYN(CER_SYN_???) latitude and longitude information.
            /// Latitude and longitude are provided; some redundant CO-Latitude and longitude are removed from the final DDS.
            void PrepareCERAVGSYN () ;

            /// Special method to prepare CERES ES4 (CER_ES4_???) and CERES ISCCP GEO(CER_ISCCP__???GEO) latitude and longitude information.
            /// Essentially the lat/lon need to be condensed to 1-D for the geographic projection.
            void PrepareCERES4IG () ;

            /// Special method to prepare CERES SAVG (CER_SAVG_???) and CERES ISCCP DAYLIKE(CER_ISCCP__???DAYLIKE) latitude and longitude information.
            /// Essentially nested CERES 2-d  lat/lon need to be provided.
            /// https://eosweb.larc.nasa.gov/sites/default/files/project/ceres/quality_summaries/srbavg_ed2d/nestedgrid.pdf  
            void PrepareCERSAVGID () ;

            /// Special method to prepare CERES Zonal Average latitude and longitude information.
            void PrepareCERZAVG () ;

            /// Special method to prepare OBPG Level 2 latitude and longitude information. The latitude and longitude need to be interpolated.
            void PrepareOBPGL2 () ;

            /// Special method to prepare OBPG Level 3 latitude and longitude information. The latitude and longitude are calculated by using the attributes.
            void PrepareOBPGL3 () ;

            /// MODISARNSS is a special MODIS product saved as pure HDF4 files. Dimension names of different fields need to be 
            /// changed to be consistent with lat/lon.
            void PrepareMODISARNSS () ;

            /// We still provide a hook for other HDF data product although no CF compliant is followed.
            void PrepareOTHERHDF () ;

            /// Handle non-attribute lone vdatas.
            void ReadLoneVdatas(File*) const;

            /// Handle non-attribute non-lone vdatas. Note: this function is only used for
            /// handling hybrid Vdata functions.
            void ReadHybridNonLoneVdatas(const File*);

            /// Obtain vgroup attributes.
            void ReadVgattrs(int32 vgroup_id, const char *fullpath);

            /// The full path of SDS and Vdata will be obtained.
            void InsertOrigFieldPath_ReadVgVdata () ;

            /// The internal function used by InsertOrigFieldPath_ReadVgVdata.
            void obtain_path (int32 file_id, int32 sd_id, char *full_path, int32 pobj_ref) ;

            /// The internal function used to obtain the path for hybrid non-lone vdata.
            void obtain_vdata_path(int32 file_id, char *full_path, int32 pobj_ref) ;


        private:

            /// The absolute path of the file
            std::string path;

            /// Pointer to the SD instance. There is only one SD instance in an HDF4 file.
            SD *sd = nullptr;

            /// Vdata objects in this file
            std::vector < VDATA * >vds;

            ///  Vgroup attribute information. See the description of the class AttrContainer.
            std::vector<AttrContainer *>vg_attrs;


            /// HDF4 SD interface ID
            int32 sdfd = -1;	

            /// HDF4 H interfance ID 
            int32 fileid = -1;

            /// Special NASA HDF4 file types supported by this package
            SPType sptype = OTHERHDF;

            /// For the OTHERHDF files, this flag provides whether there is a non-dimension scale dimension field.
            bool OTHERHDF_Has_Dim_NoScale_Field = false;

            /// For a hybrid file, under some cases, we need to distinguish between an EOS2 grid from an EOS2 swath only by
            /// HDF4 APIs. If this flag is true, this object is an EOS2 swath. Otherwise, this should be an EOS2 grid or others.
            bool EOS2Swathflag = false;
    };

}


#endif
