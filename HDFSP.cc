/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
/// HDFSP.h and HDFSP.cc include the core part of retrieving HDF-SP Grid and Swath
/// metadata info and translate them into DAP DDS and DAS.
///
/// It currently provides the CF-compliant support for the following NASA HDF4 products.
/// Other HDF4 products can still be mapped to DAP but they are not CF-compliant.
///   TRMM version 6 Level2 1B21,2A12,2B31,2A25
///   TRMM version 6 Level3 3B42,3B43,3A46,CSH
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

/////////////////////////////////////////////////////////////////////////////
#include <sstream>
#include <algorithm>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include<libgen.h>
#include "HDFCFUtil.h"
#include "HDFSP.h"
#include "dodsutil.h"

const char *_BACK_SLASH= "/";

using namespace HDFSP;
using namespace std;

// Convenient function to handle exceptions
template < typename T, typename U, typename V, typename W, typename X > static void
_throw5 (const char *fname, int line, int numarg,
		 const T & a1, const U & a2, const V & a3, const W & a4, const X & a5)
{
    std::ostringstream ss;
    ss << fname << ":" << line << ":";
    for (int i = 0; i < numarg; ++i) {
        ss << " ";
        switch (i) {

            case 0:
                ss << a1;
                break;
            case 1:
                ss << a2;
                break;
            case 2:
                ss << a3;
                break;
            case 3:
                ss << a4;
                break;
            case 4:
                ss << a5;
		break;
        }
    }
    throw Exception (ss.str ());
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


// Convenient function to release resources.
struct delete_elem
{
    template < typename T > void operator () (T * ptr)
    {
        delete ptr;
    }
};


// Class File destructor
File::~File ()
{

    // Release SD resources
    if (this->sdfd != -1) {
        if (sd != NULL)
            delete sd;
        // No need to close SD interface since for performance reasons
        // it is handled(opened/closed) at the top level(HDF4RequestHandler.cc)
        // KY 2014-02-18
        //SDend (this->sdfd);
    }

    // Close V interface IDs and release vdata resources
    if (this->fileid != -1) {

        for (vector < VDATA * >::const_iterator i = this->vds.begin ();
	     i != this->vds.end (); ++i) {
             delete *i;
        }

        for (vector < AttrContainer * >::const_iterator i = this->vg_attrs.begin ();
             i != this->vg_attrs.end (); ++i) {
             delete *i;
        }

        Vend (this->fileid);
        // No need to close H interface since for performance reasons 
        // it is handled(opened/closed) at the top level(HDF4RequestHandler.cc) 
        //Hclose (this->fileid);
    }
}

// Destructor to release vdata resources
VDATA::~VDATA ()
{
    // Release vdata field pointers
    std::for_each (this->vdfields.begin (), this->vdfields.end (),
        delete_elem ());

   // Release vdata attributes
    std::for_each (this->attrs.begin (), this->attrs.end (), delete_elem ());
}

// Destructor to release SD resources
SD::~SD ()
{
    // Release vdata attributes
    std::for_each (this->attrs.begin (), this->attrs.end (), delete_elem ());

    // Release SD field pointers
    std::for_each (this->sdfields.begin (), this->sdfields.end (),
        delete_elem ());

}

// Destructor to release SD field resources
SDField::~SDField ()
{
    // Release dimension resources
    std::for_each (this->dims.begin (), this->dims.end (), delete_elem ());

    // Release corrected dimension resources 
    std::for_each (this->correcteddims.begin (), this->correcteddims.end (),
        delete_elem ());

    // Release attribute container dims_info resources(Only apply for the OTHERHDF case)
    std::for_each (this->dims_info.begin (), this->dims_info.end (), delete_elem ());
}

// Vdata field constructors, nothing needs to do here. We don't provide vdata dimensions.
// Only when mapping to DDS (at hdfdesc.cc,search VDFDim0), we add the dimension info. to DDS. The addition
// may not be in a good place, however, the good part is that we don't need to allocate dimension resources
//  for vdata.

VDField::~VDField ()
{
}

// We only need to release attributes since that's shared for both Vdata fields and SDS fields.
Field::~Field ()
{
    std::for_each (this->attrs.begin (), this->attrs.end (), delete_elem ());
}

// Release attribute container resources. This should only apply to the OTHERHDF case.
AttrContainer::~AttrContainer() 
{
    std::for_each (this->attrs.begin (), this->attrs.end (), delete_elem ());
}


// Retrieve all the information from an HDF file; this is the same approach
// as the way to handle HDF-EOS2 files.
File *
File::Read (const char *path, int32 mysdid, int32 myfileid)
throw (Exception)
{

    // Allocate a new file object.
    File *file = new File (path);
//cerr<<"FIle is opened for HDF4 "<<endl;

#if 0
    int32 mysdid = -1;

    // Obtain the SD ID.
    if ((mysdid =
        SDstart (const_cast < char *>(file->path.c_str ()),
                 DFACC_READ)) == -1) {
        delete file;
        throw2 ("SDstart", path);
    }
#endif

    // Old comments just for reminders(KY 2014-02-18)
    // A strange compiling bug was found if we don't pass the file id to this fuction.
    // It will always give 0 number as the ID and the HDF4 library doesn't complain!! 
    // Will try dumplicating the problem and submit a bug report. KY 2010-7-14
    file->sdfd = mysdid;
    file->fileid = myfileid;

    // Start V interface
    int32 status = Vstart (file->fileid);
    if (status == FAIL)  {
        delete file;
        throw2 ("Cannot start vdata/vgroup interface", path);
    }

    try {
        // Read SDS info.
        file->sd = SD::Read (file->sdfd, file->fileid);

        // Handle lone vdatas, non-lone vdatas will be handled in Prepare().
        // Read lone vdata.
        file->ReadLoneVdatas(file);
    }
    catch(...) {
        delete file;
        throw;
    }

    return file;
}

// Retrieve all the information from the additional SDS objects of an HDF file; this is the same approach
// as the way to handle other HDF4 files.
File *
File::Read_Hybrid (const char *path, int32 mysdid, int32 myfileid)
throw (Exception)
{
    // New File 
    File *file = new File (path);
//cerr<<"File is opened for HDF4 "<<endl;

#if 0
    // Obtain SD interface
    int32 mysdid = -1;
    if ((mysdid =
        SDstart (const_cast < char *>(file->path.c_str ()),
		  DFACC_READ)) == -1) {
        delete file;
        throw2 ("SDstart", path);
    }
#endif

    // Old comments just for reminders. The HDF4 issue may still exist. KY 2014-02-18
    // A strange compiling bug was found if we don't pass the file id to this fuction.
    // It will always give 0 number as the ID and the HDF4 library doesn't complain!! 
    // Will try dumplicating the problem and submit a bug report. KY 2010-7-14
    file->sdfd = mysdid;
    file->fileid = myfileid;

    // Start V interface
    int status = Vstart (file->fileid);
    if (status == FAIL) {
        delete file;
        throw2 ("Cannot start vdata/vgroup interface", path);
    }

    try {

        // Retrieve extra SDS info.
        file->sd = SD::Read_Hybrid(file->sdfd, file->fileid);

        // Retrieve lone vdata info.(HDF-EOS2 doesn't support any lone vdata)
        file->ReadLoneVdatas(file);

        // Retrieve extra non-lone vdata in the hybrid HDF-EOS2 file
        file->ReadHybridNonLoneVdatas(file);
    }
    catch(...) {
        delete file;
        throw;
    }
         
    return file;
}

// Retrieve lone vdata info.
void
File::ReadLoneVdatas(File *file) throw(Exception) {

    int status = -1;
    // No need to start V interface again
#if 0
    // Start V interface
    int status = Vstart (file->fileid);
    if (status == FAIL)
        throw2 ("Cannot start vdata/vgroup interface", path);
#endif

    // Obtain number of lone vdata.
    int num_lone_vdata = VSlone (file->fileid, NULL, 0);

    if (num_lone_vdata == FAIL)
        throw2 ("Fail to obtain lone vdata number", path);

    // Currently the vdata name buffer has to be static allocated according to HDF4 reference manual. KY 2010-7-14
    // Now HDF4 provides a dynamic way to allocate the length of vdata_class, should update to use that in the future.
    // Documented in a jira ticket HFRHANDLER-168.
    // KY 2013-07-11
    char vdata_class[VSNAMELENMAX], vdata_name[VSNAMELENMAX];

    if (num_lone_vdata > 0) {

        vector<int32>ref_array;
        ref_array.resize(num_lone_vdata);

        if (VSlone (file->fileid, &ref_array[0], num_lone_vdata) == FAIL) {
            throw2 ("cannot obtain lone vdata reference arrays", path);
        }

        for (int i = 0; i < num_lone_vdata; i++) {

            int32 vdata_id = -1;

            vdata_id = VSattach (file->fileid, ref_array[i], "r");
            if (vdata_id == FAIL) {
                throw2 ("Fail to attach Vdata", path);
            }
            status = VSgetclass (vdata_id, vdata_class);
            if (status == FAIL) {
                VSdetach (vdata_id);
                throw2 ("Fail to obtain Vdata class", path);
            }

            if (VSgetname (vdata_id, vdata_name) == FAIL) {
                VSdetach (vdata_id);
                throw3 ("Fail to obtain Vdata name", path, vdata_name);
            }

            // Ignore any vdata that is either an HDF4 attribute or is used 
            // to store internal data structures.
            if (VSisattr (vdata_id) == TRUE
                || !strncmp (vdata_class, _HDF_CHK_TBL_CLASS,
                strlen (_HDF_CHK_TBL_CLASS))
                || !strncmp (vdata_class, _HDF_SDSVAR, strlen (_HDF_SDSVAR))
                || !strncmp (vdata_class, _HDF_CRDVAR, strlen (_HDF_CRDVAR))
                || !strncmp (vdata_class, DIM_VALS, strlen (DIM_VALS))
                || !strncmp (vdata_class, DIM_VALS01, strlen (DIM_VALS01))
                || !strncmp (vdata_class, RIGATTRCLASS, strlen (RIGATTRCLASS))
                || !strncmp (vdata_name, RIGATTRNAME, strlen (RIGATTRNAME))) {

                status = VSdetach (vdata_id);
                if (status == FAIL) {
                    throw3 ("VSdetach failed ", "Vdata name ", vdata_name);
                }
            }	

            else {
                VDATA*vdataobj = NULL;

                try {
                    // Read vdata information
                    vdataobj = VDATA::Read (vdata_id, ref_array[i]);
                }
                catch (...) {
                    VSdetach(vdata_id);
                    throw;
                }

                // We want to map fields of vdata with more than 10 records to DAP variables
                // and we need to add the path and vdata name to the new vdata field name
                if (!vdataobj->getTreatAsAttrFlag ()) {
                    for (std::vector < VDField * >::const_iterator it_vdf =
                        vdataobj->getFields ().begin ();
                        it_vdf != vdataobj->getFields ().end (); it_vdf++) {

                        // vdata name conventions. 
                        // "vdata"+vdata_newname+"_vdf_"+(*it_vdf)->newname
                        (*it_vdf)->newname =
                                            "vdata_" + vdataobj->newname + "_vdf_" +
                                             (*it_vdf)->name;

                        //Make sure the name is following CF, KY 2012-6-26
                        (*it_vdf)->newname = HDFCFUtil::get_CF_string((*it_vdf)->newname);
                    }
                }

                // Save this vdata info. in the file instance.
                file->vds.push_back (vdataobj);

                // THe following code should be replaced by using the VDField member functions in the future
                // The code has largely overlapped with VDField member functions, but not for this release.
                // KY 2010-8-11
              
                // To know if the data product is CERES, we need to check Vdata CERE_metadata(CERE_META_NAME).
                //  One field name LOCALGRANULEID(CERE_META_FIELD_NAME) includes the product name. 
                //  We want to assign the filetype of this CERES file based on the LOCALGRANULEID.
                //  Please note that CERES products we support to follow CF are pure HDF4 files. 
                //  For hybrid HDF-EOS2 files, this if loop is simply skipped.

                //  When the vdata name indicates this is a CERES product, we need to do the following:
                if (false == strncmp
                    (vdata_name, CERE_META_NAME, strlen (CERE_META_NAME))) {

                    char *fieldname = NULL;

                    // Obtain number of vdata fields
                    int num_field = VFnfields (vdata_id);
                    if (num_field == FAIL) {
                        VSdetach (vdata_id);
                        throw3 ("number of fields at Vdata ", vdata_name," is -1");
                    }

                    // Search through the number of vdata fields
                    for (int j = 0; j < num_field; j++) {

                        fieldname = VFfieldname (vdata_id, j);
                        if (fieldname == NULL) {
                            VSdetach (vdata_id);
                            throw5 ("vdata ", vdata_name, " field index ", j,
									" field name is NULL.");
                        }

                        // If the field name matches CERES's specific field name"LOCALGRANULEID"
                        if (!strcmp (fieldname, CERE_META_FIELD_NAME)) {

                            int32 fieldsize = -1; 
                            int32 nelms = -1;

                            // Obtain field size
                            fieldsize = VFfieldesize (vdata_id, j);
                            if (fieldsize == FAIL) {
                                VSdetach (vdata_id);
                                throw5 ("vdata ", vdata_name, " field ",fieldname, " size is wrong.");
                            }

                            // Obtain number of elements
                            nelms = VSelts (vdata_id);
                            if (nelms == FAIL) {
                                VSdetach (vdata_id);
                                throw5 ("vdata ", vdata_name,
                                        " number of field record ", nelms," is wrong.");
                            }

                            // Allocate data buf
                            char *databuf = (char *) malloc (fieldsize * nelms);
                            if (databuf == NULL) {
                                VSdetach (vdata_id);
                                throw1("no enough memory to allocate buffer.");
                            }

                            // Initialize the seeking process
                            if (VSseek (vdata_id, 0) == FAIL) {
                                VSdetach (vdata_id);
                                free (databuf);
                                throw5 ("vdata ", vdata_name, "field ",
                                         CERE_META_FIELD_NAME," VSseek failed.");
                            }

                            // The field to seek is CERE_META_FIELD_NAME
                            if (VSsetfields (vdata_id, CERE_META_FIELD_NAME) == FAIL) {
                                VSdetach (vdata_id);
                                free (databuf);
                                throw5 ("vdata ", vdata_name, "field ",
                                         CERE_META_FIELD_NAME," VSsetfields failed.");
                            }

                            // Read this vdata field value
                            if (VSread(vdata_id, (uint8 *) databuf, 1,FULL_INTERLACE) 
                                == FAIL) {
                                VSdetach (vdata_id);
                                free (databuf);
                                throw5 ("vdata ", vdata_name, "field ",
                                         CERE_META_FIELD_NAME," VSread failed.");
                            }

                            // Assign the corresponding special product indicator we supported for CF
                            if (!strncmp(databuf, CER_AVG_NAME,strlen (CER_AVG_NAME)))
                                file->sptype = CER_AVG;
                            else if (!strncmp
                                     (databuf, CER_ES4_NAME,strlen(CER_ES4_NAME)))
                                file->sptype = CER_ES4;
                            else if (!strncmp
                                     (databuf, CER_CDAY_NAME,strlen (CER_CDAY_NAME)))
                                file->sptype = CER_CDAY;
                            else if (!strncmp
                                     (databuf, CER_CGEO_NAME,strlen (CER_CGEO_NAME)))
                                file->sptype = CER_CGEO;
                            else if (!strncmp
                                     (databuf, CER_SRB_NAME,strlen (CER_SRB_NAME)))
                                file->sptype = CER_SRB;
                            else if (!strncmp
                                     (databuf, CER_SYN_NAME,strlen (CER_SYN_NAME)))
                                file->sptype = CER_SYN;
                            else if (!strncmp
                                     (databuf, CER_ZAVG_NAME,
                                      strlen (CER_ZAVG_NAME)))
                                file->sptype = CER_ZAVG;
                            else;

                            free (databuf);
                        }
                    }
                }
            }

            VSdetach (vdata_id);
        }
    }
}

// Handle non-attribute non-lone vdata for Hybrid HDF-EOS2 files.
void
File::ReadHybridNonLoneVdatas(File *file) throw(Exception) {


    int32 status         = -1;
    int32 file_id        = -1;
    int32 vgroup_id      = -1;
    int32 vdata_id       = -1;
    //int32 vgroup_ref     = -1;
    //int32 obj_index      = -1;
    //int32 num_of_vg_objs = -1;
    int32 obj_tag        = -1;
    int32 obj_ref        = -1;

    int32 lone_vg_number = 0;
    int32 num_of_lones   = -1;
    int32 num_gobjects   = 0;

    // This can be updated in the future with new HDF4 APIs that can provide the actual length of an object name.
    // Documented in a jira ticket HFRHANDLER-168.
    // KY 2013-07-11
    char vdata_name[VSNAMELENMAX];
    char vdata_class[VSNAMELENMAX];
    char vgroup_name[VGNAMELENMAX*4];
    char vgroup_class[VGNAMELENMAX*4];

    // Full path of this vgroup
    char *full_path      = NULL;

    // Copy of a full path of this vgroup 
    char *cfull_path     = NULL;

    // Obtain H interface ID
    file_id = file->fileid;

    // No need to start V interface again.
#if 0
    // Start V interface
    status = Vstart (file_id);
    if (status == FAIL) 
        throw2 ("Cannot start vdata/vgroup interface", path);
#endif

    // No NASA HDF4 files have the vgroup that forms a ring; so ignore this case.
    // First, call Vlone with num_of_lones set to 0 to get the number of
    // lone vgroups in the file, but not to get their reference numbers.
    num_of_lones = Vlone (file_id, NULL, 0);
    if (num_of_lones == FAIL)
        throw3 ("Fail to obtain lone vgroup number", "file id is", file_id);

    // if there are any lone vgroups,
    if (num_of_lones > 0) {

        // Use the num_of_lones returned to allocate sufficient space for the
        // buffer ref_array to hold the reference numbers of all lone vgroups,

        // Use vectors to avoid the clean-up of the memory
        vector<int32>ref_array;
        ref_array.resize(num_of_lones);

        // Call Vlone again to retrieve the reference numbers into
        // the buffer ref_array.
        num_of_lones = Vlone (file_id, &ref_array[0], num_of_lones);
        if (num_of_lones == FAIL) {
            throw3 ("Cannot obtain lone vgroup reference arrays ",
                    "file id is ", file_id);
        }

        // Loop the lone vgroups.
        for (lone_vg_number = 0; lone_vg_number < num_of_lones;
             lone_vg_number++) {

            // Attach to the current vgroup 
            vgroup_id = Vattach (file_id, ref_array[lone_vg_number], "r");
            if (vgroup_id == FAIL) {
                throw3 ("Vattach failed ", "Reference number is ",
                         ref_array[lone_vg_number]);
            }

            // Obtain the vgroup name.
            status = Vgetname (vgroup_id, vgroup_name);
            if (status == FAIL) {
                Vdetach (vgroup_id);
                throw3 ("Vgetname failed ", "vgroup_id is ", vgroup_id);
            }

            // Obtain the vgroup_class name.
            status = Vgetclass (vgroup_id, vgroup_class);
            if (status == FAIL) {
                Vdetach (vgroup_id);
                throw3 ("Vgetclass failed ", "vgroup_name is ", vgroup_name);
            }

            //Ignore internal HDF groups
            if (strcmp (vgroup_class, _HDF_ATTRIBUTE) == 0
                || strcmp (vgroup_class, _HDF_VARIABLE) == 0
                || strcmp (vgroup_class, _HDF_DIMENSION) == 0
                || strcmp (vgroup_class, _HDF_UDIMENSION) == 0
                || strcmp (vgroup_class, _HDF_CDF) == 0
                || strcmp (vgroup_class, GR_NAME) == 0
                || strcmp (vgroup_class, RI_NAME) == 0) {
                Vdetach(vgroup_id);
                continue;
            }

            // Obtain number of objects under this vgroup
            num_gobjects = Vntagrefs (vgroup_id);
            if (num_gobjects < 0) {
                Vdetach (vgroup_id);
                throw3 ("Vntagrefs failed ", "vgroup_name is ", vgroup_name);
            }

            // Allocate enough buffer for the full path
            // MAX_FULL_PATH_LEN(1024) is long enough
            // to cover any HDF4 object path for all NASA HDF4 products.
            // We replace strcpy and strcat with strncpy and strncat as suggested. KY 2013-08-29
            full_path = (char *) malloc (MAX_FULL_PATH_LEN);
            if (full_path == NULL) {
                Vdetach (vgroup_id);
                throw1 ("No enough memory to allocate the buffer.");
            }
            memset(full_path,'\0',MAX_FULL_PATH_LEN);
            
            // Obtain the full path of this vgroup
            strncpy (full_path,_BACK_SLASH,strlen(_BACK_SLASH));
            strncat(full_path,vgroup_name,strlen(vgroup_name));
            strncat(full_path,_BACK_SLASH,strlen(_BACK_SLASH));

            // Make a copy the current vgroup full path since full path may be passed to a recursive routine
            cfull_path = (char *) malloc (MAX_FULL_PATH_LEN);
            if (cfull_path == NULL) {
                Vdetach (vgroup_id);
                free (full_path);
                throw1 ("No enough memory to allocate the buffer.");
            }
            memset(cfull_path,'\0',MAX_FULL_PATH_LEN);
            strncpy(cfull_path,full_path,strlen(full_path));

            // Loop all vgroup objects
            for (int i = 0; i < num_gobjects; i++) {

                // Obtain the object tag/ref pair of an object
                if (Vgettagref (vgroup_id, i, &obj_tag, &obj_ref) == FAIL) {
                    Vdetach (vgroup_id);
                    free (full_path);
                    free (cfull_path);
                    throw5 ("Vgettagref failed ", "vgroup_name is ",
                             vgroup_name, " reference number is ", obj_ref);
                }

                // If the object is a vgroup,always pass the original full path to its decendant vgroup
                // The reason to use a copy is because the full_path will be changed when it goes down to its descendant.
                if (Visvg (vgroup_id, obj_ref) == TRUE) {
                    strncpy(full_path,cfull_path,strlen(cfull_path)+1);
                    full_path[strlen(cfull_path)]='\0';
                    obtain_vdata_path (file_id,  full_path, obj_ref);
                }

                // If this object is vdata
                else if (Visvs (vgroup_id, obj_ref)) {

                    // Obtain vdata ID
                    vdata_id = VSattach (file_id, obj_ref, "r");
                    if (vdata_id == FAIL) {
                        Vdetach (vgroup_id);
                        free (full_path);
                        free (cfull_path);
                        throw5 ("VSattach failed ", "vgroup_name is ",
                                 vgroup_name, " reference number is ",
                                 obj_ref);
                    }
                  
                    // Obtain vdata name
                    status = VSgetname (vdata_id, vdata_name);
                    if (status == FAIL) {
                        Vdetach (vgroup_id);
                        free (full_path);
                        free (cfull_path);
                        throw5 ("VSgetclass failed ", "vgroup_name is ",
                                 vgroup_name, " reference number is ",
                                 obj_ref);
                    }

                    // Obtain vdata class name
                    status = VSgetclass (vdata_id, vdata_class);
                    if (status == FAIL) {
                        Vdetach (vgroup_id);
                        free (full_path);
                        free (cfull_path);
                        throw5 ("VSgetclass failed ", "vgroup_name is ",
                                 vgroup_name, " reference number is ",
                                 obj_ref);
                    }

                    // Ignore the vdata to store internal HDF structure and the vdata used as an attribute
                    if (VSisattr (vdata_id) == TRUE
                        || !strncmp (vdata_class, _HDF_CHK_TBL_CLASS,
                            strlen (_HDF_CHK_TBL_CLASS))
                        || !strncmp (vdata_class, _HDF_SDSVAR,
                            strlen (_HDF_SDSVAR))
                        || !strncmp (vdata_class, _HDF_CRDVAR,
                            strlen (_HDF_CRDVAR))
                        || !strncmp (vdata_class, DIM_VALS, strlen (DIM_VALS))
                        || !strncmp (vdata_class, DIM_VALS01,
                            strlen (DIM_VALS01))
                        || !strncmp (vdata_class, RIGATTRCLASS,
                            strlen (RIGATTRCLASS))
                        || !strncmp (vdata_name, RIGATTRNAME,
                            strlen (RIGATTRNAME))) {

                        status = VSdetach (vdata_id);
                        if (status == FAIL) {
                            Vdetach (vgroup_id);
                            free (full_path);
                            free (cfull_path);
                            throw3 ("VSdetach failed ",
                                    "Vdata is under vgroup ", vgroup_name);
                        }

                    }
                    // Now user-defined vdata
                    else {

                        VDATA *vdataobj = NULL;
                        try {
                            vdataobj = VDATA::Read (vdata_id, obj_ref);
                        }
                        catch(...) {
                            free (full_path);
                            free (cfull_path);
                            VSdetach(vdata_id);
                            Vdetach (vgroup_id);
                            throw;
                        }

                        vdataobj->newname = full_path +vdataobj->name;

                        //We want to map fields of vdata with more than 10 records to DAP variables
                        // and we need to add the path and vdata name to the new vdata field name
                        if (!vdataobj->getTreatAsAttrFlag ()) {
                            for (std::vector <VDField * >::const_iterator it_vdf =
                                vdataobj->getFields ().begin ();
                                it_vdf != vdataobj->getFields ().end ();
                                it_vdf++) {

                                // Change vdata name conventions. 
                                // "vdata"+vdata_newname+"_vdf_"+(*it_vdf)->newname

                                (*it_vdf)->newname =
                                    "vdata" + vdataobj->newname + "_vdf_" + (*it_vdf)->name;

                                //Make sure the name is following CF, KY 2012-6-26
                                (*it_vdf)->newname = HDFCFUtil::get_CF_string((*it_vdf)->newname);
                            }

                        }
                
                        // Make sure the name is following CF
                        vdataobj->newname = HDFCFUtil::get_CF_string(vdataobj->newname);

                        // Save back this vdata
                        this->vds.push_back (vdataobj);

                        status = VSdetach (vdata_id);
                        if (status == FAIL) {
                            Vdetach (vgroup_id);
                            free (full_path);
                            free (cfull_path);
                            throw3 ("VSdetach failed ",
                                    "Vdata is under vgroup ", vgroup_name);
                        }
                    }
                }

                //Ignore the handling of SDS objects. They are handled elsewhere. 
                else;
            }
            free (full_path);
            free (cfull_path);

            status = Vdetach (vgroup_id);
            if (status == FAIL) {
                throw3 ("Vdetach failed ", "vgroup_name is ", vgroup_name);
            }
        }//end of the for loop

    }// end of the if loop

}

// Check if this is a special SDS(MOD08_M3) that needs the connection between CVs and dimension names.
// General algorithm:
// 1. Insert a set for fields' dimensions, 
// 2. in the mean time, insert a set for 1-D field
// 3. For each dimension in the set, search if one can find the corresponding field that has the same dimension name in the set.
// Return false if non-found occurs. 
// Else return true.

bool
File::Check_update_special(const string& grid_name) throw(Exception) {

    set<string> dimnameset;
    set<SDField*> fldset; 

    // Build up a dimension set and a 1-D field set.
    // We already know that XDim and YDim should be in the dimension set. so inserting them.
    // Hopefully by doing this, we can save some time since many variables have dimensions 
    // "XDim" and "YDim" and excluding "XDim" and "YDim" may save some time if there are many 
    // dimensions in the dimnameset.

    string FullXDim,FullYDim;
    FullXDim="XDim:" ;
    FullYDim="YDim:";

    FullXDim= FullXDim+grid_name;
    FullYDim =FullYDim+grid_name;

    for (vector < SDField * >::const_iterator i =
        this->sd->getFields ().begin ();
        i != this->sd->getFields ().end (); ++i) {

        for (vector < Dimension * >::const_iterator k =
            (*i)->getDimensions ().begin ();
            k != (*i)->getDimensions ().end (); ++k) {
            if((*k)->getName() !=FullXDim && (*k)->getName()!=FullYDim)
               dimnameset.insert((*k)->getName());
        }

        if (1==(*i)->getRank())
            fldset.insert((*i));

    }

//cerr<<"field size is= "<<fldset.size() <<endl;
//cerr<<"dim. size is= "<<dimnameset.size() <<endl;
   
    // Check if all dimension names in the dimension set can be found in the 1-D variable sets. Moreover, the size of a dimension 
    // should be smaller or the same as the size of 1-D variable.
    // Plus XDim and YDim for number of dimensions 
    if (fldset.size() < (dimnameset.size()+2))
        return false;

    int total_num_dims = 0;
    size_t grid_name_size = grid_name.size();
    string reduced_dimname;

    for(set<SDField*>::const_iterator j =
            fldset.begin(); j!=fldset.end(); ++ j) {
        
        size_t dim_size = ((*j)->getDimensions())[0]->getName().size();
        if( dim_size > grid_name_size){
           reduced_dimname = ((*j)->getDimensions())[0]->getName().substr(0,dim_size-grid_name_size-1);
           if ((*j)->getName() == reduced_dimname)
                total_num_dims++;
        }
    }
 
    if((size_t)total_num_dims != (dimnameset.size()+2))
        return false;

    // Updated dimension names for all variables: removing the grid_name prefix.
    for (vector < SDField * >::const_iterator i =
        this->sd->getFields ().begin ();
        i != this->sd->getFields ().end (); ++i) {

        for (vector < Dimension * >::const_iterator k =
            (*i)->getDimensions ().begin ();
            k != (*i)->getDimensions ().end (); ++k) {

             size_t dim_size = (*k)->getName().size();
             if( dim_size > grid_name_size){
                reduced_dimname = (*k)->getName().substr(0,dim_size-grid_name_size-1);
                (*k)->name = reduced_dimname;
             }
             else // Here we enforce that the dimension name has the grid suffix. This can be lifted in the future. KY 2014-01-16
                return false;
        }

    }

    // Build up Dimensions for DDS and DAS. 
    for(std::set<SDField*>::const_iterator j =
		    fldset.begin(); j!=fldset.end(); ++ j) {

           if ((*j)->getName() == ((*j)->getDimensions())[0]->getName()) {
 
            if("XDim" == (*j)->getName()){
                std::string tempunits = "degrees_east";
                (*j)->setUnits (tempunits); 
                (*j)->fieldtype = 2;
            }

            else if("YDim" == (*j)->getName()){
                std::string tempunits = "degrees_north";
                (*j)->setUnits (tempunits); 
                (*j)->fieldtype = 1;
            }

            else if("Pressure_Level" == (*j)->getName()) {
                std::string tempunits = "hPa";
                (*j)->setUnits (tempunits);
                (*j)->fieldtype = 3;
            }
            else {
                std::string tempunits = "level";
                (*j)->setUnits (tempunits);
                (*j)->fieldtype = 3;
            }
        }
    }

    return true;

}

#if 0
// This routine is used to check if this grid is a special MOD08M3-like grid in DDS-build. 
// Check_if_special is used when building DAS. The reason to separate is that we pass the 
// File pointer from DAS to DDS to reduce the building time.
// How to check: 
// 1) 

bool
File::Check_if_special(const string& grid_name) throw(Exception) {


     bool xdim_is_lon = false;
     bool ydim_is_lat = false;
     bool pre_unit_hpa = true;
     for (vector < SDField * >::const_iterator i =
        this->sd->getFields ().begin ();
        i != this->sd->getFields ().end (); ++i) {
        if (1==(*i)->getRank()) {
            if(1 == ((*i)->fieldtype)) {
                if("YDim" == (*j)->getName()

            }

        }
    }
}
#endif
void
File::Handle_AIRS_L23() throw(Exception) {

    File *file = this;

    bool airs_l3 = true;
    if(basename(file->path).find(".L2.")!=string::npos)
        airs_l3 = false;

    // set of names of dimensions that have dimension scales.
    set<string> scaled_dname_set;

    // set of names of dimensions that don't have dimension scales.
    set<string> non_scaled_dname_set;
    pair<set<string>::iterator,bool> ret;

    // For dimensions that don't have dimension scales, a map between dimension name and size.
    map<string,int> non_scaled_dname_to_size;

    // 1.  Loop through SDS fields and remove suffixes(:???) of the dimension names and the variable names. 
    //     Also create scaled dim. name set and non-scaled dim. name set.
    for (std::vector < SDField * >::const_iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

        string tempname = (*i)->name;
        size_t found_colon = tempname.find_first_of(':');
        if(found_colon!=string::npos) 
            (*i)->newname = tempname.substr(0,found_colon);

        for (vector < Dimension * >::const_iterator k =
            (*i)->getDimensions ().begin ();
            k != (*i)->getDimensions ().end (); ++k) {

            string tempname = (*k)->name;
            size_t found_colon = tempname.find_first_of(':');
            if(found_colon!=string::npos) 
                (*k)->name = tempname.substr(0,found_colon);

            if(0==(*k)->getType()) {
                ret = non_scaled_dname_set.insert((*k)->name);
                if (true == ret.second)
                    non_scaled_dname_to_size[(*k)->name] = (*k)->dimsize;
            }
            else{
                scaled_dname_set.insert((*k)->name);
           }
                      
        }

    }
#if 0
for(set<string>::const_iterator sdim_it = scaled_dname_set.begin();
                                    sdim_it !=scaled_dname_set.end();
                                    ++sdim_it) {
cerr<<"scaled dim. name "<<*sdim_it <<endl;

}
#endif

    // For AIRS level 3 only ****
    // 2. Remove potential redundant CVs
    // For AIRS level 3 version 6 products, many dimension scale variables shared the same value. Physically they are the same.
    // So to keep the performance optimal and reduce the non-necessary clutter, I remove the duplicate variables.
    // An Example: StdPressureLev:asecending is the same as the StdPressureLev:descending, reduce to StdPressureLev 

    // Make a copy of the scaled-dim name set:scaled-dim-marker
  if(true == airs_l3) {
    set<string>scaled_dname_set_marker = scaled_dname_set;
  
    // Loop through all the SDS objects, 
    // If finding a 1-D variable name 
    // b1) in both the scaled-dim name set and the scaled-dim-marker set, 
    //     keep this variable but remove the variable name from the scaled-dim-marker. 
    //     Mark this variable as a CV.(XDim: 2, YDim:1 Others: 3). 
    // b2) In the scaled-dim name set but not in the scaled-dim-marker set, 
    //     remove the variable from the variable vector.
    for (std::vector < SDField * >::iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
        if(1 == (*i)->getRank()) {
            if(scaled_dname_set.find((*i)->getNewName())!=scaled_dname_set.end()) {
                if(scaled_dname_set_marker.find((*i)->getNewName())!=scaled_dname_set_marker.end()) {
                    scaled_dname_set_marker.erase((*i)->getNewName());
                }

                else {// Redundant variables
//cerr<<"redundant variable names "<< (*i)->newname <<endl;
                    delete(*i);
                    file->sd->sdfields.erase(i);
                    // when erasing the iterator, it always goes to the next elment, so move back.
                    i--;
                }
            }
        }
        // Remove Latitude and Longitude 
        else if( 2 == (*i)->getRank()) {
            if ("Latitude" == (*i)->getNewName() ||  "Longitude" == (*i)->getNewName()) {
                delete(*i);
                file->sd->sdfields.erase(i);
                i--;
            }
        }
        else 
            ++i;
    }
  }

#if 0
for(set<string>::const_iterator sdim_it = scaled_dname_set.begin();
                                    sdim_it !=scaled_dname_set.end();
                                    ++sdim_it) {
cerr<<"new scaled dim. name "<<*sdim_it <<endl;

}
#endif

    //3. Add potential missing CVs
    
    // 3.1 Find the true dimensions that don't have dimension scales.
    set<string>final_non_scaled_dname_set;
    for(set<string>::const_iterator non_sdim_it = non_scaled_dname_set.begin(); 
                                    non_sdim_it !=non_scaled_dname_set.end(); 
                                    ++non_sdim_it) {
//cerr<<"non-scaled dim. name "<<*non_sdim_it <<endl;
        if(scaled_dname_set.find(*non_sdim_it)==scaled_dname_set.end())
            final_non_scaled_dname_set.insert(*non_sdim_it);
    }

    // 3.2 Create the missing CVs based on the non-scaled dimensions.
    for(set<string>::const_iterator non_sdim_it = final_non_scaled_dname_set.begin();
                                    non_sdim_it !=final_non_scaled_dname_set.end();
                                    ++non_sdim_it) {

        SDField *missingfield = new SDField ();

        // The name of the missingfield is not necessary.
        // We only keep here for consistency.
        missingfield->type = DFNT_INT32;
        missingfield->name = *non_sdim_it;
        missingfield->newname = *non_sdim_it;
        missingfield->rank = 1;
        missingfield->fieldtype = 4;
        missingfield->setUnits("level");
        Dimension *dim = new Dimension (*non_sdim_it,non_scaled_dname_to_size[*non_sdim_it] , 0);

        missingfield->dims.push_back (dim);
        file->sd->sdfields.push_back (missingfield);
    }

    // For AIRS level 3 only
    // Change XDim to Longitude and YDim to Latitude for field name and dimension names

  if(true == airs_l3) {
    for (std::vector < SDField * >::const_iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

        if(1 ==(*i)->getRank()){
            if ("XDim" == (*i)->newname)
                (*i)->newname = "Longitude";
            else if ("YDim" == (*i)->newname)
                (*i)->newname = "Latitude";
        }

        for (vector < Dimension * >::const_iterator k =
            (*i)->getDimensions ().begin ();
            k != (*i)->getDimensions ().end (); ++k) {
            if("XDim" == (*k)->name)
                (*k)->name = "Longitude";
            else if ("YDim" == (*k)->name)
                (*k)->name = "Latitude";
        }

    }
  }
  
    // For AIRS level 2 only
    if(false == airs_l3) {

        bool change_lat_unit = false;
        bool change_lon_unit = false;
        string ll_dimname1 = "";
        string ll_dimname2 = "";

        // 1. Assign the lat/lon units according to the CF conventions. 
        // 2. Obtain dimension names of lat/lon.
        for (std::vector < SDField * >::const_iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

            if(2 == (*i)->getRank()) {
                if("Latitude" == (*i)->newname){
                    (*i)->fieldtype = 1;
                    change_lat_unit = true;
                    string tempunits = "degrees_north";
                    (*i)->setUnits(tempunits);
                    ll_dimname1 = (*i)->getDimensions()[0]->getName();
                    ll_dimname2 = (*i)->getDimensions()[1]->getName();

                }
                else if("Longitude" == (*i)->newname) {
                    (*i)->fieldtype = 2;
                    change_lon_unit = true;
                    string tempunits = "degrees_east";
                    (*i)->setUnits(tempunits);
                }
                if((true == change_lat_unit) && (true == change_lon_unit))
                    break;
            }
        }

        // 2. Generate the coordinate attribute
        string tempcoordinates = "";
        string tempfieldname   = "";
        int tempcount = 0;
        
        for (std::vector < SDField * >::const_iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

            // We don't want to add "coordinates" attributes to all dimension scale variables.
            bool dimscale_var = false;
            dimscale_var = ((*i)->rank == 1) & (((*i)->newname) == ((*i)->getDimensions()[0]->getName()));
           
            if((0 ==(*i)->fieldtype) && (false == dimscale_var)) {

                tempcount = 0;
                tempcoordinates = "";
                tempfieldname = "";

                // First check if the dimension names of this variable include both ll_dimname1 and ll_dimname2.
                bool has_lldim1 = false;
                bool has_lldim2 = false;
                for (std::vector < Dimension * >::const_iterator j =
                    (*i)->getDimensions ().begin ();
                    j != (*i)->getDimensions ().end (); ++j) {
                    if((*j)->name == ll_dimname1)
                        has_lldim1 = true;
                    else if ((*j)->name == ll_dimname2)
                        has_lldim2 = true;
                    if((true == has_lldim1) && (true == has_lldim2))
                        break;
 
                }
              
               
                if((true == has_lldim1) && (true == has_lldim2)) {
                  for (std::vector < Dimension * >::const_iterator j =
                    (*i)->getDimensions ().begin ();
                    j != (*i)->getDimensions ().end (); ++j) {
                    if((*j)->name == ll_dimname1)
                        tempfieldname = "Latitude";
                    else if ((*j)->name == ll_dimname2)
                        tempfieldname = "Longitude";
                    else 
                        tempfieldname = (*j)->name;
                
                    if (0 == tempcount)
                        tempcoordinates = tempfieldname;
                    else
                        tempcoordinates = tempcoordinates + " " + tempfieldname;
                    tempcount++;
                  }
                }
                else {
                  for (std::vector < Dimension * >::const_iterator j =
                    (*i)->getDimensions ().begin ();
                    j != (*i)->getDimensions ().end (); ++j) {
                    if (0 == tempcount)
                        tempcoordinates = (*j)->name;
                    else
                        tempcoordinates = tempcoordinates + " " + (*j)->name;
                    tempcount++;
                  }

                }
                (*i)->setCoordinates (tempcoordinates);

            }
        }
    }
}

//  This method will check if the HDF4 file is one of TRMM or OBPG products or MODISARNSS we supported.
void
File::CheckSDType ()
throw (Exception)
{

    // check the TRMM version 7  cases
    // The default sptype is OTHERHDF.
    // 2A,2B check attribute FileHeader, FileInfo and SwathHeader
    // 3A,3B check attribute FileHeader, FileInfo and GridHeader
    // 3A25 check attribute FileHeader, FileInfo and GridHeader1, GridHeader2
    if (this->sptype == OTHERHDF) {

        int trmm_multi_gridflag = 0;
        int trmm_single_gridflag = 0;
        int trmm_swathflag = 0;

        for (std::vector < Attribute * >::const_iterator i =
            this->sd->getAttributes ().begin ();
            i != this->sd->getAttributes ().end (); ++i) {
            if ((*i)->getName () == "FileHeader") {
                trmm_multi_gridflag++;
                trmm_single_gridflag++;
                trmm_swathflag++;
            }
            if ((*i)->getName () == "FileInfo") {
                trmm_multi_gridflag++;
                trmm_single_gridflag++;
                trmm_swathflag++;
            }
            if ((*i)->getName () == "SwathHeader") 
                trmm_swathflag++;

            if ((*i)->getName () == "GridHeader")
                trmm_single_gridflag++;

            else if (((*i)->getName ().find ("GridHeader") == 0) &&
                     (((*i)->getName()).size() >10))
                trmm_multi_gridflag++;

        }

        
        if(3 == trmm_single_gridflag) 
            this->sptype = TRMML3S_V7;
        else if(3 == trmm_swathflag) 
            this->sptype = TRMML2_V7;
        else if(trmm_multi_gridflag >3)
            this->sptype = TRMML3M_V7;
            
    }
 
    // check the TRMM and MODARNSS/MYDARNSS cases
    // The default sptype is OTHERHDF.
    if (this->sptype == OTHERHDF) {

        int metadataflag = 0;

        for (std::vector < Attribute * >::const_iterator i =
            this->sd->getAttributes ().begin ();
            i != this->sd->getAttributes ().end (); ++i) {
            if ((*i)->getName () == "CoreMetadata.0")
            metadataflag++;
            if ((*i)->getName () == "ArchiveMetadata.0")
                metadataflag++;
            if ((*i)->getName () == "StructMetadata.0")
                metadataflag++;
            if ((*i)->getName ().find ("SubsettingMethod") !=
                std::string::npos)
                metadataflag++;
        }

        // This is a very special MODIS product. It includes StructMetadata.0 
        // but it is not an HDF-EOS2 file. We use metadata name "SubsettingMethod" as an indicator.
        // We find this metadata name is uniquely applied to this MODIS product.
        // We need to change the way if HDF-EOS MODIS files also use this metadata name. 
        if (metadataflag == 4)	 
            this->sptype = MODISARNSS;

        // DATA_GRANULE is the TRMM "swath" name; geolocation 
        // is the TRMM "geolocation" field.
        if (metadataflag == 2) {	

            for (std::vector < SDField * >::const_iterator i =
                this->sd->getFields ().begin ();
                i != this->sd->getFields ().end (); ++i) {
                if (((*i)->getName () == "geolocation")
                    && (*i)->getNewName ().find ("DATA_GRANULE") !=
                    std::string::npos
                    && (*i)->getNewName ().find ("SwathData") !=
                    std::string::npos && (*i)->getRank () == 3) {
                    this->sptype = TRMML2_V6;
                    break;
                }
            }

            // For TRMM Level 3 3A46, CSH, 3B42 and 3B43 data. 
            // The vgroup name is DATA_GRANULE.
            // For 3B42 and 3B43, at least one field is 1440*400 array. 
            // For CSH and 3A46 the number of dimension should be >2.
            // CSH: 2 dimensions should be 720 and 148.
            // 3A46: 2 dimensions should be 180 and 360.
            // The information is obtained from 
	    // http://disc.sci.gsfc.nasa.gov/additional/faq/precipitation_faq.shtml#lat_lon
            if (this->sptype == OTHERHDF) {
                for (std::vector < SDField * >::const_iterator i =
                    this->sd->getFields ().begin ();
                    i != this->sd->getFields ().end (); ++i) {
                    if ((*i)->getNewName ().find ("DATA_GRANULE") !=
                        std::string::npos) {
                        bool l3b_v6_lonflag = false;
                        bool l3b_v6_latflag = false;
                        for (std::vector < Dimension * >::const_iterator k =
                            (*i)->getDimensions ().begin ();
                            k != (*i)->getDimensions ().end (); ++k) {
                            if ((*k)->getSize () == 1440)
                            l3b_v6_lonflag = true;

                            if ((*k)->getSize () == 400)
                                l3b_v6_latflag = true;
                        }
                        if (l3b_v6_lonflag == true && l3b_v6_latflag == true) {
                            this->sptype = TRMML3B_V6;
                            break;
                        }

                        
                        bool l3a_v6_latflag = false;
                        bool l3a_v6_lonflag = false;

                        bool l3c_v6_lonflag = false;
                        bool l3c_v6_latflag = false;

                        if ((*i)->getRank()>2) {
                            for (std::vector < Dimension * >::const_iterator k =
                            (*i)->getDimensions ().begin ();
                            k != (*i)->getDimensions ().end (); ++k) {
                            if ((*k)->getSize () == 360)
                                l3a_v6_lonflag = true;

                            if ((*k)->getSize () == 180)
                                l3a_v6_latflag = true;

                            if ((*k)->getSize () == 720)
                                l3c_v6_lonflag = true;

                            if ((*k)->getSize () == 148)
                                l3c_v6_latflag = true;
                           }
                           
                        }

                        if (true == l3a_v6_latflag && true == l3a_v6_lonflag) {
                            this->sptype = TRMML3A_V6;
                            break;
                        }

                        if (true == l3c_v6_latflag && true == l3c_v6_lonflag) {
                            this->sptype = TRMML3C_V6;
                            break;
                        }


                    }
                }
            }
        }
    }
#if 0
if(this->sptype == TRMML3A_V6) 
cerr<<"3A46 products "<<endl;
if(this->sptype == TRMML3C_V6) 
cerr<<"CSH products "<<endl;
#endif



    // Check the OBPG case
    // OBPG includes SeaWIFS,OCTS,CZCS,MODISA,MODIST
    // One attribute "Product Name" includes unique information for each product,
    // For example, SeaWIFS L2 data' "Product Name" is S2003006001857.L2_MLAC 
    // Since we only support Level 2 and Level 3m data, we just check if those characters exist inside the attribute.
    // The reason we cannot support L1A data is that lat/lon consists of fill values.

    if (this->sptype == OTHERHDF) {

        // MODISA level 2 product flag
        int modisal2flag = 0;

        // MODIST level 2 product flag
        int modistl2flag = 0;

        // OCTS level 2 product flag
        int octsl2flag = 0;

        // SeaWiFS level 2 product flag
        int seawifsl2flag = 0;

        // CZCS level 2 product flag
        int czcsl2flag = 0;

        // MODISA level 3m product flag
        int modisal3mflag = 0;

        // MODIST level 3m product flag
        int modistl3mflag = 0;

        // OCTS level 3m product flag
        int octsl3mflag = 0;

        // SeaWIFS level 3m product flag
        int seawifsl3mflag = 0;

        // CZCS level 3m product flag
        int czcsl3mflag = 0;

        // Loop the global attributes and find the attribute called "Product Name"
        // and the attribute called "Sensor Name",
        // then identify different products.
        for (std::vector < Attribute * >::const_iterator i =
            this->sd->getAttributes ().begin ();
            i != this->sd->getAttributes ().end (); ++i) {
            if ((*i)->getName () == "Product Name") {

            std::string attrvalue ((*i)->getValue ().begin (),
                                   (*i)->getValue ().end ());
            if ((attrvalue.find_first_of ('A', 0) == 0)
                && (attrvalue.find (".L2", 0) != std::string::npos))
                modisal2flag++;
            else if ((attrvalue.find_first_of ('A', 0) == 0)
                    && (attrvalue.find (".L3m", 0) != std::string::npos))
                modisal3mflag++;
            else if ((attrvalue.find_first_of ('T', 0) == 0)
                    && (attrvalue.find (".L2", 0) != std::string::npos))
                modistl2flag++;
            else if ((attrvalue.find_first_of ('T', 0) == 0)
                    && (attrvalue.find (".L3m", 0) != std::string::npos))
                modistl3mflag++;
            else if ((attrvalue.find_first_of ('O', 0) == 0)
                    && (attrvalue.find (".L2", 0) != std::string::npos))
                octsl2flag++;
            else if ((attrvalue.find_first_of ('O', 0) == 0)
                    && (attrvalue.find (".L3m", 0) != std::string::npos))
                octsl3mflag++;
            else if ((attrvalue.find_first_of ('S', 0) == 0)
                    && (attrvalue.find (".L2", 0) != std::string::npos))
                seawifsl2flag++;
            else if ((attrvalue.find_first_of ('S', 0) == 0)
                    && (attrvalue.find (".L3m", 0) != std::string::npos))
                seawifsl3mflag++;
            else if ((attrvalue.find_first_of ('C', 0) == 0)
                    && ((attrvalue.find (".L2", 0) != std::string::npos)
                    ||
                    ((attrvalue.find (".L1A", 0) != std::string::npos))))
                czcsl2flag++;
            else if ((attrvalue.find_first_of ('C', 0) == 0)
                    && (attrvalue.find (".L3m", 0) != std::string::npos))
                czcsl3mflag++;
            else;
        }
        if ((*i)->getName () == "Sensor Name") {

            std::string attrvalue ((*i)->getValue ().begin (),
                                   (*i)->getValue ().end ());
            if (attrvalue.find ("MODISA", 0) != std::string::npos) {
                modisal2flag++;
		modisal3mflag++;
            }
            else if (attrvalue.find ("MODIST", 0) != std::string::npos) {
                modistl2flag++;
                modistl3mflag++;
            }
            else if (attrvalue.find ("OCTS", 0) != std::string::npos) {
                octsl2flag++;
                octsl3mflag++;
            }
            else if (attrvalue.find ("SeaWiFS", 0) != std::string::npos) {
                seawifsl2flag++;
                seawifsl3mflag++;
            }
            else if (attrvalue.find ("CZCS", 0) != std::string::npos) {
                czcsl2flag++;
                czcsl3mflag++;
            }
            else;
        }

        if ((modisal2flag == 2) || (modisal3mflag == 2)
            || (modistl2flag == 2) || (modistl3mflag == 2)
            || (octsl2flag == 2) || (octsl3mflag == 2)
            || (seawifsl2flag == 2) || (seawifsl3mflag == 2)
            || (czcsl2flag == 2) || (czcsl3mflag == 2))
            break;

        }
        // Only when both the sensor name and the product name match, we can
        // be sure the products are OBPGL2 or OBPGL3m.
        if ((modisal2flag == 2) || (modistl2flag == 2) ||
            (octsl2flag == 2) || (seawifsl2flag == 2) || (czcsl2flag == 2))
            this->sptype = OBPGL2;

        if ((modisal3mflag == 2) ||
            (modistl3mflag == 2) || (octsl3mflag == 2) ||
            (seawifsl3mflag == 2) || (czcsl3mflag == 2))
            this->sptype = OBPGL3;

    }
}

// Read SDS information from the HDF4 file
SD *
SD::Read (int32 sdfd, int32 fileid)
throw (Exception)
{

    // Indicator of status
    int32  status        = 0;

    // Number of SDS objects in this file
    int32  n_sds         = 0;

    // Number of SDS attributes in this file
    int32  n_sd_attrs    = 0;

    // Object index 
    int    sds_index     = 0;

    // SDS ID
    int32  sds_id        = 0;

    // Dimension sizes
    int32  dim_sizes[H4_MAX_VAR_DIMS];

    // number of SDS attributes
    int32  n_sds_attrs   = 0;

    // In the future, we may use the latest HDF4 APIs to obtain the length of object names etc. dynamically.
    // Documented in a jira ticket HFRHANDLER-168.

    // SDS name
    char   sds_name[H4_MAX_NC_NAME];

    // SDS dimension names
    char   dim_name[H4_MAX_NC_NAME];

    // SDS attribute names
    char   attr_name[H4_MAX_NC_NAME];

    // Dimension size
    int32  dim_size      = 0;

    // SDS reference number
    int32  sds_ref       = 0;

    // Dimension type(if dimension type is 0, this dimension doesn't have dimension scales)
    // Otherwise, this dimension type is the datatype of this dimension scale.
    int32  dim_type      = 0; 

    // Number of dimension attributes(This is almost never used)
    //int32  num_dim_attrs = 0;

    // Attribute value count
    int32  attr_value_count = 0;

    // Obtain a SD instance
    SD* sd = new SD (sdfd, fileid);

    // Obtain number of SDS objects and number of SD(file) attributes
    if (SDfileinfo (sdfd, &n_sds, &n_sd_attrs) == FAIL) {
        delete sd;
        throw2 ("SDfileinfo failed ", sdfd);
    }

    // Go through the SDS object
    for (sds_index = 0; sds_index < n_sds; sds_index++) {

        // New SDField instance
        SDField *field = new SDField ();

        // Obtain SDS ID.
        sds_id = SDselect (sdfd, sds_index);
        if (sds_id == FAIL) {
            delete sd;
            delete field;
            // We only need to close SDS ID. SD ID will be closed when the destructor is called.
            SDendaccess (sds_id);
            throw3 ("SDselect Failed ", "SDS index ", sds_index);
        }

        // Obtain SDS reference number
        sds_ref = SDidtoref (sds_id);
        if (sds_ref == FAIL) {
            delete sd;
            delete field;
            SDendaccess (sds_id);
            throw3 ("Cannot obtain SDS reference number", " SDS ID is ",
                     sds_id);
        }

        // Obtain object name, rank, size, field type and number of SDS attributes
        status = SDgetinfo (sds_id, sds_name, &field->rank, dim_sizes,
                            &field->type, &n_sds_attrs);
        if (status == FAIL) {
            delete sd;
            delete field;
           SDendaccess (sds_id);
            throw2 ("SDgetinfo failed ", sds_name);
        }

        //Assign SDS field info. to class field instance.
        string tempname (sds_name);
        field->name = tempname;
        field->newname = field->name;
        field->fieldref = sds_ref;
        // This will be used to obtain the SDS full path later.
        sd->refindexlist[sds_ref] = sds_index;

        // Handle dimension scale
        bool dim_no_dimscale = false;
        vector <int> dimids;
        if (field->rank >0) 
            dimids.assign(field->rank,0);

        // Assign number of dimension attribute vector
        vector <int>num_dim_attrs;
        if (field->rank >0)
            num_dim_attrs.assign(field->rank,0);

        // Handle dimensions with original dimension names
        for (int dimindex = 0; dimindex < field->rank; dimindex++)  {

            // Obtain dimension ID.
            int dimid = SDgetdimid (sds_id, dimindex);
            if (dimid == FAIL) {
                delete sd;
                delete field;
                SDendaccess (sds_id);
                throw5 ("SDgetdimid failed ", "SDS name ", sds_name,
                         "dim index= ", dimindex);
            }	

            // Obtain dimension info.: dim_name, dim_size,dim_type and num of dim. attrs.
            int temp_num_dim_attrs = 0;
            status =
                SDdiminfo (dimid, dim_name, &dim_size, &dim_type,
                           (int32*)&temp_num_dim_attrs);
            if (status == FAIL) {
               delete sd;
               delete field;
               SDendaccess (sds_id);
                throw5 ("SDdiminfo failed ", "SDS name ", sds_name,
                        "dim index= ", dimindex);
            }

            num_dim_attrs[dimindex] = temp_num_dim_attrs;

            // No dimension attribute has been found in NASA files, 
            // so don't handle it now. KY 2010-06-08

            // Dimension attributes are found in one JPL file(S2000415.HDF). 
            // So handle it. 
            // If the corresponding dimension scale exists, no need to 
            // specially handle the attributes.
            // But when the dimension scale doesn't exist, we would like to 
            // handle the attributes following
            // the default HDF4 handler. We will add attribute containers. 
            // For example, variable name foo has
            // two dimensions, foo1, foo2. We just create two attribute names: 
            // foo_dim0, foo_dim1,
            // foo_dim0 will include an attribute "name" with the value as 
            // foo1 and other attributes.
            // KY 2012-09-11

            string dim_name_str (dim_name);

            // Since dim_size will be 0 if the dimension is 
            // unlimited dimension, so use dim_sizes instead
            Dimension *dim =
                new Dimension (dim_name_str, dim_sizes[dimindex], dim_type);

            // Save this dimension
            field->dims.push_back (dim);

            // First check if there are dimensions in this field that 
            // don't have dimension scales.
            dimids[dimindex] = dimid;
            if (0 == dim_type) {
                if (false == dim_no_dimscale) 
                    dim_no_dimscale = true;
                if ((dim_name_str == field->name) && (1 == field->rank))
                    field->is_noscale_dim = true;
            }
        }
                      
        // Find dimensions that have no dimension scales, 
        // add attribute for this whole field ???_dim0, ???_dim1 etc.

        if( true == dim_no_dimscale) {

            for (int dimindex = 0; dimindex < field->rank; dimindex++) {

                string dim_name_str = (field->dims)[dimindex]->name;
                AttrContainer *dim_info = new AttrContainer ();
                string index_str;
                stringstream out_index;
                out_index  << dimindex;
                index_str = out_index.str();
                dim_info->name = "_dim_" + index_str;

                // newname will be created at the final stage.

                bool dimname_flag = false;

                int32 dummy_type = 0;
                int32 dummy_value_count = 0;

                // Loop through to check if an attribute called "name" exists and set a flag.
                for (int attrindex = 0; attrindex < num_dim_attrs[dimindex]; attrindex++) {

                    status = SDattrinfo(dimids[dimindex],attrindex,attr_name,
                                        &dummy_type,&dummy_value_count);
                    if (status == FAIL) {
                        delete sd;
                        delete field;
                        SDendaccess (sds_id);
                        throw3 ("SDattrinfo failed ", "SDS name ", sds_name);
                    }

                    string tempname(attr_name);
                    if ("name"==tempname) {
                        dimname_flag = true;
                        break;
                    }
                }
                                   
                // Loop through to obtain the dimension attributes and save the corresponding attributes to dim_info.
                for (int attrindex = 0; attrindex < num_dim_attrs[dimindex]; attrindex++) {

                    Attribute *attr = new Attribute();
                    status = SDattrinfo(dimids[dimindex],attrindex,attr_name,
                                               &attr->type,&attr_value_count);
                    if (status == FAIL) {
                        delete sd;
                        delete field;
                        SDendaccess (sds_id);
                        throw3 ("SDattrinfo failed ", "SDS name ", sds_name);
                    }
                    string tempname (attr_name);
                    attr->name = tempname;
                    tempname = HDFCFUtil::get_CF_string(tempname);

                    attr->newname = tempname;
                    attr->count = attr_value_count;
                    attr->value.resize (attr_value_count * DFKNTsize (attr->type));
                    if (SDreadattr (dimids[dimindex], attrindex, &attr->value[0]) == -1) {
                        delete sd;
                        delete field;
                        SDendaccess (sds_id);
                        throw5 ("read SDS attribute failed ", "Field name ",
                                 field->name, " Attribute name ", attr->name);
                    }

                    dim_info->attrs.push_back (attr);
	
                }
                            
                // If no attribute called "name", we create an attribute "name" and save the name of the attribute
                // as the attribute value.
                if (false == dimname_flag) { 

                    Attribute *attr = new Attribute();
                    attr->name = "name";
                    attr->newname = "name";
                    attr->type = DFNT_CHAR;
                    attr->count = dim_name_str.size();
                    attr->value.resize(attr->count);
                    copy(dim_name_str.begin(),dim_name_str.end(),attr->value.begin());
                    dim_info->attrs.push_back(attr);

                }
                field->dims_info.push_back(dim_info);
            }
        }

        // Loop through all the SDS attributes and save them to the class field instance.
        for (int attrindex = 0; attrindex < n_sds_attrs; attrindex++) {
            Attribute *attr = new Attribute ();
            status =
                SDattrinfo (sds_id, attrindex, attr_name, &attr->type,
                            &attr_value_count);

            if (status == FAIL) {
                delete attr;
                delete sd;
                delete field;
                SDendaccess (sds_id);
                throw3 ("SDattrinfo failed ", "SDS name ", sds_name);
            }

            string tempname (attr_name);
            attr->name = tempname;
            tempname = HDFCFUtil::get_CF_string(tempname);

            attr->newname = tempname;
            attr->count = attr_value_count;
            attr->value.resize (attr_value_count * DFKNTsize (attr->type));
            if (SDreadattr (sds_id, attrindex, &attr->value[0]) == -1) {
                delete attr;
                delete sd;
                delete field;
                SDendaccess (sds_id);
                throw5 ("read SDS attribute failed ", "Field name ",
                         field->name, " Attribute name ", attr->name);
            }
            field->attrs.push_back (attr);
        }
        SDendaccess (sds_id);
        sd->sdfields.push_back (field);
    }

    // Loop through all SD(file) attributes and save them to the class sd instance.
    for (int attrindex = 0; attrindex < n_sd_attrs; attrindex++) {

        Attribute *attr = new Attribute ();
        status = SDattrinfo (sdfd, attrindex, attr_name, &attr->type,
                             &attr_value_count);
        if (status == FAIL) {
            delete attr;
            delete sd;
            throw3 ("SDattrinfo failed ", "SD id ", sdfd);
        }
        std::string tempname (attr_name);
        attr->name = tempname;

        // Checking and handling the special characters for the SDS attribute name.
        tempname = HDFCFUtil::get_CF_string(tempname);
        attr->newname = tempname;
        attr->count = attr_value_count;
        attr->value.resize (attr_value_count * DFKNTsize (attr->type));
        if (SDreadattr (sdfd, attrindex, &attr->value[0]) == -1) {
            delete attr;
            delete sd;
            throw3 ("Cannot read SD attribute", " Attribute name ",
                     attr->name);
        }
        sd->attrs.push_back (attr);
    }

    return sd;

}

// Retrieve the extra SDS object info. from a hybrid HDF-EOS2 file
SD *
SD::Read_Hybrid (int32 sdfd, int32 fileid)
throw (Exception)
{
    // Indicator of status
    int32  status        = 0;

    // Number of SDS objects in this file
    int32  n_sds         = 0;

    // Number of SDS attributes in this file
    int32  n_sd_attrs    = 0;

    // SDS Object index 
    int    sds_index     = 0;

    // Extra SDS object index
    int extra_sds_index = 0;

    // SDS ID
    int32  sds_id        = 0;

    // Dimension sizes
    int32  dim_sizes[H4_MAX_VAR_DIMS];

    // number of SDS attributes
    int32  n_sds_attrs   = 0;

    // In the future, we may use the latest HDF4 APIs to obtain the length of object names etc. dynamically.
    // Documented in a jira ticket HFRHANDLER-168.

    // SDS name
    char   sds_name[H4_MAX_NC_NAME];

    // SDS dimension names
    char   dim_name[H4_MAX_NC_NAME];

    // SDS attribute names
    char   attr_name[H4_MAX_NC_NAME];

    // Dimension size
    int32  dim_size      = 0;

    // SDS reference number
    int32  sds_ref       = 0;

    // Dimension type(if dimension type is 0, this dimension doesn't have dimension scales)
    // Otherwise, this dimension type is the datatype of this dimension scale.
    int32  dim_type      = 0; 

    // Number of dimension attributes(This is almost never used)
    int32  num_dim_attrs = 0;

    // Attribute value count
    int32  attr_value_count = 0;


    // TO OBTAIN the full path of the SDS objects 
    int32 vgroup_id = 0; 

    // lone vgroup index
    int32 lone_vg_number =  0; 

    // number of lone vgroups
    int32 num_of_lones =  -1; 

    int32 num_gobjects;

    // Object reference and tag pair. Key to find an HDF4 object
    int32 obj_ref = 0;
    int32 obj_tag = 0;

    // Temporary index.
    int i;

    // In the future, we may use the latest HDF4 APIs to obtain the length of object names etc. dynamically.
    // Documented in a jira ticket HFRHANDLER-168.
    char vgroup_name[VGNAMELENMAX*4];
    char vgroup_class[VGNAMELENMAX*4];

    //std::string full_path;

    // full path of an object
    char *full_path;
   // char full_path[MAX_FULL_PATH_LEN];

    // copy of the full path
    char *cfull_path;
//    char cfull_path[MAX_FULL_PATH_LEN];

    // Obtain a SD instance
    SD *sd = new SD (sdfd, fileid);

    // Obtain number of SDS objects and number of SD(file) attributes
    if (SDfileinfo (sdfd, &n_sds, &n_sd_attrs) == FAIL) {
        delete sd;
        throw2 ("SDfileinfo failed ", sdfd);
    }

    // Loop through all SDS objects to obtain the SDS reference numbers.
    // Then save the reference numbers into the SD instance sd.
    for (sds_index = 0; sds_index < n_sds; sds_index++) {
        sds_id = SDselect (sdfd, sds_index);

        if (sds_id == FAIL) {
            delete sd;
            // We only need to close SDS ID. SD ID will be closed when 
            // the destructor is called.
            SDendaccess (sds_id);
            throw3 ("SDselect Failed ", "SDS index ", sds_index);
        }

        sds_ref = SDidtoref (sds_id);
        if (sds_ref == FAIL) {
            delete sd;
            SDendaccess (sds_id);
            throw3 ("Cannot obtain SDS reference number", " SDS ID is ",
                     sds_id);
        }
        sd->sds_ref_list.push_back(sds_ref);
        SDendaccess(sds_id);
    }

    // Now we need to obtain the sds reference numbers 
    // for SDS objects that are accessed as the HDF-EOS2 grid or swath.
    // No NASA HDF4 files have the vgroup that forms a ring; so ignore this case.
    // First, call Vlone with num_of_lones set to 0 to get the number of
    // lone vgroups in the file, but not to get their reference numbers.

    num_of_lones = Vlone (fileid, NULL, 0);
    if (num_of_lones == FAIL){
        delete sd;
        throw3 ("Fail to obtain lone vgroup number", "file id is", fileid);
    }

    // if there are any lone vgroups,
    if (num_of_lones > 0) {

        // use the num_of_lones returned to allocate sufficient space for the
        // buffer ref_array to hold the reference numbers of all lone vgroups,
        vector<int32>ref_array;
        ref_array.resize(num_of_lones);

        // and call Vlone again to retrieve the reference numbers into
        // the buffer ref_array.
        num_of_lones = Vlone (fileid, &ref_array[0], num_of_lones);
        if (num_of_lones == FAIL) {
            delete sd;
            throw3 ("Cannot obtain lone vgroup reference arrays ",
                    "file id is ", fileid);
        }

        // loop through all the lone vgroup objects
        for (lone_vg_number = 0; lone_vg_number < num_of_lones;
             lone_vg_number++) {

            // Attach to the current vgroup 
            vgroup_id = Vattach (fileid, ref_array[lone_vg_number], "r");
            if (vgroup_id == FAIL) {
                delete sd;
                throw3 ("Vattach failed ", "Reference number is ",
                         ref_array[lone_vg_number]);
            }

            status = Vgetname (vgroup_id, vgroup_name);
            if (status == FAIL) {
                delete sd;
                Vdetach (vgroup_id);
                throw3 ("Vgetname failed ", "vgroup_id is ", vgroup_id);
            }

            status = Vgetclass (vgroup_id, vgroup_class);
            if (status == FAIL) {
                delete sd;
                Vdetach (vgroup_id);
                throw3 ("Vgetclass failed ", "vgroup_name is ", vgroup_name);
            }

            //Ignore internal HDF groups
            if (strcmp (vgroup_class, _HDF_ATTRIBUTE) == 0
                || strcmp (vgroup_class, _HDF_VARIABLE) == 0
                || strcmp (vgroup_class, _HDF_DIMENSION) == 0
                || strcmp (vgroup_class, _HDF_UDIMENSION) == 0
                || strcmp (vgroup_class, _HDF_CDF) == 0
                || strcmp (vgroup_class, GR_NAME) == 0
                || strcmp (vgroup_class, RI_NAME) == 0) {
                Vdetach (vgroup_id);
                continue;
            }

            // Obtain the number of objects of this vgroup
            num_gobjects = Vntagrefs (vgroup_id);
            if (num_gobjects < 0) {
                delete sd;
                Vdetach (vgroup_id);
                throw3 ("Vntagrefs failed ", "vgroup_name is ", vgroup_name);
            }

            // Obtain the vgroup full path and the copied vgroup full path
            // MAX_FULL_PATH_LEN(1024) is long enough
            // to cover any HDF4 object path for all NASA HDF4 products.
            // So using strcpy and strcat is safe in a practical sense.
            // However, in the future, we should update the code to  use HDF4 APIs to obtain vgroup_name length dynamically.
            // At that time, we will use strncpy and strncat instead. We may even think to use C++ vector <char>.
            // Documented in a jira ticket HFRHANDLER-168. 
            // KY 2013-07-12
            // We replace strcpy and strcat with strncpy and strncat as suggested. KY 2013-08-29
            
            full_path = (char *) malloc (MAX_FULL_PATH_LEN);
            if (full_path == NULL) {
                delete sd;
                Vdetach (vgroup_id);
                throw1 ("No enough memory to allocate the buffer.");
            }

            memset(full_path,'\0',MAX_FULL_PATH_LEN);
            strncpy(full_path,_BACK_SLASH,strlen(_BACK_SLASH));
            strncat(full_path, vgroup_name,strlen(vgroup_name));

            cfull_path = (char *) malloc (MAX_FULL_PATH_LEN);
            if (cfull_path == NULL) {
                delete sd;
                Vdetach (vgroup_id);
                free (full_path);
                throw1 ("No enough memory to allocate the buffer.");
            }
            memset(cfull_path,'\0',MAX_FULL_PATH_LEN);
            strncpy (cfull_path, full_path,strlen(full_path));

            // Loop all objects in this vgroup
            for (i = 0; i < num_gobjects; i++) {

                // Obtain the object reference and tag of this object
                if (Vgettagref (vgroup_id, i, &obj_tag, &obj_ref) == FAIL) {
                    delete sd;
                    Vdetach (vgroup_id);
                    free (full_path);
                    free (cfull_path);
                    throw5 ("Vgettagref failed ", "vgroup_name is ",
                             vgroup_name, " reference number is ", obj_ref);
                }

                // If this object is a vgroup, will call recursively to obtain the SDS path.
                if (Visvg (vgroup_id, obj_ref) == TRUE) {
                    strncpy(full_path,cfull_path,strlen(cfull_path)+1);
                    full_path[strlen(cfull_path)]='\0';
                    sd->obtain_noneos2_sds_path (fileid, full_path, obj_ref);
                }

                // These are SDS objects
                else if (obj_tag == DFTAG_NDG || obj_tag == DFTAG_SDG
                         || obj_tag == DFTAG_SD) {

                    // Here we need to check if the SDS is an EOS object by checking
                    // if the the path includes "Data Fields" or "Geolocation Fields".
                    // If the object is an EOS object, we will remove the sds 
                    // reference number from the list.
                    string temp_str = string(full_path);
                    if((temp_str.find("Data Fields") != std::string::npos)||
                        (temp_str.find("Geolocation Fields") != std::string::npos))                                   
                        sd->sds_ref_list.remove(obj_ref);

                }
                // Do nothing for other objects
                else;
            }
            free (full_path);
            free (cfull_path);

            status = Vdetach (vgroup_id);

            if (status == FAIL) {
                delete sd;
                throw3 ("Vdetach failed ", "vgroup_name is ", vgroup_name);
            }
        }//end of the for loop

    }// end of the if loop

    // Loop through the sds reference list; now the list should only include non-EOS SDS objects.
    for(std::list<int32>::iterator sds_ref_it = sd->sds_ref_list.begin(); 
        sds_ref_it!=sd->sds_ref_list.end();++sds_ref_it) {

        extra_sds_index = SDreftoindex(sdfd,*sds_ref_it);
        if(extra_sds_index == FAIL) { 
            delete sd;
            throw3("SDreftoindex Failed ","SDS reference number ", *sds_ref_it);
        }

        SDField *field = new SDField ();
        sds_id = SDselect (sdfd, extra_sds_index);
       	if (sds_id == FAIL) {
            delete field;
            delete sd;
            // We only need to close SDS ID. SD ID will be closed when the destructor is called.
            SDendaccess (sds_id);
            throw3 ("SDselect Failed ", "SDS index ", extra_sds_index);
        }

        // Obtain object name, rank, size, field type and number of SDS attributes
        status = SDgetinfo (sds_id, sds_name, &field->rank, dim_sizes,
                            &field->type, &n_sds_attrs);
        if (status == FAIL) {
            delete field;
            delete sd;
            SDendaccess (sds_id);
            throw2 ("SDgetinfo failed ", sds_name);
        }

        // new name for added SDS objects are renamed as original_name + "NONEOS".
        string tempname (sds_name);
        field->name = tempname;
        tempname = HDFCFUtil::get_CF_string(tempname);
        field->newname = tempname+"_"+"NONEOS";
        field->fieldref = *sds_ref_it;
        sd->refindexlist[*sds_ref_it] = extra_sds_index;

        // Handle dimensions with original dimension names
        for (int dimindex = 0; dimindex < field->rank; dimindex++) {
            int dimid = SDgetdimid (sds_id, dimindex);
            if (dimid == FAIL) {
                delete field;
                delete sd;
                SDendaccess (sds_id);
                throw5 ("SDgetdimid failed ", "SDS name ", sds_name,
                        "dim index= ", dimindex);
            }
            status = SDdiminfo (dimid, dim_name, &dim_size, &dim_type,
                                &num_dim_attrs);

            if (status == FAIL) {
                delete field;
                delete sd;
                SDendaccess (sds_id);
                throw5 ("SDdiminfo failed ", "SDS name ", sds_name,
                        "dim index= ", dimindex);
            }

            string dim_name_str (dim_name);

            // Since dim_size will be 0 if the dimension is unlimited dimension, 
            // so use dim_sizes instead
            Dimension *dim =
                new Dimension (dim_name_str, dim_sizes[dimindex], dim_type);

            field->dims.push_back (dim);

            // The corrected dims are added simply for the consistency in hdfdesc.cc, 
            // it doesn't matter
            // for the added SDSes at least for now. KY 2011-2-13

            // However, some dimension names have special characters. 
            // We need to remove special characters.
            // Since no coordinate attributes will be provided for 
            // these extra SDSes, we don't need to
            // make the dimension names consistent with other dimension names. 
            // But we need to keep an eye
            // on if the units of the extra SDSes are degrees_north or degrees_east. 
            // This will make the tools
            // automatically treat them as latitude or longitude.
            //  Need to double check. KY 2011-2-17
            // So far we don't meet the above case. KY 2013-07-12

            string cfdimname =  HDFCFUtil::get_CF_string(dim_name_str);
            Dimension *correcteddim =
                new Dimension (cfdimname, dim_sizes[dimindex], dim_type);

            field->correcteddims.push_back (correcteddim);
                       
        }

        // Loop through all SDS attributes and save them to field.
        for (int attrindex = 0; attrindex < n_sds_attrs; attrindex++) {

            Attribute *attr = new Attribute ();

            status = SDattrinfo (sds_id, attrindex, attr_name, &attr->type,
                                 &attr_value_count);
            if (status == FAIL) {
                delete attr;
                delete field;
                delete sd;
                SDendaccess (sds_id);
                throw3 ("SDattrinfo failed ", "SDS name ", sds_name);
            }

            string tempname (attr_name);
            attr->name = tempname;

            // Checking and handling the special characters for the SDS attribute name.
            tempname = HDFCFUtil::get_CF_string(tempname);
            attr->newname = tempname;
            attr->count = attr_value_count;
            attr->value.resize (attr_value_count * DFKNTsize (attr->type));
            if (SDreadattr (sds_id, attrindex, &attr->value[0]) == -1) {
                delete attr;
                delete field;
                delete sd;
                SDendaccess (sds_id);
                throw5 ("read SDS attribute failed ", "Field name ",
                         field->name, " Attribute name ", attr->name);
            }
            field->attrs.push_back (attr);
        }
        SDendaccess (sds_id);
        sd->sdfields.push_back (field);
    }
    return sd;
}

// Retrieve Vdata information from the HDF4 file
VDATA *
VDATA::Read (int32 vdata_id, int32 obj_ref)
throw (Exception)
{

    // Vdata field size 
    int32 fieldsize   = 0;

    // Vdata field datatype
    int32 fieldtype   = 0;

    // Vdata field order
    int32 fieldorder  = 0;

    // Vdata field name
    char *fieldname   = NULL;

     // In the future, we may use the latest HDF4 APIs to obtain the length of object names etc. dynamically.
    // Documented in a jira ticket HFRHANDLER-168. 
    char vdata_name[VSNAMELENMAX];

    VDATA *vdata = new VDATA (vdata_id, obj_ref);

    vdata->vdref = obj_ref;

    if (VSQueryname (vdata_id, vdata_name) == FAIL){
        delete vdata;
        throw3 ("VSQueryname failed ", "vdata id is ", vdata_id);
    }

    string vdatanamestr (vdata_name);

    vdata->name = vdatanamestr;
    vdata->newname = HDFCFUtil::get_CF_string(vdata->name);
    int32 num_field = VFnfields (vdata_id);

    if (num_field == -1){
        delete vdata;
        throw3 ("For vdata, VFnfields failed. ", "vdata name is ",
                 vdata->name);
    }

    int32 num_record = VSelts (vdata_id);

    if (num_record == -1) {
        delete vdata;
        throw3 ("For vdata, VSelts failed. ", "vdata name is ", vdata->name);
    }


    // Using the BES KEY for people to choose to map the vdata to attribute for a smaller number of record.
    // KY 2012-6-26 
  
    string check_vdata_to_attr_key="H4.EnableVdata_to_Attr";
    bool turn_on_vdata_to_attr_key = false;

    turn_on_vdata_to_attr_key = HDFCFUtil::check_beskeys(check_vdata_to_attr_key);

    // The reason to add this flag is if the number of record is too big, the DAS table is too huge to allow some clients to work.
    // Currently if the number of record is >=10; one vdata field is mapped to a DAP variable.
    // Otherwise, it is mapped to a DAP attribute.

    if (num_record <= 10 && true == turn_on_vdata_to_attr_key)
        vdata->TreatAsAttrFlag = true;
    else
        vdata->TreatAsAttrFlag = false;

    // Loop through all fields and save information to a vector 
    for (int i = 0; i < num_field; i++) {

        VDField *field = new VDField ();
        fieldsize = VFfieldesize (vdata_id, i);
        if (fieldsize == FAIL) {
            delete field;
            delete vdata;
            throw5 ("For vdata field, VFfieldsize failed. ", "vdata name is ",
                     vdata->name, " index is ", i);
        }

        fieldname = VFfieldname (vdata_id, i);
        if (fieldname == NULL) {
            delete field;
            delete vdata;
            throw5 ("For vdata field, VFfieldname failed. ", "vdata name is ",
                     vdata->name, " index is ", i);
        }

        fieldtype = VFfieldtype (vdata_id, i);
        if (fieldtype == FAIL) {
            delete field;
            delete vdata;
            throw5 ("For vdata field, VFfieldtype failed. ", "vdata name is ",
                     vdata->name, " index is ", i);
        }

        fieldorder = VFfieldorder (vdata_id, i);
        if (fieldorder == FAIL) {
            delete field;
            delete vdata;
            throw5 ("For vdata field, VFfieldtype failed. ", "vdata name is ",
                     vdata->name, " index is ", i);
        }

        field->name = fieldname;
        field->newname = HDFCFUtil::get_CF_string(field->name);
        field->type = fieldtype;
        field->order = fieldorder;
        field->size = fieldsize;
        field->rank = 1;
        field->numrec = num_record;
//cerr<<"vdata field name is "<<field->name <<endl;
//cerr<<"vdata field type is "<<field->type <<endl;

        
        if (vdata->getTreatAsAttrFlag () && num_record > 0) {	// Currently we only save small size vdata to attributes

            field->value.resize (num_record * fieldsize);
            if (VSseek (vdata_id, 0) == FAIL) {
                delete field;
                delete vdata;
                throw5 ("vdata ", vdata_name, "field ", fieldname,
                        " VSseek failed.");
            }

            if (VSsetfields (vdata_id, fieldname) == FAIL) {
                delete field;
                delete vdata;
                throw3 ("vdata field ", fieldname, " VSsetfields failed.");
            }

            if (VSread
                (vdata_id, (uint8 *) & field->value[0], num_record,
                 FULL_INTERLACE) == FAIL){
                delete field;
                delete vdata;
                throw3 ("vdata field ", fieldname, " VSread failed.");
            }

        }

        try {
            // Read field attributes
            field->ReadAttributes (vdata_id, i);
        }
        catch(...) {
            delete field;
            delete vdata;
            throw;
        }
        vdata->vdfields.push_back (field);
    }

    try {
        // Read Vdata attributes
        vdata->ReadAttributes (vdata_id);
    }
    catch(...) {
        delete vdata;
        throw;
    }
    return vdata;

}

// Read Vdata attributes and save them into vectors
void
VDATA::ReadAttributes (int32 vdata_id)
throw (Exception)
{
    // Vdata attribute name
     // In the future, we may use the latest HDF4 APIs to obtain the length of object names etc. dynamically.
    // Documented in a jira ticket HFRHANDLER-168. 
    char attr_name[H4_MAX_NC_NAME];

    // Number of attributes
    int32 nattrs = 0;

    // Number of attribute size
    int32 attrsize = 0;

    // API status indicator
    int32 status = 0;

    // Number of vdata attributes
    nattrs = VSfnattrs (vdata_id, _HDF_VDATA);

//  This is just to check if the weird MacOS portability issue go away.KY 2011-3-31
#if 0
    if (nattrs == FAIL)
        throw3 ("VSfnattrs failed ", "vdata id is ", vdata_id);
#endif

    if (nattrs > 0) {

        // Obtain number of vdata attributes 
        for (int i = 0; i < nattrs; i++) {

            Attribute *attr = new Attribute ();

            status = VSattrinfo (vdata_id, _HDF_VDATA, i, attr_name,
                                 &attr->type, &attr->count, &attrsize);
            if (status == FAIL) {
                delete attr;
                throw5 ("VSattrinfo failed ", "vdata id is ", vdata_id,
                        " attr index is ", i);
            }

            // Checking and handling the special characters for the vdata attribute name.
            string tempname(attr_name);
            attr->name = tempname;
            attr->newname = HDFCFUtil::get_CF_string(attr->name);
            attr->value.resize (attrsize);
	    if (VSgetattr (vdata_id, _HDF_VDATA, i, &attr->value[0]) == FAIL) {
                delete attr;
                throw5 ("VSgetattr failed ", "vdata id is ", vdata_id,
                        " attr index is ", i);
            }
            attrs.push_back (attr);
        }
    }
}


// Retrieve VD field attributes from the HDF4 file.
// Input parameters are vdata ID and vdata field index
void
VDField::ReadAttributes (int32 vdata_id, int32 fieldindex)
throw (Exception)
{
    // In the future, we may use the latest HDF4 APIs to obtain the length of object names etc. dynamically.
    // Documented in a jira ticket HFRHANDLER-168. 
    // vdata field attribute name
    char attr_name[H4_MAX_NC_NAME];

    // Number of vdata field attributes
    int32 nattrs = 0;

    // Vdata attribute size
    int32 attrsize = 0;

    // Indicator of vdata field APIs
    int32 status = 0;

    // Obtain 
    nattrs = VSfnattrs (vdata_id, fieldindex);

// This is just to check if the weird MacOS portability issue go away.KY 2011-3-9
#if 0
    if (nattrs == FAIL)
        throw5 ("VSfnattrs failed ", "vdata id is ", vdata_id,
                "Field index is ", fieldindex);
#endif

    if (nattrs > 0) {

        // Obtain vdata field attributes
        for (int i = 0; i < nattrs; i++) {

            Attribute *attr = new Attribute ();

            status = VSattrinfo (vdata_id, fieldindex, i, attr_name,
                                 &attr->type, &attr->count, &attrsize);

            if (status == FAIL) {
                delete attr;
                throw5 ("VSattrinfo failed ", "vdata field index ",
                         fieldindex, " attr index is ", i);
            }

            string tempname(attr_name);
            attr->name = tempname;

            // Checking and handling the special characters for the vdata field attribute name.
            attr->newname = HDFCFUtil::get_CF_string(attr->name);

            attr->value.resize (attrsize);
            if (VSgetattr (vdata_id, fieldindex, i, &attr->value[0]) == FAIL) {
                delete attr;
                throw5 ("VSgetattr failed ", "vdata field index is ",
                         fieldindex, " attr index is ", i);
            }
            attrs.push_back (attr);
        }
    }
}

void 
File::ReadVgattrs(int32 vgroup_id,char*fullpath) throw(Exception) {

    intn status_n;
    //int  n_attr_value = 0;
    char attr_name[H4_MAX_NC_NAME];
    AttrContainer *vg_attr = NULL;

    intn n_attrs = Vnattrs(vgroup_id);
    if(n_attrs == FAIL) 
        throw1("Vnattrs failed");
    if(n_attrs > 0) {
        vg_attr = new AttrContainer();
        string temp_container_name(fullpath);
        vg_attr->name = HDFCFUtil::get_CF_string(temp_container_name);
    }
 
    for(int attr_index = 0; attr_index <n_attrs; attr_index++) {

        Attribute *attr = new Attribute();
        int32 value_size_32 = 0;
        status_n = Vattrinfo(vgroup_id, (intn)attr_index, attr_name, &attr->type, 
                            &attr->count, &value_size_32);
        if(status_n == FAIL) {
            delete attr;
            throw1("Vattrinfo failed.");
        }
        int value_size = value_size_32;

	string tempname (attr_name);
        attr->name = tempname;
        tempname = HDFCFUtil::get_CF_string(tempname);
        attr->newname = tempname;
        attr->value.resize (value_size);

        status_n = Vgetattr(vgroup_id,(intn)attr_index,&attr->value[0]);
        if(status_n == FAIL) {
            delete attr;
            throw3("Vgetattr failed. ","The attribute name is ",attr->name);
        }
        vg_attr->attrs.push_back(attr);
    }

    if(vg_attr !=NULL)
        vg_attrs.push_back(vg_attr);


}

// This code is used to obtain the full path of SDS and vdata.
// Since it uses HDF4 library a lot, we keep the C style. KY 2010-7-13
void
File::InsertOrigFieldPath_ReadVgVdata ()
throw (Exception)
{
    /************************* Variable declaration **************************/

    // Status indicator of HDF4 APIs
    int32 status = 0;			

    // H interface ID
    int32 file_id = 0;

    // vgroup ID
    int32 vgroup_id = 0;

    // Vdata ID
    int32 vdata_id = 0;

    // Lone vgroup index 
    int32 lone_vg_number = 0;		 

    // Number of lone vgroups
    int32 num_of_lones = -1;		

    // Number of vgroup objects
    int32 num_gobjects = 0;

    // Object reference number and tag(The pair is a key to identify an HDF4 object)
    int32 obj_ref = 0;
    int32 obj_tag = 0;

    // We may use new HDF4 APIs to obtain the length of the following object names and then
    // allocate a buffer to store the names dynamically.  Documented in a jira ticket HFRHANDLER-168.

    // Vdata name 
    char vdata_name[VSNAMELENMAX];

    // Vdata class  
    char vdata_class[VSNAMELENMAX];

    // Vgroup name
    char vgroup_name[VGNAMELENMAX*4];

    // Vgroup class 
    char vgroup_class[VGNAMELENMAX*4];

    // Full path of an object
    char *full_path;

    // Copy of a full path of an object
    char *cfull_path;

    // SD interface ID
    int32 sd_id;

    file_id = this->fileid;
    sd_id   = this->sdfd;

    // No NASA HDF4 files have the vgroup that forms a ring; so ignore this case.
    // First, call Vlone with num_of_lones set to 0 to get the number of
    // lone vgroups in the file, but not to get their reference numbers.
    num_of_lones = Vlone (file_id, NULL, 0);
    if (num_of_lones == FAIL)
        throw3 ("Fail to obtain lone vgroup number", "file id is", file_id);

    // if there are any lone vgroups,
    if (num_of_lones > 0) {

        // Use the num_of_lones returned to allocate sufficient space for the
        // buffer ref_array to hold the reference numbers of all lone vgroups,
        vector<int32>ref_array;
        ref_array.resize(num_of_lones);

        // And call Vlone again to retrieve the reference numbers into
        // the buffer ref_array.
        num_of_lones = Vlone (file_id, &ref_array[0], num_of_lones);
        if (num_of_lones == FAIL) {
            throw3 ("Cannot obtain lone vgroup reference arrays ",
                    "file id is ", file_id);
        }

        // Loop through all lone vgroups
        for (lone_vg_number = 0; lone_vg_number < num_of_lones;
             lone_vg_number++) {

            // Attach to the current vgroup 
            vgroup_id = Vattach (file_id, ref_array[lone_vg_number], "r");
            if (vgroup_id == FAIL) {
                throw3 ("Vattach failed ", "Reference number is ",
                         ref_array[lone_vg_number]);
            }

            status = Vgetname (vgroup_id, vgroup_name);
            if (status == FAIL) {
                Vdetach (vgroup_id);
                throw3 ("Vgetname failed ", "vgroup_id is ", vgroup_id);
            }

            status = Vgetclass (vgroup_id, vgroup_class);
            if (status == FAIL) {
                Vdetach (vgroup_id);
                throw3 ("Vgetclass failed ", "vgroup_name is ", vgroup_name);
            }

            //Ignore internal HDF groups
            if (strcmp (vgroup_class, _HDF_ATTRIBUTE) == 0
                || strcmp (vgroup_class, _HDF_VARIABLE) == 0
                || strcmp (vgroup_class, _HDF_DIMENSION) == 0
                || strcmp (vgroup_class, _HDF_UDIMENSION) == 0
                || strcmp (vgroup_class, _HDF_CDF) == 0
                || strcmp (vgroup_class, GR_NAME) == 0
                || strcmp (vgroup_class, RI_NAME) == 0) {
                Vdetach(vgroup_id);
                continue;
            }

            num_gobjects = Vntagrefs (vgroup_id);
            if (num_gobjects < 0) {
                Vdetach (vgroup_id);
                throw3 ("Vntagrefs failed ", "vgroup_name is ", vgroup_name);
            }

            // Obtain full path and cfull_path. 
            // MAX_FULL_PATH_LEN(1024) is long enough
            // to cover any HDF4 object path for all NASA HDF4 products.
            // KY 2013-07-12
            //
            full_path = (char *) malloc (MAX_FULL_PATH_LEN);
            if (full_path == NULL) {
                Vdetach (vgroup_id);
                throw1 ("No enough memory to allocate the buffer.");
            }

            memset(full_path,'\0',MAX_FULL_PATH_LEN);
            strncpy (full_path,_BACK_SLASH,strlen(_BACK_SLASH));
            strncat(full_path,vgroup_name,strlen(vgroup_name));
            try {
                ReadVgattrs(vgroup_id,full_path);

            }
            catch(...) {
                Vdetach (vgroup_id);
                free (full_path);
                throw1 ("ReadVgattrs failed ");
            }
            strncat(full_path,_BACK_SLASH,strlen(_BACK_SLASH));

            cfull_path = (char *) malloc (MAX_FULL_PATH_LEN);
            if (cfull_path == NULL) {
                Vdetach (vgroup_id);
                free (full_path);
                throw1 ("No enough memory to allocate the buffer.");
            }
            memset(cfull_path,'\0',MAX_FULL_PATH_LEN);
            strncpy (cfull_path, full_path,strlen(full_path));

            // Loop all vgroup objects
            for (int i = 0; i < num_gobjects; i++) {
                if (Vgettagref (vgroup_id, i, &obj_tag, &obj_ref) == FAIL) {
                    Vdetach (vgroup_id);
                    free (full_path);
                    free (cfull_path);
                    throw5 ("Vgettagref failed ", "vgroup_name is ",
                             vgroup_name, " reference number is ", obj_ref);
                }

                // If this is a vgroup, recursively obtain information
                if (Visvg (vgroup_id, obj_ref) == TRUE) {
                    strncpy (full_path, cfull_path,strlen(cfull_path)+1);
                    full_path[strlen(cfull_path)]='\0';
                    obtain_path (file_id, sd_id, full_path, obj_ref);
                }
                // This is a vdata
                else if (Visvs (vgroup_id, obj_ref)) {

                    vdata_id = VSattach (file_id, obj_ref, "r");
                    if (vdata_id == FAIL) {
                        Vdetach (vgroup_id);
                        free (full_path);
                        free (cfull_path);
                        throw5 ("VSattach failed ", "vgroup_name is ",
                                 vgroup_name, " reference number is ",
                                 obj_ref);
                    }
                    status = VSgetname (vdata_id, vdata_name);
                    if (status == FAIL) {
                        Vdetach (vgroup_id);
                        free (full_path);
                        free (cfull_path);
                        throw5 ("VSgetclass failed ", "vgroup_name is ",
                                 vgroup_name, " reference number is ",
                                 obj_ref);
                    }

                    status = VSgetclass (vdata_id, vdata_class);
                    if (status == FAIL) {
                        Vdetach (vgroup_id);
                        free (full_path);
                        free (cfull_path);
                        throw5 ("VSgetclass failed ", "vgroup_name is ",
                                 vgroup_name, " reference number is ",
                                 obj_ref);
                    }

                    //NOTE: I found that for 1BTRMMdata(1B21...), there
                    // is an attribute stored in vdata under vgroup SwathData that cannot 
                    // be retrieved by any attribute APIs. I suspect this is an HDF4 bug.
                    // This attribute is supposed to be an attribute under vgroup SwathData.
                    // Since currently the information has been preserved by the handler,
                    // so we don't have to handle this. It needs to be reported to the
                    // HDF4 developer. 2010-6-25 ky

                    // Vdata that is either used to store attributes or internal HDF4 classes. ignore.
                    if (VSisattr (vdata_id) == TRUE
                        || !strncmp (vdata_class, _HDF_CHK_TBL_CLASS,
                            strlen (_HDF_CHK_TBL_CLASS))
                        || !strncmp (vdata_class, _HDF_SDSVAR,
                            strlen (_HDF_SDSVAR))
                        || !strncmp (vdata_class, _HDF_CRDVAR,
                            strlen (_HDF_CRDVAR))
                        || !strncmp (vdata_class, DIM_VALS, strlen (DIM_VALS))
                        || !strncmp (vdata_class, DIM_VALS01,
                            strlen (DIM_VALS01))
                        || !strncmp (vdata_class, RIGATTRCLASS,
                            strlen (RIGATTRCLASS))
                        || !strncmp (vdata_name, RIGATTRNAME,
                            strlen (RIGATTRNAME))) {

                        status = VSdetach (vdata_id);
                        if (status == FAIL) {
                            Vdetach (vgroup_id);
                            free (full_path);
                            free (cfull_path);
                            throw3 ("VSdetach failed ",
                                    "Vdata is under vgroup ", vgroup_name);
                        }

                    }
                    else {

                        VDATA*vdataobj = NULL;
                        try {
                            vdataobj = VDATA::Read (vdata_id, obj_ref);
                        }
                        catch(...) {
                            free (full_path);
                            free (cfull_path);
                            VSdetach(vdata_id);
                            Vdetach (vgroup_id);
                            throw;
                        }


                        vdataobj->newname = full_path +vdataobj->name;

                        //We want to map fields of vdata with more than 10 records to DAP variables
                        // and we need to add the path and vdata name to the new vdata field name
                        if (!vdataobj->getTreatAsAttrFlag ()) {
                            for (std::vector <VDField * >::const_iterator it_vdf =
                                vdataobj->getFields ().begin ();
                                it_vdf != vdataobj->getFields ().end ();
                                it_vdf++) {

                                // Change vdata name conventions. 
                                // "vdata"+vdata_newname+"_vdf_"+(*it_vdf)->newname

                                (*it_vdf)->newname =
                                    "vdata" + vdataobj->newname + "_vdf_" + (*it_vdf)->name;

                                //Make sure the name is following CF, KY 2012-6-26
                                (*it_vdf)->newname = HDFCFUtil::get_CF_string((*it_vdf)->newname);
                            }

                        }
                
                        vdataobj->newname = HDFCFUtil::get_CF_string(vdataobj->newname);

                        this->vds.push_back (vdataobj);

                        status = VSdetach (vdata_id);
                        if (status == FAIL) {
                            Vdetach (vgroup_id);
                            free (full_path);
                            free (cfull_path);
                            throw3 ("VSdetach failed ",
                                    "Vdata is under vgroup ", vgroup_name);
                        }
                    }
                }

                // These are SDS objects
                else if (obj_tag == DFTAG_NDG || obj_tag == DFTAG_SDG
                         || obj_tag == DFTAG_SD) {

                    // We need to obtain the SDS index; also need to store the new name(object name + full_path).
                    if (this->sd->refindexlist.find (obj_ref) !=
                    sd->refindexlist.end ())
                        this->sd->sdfields[this->sd->refindexlist[obj_ref]]->newname =
                              full_path +
                              this->sd->sdfields[this->sd->refindexlist[obj_ref]]->name;
                    else {
                        Vdetach (vgroup_id);
                        free (full_path);
                        free (cfull_path);
                        throw3 ("SDS with the reference number ", obj_ref,
                                " is not found");
                    }
                }
                else;
            }
            free (full_path);
            free (cfull_path);

            status = Vdetach (vgroup_id);
            if (status == FAIL) {
                throw3 ("Vdetach failed ", "vgroup_name is ", vgroup_name);
            }
        }//end of the for loop
    }// end of the if loop

}

// This fuction is called recursively to obtain the full path of an HDF4 object.
// obtain_path, obtain_noneos2_sds_path,obtain_vdata_path are very similar.
// We may combine them in the future. Documented at HFRHANDLER-166.
void
File::obtain_path (int32 file_id, int32 sd_id, char *full_path,
				   int32 pobj_ref)
throw (Exception)
{
    // Vgroup parent ID
    int32 vgroup_pid = 0;

    // Indicator of status
    int32 status     = 0;

    // Index i
    int i            = 0;

    // Number of group objects
    int num_gobjects = 0;

    // The following names are statically allocated.
    // This can be updated in the future with new HDF4 APIs that can provide the actual length of an object name.
    // Documented in a jira ticket HFRHANDLER-168.
    // KY 2013-07-11

    // Child vgroup name 
    char cvgroup_name[VGNAMELENMAX*4];

    // Vdata name
    char vdata_name[VSNAMELENMAX];

    // Vdata class name
    char vdata_class[VSNAMELENMAX];

    // Vdata ID
    int32 vdata_id = -1;

    // Object tag
    int32 obj_tag = 0; 

    // Object reference
    int32 obj_ref = 0;

    // full path of the child group
    char *cfull_path;

    // MAX_FULL_PATH_LEN(1024) is long enough
    // to cover any HDF4 object path for all NASA HDF4 products.
    // So using strcpy and strcat is safe in a practical sense.
    // However, in the future, we should update the code to  use HDF4 APIs to obtain vgroup_name length dynamically.
    // At that time, we will use strncpy and strncat instead. We may even think to use C++ vector <char>.
    // Documented in a jira ticket HFRHANDLER-168. 
    // KY 2013-07-12
    // We use strncpy and strncat to replace strcpy and strcat. KY 2013-09-06

    cfull_path = (char *) malloc (MAX_FULL_PATH_LEN);
    if (cfull_path == NULL)
        throw1 ("No enough memory to allocate the buffer");

    memset(cfull_path,'\0',MAX_FULL_PATH_LEN);
    vgroup_pid = Vattach (file_id, pobj_ref, "r");
    if (vgroup_pid == FAIL) {
        free (cfull_path);
        throw3 ("Vattach failed ", "Object reference number is ", pobj_ref);
    }

    if (Vgetname (vgroup_pid, cvgroup_name) == FAIL) {
        Vdetach (vgroup_pid);
        free (cfull_path);
        throw3 ("Vgetname failed ", "Object reference number is ", pobj_ref);
    }
    num_gobjects = Vntagrefs (vgroup_pid);
    if (num_gobjects < 0) {
        Vdetach (vgroup_pid);
        free (cfull_path);
        throw3 ("Vntagrefs failed ", "Object reference number is ", pobj_ref);
    }

    strncpy(cfull_path,full_path,strlen(full_path));
    strncat(cfull_path,cvgroup_name,strlen(cvgroup_name));
    try {
        ReadVgattrs(vgroup_pid,cfull_path);

    }
    catch(...) {
        Vdetach (vgroup_pid);
        free (cfull_path);
        throw1 ("ReadVgattrs failed ");
    }

    strncat(cfull_path,_BACK_SLASH,strlen(_BACK_SLASH));



    for (i = 0; i < num_gobjects; i++) {

        if (Vgettagref (vgroup_pid, i, &obj_tag, &obj_ref) == FAIL) {
            Vdetach (vgroup_pid);
            free (cfull_path);
            throw3 ("Vgettagref failed ", "object index is ", i);
        }

        if (Visvg (vgroup_pid, obj_ref) == TRUE) {
            strncpy(full_path,cfull_path,strlen(cfull_path)+1);
            full_path[strlen(cfull_path)]='\0';
            obtain_path (file_id, sd_id, full_path, obj_ref);
        }
        else if (Visvs (vgroup_pid, obj_ref)) {

            vdata_id = VSattach (file_id, obj_ref, "r");
            if (vdata_id == FAIL) {
                Vdetach (vgroup_pid);
                free (cfull_path);
                throw3 ("VSattach failed ", "object index is ", i);
            }


            status = VSQueryname (vdata_id, vdata_name);
            if (status == FAIL) {
                Vdetach (vgroup_pid);
                free (cfull_path);
                throw3 ("VSgetclass failed ", "object index is ", i);
            }

            status = VSgetclass (vdata_id, vdata_class);
            if (status == FAIL) {
                Vdetach (vgroup_pid);
                free (cfull_path);
                throw3 ("VSgetclass failed ", "object index is ", i);
            }

            if (VSisattr (vdata_id) != TRUE) {
                if (strncmp
                           (vdata_class, _HDF_CHK_TBL_CLASS,
                            strlen (_HDF_CHK_TBL_CLASS))) {

                    VDATA *vdataobj = NULL;

                    try {
                        vdataobj = VDATA::Read (vdata_id, obj_ref);
                    }
                    catch(...) {
                        free (cfull_path);
                        Vdetach (vgroup_pid);
                        throw;
                    }


                    // The new name conventions require the path prefixed before the object name.
                    vdataobj->newname = cfull_path + vdataobj->name;
                    // We want to map fields of vdata with more than 10 records to DAP variables
                    // and we need to add the path and vdata name to the new vdata field name
                    if (!vdataobj->getTreatAsAttrFlag ()) {
                        for (std::vector <VDField * >::const_iterator it_vdf =
                            vdataobj->getFields ().begin ();
                            it_vdf != vdataobj->getFields ().end ();
                            it_vdf++) {

                            // Change vdata name conventions. 
                            // "vdata"+vdata_newname+"_vdf_"+(*it_vdf)->newname
                            (*it_vdf)->newname =
                                                "vdata" + vdataobj->newname + "_vdf_" +
                                                (*it_vdf)->name;

                            (*it_vdf)->newname = HDFCFUtil::get_CF_string((*it_vdf)->newname);
                        }
                    }
                                       
                    vdataobj->newname = HDFCFUtil::get_CF_string(vdataobj->newname);
                    this->vds.push_back (vdataobj);
                }
            }
            status = VSdetach (vdata_id);
            if (status == FAIL) {
                free(cfull_path);
                throw3 ("VSdetach failed ", "object index is ", i);
            }
        }
        else if (obj_tag == DFTAG_NDG || obj_tag == DFTAG_SDG
                 || obj_tag == DFTAG_SD) {
            if (this->sd->refindexlist.find (obj_ref) !=
                this->sd->refindexlist.end ())
                this->sd->sdfields[this->sd->refindexlist[obj_ref]]->newname =
                // New name conventions require the path to be prefixed before the object name
                    cfull_path + this->sd->sdfields[this->sd->refindexlist[obj_ref]]->name;
            else {
                Vdetach (vgroup_pid);
                free (cfull_path);
                throw3 ("SDS with the reference number ", obj_ref,
                        " is not found");;
            }
        }
        else;
    }
    status = Vdetach (vgroup_pid);
    if (status == FAIL) {
        free (cfull_path);
        throw3 ("Vdetach failed ", "vgroup name is ", cvgroup_name);
    }
    free (cfull_path);

}

// This fuction is called recursively to obtain the full path of an HDF4 SDS path for extra SDS objects
// in a hybrid HDF-EOS2 file.
// obtain_path, obtain_noneos2_sds_path,obtain_vdata_path are very similar.
// We may combine them in the future. Documented at HFRHANDLER-166.
// Also we only add minimum comments since this code may be removed in the future.
void 
SD::obtain_noneos2_sds_path (int32 file_id, char *full_path, int32 pobj_ref)
throw (Exception)
{

    int32 vgroup_cid = 0;
    int32 status = 0;
    int i = 0;
    int num_gobjects = 0;

    // Now HDF4 provides a dynamic way to allocate the length of an HDF4 object name, should update to use that in the future.
    // Documented in a jira ticket HFRHANDLER-168.
    // KY 2013-07-11 
    char cvgroup_name[VGNAMELENMAX*4];

    int32 obj_tag, obj_ref;
    char *cfull_path;

    // MAX_FULL_PATH_LEN(1024) is long enough
    // to cover any HDF4 object path for all NASA HDF4 products.
    // So using strcpy and strcat is safe in a practical sense.
    // However, in the future, we should update the code to  use HDF4 APIs to obtain vgroup_name length dynamically.
    // At that time, we will use strncpy and strncat instead. We may even think to use C++ vector <char>.
    // Documented in a jira ticket HFRHANDLER-168. 
    // KY 2013-07-12
    // We use strncpy and strncat to replace strcpy and strcat. KY 2013-09-06

    cfull_path = (char *) malloc (MAX_FULL_PATH_LEN);
    if (cfull_path == NULL)
        throw1 ("No enough memory to allocate the buffer");

    memset(cfull_path,'\0',MAX_FULL_PATH_LEN);

    vgroup_cid = Vattach (file_id, pobj_ref, "r");
    if (vgroup_cid == FAIL) {
        free (cfull_path);
        throw3 ("Vattach failed ", "Object reference number is ", pobj_ref);
    }

    if (Vgetname (vgroup_cid, cvgroup_name) == FAIL) {
        Vdetach (vgroup_cid);
        free (cfull_path);
        throw3 ("Vgetname failed ", "Object reference number is ", pobj_ref);
    }
    num_gobjects = Vntagrefs (vgroup_cid);
    if (num_gobjects < 0) {
        Vdetach (vgroup_cid);
        free (cfull_path);
        throw3 ("Vntagrefs failed ", "Object reference number is ", pobj_ref);
    }

    // NOTE: The order of cat gets changed.
    strncpy(cfull_path,full_path,strlen(full_path));
    strncat(cfull_path,cvgroup_name,strlen(cvgroup_name));
    strncat(cfull_path,_BACK_SLASH,strlen(_BACK_SLASH));

    for (i = 0; i < num_gobjects; i++) {

        if (Vgettagref (vgroup_cid, i, &obj_tag, &obj_ref) == FAIL) {
            Vdetach (vgroup_cid);
            free (cfull_path);
            throw3 ("Vgettagref failed ", "object index is ", i);
        }

        if (Visvg (vgroup_cid, obj_ref) == TRUE) {
            strncpy (full_path, cfull_path,strlen(cfull_path)+1);
            full_path[strlen(cfull_path)]='\0';
            obtain_noneos2_sds_path (file_id, full_path, obj_ref);
        }
        else if (obj_tag == DFTAG_NDG || obj_tag == DFTAG_SDG
                 || obj_tag == DFTAG_SD) {

            // Here we need to check if the SDS is an EOS object by checking
            // if the the path includes "Data Fields" or "Geolocation Fields".
            // If the object is an EOS object, we will remove it from the list.

            string temp_str = string(cfull_path);
            if((temp_str.find("Data Fields") != std::string::npos)||
               (temp_str.find("Geolocation Fields") != std::string::npos))
                sds_ref_list.remove(obj_ref);
            }
            else;
        }
        status = Vdetach (vgroup_cid);
        if (status == FAIL) {
            free (cfull_path);
	    throw3 ("Vdetach failed ", "vgroup name is ", cvgroup_name);
        }
        free (cfull_path);

}


// This fuction is called recursively to obtain the full path of the HDF4 vgroup.
// This function is especially used when obtaining non-lone vdata objects for a hybrid file.
// obtain_path, obtain_noneos2_sds_path,obtain_vdata_path are very similar.
// We may combine them in the future. Documented at HFRHANDLER-166.

void
File::obtain_vdata_path (int32 file_id,  char *full_path,
				   int32 pobj_ref)
throw (Exception)
{

    int32 vgroup_cid = -1;
    int32 status = -1;
    int i = -1;
    int num_gobjects = -1;

    // Now HDF4 provides  dynamic ways to allocate the length of object names, should update to use that in the future.
    // Documented in a jira ticket HFRHANDLER-168.
    // KY 2013-07-11 
    char cvgroup_name[VGNAMELENMAX*4];
    char vdata_name[VSNAMELENMAX];
    char vdata_class[VSNAMELENMAX];
    int32 vdata_id = -1;
    int32 obj_tag = -1;
    int32 obj_ref = -1;
    char *cfull_path;

    // MAX_FULL_PATH_LEN(1024) is long enough
    // to cover any HDF4 object path for all NASA HDF4 products.
    // So using strcpy and strcat is safe in a practical sense.
    // However, in the future, we should update the code to  use HDF4 APIs to obtain vgroup_name length dynamically.
    // At that time, we will use strncpy and strncat instead. We may even think to use C++ vector <char>.
    // Documented in a jira ticket HFRHANDLER-168. 
    // KY 2013-07-12
    // We replace strcpy and strcat with strncpy and strncat as suggested. KY 2013-08-29

    cfull_path = (char *) malloc (MAX_FULL_PATH_LEN);
    if (cfull_path == NULL)
        throw1 ("No enough memory to allocate the buffer");
    memset(cfull_path,'\0',MAX_FULL_PATH_LEN);

    vgroup_cid = Vattach (file_id, pobj_ref, "r");
    if (vgroup_cid == FAIL) {
        free (cfull_path);
        throw3 ("Vattach failed ", "Object reference number is ", pobj_ref);
    }

    if (Vgetname (vgroup_cid, cvgroup_name) == FAIL) {
        Vdetach (vgroup_cid);
        free (cfull_path);
        throw3 ("Vgetname failed ", "Object reference number is ", pobj_ref);
    }
    num_gobjects = Vntagrefs (vgroup_cid);
    if (num_gobjects < 0) {
        Vdetach (vgroup_cid);
        free (cfull_path);
        throw3 ("Vntagrefs failed ", "Object reference number is ", pobj_ref);
    }

    strncpy(cfull_path,full_path,strlen(full_path));
    strncat(cfull_path,cvgroup_name,strlen(cvgroup_name));
    strncat(cfull_path,_BACK_SLASH,strlen(_BACK_SLASH));


    // If having a vgroup "Geolocation Fields", we would like to set the EOS2Swath flag.
    std::string temp_str = std::string(cfull_path);

    if (temp_str.find("Geolocation Fields") != string::npos) {
            if(false == this->EOS2Swathflag) 
                this->EOS2Swathflag = true;
    }
 
    for (i = 0; i < num_gobjects; i++) {

        if (Vgettagref (vgroup_cid, i, &obj_tag, &obj_ref) == FAIL) {
            Vdetach (vgroup_cid);
            free (cfull_path);
            throw3 ("Vgettagref failed ", "object index is ", i);
        }

        if (Visvg (vgroup_cid, obj_ref) == TRUE) {
            strncpy(full_path,cfull_path,strlen(cfull_path)+1);
            full_path[strlen(cfull_path)] = '\0';
            obtain_vdata_path (file_id, full_path, obj_ref);
        }
        else if (Visvs (vgroup_cid, obj_ref)) {

            vdata_id = VSattach (file_id, obj_ref, "r");
            if (vdata_id == FAIL) {
                Vdetach (vgroup_cid);
                free (cfull_path);
                throw3 ("VSattach failed ", "object index is ", i);
            }

            status = VSQueryname (vdata_id, vdata_name);
            if (status == FAIL) {
                Vdetach (vgroup_cid);
                free (cfull_path);
                throw3 ("VSgetclass failed ", "object index is ", i);
            }

            status = VSgetclass (vdata_id, vdata_class);
            if (status == FAIL) {
                Vdetach (vgroup_cid);
                free (cfull_path);
                throw3 ("VSgetclass failed ", "object index is ", i);
            }

            // Obtain the C++ string format of the path.
            string temp_str = string(cfull_path);

            // Swath 1-D is mapped to Vdata, we need to ignore them.
            // But if vdata is added to a grid, we should not ignore.
            // Since "Geolocation Fields" will always appear before 
            // the "Data Fields", we can use a flag to distinguish 
            // the swath from the grid. Swath includes both "Geolocation Fields"
            // and "Data Fields". Grid only has "Data Fields".
            // KY 2013-01-03

            bool ignore_eos2_geo_vdata = false;
            bool ignore_eos2_data_vdata = false;
            if (temp_str.find("Geolocation Fields") != string::npos) {
                ignore_eos2_geo_vdata = true;
            }

            // Only ignore "Data Fields" vdata when "Geolocation Fields" appears.
            if (temp_str.find("Data Fields") != string::npos) {
                if (true == this->EOS2Swathflag)  
                    ignore_eos2_data_vdata = true;
            }
            if ((true == ignore_eos2_data_vdata)  
                ||(true == ignore_eos2_geo_vdata)
                || VSisattr (vdata_id) == TRUE
                || !strncmp (vdata_class, _HDF_CHK_TBL_CLASS,
                            strlen (_HDF_CHK_TBL_CLASS))
                || !strncmp (vdata_class, _HDF_SDSVAR,
                            strlen (_HDF_SDSVAR))
                || !strncmp (vdata_class, _HDF_CRDVAR,
                            strlen (_HDF_CRDVAR))
                || !strncmp (vdata_class, DIM_VALS, strlen (DIM_VALS))
                || !strncmp (vdata_class, DIM_VALS01,
                            strlen (DIM_VALS01))
                || !strncmp (vdata_class, RIGATTRCLASS,
                            strlen (RIGATTRCLASS))
                || !strncmp (vdata_name, RIGATTRNAME,
                            strlen (RIGATTRNAME))) {

                status = VSdetach (vdata_id);
                if (status == FAIL) {
                    Vdetach (vgroup_cid);
                    free (cfull_path);
                    throw3 ("VSdetach failed ",
                            "Vdata is under vgroup ", cvgroup_name);
                }
            }
            else {
 
                VDATA *vdataobj = NULL;
                try {
                    vdataobj = VDATA::Read (vdata_id, obj_ref);
                }
                catch(...) {
                    free (cfull_path);
                    VSdetach(vdata_id);
                    Vdetach (vgroup_cid);
                    throw;
                }

                // The new name conventions require the path prefixed before the object name.
                vdataobj->newname = cfull_path + vdataobj->name;
                // We want to map fields of vdata with more than 10 records to DAP variables
                // and we need to add the path and vdata name to the new vdata field name
                if (!vdataobj->getTreatAsAttrFlag ()) {
                    for (std::vector <VDField * >::const_iterator it_vdf =
                        vdataobj->getFields ().begin ();
                        it_vdf != vdataobj->getFields ().end ();
                        it_vdf++) {

                        // Change vdata name conventions. 
                        // "vdata"+vdata_newname+"_vdf_"+(*it_vdf)->newname
                        (*it_vdf)->newname =
                                            "vdata" + vdataobj->newname + "_vdf_" +
                                            (*it_vdf)->name;

                        (*it_vdf)->newname = HDFCFUtil::get_CF_string((*it_vdf)->newname);
                    }
                }
                                       
                vdataobj->newname = HDFCFUtil::get_CF_string(vdataobj->newname);
                this->vds.push_back (vdataobj);
                status = VSdetach (vdata_id);
                if (status == FAIL) {
                    free(cfull_path);
                    Vdetach(vgroup_cid);
                    throw3 ("VSdetach failed ", "object index is ", i);
                }
            }
        }
        else;
    }
    status = Vdetach (vgroup_cid);
    if (status == FAIL) {
        free (cfull_path);
        throw3 ("Vdetach failed ", "vgroup name is ", cvgroup_name);
    }
    free (cfull_path);

}

// Handle SDS fakedim names: make the dimensions with the same dimension size 
// share the same dimension name. In this way, we can reduce many fakedims.
void
File::handle_sds_fakedim_names() throw(Exception) {

    File *file = this;

    // Build Dimension name list
    // We have to assume that NASA HDF4 SDSs provide unique dimension names under each vgroup
    // Find unique dimension name list
    // Build a map from unique dimension name list to the original dimension name list
    // Don't count fakeDim ......
    // Based on the new dimension name list, we will build a coordinate field for each dimension 
	// for each product we support. If dimension scale data are found, that dimension scale data will
    // be retrieved according to our knowledge to the data product. 
    // The unique dimension name is the dimension name plus the full path
    // We should build a map to obtain the final coordinate fields of each field

    std::string tempdimname;
    std::pair < std::set < std::string >::iterator, bool > ret;
    std::string temppath;
    std::set < int32 > fakedimsizeset;
    std::pair < std::set < int32 >::iterator, bool > fakedimsizeit;
    std::map < int32, std::string > fakedimsizenamelist;
    std::map < int32, std::string >::iterator fakedimsizenamelistit;

    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

        for (std::vector < Dimension * >::const_iterator j =
            (*i)->getDimensions ().begin ();
            j != (*i)->getDimensions ().end (); ++j) {

            //May treat corrected dimension names as the original dimension names the SAME, CORRECT it in the future.
            if (file->sptype != OTHERHDF)
                tempdimname = (*j)->getName ();
            else
                tempdimname = (*j)->getName () + temppath;

            Dimension *dim =
                new Dimension (tempdimname, (*j)->getSize (),
                               (*j)->getType ());
            (*i)->correcteddims.push_back (dim);
            if (tempdimname.find ("fakeDim") != std::string::npos) {
                fakedimsizeit = fakedimsizeset.insert ((*j)->getSize ());
                if (fakedimsizeit.second == true) {
                    fakedimsizenamelist[(*j)->getSize ()] = (*j)->getName ();	//Here we just need the original name since fakeDim is globally generated.
                }
            }
        }
    }

    // The CF conventions have to be followed for products(TRMM etc.) that use fakeDims . KY 2012-6-26
    // Sequeeze "fakeDim" names according to fakeDim size. For example, if fakeDim1, fakeDim3, fakeDim5 all shares the same size,
    // we use one name(fakeDim1) to be the dimension name. This will reduce the number of fakeDim names. 
        
    if (file->sptype != OTHERHDF) {
        for (std::vector < SDField * >::const_iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
            for (std::vector < Dimension * >::const_iterator j =
                (*i)->getCorrectedDimensions ().begin ();
                j != (*i)->getCorrectedDimensions ().end (); ++j) {
                if ((*j)->getName ().find ("fakeDim") != std::string::npos) {
                    if (fakedimsizenamelist.find ((*j)->getSize ()) !=
                        fakedimsizenamelist.end ()) {
                        (*j)->name = fakedimsizenamelist[(*j)->getSize ()];	//sequeeze the redundant fakeDim with the same size
                    }
                    else
                        throw5 ("The fakeDim name ", (*j)->getName (),
                                "with the size", (*j)->getSize (),
                                "does not in the fakedimsize list");
                }
            }
        }
    }
}

// Create the new dimension name set and the dimension name to size map.
void File::create_sds_dim_name_list() {

    File *file = this;

    // Create the new dimension name set and the dimension name to size map.
    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
        for (std::vector < Dimension * >::const_iterator j =
            (*i)->getCorrectedDimensions ().begin ();
            j != (*i)->getCorrectedDimensions ().end (); ++j) {
            std::pair < std::set < std::string >::iterator, bool > ret;
            ret = file->sd->fulldimnamelist.insert ((*j)->getName ());

            // Map from the unique dimension name to its size
            if (ret.second == true) {
                file->sd->n1dimnamelist[(*j)->getName ()] = (*j)->getSize ();
            }
        }
    }

}

// Add the missing coordinate variables based on the corrected dimension name list
void File::handle_sds_missing_fields() {

    File *file = this;

    // Adding the missing coordinate variables based on the corrected dimension name list
    // For some CERES products, there are so many vgroups, so there are potentially many missing fields.
    // Go through the n1dimnamelist and check the map dimcvarlist; if no dimcvarlist[dimname], then this dimension namelist must be a missing field
    // Create the missing field and insert the missing field to the SDField list. 

    for (std::map < std::string, int32 >::const_iterator i =
         file->sd->n1dimnamelist.begin ();
         i != file->sd->n1dimnamelist.end (); ++i) {

        if (file->sd->nonmisscvdimnamelist.find ((*i).first) == file->sd->nonmisscvdimnamelist.end ()) {// Create a missing Z-dimension field  

            SDField *missingfield = new SDField ();

            // The name of the missingfield is not necessary.
            // We only keep here for consistency.

            missingfield->type = DFNT_INT32;
            missingfield->name = (*i).first;
            missingfield->newname = (*i).first;
            missingfield->rank = 1;
            missingfield->fieldtype = 4;
            Dimension *dim = new Dimension ((*i).first, (*i).second, 0);

            missingfield->dims.push_back (dim);
            dim = new Dimension ((*i).first, (*i).second, 0);
            missingfield->correcteddims.push_back (dim);
            file->sd->sdfields.push_back (missingfield);
        }
    }
}

// Create the final CF-compliant dimension name list for each field
void File::handle_sds_final_dim_names() throw(Exception) {

    File * file = this;

    /// Handle dimension name clashings
    // We will create the final unique dimension name list(erasing special characters etc.) 
    // After erasing special characters, the nameclashing for dimension name is still possible. 
    // So still handle the name clashings.

    vector<string>tempfulldimnamelist;
    for (std::set < std::string >::const_iterator i =
        file->sd->fulldimnamelist.begin ();
        i != file->sd->fulldimnamelist.end (); ++i) 
        tempfulldimnamelist.push_back(HDFCFUtil::get_CF_string(*i));

    HDFCFUtil::Handle_NameClashing(tempfulldimnamelist);

    // Not the most efficient way, but to keep the original code structure,KY 2012-6-27
    int total_dcounter = 0;
    for (std::set < std::string >::const_iterator i =
        file->sd->fulldimnamelist.begin ();
        i != file->sd->fulldimnamelist.end (); ++i) {
        HDFCFUtil::insert_map(file->sd->n2dimnamelist, (*i), tempfulldimnamelist[total_dcounter]);
        total_dcounter++;
    }

    // change the corrected dimension name list for each SDS field
    std::map < std::string, std::string >::iterator tempmapit;
    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
        for (std::vector < Dimension * >::const_iterator j =
            (*i)->getCorrectedDimensions ().begin ();
            j != (*i)->getCorrectedDimensions ().end (); ++j) {
            tempmapit = file->sd->n2dimnamelist.find ((*j)->getName ());
            if (tempmapit != file->sd->n2dimnamelist.end ())
                (*j)->name = tempmapit->second;
            else {	//When the dimension name is fakeDim***, we will ignore. this dimension will not have the corresponding coordinate variable. 
                throw5 ("This dimension with the name ", (*j)->name,
                        "and the field name ", (*i)->name,
                        " is not found in the dimension list.");
            }
        }
    }

}

// Create the final CF-compliant field name list
void 
File::handle_sds_names(bool & COARDFLAG, string & lldimname1, string&lldimname2) throw(Exception) 
{

    File * file = this;

    // Handle name clashings

    // There are many fields in CERES data(a few hundred) and the full name(with the additional path)
    // is very long. It causes Java clients choken since Java clients append names in the URL
    // To improve the performance and to make Java clients access the data, simply use the field names for
    // these fields. Users can turn off this feature by commenting out the line: H4.EnableCERESMERRAShortName=true
    // or set the H4.EnableCERESMERRAShortName=false
    // KY 2012-6-27

    string check_ceres_short_name_key="H4.EnableCERESMERRAShortName";
    bool turn_on_ceres_short_name_key= false;
        
    turn_on_ceres_short_name_key = HDFCFUtil::check_beskeys(check_ceres_short_name_key);

    if (true == turn_on_ceres_short_name_key && (file->sptype == CER_ES4 || file->sptype == CER_SRB
        || file->sptype == CER_CDAY || file->sptype == CER_CGEO
        || file->sptype == CER_SYN || file->sptype == CER_ZAVG
        || file->sptype == CER_AVG)) {
        
        for (unsigned int i = 0; i < file->sd->sdfields.size (); ++i) {
            file->sd->sdfields[i]->special_product_fullpath = file->sd->sdfields[i]->newname;
            file->sd->sdfields[i]->newname = file->sd->sdfields[i]->name;
        }    
    }

        
    vector<string>sd_data_fieldnamelist;
    vector<string>sd_latlon_fieldnamelist;
    vector<string>sd_nollcv_fieldnamelist;

    set<string>sd_fieldnamelist;

    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
        if ((*i)->fieldtype ==0)  
            sd_data_fieldnamelist.push_back(HDFCFUtil::get_CF_string((*i)->newname));
        else if ((*i)->fieldtype == 1 || (*i)->fieldtype == 2)
            sd_latlon_fieldnamelist.push_back(HDFCFUtil::get_CF_string((*i)->newname));
        else 
            sd_nollcv_fieldnamelist.push_back(HDFCFUtil::get_CF_string((*i)->newname));
    }

    HDFCFUtil::Handle_NameClashing(sd_data_fieldnamelist,sd_fieldnamelist);
    HDFCFUtil::Handle_NameClashing(sd_latlon_fieldnamelist,sd_fieldnamelist);
    HDFCFUtil::Handle_NameClashing(sd_nollcv_fieldnamelist,sd_fieldnamelist);

    // Check the special characters and change those characters to _ for field namelist
    // Also create dimension name to coordinate variable name list

    int total_data_counter = 0;
    int total_latlon_counter = 0;
    int total_nollcv_counter = 0;

    //bool COARDFLAG = false;
    //string lldimname1;
    //string lldimname2;

    // change the corrected dimension name list for each SDS field
    std::map < std::string, std::string >::iterator tempmapit;


    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

        // Handle dimension name to coordinate variable map 
        // Currently there is a backward compatibility  issue in the CF conventions,
        // If a field temp[ydim = 10][xdim =5][zdim=2], the
        // coordinate variables are lat[ydim=10][xdim=5],
        // lon[ydim =10][xdim=5], zdim[zdim =2]. Panoply and IDV will 
        // not display these properly because they think the field is
        // following COARD conventions based on zdim[zdim =2].
        // To make the IDV and Panoply work, we have to change zdim[zdim=2]
        // to something like zdim_v[zdim=2] to distinguish the dimension name
        // from the variable name. 
        // KY 2010-7-21
        // set a flag

        if ((*i)->fieldtype != 0) {
            if ((*i)->fieldtype == 1 || (*i)->fieldtype == 2) {

                (*i)->newname = sd_latlon_fieldnamelist[total_latlon_counter];
                total_latlon_counter++;

                if ((*i)->getRank () > 2)
                    throw3 ("the lat/lon rank should NOT be greater than 2",
                            (*i)->name, (*i)->getRank ());
                else if ((*i)->getRank () == 2) {// Each lat/lon must be 2-D under the same group.
                    for (std::vector < Dimension * >::const_iterator j =
                        (*i)->getCorrectedDimensions ().begin ();
                        j != (*i)->getCorrectedDimensions ().end (); ++j) {
                        tempmapit =
                            file->sd->dimcvarlist.find ((*j)->getName ());
                        if (tempmapit == file->sd->dimcvarlist.end ()) {
                            HDFCFUtil::insert_map(file->sd->dimcvarlist, (*j)->name, (*i)->newname);

                            // Save this dim. to lldims
                            if (lldimname1 =="") 
                                lldimname1 =(*j)->name;
                            else 
                                lldimname2 = (*j)->name;
                            break;
                        }
                    }
                }

                else {	
                    // When rank = 1, must follow COARD conventions. 
                    // Here we don't check name clashing for the performance
                    // reason, the chance of clashing is very,very rare.
                    (*i)->newname =
                        (*i)->getCorrectedDimensions ()[0]->getName ();
                    HDFCFUtil::insert_map(file->sd->dimcvarlist, (*i)->getCorrectedDimensions()[0]->getName(), (*i)->newname);
                    COARDFLAG = true;

                }
            }
        }
        else {
            (*i)->newname = sd_data_fieldnamelist[total_data_counter];
            total_data_counter++;
        }
    }


    for (std::vector < SDField * >::const_iterator i =
         file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {


        // Handle dimension name to coordinate variable map 
        // Currently there is a backward compatibility  issue in the CF conventions,
        // If a field temp[ydim = 10][xdim =5][zdim=2], the
        // coordinate variables are lat[ydim=10][xdim=5],
        // lon[ydim =10][xdim=5], zdim[zdim =2]. Panoply and IDV will 
        // not display these properly because they think the field is
        // following COARD conventions based on zdim[zdim =2].
        // To make the IDV and Panoply work, we have to change zdim[zdim=2]
        // to something like zdim_v[zdim=2] to distinguish the dimension name
        // from the variable name. 
        // KY 2010-7-21
        // set a flag

        if ((*i)->fieldtype != 0) {
            if ((*i)->fieldtype != 1 && (*i)->fieldtype != 2) {	
            // "Missing" coordinate variables or coordinate variables having dimensional scale data

                (*i)->newname = sd_nollcv_fieldnamelist[total_nollcv_counter];
                total_nollcv_counter++;

                if ((*i)->getRank () > 1)
                    throw3 ("The lat/lon rank should be 1", (*i)->name,
                            (*i)->getRank ());

                // The current OTHERHDF case we support(MERRA and SDS dimension scale)
                // follow COARDS conventions. Panoply fail to display the data,
                // if we just follow CF conventions. So following COARD. KY-2011-3-4
                if (COARDFLAG || file->sptype == OTHERHDF)//  Follow COARD Conventions
                    (*i)->newname =
                        (*i)->getCorrectedDimensions ()[0]->getName ();
	        else 
                // It seems that netCDF Java stricts following COARDS conventions, so change the dimension name back. KY 2012-5-4
                    (*i)->newname =
                                   (*i)->getCorrectedDimensions ()[0]->getName ();
//				(*i)->newname =
//				(*i)->getCorrectedDimensions ()[0]->getName () + "_d";
                HDFCFUtil::insert_map(file->sd->dimcvarlist, (*i)->getCorrectedDimensions()[0]->getName(), (*i)->newname);

            }
        }
    }
}

// Create "coordinates", "units" CF attributes
void
File::handle_sds_coords(bool & COARDFLAG,std::string & lldimname1, std::string & lldimname2) throw(Exception) {

    File *file = this;

    // 9. Generate "coordinates " attribute

    std::map < std::string, std::string >::iterator tempmapit;
    int tempcount;

    std::string tempcoordinates, tempfieldname;
    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
        if ((*i)->fieldtype == 0) {
            tempcount = 0;
            tempcoordinates = "";
            tempfieldname = "";

            for (std::vector < Dimension * >::const_iterator j =
                (*i)->getCorrectedDimensions ().begin ();
                j != (*i)->getCorrectedDimensions ().end (); ++j) {
                tempmapit = (file->sd->dimcvarlist).find ((*j)->getName ());
                if (tempmapit != (file->sd->dimcvarlist).end ())
                    tempfieldname = tempmapit->second;
                else
                    throw3 ("The dimension with the name ", (*j)->getName (),
                            "must have corresponding coordinate variables.");
                if (tempcount == 0)
                    tempcoordinates = tempfieldname;
                else
                    tempcoordinates = tempcoordinates + " " + tempfieldname;
                tempcount++;
            }
            (*i)->setCoordinates (tempcoordinates);
        }

        // Add units for latitude and longitude
        if ((*i)->fieldtype == 1) {	// latitude,adding the "units" attribute  degrees_east.
            std::string tempunits = "degrees_north";
            (*i)->setUnits (tempunits);
        }

        if ((*i)->fieldtype == 2) {	// longitude, adding the units of
            std::string tempunits = "degrees_east";
            (*i)->setUnits (tempunits);
        }

        // Add units for Z-dimension, now it is always "level"
        if (((*i)->fieldtype == 3) || ((*i)->fieldtype == 4)) {
            std::string tempunits = "level";
            (*i)->setUnits (tempunits);
        }
    }

    // Remove some coordinates attribute for some variables. This happens when a field just share one dimension name with 
    // latitude/longitude that have 2 dimensions. For example, temp[latlondim1][otherdim] with lat[latlondim1][otherdim]; the
    // "coordinates" attribute may become "lat ???", which is not correct. Remove the coordinates for this case.

    if (false == COARDFLAG) {
        for (std::vector < SDField * >::const_iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
            if ((*i)->fieldtype == 0) {
                bool has_lldim1 = false;
                bool has_lldim2 = false;
                for (std::vector < Dimension * >::const_iterator j =
                    (*i)->getCorrectedDimensions ().begin ();
                    j != (*i)->getCorrectedDimensions ().end (); ++j) {
                    if(lldimname1 == (*j)->name)
                        has_lldim1 = true;
                    else if(lldimname2 == (*j)->name)
                        has_lldim2 = true;
                }

                // Currently we don't remove the "coordinates" attribute if no lat/lon dimension names are used.
                if (has_lldim1^has_lldim2)
                    (*i)->coordinates = "";
            }
        }
    }
}

    
// Handle Vdata
void 
File::handle_vdata() throw(Exception) {

    // Define File 
    File *file = this;

    // Handle vdata, only need to check name clashings and special characters for vdata field names 
    // 
    // Check name clashings, the chance for the nameclashing between SDS and Vdata fields are almost 0. Not
    // to add performance burden, I won't consider the nameclashing check between SDS and Vdata fields. KY 2012-6-28
    // 

    string check_disable_vdata_nameclashing_key="H4.DisableVdataNameclashingCheck";
    bool turn_on_disable_vdata_nameclashing_key = false;

    turn_on_disable_vdata_nameclashing_key = HDFCFUtil::check_beskeys(check_disable_vdata_nameclashing_key);

    if (false == turn_on_disable_vdata_nameclashing_key) {

        vector<string> tempvdatafieldnamelist;

	for (std::vector < VDATA * >::const_iterator i = file->vds.begin ();
	    i != file->vds.end (); ++i) {
	    for (std::vector < VDField * >::const_iterator j =
                (*i)->getFields ().begin (); j != (*i)->getFields ().end ();
                ++j) 
                tempvdatafieldnamelist.push_back((*j)->newname);
        }
	
        HDFCFUtil::Handle_NameClashing(tempvdatafieldnamelist);	

        int total_vfd_counter = 0;

        for (std::vector < VDATA * >::const_iterator i = file->vds.begin ();
            i != file->vds.end (); ++i) {
            for (std::vector < VDField * >::const_iterator j =
                (*i)->getFields ().begin (); j != (*i)->getFields ().end ();
                ++j) {
                (*j)->newname = tempvdatafieldnamelist[total_vfd_counter];
                total_vfd_counter++;
            }
        }
    }


}

// This is the main function that make the HDF SDS objects follow the CF convention.
void
File::Prepare() throw(Exception)
{

    File *file = this;

    // 1. Obtain the original SDS and Vdata path,
    // Start with the lone vgroup they belong to and add the path
    // This also add Vdata objects that belong to lone vgroup
    file->InsertOrigFieldPath_ReadVgVdata ();

    // 2. Check the SDS special type(CERES special type has been checked at the Read function)
    file->CheckSDType ();

    // 2.1 Remove AttrContainer from the Dimension list for non-OTHERHDF products
    if (file->sptype != OTHERHDF) {

        for (std::vector < SDField * >::const_iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
            for (vector<AttrContainer *>::iterator j = (*i)->dims_info.begin();
                j!= (*i)->dims_info.end(); ++j) { 
                delete (*j);
                (*i)->dims_info.erase(j);
                j--;
            }
            if ((*i)->dims_info.size() != 0) 
                throw1("Not totally erase the dimension container ");
        }
    }

    // 3. Handle fake dimensions of HDF4 SDS objects. make the dimensions with the same dimension size 
    // share the same dimension name. In this way, we can reduce many fakedims.

    handle_sds_fakedim_names();

    // 4. Prepare the latitude/longitude "coordinate variable" list for each special NASA HDF product
    switch (file->sptype) {
        case TRMML2_V6:
        {
            file->PrepareTRMML2_V6 ();
            break;
        }
	case TRMML3B_V6:
        {
            file->PrepareTRMML3B_V6 ();
            break;
        }
      	case TRMML3A_V6:
        {
            file->PrepareTRMML3A_V6 ();
            break;
        }
        case TRMML3C_V6:
        {
            file->PrepareTRMML3C_V6 ();
            break;
        }
        case TRMML2_V7:
        {
            file->PrepareTRMML2_V7 ();
            break;
        }
	case TRMML3S_V7:
        {
            file->PrepareTRMML3S_V7 ();
            break;
        }
	case TRMML3M_V7:
        {
            file->PrepareTRMML3M_V7 ();
            break;
        }
	case CER_AVG:
        {
            file->PrepareCERAVGSYN ();
            break;
        }
	case CER_ES4:
        {
            file->PrepareCERES4IG ();
            break;
        }
	case CER_CDAY:
        {
            file->PrepareCERSAVGID ();
            break;
        }
        case CER_CGEO:
        {
            file->PrepareCERES4IG ();
            break;
        }
        case CER_SRB:
        {
            file->PrepareCERSAVGID ();
            break;
        }
        case CER_SYN:
        {
            file->PrepareCERAVGSYN ();
            break;
        }
        case CER_ZAVG:
        {
            file->PrepareCERZAVG ();
            break;
        }
        case OBPGL2:
        {
            file->PrepareOBPGL2 ();
            break;
        }
        case OBPGL3:
        {
            file->PrepareOBPGL3 ();
            break;
        }

        case MODISARNSS:
        {
            file->PrepareMODISARNSS ();
            break;
        }

        case OTHERHDF:
        {
            file->PrepareOTHERHDF ();
            break;
        }
        default:
        {
            throw3 ("No such SP datatype ", "sptype is ", sptype);
            break;
        }
    }


    // 5. Create the new dimension name set and the dimension name to size map
    create_sds_dim_name_list();

    // 6. Add the missing coordinate variables based on the corrected dimension name list
    handle_sds_missing_fields();

    // 7. Create the final CF-compliant dimension name list for each field
    handle_sds_final_dim_names();

    bool COARDFLAG = false;
    string lldimname1;
    string lldimname2;

    // 8. Create the final CF-compliant field name list, pass COARDFLAG as a reference 
    //     since COARDS may requires the names to change.
    handle_sds_names(COARDFLAG, lldimname1, lldimname2);

    // 9. Create "coordinates", "units" CF attributes
    handle_sds_coords(COARDFLAG, lldimname1,lldimname2);

    // 10. Handle Vdata
    handle_vdata();
}

void File:: Obtain_TRMML3S_V7_latlon_size(int &latsize, int&lonsize) throw(Exception) {
     
    // No need to check if "GridHeader" etc. exists since this has been checked in the CheckSDType routine.
    for (std::vector < Attribute * >::const_iterator i =
        this->sd->getAttributes ().begin ();
        i != this->sd->getAttributes ().end (); ++i) {
 
        if ((*i)->getName () == "GridHeader") {
            float lat_start = 0.;
            float lon_start = 0.;
            float lat_res = 1.;
            float lon_res = 1.;
            HDFCFUtil::parser_trmm_v7_gridheader((*i)->getValue(),latsize,lonsize,
                                                 lat_start,lon_start,
                                                 lat_res,lon_res,false);
            break;
        }
    }

}

bool File:: Obtain_TRMM_V7_latlon_name(const SDField* sdfield, const int latsize, 
                                       const int lonsize, string& latname, string& lonname) throw(Exception) {

//    bool latflag = false;
//    bool lonflag = false;

    int latname_index = -1;
    int lonname_index = -1;
    for (int temp_index = 0; temp_index <sdfield->getRank(); ++temp_index) {
        if(-1==latname_index && sdfield->getCorrectedDimensions()[temp_index]->getSize() == latsize) { 
            latname_index = temp_index;
//cerr<<"lat name index = "<<latname_index <<endl;
            latname = sdfield->getCorrectedDimensions()[temp_index]->getName();
        }
        else if (-1 == lonname_index && sdfield->getCorrectedDimensions()[temp_index]->getSize() == lonsize) {
            lonname_index = temp_index;
//cerr<<"lon name index = "<<lonname_index <<endl;
            lonname = sdfield->getCorrectedDimensions()[temp_index]->getName();
        }
    }

    return (latname_index + lonname_index == 1);

}
void File::PrepareTRMML2_V7() throw(Exception) {

    File *file = this;

    // 1. Obtain the geolocation field: type,dimension size and dimension name
    // 2. Create latitude and longtiude fields according to the geolocation field.
    std::string tempdimname1, tempdimname2;
    std::string tempnewdimname1, tempnewdimname2;
    std::string temppath;

    //int32 tempdimsize1, tempdimsize2;
    //SDField *longitude;
    //SDField *latitude;

    // Create a temporary map from the dimension size to the dimension name
    std::set < int32 > tempdimsizeset;
    std::map < int32, std::string > tempdimsizenamelist;
    std::map < int32, std::string >::iterator tempsizemapit;
    std::pair < std::set < int32 >::iterator, bool > tempsetit;

    // Reduce the fakeDim list. FakeDim is found to be used by TRMM team.
    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
        for (std::vector < Dimension * >::const_iterator j =
            (*i)->getCorrectedDimensions ().begin ();
            j != (*i)->getCorrectedDimensions ().end (); ++j) {
            if (((*j)->getName ()).find ("fakeDim") == std::string::npos) {	//No fakeDim in the string
                tempsetit = tempdimsizeset.insert ((*j)->getSize ());
                if (tempsetit.second == true)
                    tempdimsizenamelist[(*j)->getSize ()] = (*j)->getName ();
	    }
        }
    }

    // Reduce the fakeDim list. FakeDim is found to be used by TRMM team.
    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

        string temp_name = (*i)->newname.substr(1) ;
        size_t temp_pos = temp_name.find_first_of('/');
        if (temp_pos !=string::npos) 
            (*i)->newname = temp_name.substr(temp_pos+1);
//cerr<<"temp_name is "<<temp_name<<endl;
        
    }


    for (std::vector < SDField * >::const_iterator i =
         file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

        if((*i)->getName() == "Latitude") {
            if((*i)->getRank() ==2) {

                tempnewdimname1 =
                    ((*i)->getCorrectedDimensions ())[0]->getName ();
                tempnewdimname2 =
                    ((*i)->getCorrectedDimensions ())[1]->getName ();
            }

            (*i)->fieldtype = 1;

        }
        else if ((*i)->getName() == "Longitude") {
            (*i)->fieldtype = 2;

        }
        else {

            // Use the temp. map (size to name) to replace the name of "fakeDim???" with the dimension name having the same dimension length
            // This is done only for TRMM. It should be evaluated if this can be applied to other products.
            for (std::vector < Dimension * >::const_iterator k =
                (*i)->getCorrectedDimensions ().begin ();
                k != (*i)->getCorrectedDimensions ().end (); ++k) {
                size_t fakeDimpos = ((*k)->getName ()).find ("fakeDim");

                if (fakeDimpos != std::string::npos) {
                    tempsizemapit =
                        tempdimsizenamelist.find ((*k)->getSize ());
                    if (tempsizemapit != tempdimsizenamelist.end ())
                        (*k)->name = tempdimsizenamelist[(*k)->getSize ()];// Change the dimension name
                }
            }
        
        }
    }

    // Create the <dimname,coordinate variable> map from the corresponding dimension names to the latitude and the longitude
    if(tempnewdimname1.empty()!=true)
        file->sd->nonmisscvdimnamelist.insert (tempnewdimname1);

    if(tempnewdimname2.empty()!=true)
        file->sd->nonmisscvdimnamelist.insert (tempnewdimname2);

    string base_filename;
    size_t last_slash_pos = file->getPath().find_last_of("/");
    if(last_slash_pos != string::npos)
        base_filename = file->getPath().substr(last_slash_pos+1);
    if(""==base_filename)
       base_filename = file->getPath();


    if(base_filename.find("2A12")!=string::npos) {
    SDField *nlayer = NULL;
    string nlayer_name ="nlayer";

    for (vector < SDField * >::iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

         bool has_nlayer = false;

        for (vector < Dimension * >::const_iterator k =
            (*i)->getDimensions ().begin ();
            k != (*i)->getDimensions ().end (); ++k) {

            if((*k)->getSize() == 28 && (*k)->name == nlayer_name) {
                 
                nlayer = new SDField();
                nlayer->name = nlayer_name;
                nlayer->rank = 1;
                nlayer->type = DFNT_FLOAT32;
                nlayer->fieldtype = 6;

                nlayer->newname = nlayer->name ;
                Dimension *dim =
                        new Dimension (nlayer->name, (*k)->getSize (), 0);
                nlayer->dims.push_back(dim);

                dim = new Dimension(nlayer->name,(*k)->getSize(),0);
                nlayer->correcteddims.push_back(dim);

                has_nlayer = true;
                break;
            }

        }

        if(true == has_nlayer)
          break;
    }

    if(nlayer !=NULL) {
        file->sd->sdfields.push_back(nlayer);
        file->sd->nonmisscvdimnamelist.insert (nlayer_name);
    }
 }
 
}

void
File::PrepareTRMML3S_V7() throw(Exception) {

    File *file = this;
    for (std::vector < SDField * >::iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

        //According to GES DISC, the next three variables should be removed from the list.
        if((*i)->name == "InputFileNames") {
            delete (*i);
            file->sd->sdfields.erase(i); 
            i--;
        }
        else if((*i)->name == "InputAlgorithmVersions") {
            delete (*i);
            file->sd->sdfields.erase(i);
            i--;
        }
        else if((*i)->name == "InputGenerationDateTimes") {
            delete (*i);
            file->sd->sdfields.erase(i);
            i--;
        }
        else // Just use SDS names and for performance reasons, change them here.
            (*i)->newname = (*i)->name;
    }

    
    SDField *nlayer = NULL;
    string nlayer_name ="nlayer";

    for (vector < SDField * >::iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

         bool has_nlayer = false;

        for (vector < Dimension * >::const_iterator k =
            (*i)->getDimensions ().begin ();
            k != (*i)->getDimensions ().end (); ++k) {

            if((*k)->getSize() == 28 && (*k)->name == nlayer_name) {
                 
                nlayer = new SDField();
                nlayer->name = nlayer_name;
                nlayer->rank = 1;
                nlayer->type = DFNT_FLOAT32;
                nlayer->fieldtype = 6;

                nlayer->newname = nlayer->name ;
                Dimension *dim =
                        new Dimension (nlayer->name, (*k)->getSize (), 0);
                nlayer->dims.push_back(dim);

                dim = new Dimension(nlayer->name,(*k)->getSize(),0);
                nlayer->correcteddims.push_back(dim);

                has_nlayer = true;
                break;
            }

        }

        if(true == has_nlayer)
          break;
    }

    if(nlayer !=NULL) {
        file->sd->sdfields.push_back(nlayer);
        file->sd->nonmisscvdimnamelist.insert (nlayer_name);
    }
        
    int latsize = 0;
    int lonsize = 0;

    Obtain_TRMML3S_V7_latlon_size(latsize,lonsize);
//cerr<<"latsize "<<latsize <<endl;
//cerr<<"lonsize "<<lonsize <<endl;

    string latname;
    string lonname;

    bool llname_found = false;
    for (std::vector < SDField * >::iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

        if(2 == (*i)->getRank()) {

            llname_found = Obtain_TRMM_V7_latlon_name((*i),latsize,lonsize,latname,lonname);
            if (true == llname_found)
                break;

        }
    }

//cerr<<"latitude name "<<latname <<endl;
//cerr<<"longitude name "<<lonname <<endl;
    // Create lat/lon SD fields.
    SDField* longitude = new SDField ();
    longitude->name = lonname;
    longitude->rank = 1;
    longitude->type = DFNT_FLOAT32;
    longitude->fieldtype = 2;

    longitude->newname = longitude->name;
    Dimension *dim =
                    new Dimension (lonname, lonsize, 0);
    longitude->dims.push_back (dim);

    dim = new Dimension (lonname, lonsize, 0);
    longitude->correcteddims.push_back (dim);
    file->sd->sdfields.push_back(longitude);

    SDField* latitude = new SDField ();
    latitude->name = latname;
    latitude->rank = 1;
    latitude->type = DFNT_FLOAT32;
    latitude->fieldtype = 1;

    latitude->newname = latitude->name;
    dim = new Dimension (latname, latsize, 0);
    latitude->dims.push_back (dim);

    dim = new Dimension (latname, latsize, 0);
    latitude->correcteddims.push_back (dim);
    file->sd->sdfields.push_back(latitude);
     
    // Create the <dimname,coordinate variable> map from the corresponding dimension names to the latitude and the longitude
    file->sd->nonmisscvdimnamelist.insert (latname);
    file->sd->nonmisscvdimnamelist.insert (lonname);


    // Now we want to handle the special CVs for 3A26 products. Since these special CVs only apply to the 3A26 products,
    //  we don't want to find them from other products to reduce the performance cost. So here I simply check the filename
    string base_filename;
    if(path.find_last_of("/") != string::npos) 
        base_filename = path.substr(path.find_last_of("/")+1);
    if(base_filename.find("3A26")!=string::npos) {

        bool ZOflag = false;
        bool SRTflag = false;
        bool HBflag = false;
        string nthrsh_base_name = "nthrsh";
        string nthrsh_zo_name ="nthrshZO";
        string nthrsh_hb_name ="nthrshHB";
        string nthrsh_srt_name ="nthrshSRT";

        SDField* nthrsh_zo = NULL;
        SDField* nthrsh_hb = NULL;
        SDField* nthrsh_srt = NULL;
        
        for (vector < SDField * >::iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

            if(ZOflag != true) {
                if((*i)->name.find("Order")!=string::npos) {
                    for (vector < Dimension * >::const_iterator k =
                         (*i)->getDimensions ().begin ();
                         k != (*i)->getDimensions ().end (); ++k) {

                        if((*k)->getSize() == 6  && (*k)->name == nthrsh_base_name) {
                            nthrsh_zo = new SDField();
                            nthrsh_zo->name = nthrsh_zo_name;
                            nthrsh_zo->rank = 1;
                            nthrsh_zo->type = DFNT_FLOAT32;
                            nthrsh_zo->fieldtype = 6;

                            nthrsh_zo->newname = nthrsh_zo->name ;
                            Dimension *dim =
                                    new Dimension (nthrsh_zo->name, (*k)->getSize (), 0);
                            nthrsh_zo->dims.push_back(dim);

                            dim = new Dimension(nthrsh_zo->name,(*k)->getSize(),0);
                            nthrsh_zo->correcteddims.push_back(dim);

                            ZOflag = true;
 
                        }
                    }
                }
                
            }

            else if(SRTflag != true) {
                if((*i)->name.find("2A25")!=string::npos) {

                    for (vector < Dimension * >::const_iterator k =
                         (*i)->getDimensions ().begin ();
                         k != (*i)->getDimensions ().end (); ++k) {

                        if((*k)->getSize() == 6  && (*k)->name == nthrsh_base_name) {
                            nthrsh_srt = new SDField();
                            nthrsh_srt->name = nthrsh_srt_name;
                            nthrsh_srt->rank = 1;
                            nthrsh_srt->type = DFNT_FLOAT32;
                            nthrsh_srt->fieldtype = 6;

                            nthrsh_srt->newname = nthrsh_srt->name ;
                            Dimension *dim =
                                    new Dimension (nthrsh_srt->name, (*k)->getSize (), 0);
                            nthrsh_srt->dims.push_back(dim);

                            dim = new Dimension(nthrsh_srt->name,(*k)->getSize(),0);
                            nthrsh_srt->correcteddims.push_back(dim);

                            SRTflag = true;
 
                        }
                    }
                }
            }
            else if(HBflag != true) {
                if((*i)->name.find("hb")!=string::npos || (*i)->name.find("HB")!=string::npos) {

                    for (vector < Dimension * >::const_iterator k =
                         (*i)->getDimensions ().begin ();
                         k != (*i)->getDimensions ().end (); ++k) {

                        if((*k)->getSize() == 6  && (*k)->name == nthrsh_base_name) {

                            nthrsh_hb = new SDField();
                            nthrsh_hb->name = nthrsh_hb_name;
                            nthrsh_hb->rank = 1;
                            nthrsh_hb->type = DFNT_FLOAT32;
                            nthrsh_hb->fieldtype = 6;

                            nthrsh_hb->newname = nthrsh_hb->name ;
                            Dimension *dim =
                                    new Dimension (nthrsh_hb->name, (*k)->getSize (), 0);
                            nthrsh_hb->dims.push_back(dim);

                            dim = new Dimension(nthrsh_hb->name,(*k)->getSize(),0);
                            nthrsh_hb->correcteddims.push_back(dim);

                            HBflag = true;
 
                        }
                    }
                }
            }
        }


        for (vector < SDField * >::iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

            if((*i)->name.find("Order")!=string::npos && ZOflag == true) {
                for (vector < Dimension * >::const_iterator k =
                     (*i)->getDimensions ().begin ();
                     k != (*i)->getDimensions ().end (); ++k) {

                    if((*k)->getSize() == 6  && (*k)->name == nthrsh_base_name) {
                       (*k)->name = nthrsh_zo_name;
                       break;
                    }
                }

                for (std::vector < Dimension * >::const_iterator k =
                    (*i)->getCorrectedDimensions ().begin ();
                    k != (*i)->getCorrectedDimensions ().end (); ++k) {
                    if((*k)->getSize() == 6  && (*k)->name == nthrsh_base_name) {
                       (*k)->name = nthrsh_zo_name;
                       break;
                    }
                }
 
            }

            else if(((*i)->name.find("hb")!=string::npos || (*i)->name.find("HB")!=string::npos)&& HBflag == true) {
                for (vector < Dimension * >::const_iterator k =
                     (*i)->getDimensions ().begin ();
                     k != (*i)->getDimensions ().end (); ++k) {

                    if((*k)->getSize() == 6  && (*k)->name == nthrsh_base_name) {
                       (*k)->name = nthrsh_hb_name;
                       break;
                    }
                }

                for (std::vector < Dimension * >::const_iterator k =
                    (*i)->getCorrectedDimensions ().begin ();
                    k != (*i)->getCorrectedDimensions ().end (); ++k) {
                    if((*k)->getSize() == 6  && (*k)->name == nthrsh_base_name) {
                       (*k)->name = nthrsh_hb_name;
                       break;
                    }
                }
 
            }
            else if(((*i)->name.find("2A25")!=string::npos) && SRTflag == true) {
                for (vector < Dimension * >::const_iterator k =
                     (*i)->getDimensions ().begin ();
                     k != (*i)->getDimensions ().end (); ++k) {

                    if((*k)->getSize() == 6  && (*k)->name == nthrsh_base_name) {
                       (*k)->name = nthrsh_srt_name;
                       break;
                    }
                }

                for (std::vector < Dimension * >::const_iterator k =
                    (*i)->getCorrectedDimensions ().begin ();
                    k != (*i)->getCorrectedDimensions ().end (); ++k) {
                    if((*k)->getSize() == 6  && (*k)->name == nthrsh_base_name) {
                       (*k)->name = nthrsh_srt_name;
                       break;
                    }
                }
 
            }


        }

        if(nthrsh_zo !=NULL) {
            file->sd->sdfields.push_back(nthrsh_zo);
            file->sd->nonmisscvdimnamelist.insert (nthrsh_zo_name);
        }

        if(nthrsh_hb !=NULL) {
            file->sd->sdfields.push_back(nthrsh_hb);
            file->sd->nonmisscvdimnamelist.insert (nthrsh_hb_name);
        }

        if(nthrsh_srt !=NULL) {
            file->sd->sdfields.push_back(nthrsh_srt);
            file->sd->nonmisscvdimnamelist.insert (nthrsh_srt_name);
        }

    }
    
}

void
File::PrepareTRMML3M_V7() throw(Exception) {

    File *file = this;
    for (std::vector < SDField * >::iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

        if((*i)->name == "InputFileNames") {
            delete (*i);
            file->sd->sdfields.erase(i); 
            i--;
        }
        else if((*i)->name == "InputAlgorithmVersions") {
            delete (*i);
            file->sd->sdfields.erase(i);
            i--;
        }
        else if((*i)->name == "InputGenerationDateTimes") {
            delete (*i);
            file->sd->sdfields.erase(i);
            i--;
        }
    }

    
#if 0
    NOTE for programming: 
    1. Outer loop: loop global attribute for GridHeader?. Retrieve ? as a number for index.
    1.5. Obtain the lat/lon sizes for this grid.
    The following steps are to retrieve lat/lon names for this grid.
    2. Inner loop: Then loop through the field
    3. Check the field rank, 
        3.1 if the rank is not 2, (if the index is the first index, change the newname to name )
               continue.
        3.2 else {
               3.2.1 Retrieve the index from the field new name(retrieve last path Grid1 then retrieve 1)
               3.2.2 If the index from the field is the same as that from the GridHeader, continue checking 
                     the lat/lon name for this grid as the single grid.
                     change the newname to name.
            }
#endif
    
    // The following code tries to be performance-friendly by looping through the fields and handling the operations
    // as less as I can.

    int first_index = -1;
    for (vector < Attribute * >::const_iterator i =
        this->sd->getAttributes ().begin ();
        i != this->sd->getAttributes ().end (); ++i) {
            
        if ((*i)->getName ().find("GridHeader")==0) {
            string temp_name = (*i)->getName();

            // The size of "GridHeader" is 10, no need to calculate.
            string str_num = temp_name.substr(10);
            stringstream ss_num(str_num);

            int grid_index;
            ss_num >> grid_index; 

            if ( -1 == first_index)
                first_index = grid_index;
            
            float lat_start = 0.;
            float lon_start = 0.;
            float lat_res = 1.;
            float lon_res = 1.;
            int latsize = 0;
            int lonsize = 0;
            
            HDFCFUtil::parser_trmm_v7_gridheader((*i)->getValue(),latsize,lonsize,
                                                 lat_start,lon_start,
                                                 lat_res, lon_res, false);
          
            string latname;
            string lonname;
            
            bool llname_found = false;
            for (std::vector < SDField * >::iterator i =
                 file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
 
                // Just loop the 2-D fields to find the lat/lon size
                if(2 == (*i)->getRank()) {

                    // If a grid has not been visited, we will check the fields attached to this grid.
                    if ((*i)->newname !=(*i)->name) {

                        string temp_field_full_path = (*i)->getNewName();
                        size_t last_path_pos = temp_field_full_path.find_last_of('/');
                        char str_index = temp_field_full_path[last_path_pos-1];
                        if(grid_index ==(int)(str_index - '0')) {
                            if(llname_found != true) 
                                llname_found = Obtain_TRMM_V7_latlon_name((*i),latsize,lonsize,latname,lonname);
                            (*i)->newname = (*i)->name;
                        }
                    }
                }
                else if (first_index == grid_index)
                    (*i)->newname = (*i)->name;
            }   

            // Create lat/lon SD fields.
            SDField* longitude = new SDField ();
            longitude->name = lonname;
            longitude->rank = 1;
            longitude->type = DFNT_FLOAT32;
            longitude->fieldtype = 2;
            longitude->fieldref = grid_index;

            longitude->newname = longitude->name;
            Dimension *dim =
                    new Dimension (lonname, lonsize, 0);
            longitude->dims.push_back (dim);

            dim = new Dimension (lonname, lonsize, 0);
            longitude->correcteddims.push_back (dim);
            file->sd->sdfields.push_back(longitude);

            SDField* latitude = new SDField ();
            latitude->name = latname;
            latitude->rank = 1;
            latitude->type = DFNT_FLOAT32;
            latitude->fieldtype = 1;
            latitude->fieldref = grid_index;

            latitude->newname = latitude->name;
            dim = new Dimension (latname, latsize, 0);
            latitude->dims.push_back (dim);

            dim = new Dimension (latname, latsize, 0);
            latitude->correcteddims.push_back (dim);
            file->sd->sdfields.push_back(latitude);
     
            // Create the <dimname,coordinate variable> map from the corresponding dimension names to the latitude and the longitude
            file->sd->nonmisscvdimnamelist.insert (latname);
            file->sd->nonmisscvdimnamelist.insert (lonname);
            
        }  
    }
}

/// Special method to prepare TRMM Level 2 latitude and longitude information.
/// Latitude and longitude are stored in one array(geolocation). Need to separate.
void
File::PrepareTRMML2_V6 ()
throw (Exception)
{

    File *file = this;

    // 1. Obtain the geolocation field: type,dimension size and dimension name
    // 2. Create latitude and longtiude fields according to the geolocation field.
    std::string tempdimname1, tempdimname2;
    std::string tempnewdimname1, tempnewdimname2;
    std::string temppath;

    int32 tempdimsize1, tempdimsize2;
    SDField *longitude;
    SDField *latitude;

    // Create a temporary map from the dimension size to the dimension name
    std::set < int32 > tempdimsizeset;
    std::map < int32, std::string > tempdimsizenamelist;
    std::map < int32, std::string >::iterator tempsizemapit;
    std::pair < std::set < int32 >::iterator, bool > tempsetit;

    // Reduce the fakeDim list. FakeDim is found to be used by TRMM team.
    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
        for (std::vector < Dimension * >::const_iterator j =
            (*i)->getCorrectedDimensions ().begin ();
            j != (*i)->getCorrectedDimensions ().end (); ++j) {
            if (((*j)->getName ()).find ("fakeDim") == std::string::npos) {	//No fakeDim in the string
                tempsetit = tempdimsizeset.insert ((*j)->getSize ());
                if (tempsetit.second == true)
                    tempdimsizenamelist[(*j)->getSize ()] = (*j)->getName ();
	    }
        }
    }

    for (std::vector < SDField * >::const_iterator i =
         file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

        if ((*i)->getName () == "geolocation") {

            // Obtain the size and the name of the first two dimensions of the geolocation field;
            // make these two dimensions the dimensions of latitude and longtiude.
            tempdimname1 = ((*i)->getDimensions ())[0]->getName ();
            tempdimsize1 = ((*i)->getDimensions ())[0]->getSize ();
            tempdimname2 = ((*i)->getDimensions ())[1]->getName ();
            tempdimsize2 = ((*i)->getDimensions ())[1]->getSize ();

            tempnewdimname1 =
                ((*i)->getCorrectedDimensions ())[0]->getName ();
            tempnewdimname2 =
                ((*i)->getCorrectedDimensions ())[1]->getName ();

            latitude = new SDField ();
            latitude->name = "latitude";
            latitude->rank = 2;
            latitude->fieldref = (*i)->fieldref;
            latitude->type = (*i)->getType ();
            temppath = (*i)->newname.substr ((*i)->name.size ());
            latitude->newname = latitude->name + temppath;
            latitude->fieldtype = 1;
            latitude->rootfieldname = "geolocation";

            Dimension *dim = new Dimension (tempdimname1, tempdimsize1, 0);

            latitude->dims.push_back (dim);

            dim = new Dimension (tempdimname2, tempdimsize2, 0);
            latitude->dims.push_back (dim);

            dim = new Dimension (tempnewdimname1, tempdimsize1, 0);
            latitude->correcteddims.push_back (dim);

            dim = new Dimension (tempnewdimname2, tempdimsize2, 0);
            latitude->correcteddims.push_back (dim);

            longitude = new SDField ();
            longitude->name = "longitude";
            longitude->rank = 2;
            longitude->fieldref = (*i)->fieldref;
            longitude->type = (*i)->getType ();
            longitude->newname = longitude->name + temppath;
            longitude->fieldtype = 2;
            longitude->rootfieldname = "geolocation";

            dim = new Dimension (tempdimname1, tempdimsize1, 0);
            longitude->dims.push_back (dim);
            dim = new Dimension (tempdimname2, tempdimsize2, 0);
            longitude->dims.push_back (dim);

            dim = new Dimension (tempnewdimname1, tempdimsize1, 0);
            longitude->correcteddims.push_back (dim);

            dim = new Dimension (tempnewdimname2, tempdimsize2, 0);
            longitude->correcteddims.push_back (dim);

        }
        else {

            // Use the temp. map (size to name) to replace the name of "fakeDim???" with the dimension name having the same dimension length
            // This is done only for TRMM. It should be evaluated if this can be applied to other products.
            for (std::vector < Dimension * >::const_iterator k =
                (*i)->getCorrectedDimensions ().begin ();
                k != (*i)->getCorrectedDimensions ().end (); ++k) {
                size_t fakeDimpos = ((*k)->getName ()).find ("fakeDim");

                if (fakeDimpos != std::string::npos) {
                    tempsizemapit =
                        tempdimsizenamelist.find ((*k)->getSize ());
                    if (tempsizemapit != tempdimsizenamelist.end ())
                        (*k)->name = tempdimsizenamelist[(*k)->getSize ()];// Change the dimension name
                }
            }
        }
    }

    file->sd->sdfields.push_back (latitude);
    file->sd->sdfields.push_back (longitude);

    // 3. Remove the geolocation field from the field list
    SDField *origeo = NULL;

    std::vector < SDField * >::iterator toeraseit;
    for (std::vector < SDField * >::iterator i = file->sd->sdfields.begin ();
         i != file->sd->sdfields.end (); ++i) {
        if ((*i)->getName () == "geolocation") {	// Check the release of dimension and other resources
            toeraseit = i;
            origeo = *i;
            break;
        }
    }

    file->sd->sdfields.erase (toeraseit);
    if (origeo != NULL)
        delete (origeo);

    // 4. Create the <dimname,coordinate variable> map from the corresponding dimension names to the latitude and the longitude
    file->sd->nonmisscvdimnamelist.insert (tempnewdimname1);
    file->sd->nonmisscvdimnamelist.insert (tempnewdimname2);

}

// Prepare TRMM Level 3, no lat/lon are in the original HDF4 file. Need to provide them.
void
File::PrepareTRMML3B_V6 ()
throw (Exception)
{

    std::string tempnewdimname1, tempnewdimname2;
    int latflag = 0;
    int lonflag = 0;

    std::string temppath;
    SDField *latitude = NULL;
    SDField *longitude = NULL;
    File *file = this;

    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

        for (std::vector < Dimension * >::const_iterator k =
            (*i)->getDimensions ().begin ();
            k != (*i)->getDimensions ().end (); ++k) {

            // This dimension has the dimension name
            if ((((*k)->getName ()).find ("fakeDim")) == std::string::npos) {

                temppath = (*i)->newname.substr ((*i)->name.size ());
                // The lat/lon formula is from GES DISC web site. http://disc.sci.gsfc.nasa.gov/additional/faq/precipitation_faq.shtml#lat_lon
                // KY 2010-7-13
                if ((*k)->getSize () == 1440 && (*k)->getType () == 0) {//No dimension scale

                    longitude = new SDField ();
                    longitude->name = "longitude";
                    longitude->rank = 1;
                    longitude->type = DFNT_FLOAT32;
                    longitude->fieldtype = 2;

                    longitude->newname = longitude->name + temppath;
                    Dimension *dim =
                        new Dimension ((*k)->getName (), (*k)->getSize (), 0);
                    longitude->dims.push_back (dim);
                    tempnewdimname2 = (*k)->getName ();

                    dim =
                        new Dimension ((*k)->getName (), (*k)->getSize (), 0);
                    longitude->correcteddims.push_back (dim);
                    lonflag++;
                }

                if ((*k)->getSize () == 400 && (*k)->getType () == 0) {

                    latitude = new SDField ();
                    latitude->name = "latitude";
                    latitude->rank = 1;
                    latitude->type = DFNT_FLOAT32;
                    latitude->fieldtype = 1;
                    latitude->newname = latitude->name + temppath;
                    Dimension *dim =
                        new Dimension ((*k)->getName (), (*k)->getSize (), 0);
                    latitude->dims.push_back (dim);
                    tempnewdimname1 = (*k)->getName ();

                    // We donot need to generate the  unique dimension name based on the full path for all the current  cases we support.
                    // Leave here just as a reference.
                    // std::string uniquedimname = (*k)->getName() +temppath;
                    //  tempnewdimname1 = uniquedimname;
                    //   dim = new Dimension(uniquedimname,(*k)->getSize(),(*i)->getType());
                    dim =
                         new Dimension ((*k)->getName (), (*k)->getSize (), 0);
                    latitude->correcteddims.push_back (dim);
                    latflag++;
                }
            }

            if (latflag == 1 && lonflag == 1)
                break;
	} 

        if (latflag == 1 && lonflag == 1)
            break;	// For this case, a field that needs lon and lot must exist

        // Need to reset the flag to avoid false alarm. For TRMM L3 we can handle, a field that has dimension 
        // which size is 400 and 1440 must exist in the file.
        latflag = 0;
        lonflag = 0;
    }

    if (latflag != 1 || lonflag != 1) {
        if(latitude != NULL)
            delete latitude;
        if(longitude != NULL)
            delete longitude;
        throw5 ("Either latitude or longitude doesn't exist.", "lat. flag= ",
                 latflag, "lon. flag= ", lonflag);
    }
    file->sd->sdfields.push_back (latitude);
    file->sd->sdfields.push_back (longitude);


    // Create the <dimname,coordinate variable> map from the corresponding dimension names to the latitude and the longitude
    file->sd->nonmisscvdimnamelist.insert (tempnewdimname1);
    file->sd->nonmisscvdimnamelist.insert (tempnewdimname2);

}

// Prepare TRMM Level 3, no lat/lon are in the original HDF4 file. Need to provide them.
void
File::PrepareTRMML3A_V6 ()
throw (Exception)
{
    std::string tempnewdimname1, tempnewdimname2;
    bool latflag = false;
    bool lonflag = false;

    SDField *latitude = NULL;
    SDField *longitude = NULL;
    File *file = this;

    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

        for (std::vector < Dimension * >::const_iterator k =
            (*i)->getDimensions ().begin ();
            k != (*i)->getDimensions ().end (); ++k) {
            if ((((*k)->getName ()).find ("latitude")) == 0) 
               (*k)->name = "fakeDim1";
            if ((((*k)->getName()).find ("longitude")) == 0)
               (*k)->name = "fakeDim2";

        }
        
        // Since we use correctedims, we also need to change them. We may
        // need to remove correctedims from HDFSP space in the future.
          for (std::vector < Dimension * >::const_iterator k =
            (*i)->getCorrectedDimensions ().begin ();
            k != (*i)->getCorrectedDimensions ().end (); ++k) {
            if ((((*k)->getName ()).find ("latitude")) == 0) 
               (*k)->name = "fakeDim1";
            if ((((*k)->getName()).find ("longitude")) == 0)
               (*k)->name = "fakeDim2";

        }
   }

    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {


        for (std::vector < Dimension * >::const_iterator k =
            (*i)->getDimensions ().begin ();
            k != (*i)->getDimensions ().end (); ++k) {


            // This dimension has the dimension name
            //if ((((*k)->getName ()).find ("fakeDim")) == std::string::npos) 

                // The lat/lon formula is from GES DISC web site. http://disc.sci.gsfc.nasa.gov/additional/faq/precipitation_faq.shtml#lat_lon
                // KY 2010-7-13
                if ((*k)->getSize () == 360 && (*k)->getType () == 0) {//No dimension scale

                    longitude = new SDField ();
                    longitude->name = "longitude";
                    longitude->rank = 1;
                    longitude->type = DFNT_FLOAT32;
                    longitude->fieldtype = 2;

                    longitude->newname = longitude->name ;
                    Dimension *dim =
                        new Dimension (longitude->getName (), (*k)->getSize (), 0);
                    longitude->dims.push_back (dim);
                    tempnewdimname2 = longitude->name; 

                    dim =
                        new Dimension (longitude->getName (), (*k)->getSize (), 0);
                    longitude->correcteddims.push_back (dim);
                    lonflag = true;
                }

                if ((*k)->getSize () == 180 && (*k)->getType () == 0) {

                    latitude = new SDField ();
                    latitude->name = "latitude";
                    latitude->rank = 1;
                    latitude->type = DFNT_FLOAT32;
                    latitude->fieldtype = 1;
                    latitude->newname = latitude->name ;
                    Dimension *dim =
                        new Dimension (latitude->getName (), (*k)->getSize (), 0);

                    latitude->dims.push_back (dim);
                    tempnewdimname1 = latitude->getName ();

                    // We donot need to generate the  unique dimension name based on the full path for all the current  cases we support.
                    // Leave here just as a reference.
                    // std::string uniquedimname = (*k)->getName() +temppath;
                    //  tempnewdimname1 = uniquedimname;
                    //   dim = new Dimension(uniquedimname,(*k)->getSize(),(*i)->getType());
                    dim =
                         new Dimension (latitude->getName (), (*k)->getSize (), 0);
                    latitude->correcteddims.push_back (dim);
                    latflag = true;
                }
            

            if (latflag == true && lonflag == true)
                break;
	} 

        if (latflag == true && lonflag == true)
            break;	// For this case, a field that needs lon and lot must exist

        // Need to reset the flag to avoid false alarm. For TRMM L3 we can handle, a field that has dimension 
        // which size is 400 and 1440 must exist in the file.
        latflag = false;
        lonflag = false;
    }

    if (latflag !=true  || lonflag != true) {
        if(latitude != NULL)
            delete latitude;
        if(longitude != NULL)
            delete longitude;
        throw5 ("Either latitude or longitude doesn't exist.", "lat. flag= ",
                 latflag, "lon. flag= ", lonflag);
    }

    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {


        for (std::vector < Dimension * >::const_iterator k =
            (*i)->getDimensions ().begin ();
            k != (*i)->getDimensions ().end (); ++k) {

                if ((*k)->getSize () == 360 ) 
                    (*k)->name = longitude->name;

                if ((*k)->getSize () == 180 ) 
                    (*k)->name = latitude->name;

        }

        for (std::vector < Dimension * >::const_iterator k =
            (*i)->getCorrectedDimensions ().begin ();
            k != (*i)->getCorrectedDimensions ().end (); ++k) {

                if ((*k)->getSize () == 360 ) 
                    (*k)->name = longitude->name;

                if ((*k)->getSize () == 180 ) 
                    (*k)->name = latitude->name;

        }


    }

   
    file->sd->sdfields.push_back (latitude);
    file->sd->sdfields.push_back (longitude);
 

    // Create the <dimname,coordinate variable> map from the corresponding dimension names to the latitude and the longitude
    file->sd->nonmisscvdimnamelist.insert (tempnewdimname1);
    file->sd->nonmisscvdimnamelist.insert (tempnewdimname2);

}

// Prepare TRMM Level 3, no lat/lon are in the original HDF4 file. Need to provide them.
void
File::PrepareTRMML3C_V6 ()
throw (Exception)
{

    std::string tempnewdimname1, tempnewdimname2,tempnewdimname3;
    bool latflag = false;
    bool lonflag = false;
    bool heiflag = false;

    SDField *latitude = NULL;
    SDField *longitude = NULL;
    SDField *height = NULL;

    File *file = this;

    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {


        for (std::vector < Dimension * >::const_iterator k =
            (*i)->getDimensions ().begin ();
            k != (*i)->getDimensions ().end (); ++k) {


            // This dimension has the dimension name
            //if ((((*k)->getName ()).find ("fakeDim")) == std::string::npos) 

                // The lat/lon formula is from GES DISC web site. http://disc.sci.gsfc.nasa.gov/additional/faq/precipitation_faq.shtml#lat_lon
                // KY 2010-7-13
                if ((*k)->getSize () == 720 && (*k)->getType () == 0) {//No dimension scale

                    longitude = new SDField ();
                    longitude->name = "longitude";
                    longitude->rank = 1;
                    longitude->type = DFNT_FLOAT32;
                    longitude->fieldtype = 2;

                    longitude->newname = longitude->name ;
                    Dimension *dim =
                        new Dimension (longitude->getName (), (*k)->getSize (), 0);
                    longitude->dims.push_back (dim);
                    tempnewdimname2 = longitude->name; 

                    dim =
                        new Dimension (longitude->getName (), (*k)->getSize (), 0);
                    longitude->correcteddims.push_back (dim);
                    lonflag = true;
                }

                if ((*k)->getSize () == 148 && (*k)->getType () == 0) {

                    latitude = new SDField ();
                    latitude->name = "latitude";
                    latitude->rank = 1;
                    latitude->type = DFNT_FLOAT32;
                    latitude->fieldtype = 1;
                    latitude->newname = latitude->name ;
                    Dimension *dim =
                        new Dimension (latitude->getName (), (*k)->getSize (), 0);

                    latitude->dims.push_back (dim);
                    tempnewdimname1 = latitude->getName ();

                    // We donot need to generate the  unique dimension name based on the full path for all the current  cases we support.
                    // Leave here just as a reference.
                    // std::string uniquedimname = (*k)->getName() +temppath;
                    //  tempnewdimname1 = uniquedimname;
                    //   dim = new Dimension(uniquedimname,(*k)->getSize(),(*i)->getType());
                    dim =
                         new Dimension (latitude->getName (), (*k)->getSize (), 0);
                    latitude->correcteddims.push_back (dim);
                    latflag = true;
                }

                if ((*k)->getSize () == 19 && (*k)->getType () == 0) {

                    height = new SDField ();
                    height->name = "height";
                    height->rank = 1;
                    height->type = DFNT_FLOAT32;
                    height->fieldtype = 6;
                    height->newname = height->name ;
                    Dimension *dim =
                        new Dimension (height->getName (), (*k)->getSize (), 0);

                    height->dims.push_back (dim);
                    tempnewdimname3 = height->getName ();

                    // We donot need to generate the  unique dimension name based on the full path for all the current  cases we support.
                    // Leave here just as a reference.
                    // std::string uniquedimname = (*k)->getName() +temppath;
                    //  tempnewdimname1 = uniquedimname;
                    //   dim = new Dimension(uniquedimname,(*k)->getSize(),(*i)->getType());
                    dim =
                         new Dimension (height->getName (), (*k)->getSize (), 0);
                    height->correcteddims.push_back (dim);
                    heiflag = true;
                }


	} 

        if (latflag == true && lonflag == true)
            break;	// For this case, a field that needs lon and lot must exist

        // Need to reset the flag to avoid false alarm. For TRMM L3 we can handle, a field that has dimension 
        // which size is 400 and 1440 must exist in the file.
        latflag = false;
        lonflag = false;
        heiflag = false;
    }

    if (latflag != true || lonflag != true) {
        if(latitude != NULL)
            delete latitude;
        if(longitude != NULL)
            delete longitude;
        throw5 ("Either latitude or longitude doesn't exist.", "lat. flag= ",
                 latflag, "lon. flag= ", lonflag);
    }

    if(height!=NULL && heiflag !=true) {
       delete height;
       throw1("Height is allocated but the flag is not true");
    }

   
    file->sd->sdfields.push_back (latitude);
    file->sd->sdfields.push_back (longitude);

    if(height!=NULL) {

        if(heiflag != true) {
            delete height;
            throw1("Height is allocated but the flag is not true");
        }
        else {
            file->sd->sdfields.push_back (height);
            file->sd->nonmisscvdimnamelist.insert (tempnewdimname3);
        }
    }

    // Create the <dimname,coordinate variable> map from the corresponding dimension names to the latitude and the longitude
    file->sd->nonmisscvdimnamelist.insert (tempnewdimname1);
    file->sd->nonmisscvdimnamelist.insert (tempnewdimname2);

}
// This applies to all OBPG level 2 products include SeaWIFS, MODISA, MODIST,OCTS, CZCS
// A formula similar to swath dimension map needs to apply to this file.
void
File::PrepareOBPGL2 ()
throw (Exception)
{
    int pixels_per_scan_line = 0;

    std::string pixels_per_scan_line_name = "Pixels per Scan Line";
    std::string number_pixels_control_points = "Number of Pixel Control Points";
    std::string tempnewdimname1, tempnewdimname2;

    File *file = this;

    // 1. Obtain the expanded size of the latitude/longitude
    for (std::vector < Attribute * >::const_iterator i =
        file->sd->getAttributes ().begin ();
	i != file->sd->getAttributes ().end (); ++i) {
        if ((*i)->getName () == pixels_per_scan_line_name) {
            int *attrvalueptr = (int *) (&((*i)->getValue ()[0]));
            pixels_per_scan_line = *attrvalueptr;
            break;
        }
    }

    if ( 0 == pixels_per_scan_line) 
        throw1("The attribute 'Pixels per Scan Line' doesn't exist");

    // 2. Obtain the latitude and longitude information
    //    Assign the new dimension name and the dimension size
    //   std::string temppath;
    int tempcountllflag = 0;

    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

        if ((*i)->getName () == "longitude" || (*i)->getName () == "latitude") {
            if ((*i)->getName () == "longitude")
                (*i)->fieldtype = 2;
	    if ((*i)->getName () == "latitude")
                (*i)->fieldtype = 1;

            tempcountllflag++;
            if ((*i)->getRank () != 2)
                throw3 ("The lat/lon rank must be 2", (*i)->getName (),
                         (*i)->getRank ());
            for (std::vector < Dimension * >::const_iterator k =
                (*i)->getDimensions ().begin ();
                k != (*i)->getDimensions ().end (); ++k) {
                if ((*k)->getName () == number_pixels_control_points) {
                    (*k)->name = pixels_per_scan_line_name;
                    (*k)->dimsize = pixels_per_scan_line;
                    break;
                }
            }

            for (std::vector < Dimension * >::const_iterator k =
                (*i)->getCorrectedDimensions ().begin ();
                k != (*i)->getCorrectedDimensions ().end (); ++k) {
                if ((*k)->getName ().find (number_pixels_control_points) !=
                    std::string::npos) {
                    (*k)->name = pixels_per_scan_line_name;
                    (*k)->dimsize = pixels_per_scan_line;
                    if (tempcountllflag == 1)
                        tempnewdimname2 = (*k)->name;
                }
                else {
                    if (tempcountllflag == 1)
                        tempnewdimname1 = (*k)->name;
                }
            }
        }
        if (tempcountllflag == 2)
            break;
    }


    // 3. Create the <dimname,coordinate variable> map from the corresponding dimension names to the latitude and the longitude
    // Obtain the corrected dimension names for latitude and longitude
    file->sd->nonmisscvdimnamelist.insert (tempnewdimname1);
    file->sd->nonmisscvdimnamelist.insert (tempnewdimname2);

}

// This applies to all OBPG l3m products include SeaWIFS, MODISA, MODIST,OCTS, CZCS
// Latitude and longitude need to be calculated based on attributes.
// 
void
File::PrepareOBPGL3 ()
throw (Exception)
{

    std::string num_lat_name = "Number of Lines";
    std::string num_lon_name = "Number of Columns";
    int32 num_lat = 0;
    int32 num_lon = 0;

    File *file = this;

    int tempcountllflag = 0;

    for (std::vector < Attribute * >::const_iterator i =
        file->sd->getAttributes ().begin ();
        i != file->sd->getAttributes ().end (); ++i) {

        if ((*i)->getName () == num_lon_name) {

            // Check later if float can be changed to float32
            int *attrvalue = (int *) (&((*i)->getValue ()[0]));

            num_lon = *attrvalue;
            tempcountllflag++;
        }

        if ((*i)->getName () == num_lat_name) {

            int *attrvalue = (int *) (&((*i)->getValue ()[0]));

            num_lat = *attrvalue;
            tempcountllflag++;
        }
        if (tempcountllflag == 2)
            break;
    }

    // Longitude
    SDField *longitude = new SDField ();

    longitude->name = "longitude";
    longitude->rank = 1;
    longitude->type = DFNT_FLOAT32;
    longitude->fieldtype = 2;

    // No need to assign fullpath, in this case, only one SDS under one file. If finding other OBPGL3 data, will handle then.
    longitude->newname = longitude->name;
    if (0 == num_lon) {
        delete longitude;
        throw3("The size of the dimension of the longitude ",longitude->name," is 0.");
    }

    Dimension *dim = new Dimension (num_lon_name, num_lon, 0);

    longitude->dims.push_back (dim);

    // Add the corrected dimension name only to be consistent with general handling of other cases.
    dim = new Dimension (num_lon_name, num_lon, 0);
    longitude->correcteddims.push_back (dim);

    // Latitude
    SDField *latitude = new SDField ();
    latitude->name = "latitude";
    latitude->rank = 1;
    latitude->type = DFNT_FLOAT32;
    latitude->fieldtype = 1;

    // No need to assign fullpath, in this case, only one SDS under one file. If finding other OBPGL3 data, will handle then.
    latitude->newname = latitude->name;
    if (0 == num_lat) {
        delete longitude;
        delete latitude;
        throw3("The size of the dimension of the latitude ",latitude->name," is 0.");
    }
            
    dim = new Dimension (num_lat_name, num_lat, 0);
    latitude->dims.push_back (dim);

    // Add the corrected dimension name only to be consistent with general handling of other cases.
    dim = new Dimension (num_lat_name, num_lat, 0);
    latitude->correcteddims.push_back (dim);

    // The dimension names of the SDS are fakeDim, so need to change them to dimension names of latitude and longitude
    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
        if ((*i)->getRank () != 2) {
            delete latitude;
            delete longitude;
            throw3 ("The lat/lon rank must be 2", (*i)->getName (),
                    (*i)->getRank ());
        }
        for (std::vector < Dimension * >::const_iterator k =
            (*i)->getDimensions ().begin ();
            k != (*i)->getDimensions ().end (); ++k) {
            if ((((*k)->getName ()).find ("fakeDim")) != std::string::npos) {
                if ((*k)->getSize () == num_lon)
                    (*k)->name = num_lon_name;
                if ((*k)->getSize () == num_lat)
                    (*k)->name = num_lat_name;
            }
        }
        for (std::vector < Dimension * >::const_iterator k =
            (*i)->getCorrectedDimensions ().begin ();
            k != (*i)->getCorrectedDimensions ().end (); ++k) {
            if ((((*k)->getName ()).find ("fakeDim")) != std::string::npos) {
                if ((*k)->getSize () == num_lon)
                    (*k)->name = num_lon_name;
                if ((*k)->getSize () == num_lat)
                    (*k)->name = num_lat_name;
            }
        }
    }
    file->sd->sdfields.push_back (latitude);
    file->sd->sdfields.push_back (longitude);

    // Set dimname,coordinate variable list
    file->sd->nonmisscvdimnamelist.insert (num_lat_name);
    file->sd->nonmisscvdimnamelist.insert (num_lon_name);

}

// This applies to CERES AVG and SYN(CER_AVG_??? and CER_SYN_??? cases)
// Latitude and longitude are provided; some redundant CO-Latitude and longitude are removed from the final DDS.
void
File::PrepareCERAVGSYN ()
throw (Exception)
{

    bool colatflag = false;
    bool lonflag = false;

    std::string tempnewdimname1, tempnewdimname2;
    std::string tempcvarname1, tempcvarname2;
    File *file = this;
    // int eraseflag = 0; Unused jhrg 3/16/11

    std::vector < SDField * >::iterator beerasedit;

    // SDField *beerased; Unused jhrg 3/16/11

    for (std::vector < SDField * >::iterator i = file->sd->sdfields.begin ();
        i != file->sd->sdfields.end (); ++i) {

        // This product uses "Colatitude".
        if (((*i)->getName ()).find ("Colatitude") != std::string::npos) {
            if (!colatflag) {
                if ((*i)->getRank () != 2)
                    throw3 ("The lat/lon rank must be 2", (*i)->getName (),
                            (*i)->getRank ());
                int dimsize0 = (*i)->getDimensions ()[0]->getSize ();
                int dimsize1 = (*i)->getDimensions ()[1]->getSize ();

                // The following comparision may not be necessary. 
                // For most cases, the C-order first dimension is cooresponding to lat(in 1-D),
                // which is mostly smaller than the dimension of lon(in 2-D). E.g. 90 for lat vs 180 for lon.
                if (dimsize0 < dimsize1) {
                    tempnewdimname1 = (*i)->getDimensions ()[0]->getName ();
                    tempnewdimname2 = (*i)->getDimensions ()[1]->getName ();
                }
                else {
                    tempnewdimname1 = (*i)->getDimensions ()[1]->getName ();
                    tempnewdimname2 = (*i)->getDimensions ()[0]->getName ();

                }

                colatflag = true;
                (*i)->fieldtype = 1;
                tempcvarname1 = (*i)->getName ();
            }
            else {//remove the redundant Colatitude field
                delete (*i);
                file->sd->sdfields.erase (i);
                // When erasing the iterator, the iterator will 
                // automatically go to the next element, 
                // so we need to go back 1 in order not to miss the next element.
                i--;
            }
        }

        else if (((*i)->getName ()).find ("Longitude") != std::string::npos) {
            if (!lonflag) {
                lonflag = true;
                (*i)->fieldtype = 2;
                tempcvarname2 = (*i)->getName ();
            }
            else {//remove the redundant Longitude field
                delete (*i);
                file->sd->sdfields.erase (i);
                // When erasing the iterator, the iterator will 
                // automatically go to the next element, so we need to go back 1 
                // in order not to miss the next element.
                i--;
            }
        }
    }

    file->sd->nonmisscvdimnamelist.insert (tempnewdimname1);
    file->sd->nonmisscvdimnamelist.insert (tempnewdimname2);

}

// Handle CERES ES4 and ISCCP-GEO cases. Essentially the lat/lon need to be condensed to 1-D for the geographic projection.
void
File::PrepareCERES4IG ()
throw (Exception)
{

    std::string tempdimname1, tempdimname2;
    int tempdimsize1 = 0;
    int tempdimsize2 = 0;
    std::string tempcvarname1, tempcvarname2;
    std::string temppath;
    std::set < std::string > tempdimnameset;
    std::pair < std::set < std::string >::iterator, bool > tempsetit;

    // bool eraseflag = false; Unused jhrg 3/16/11
    bool cvflag = false;
    File *file = this;

    // The original latitude is 3-D array; we have to use the dimension name to determine which dimension is the final dimension
    // for 1-D array. "regional colat" and "regional lon" are consistently used in these two CERES cases.
    for (std::vector < SDField * >::iterator i = file->sd->sdfields.begin ();
        i != file->sd->sdfields.end (); ++i) {
        std::string tempfieldname = (*i)->getName ();
        if (tempfieldname.find ("Colatitude") != std::string::npos) {
            // They may have more than 2 dimensions, so we have to adjust it.
            for (std::vector < Dimension * >::const_iterator j =
                (*i)->getDimensions ().begin ();
                j != (*i)->getDimensions ().end (); ++j) {
                if ((((*j)->getName ()).find ("regional colat") !=
                    std::string::npos)) {
                    tempsetit = tempdimnameset.insert ((*j)->getName ());
                    if (tempsetit.second == true) {
                        tempdimname1 = (*j)->getName ();
                        tempdimsize1 = (*j)->getSize ();
                        (*i)->fieldtype = 1;
                        (*i)->rank = 1;
                        cvflag = true;
                        break;
                    }
                }
            }

            if (cvflag) {// change the dimension from 3-D to 1-D.
			// Clean up the original dimension vector first
                for (std::vector < Dimension * >::const_iterator j =
                     (*i)->getDimensions ().begin ();
                     j != (*i)->getDimensions ().end (); ++j)
                     delete (*j);
                for (std::vector < Dimension * >::const_iterator j =
                    (*i)->getCorrectedDimensions ().begin ();
                    j != (*i)->getCorrectedDimensions ().end (); ++j)
                     delete (*j);
                (*i)->dims.clear ();
                (*i)->correcteddims.clear ();

                Dimension *dim =
                    new Dimension (tempdimname1, tempdimsize1, 0);
                (*i)->dims.push_back (dim);
                dim = new Dimension (tempdimname1, tempdimsize1, 0);
                (*i)->correcteddims.push_back (dim);
                file->sd->nonmisscvdimnamelist.insert (tempdimname1);
                cvflag = false;
            }
            else {//delete this element from the vector and erase it.
                delete (*i);
                file->sd->sdfields.erase (i);

                // When erasing the iterator, the iterator will automatically 
                // go to the next element, so we need to go back 1 in order not 
                // to miss the next element.
                i--;
            }
        }

        else if (tempfieldname.find ("Longitude") != std::string::npos) {
            for (std::vector < Dimension * >::const_iterator j =
                (*i)->getDimensions ().begin ();
                j != (*i)->getDimensions ().end (); ++j) {
                if (((*j)->getName ()).find ("regional long") !=
                    std::string::npos) {
                    tempsetit = tempdimnameset.insert ((*j)->getName ());
                    if (tempsetit.second == true) {
                        tempdimname2 = (*j)->getName ();
                        tempdimsize2 = (*j)->getSize ();
                        (*i)->fieldtype = 2;
                        (*i)->rank = 1;
                        cvflag = true;
                        break;
                    }
                // Make this the only dimension name of this field
                }
            }
            if (cvflag) {
                for (std::vector < Dimension * >::const_iterator j =
                    (*i)->getDimensions ().begin ();
                    j != (*i)->getDimensions ().end (); ++j) {
                    delete (*j);
                }
                for (std::vector < Dimension * >::const_iterator j =
                    (*i)->getCorrectedDimensions ().begin ();
                    j != (*i)->getCorrectedDimensions ().end (); ++j) {
                    delete (*j);
                }
                (*i)->dims.clear ();
                (*i)->correcteddims.clear ();

                Dimension *dim =
                    new Dimension (tempdimname2, tempdimsize2, 0);
                        (*i)->dims.push_back (dim);
                dim = new Dimension (tempdimname2, tempdimsize2, 0);
                (*i)->correcteddims.push_back (dim);

                file->sd->nonmisscvdimnamelist.insert (tempdimname2);
                cvflag = false;
            }
            else{//delete this element from the vector and erase it.
                delete (*i);
                file->sd->sdfields.erase (i);
                // When erasing the iterator, the iterator 
                // will automatically go to the next element, 
                // so we need to go back 1 in order not to miss the next element.
                i--;
            }
        }
    }
}


// CERES SAVG and CERES ISCCP-IDAY cases.
// We need provide nested CERES grid 2-D lat/lon.
// The lat and lon should be calculated following:
// http://eosweb.larc.nasa.gov/PRODOCS/ceres/SRBAVG/Quality_Summaries/srbavg_ed2d/nestedgrid.html
// or https://eosweb.larc.nasa.gov/sites/default/files/project/ceres/quality_summaries/srbavg_ed2d/nestedgrid.pdf 
// The dimension names and sizes are set according to the studies of these files.
void
File::PrepareCERSAVGID ()
throw (Exception)
{

    // bool colatflag = false; unused jhrg 3/16/11
    // bool lonflag = false; Unused jhrg 3/16/11

    std::string tempdimname1 = "1.0 deg. regional colat. zones";
    std::string tempdimname2 = "1.0 deg. regional long. zones";
    std::string tempdimname3 = "1.0 deg. zonal colat. zones";
    std::string tempdimname4 = "1.0 deg. zonal long. zones";
    int tempdimsize1 = 180;
    int tempdimsize2 = 360;
    int tempdimsize3 = 180;
    int tempdimsize4 = 1;

    std::string tempnewdimname1, tempnewdimname2;
    std::string tempcvarname1, tempcvarname2;
    File *file;

    file = this;

    SDField *latitude = new SDField ();

    latitude->name = "latitude";
    latitude->rank = 2;
    latitude->type = DFNT_FLOAT32;
    latitude->fieldtype = 1;

    // No need to obtain the full path
    latitude->newname = latitude->name;

    Dimension *dim = new Dimension (tempdimname1, tempdimsize1, 0);

    latitude->dims.push_back (dim);

    dim = new Dimension (tempdimname2, tempdimsize2, 0);
    latitude->dims.push_back (dim);

    dim = new Dimension (tempdimname1, tempdimsize1, 0);
    latitude->correcteddims.push_back (dim);

    dim = new Dimension (tempdimname2, tempdimsize2, 0);
    latitude->correcteddims.push_back (dim);
    file->sd->sdfields.push_back (latitude);

    SDField *longitude = new SDField ();

    longitude->name = "longitude";
    longitude->rank = 2;
    longitude->type = DFNT_FLOAT32;
    longitude->fieldtype = 2;

    // No need to obtain the full path
    longitude->newname = longitude->name;

    dim = new Dimension (tempdimname1, tempdimsize1, 0);
    longitude->dims.push_back (dim);

    dim = new Dimension (tempdimname2, tempdimsize2, 0);
    longitude->dims.push_back (dim);

    dim = new Dimension (tempdimname1, tempdimsize1, 0);
    longitude->correcteddims.push_back (dim);

    dim = new Dimension (tempdimname2, tempdimsize2, 0);
    longitude->correcteddims.push_back (dim);
    file->sd->sdfields.push_back (longitude);

    // For the CER_SRB case, zonal average data is also included.
    // We need only provide the latitude.
    if (file->sptype == CER_SRB) {

        SDField *latitudez = new SDField ();

        latitudez->name = "latitudez";
        latitudez->rank = 1;
        latitudez->type = DFNT_FLOAT32;
        latitudez->fieldtype = 1;
        latitudez->newname = latitudez->name;

        dim = new Dimension (tempdimname3, tempdimsize3, 0);
        latitudez->dims.push_back (dim);

        dim = new Dimension (tempdimname3, tempdimsize3, 0);
        latitudez->correcteddims.push_back (dim);
        file->sd->sdfields.push_back (latitudez);

        SDField *longitudez = new SDField ();
        longitudez->name = "longitudez";
        longitudez->rank = 1;
        longitudez->type = DFNT_FLOAT32;
        longitudez->fieldtype = 2;
        longitudez->newname = longitudez->name;

        dim = new Dimension (tempdimname4, tempdimsize4, 0);
            longitudez->dims.push_back (dim);

        dim = new Dimension (tempdimname4, tempdimsize4, 0);
            longitudez->correcteddims.push_back (dim);
        file->sd->sdfields.push_back (longitudez);
    }

    if (file->sptype == CER_SRB) {
        file->sd->nonmisscvdimnamelist.insert (tempdimname3);
        file->sd->nonmisscvdimnamelist.insert (tempdimname4);
    }

    file->sd->nonmisscvdimnamelist.insert (tempdimname1);
    file->sd->nonmisscvdimnamelist.insert (tempdimname2);
        
    if(file->sptype == CER_CDAY) {

        string odddimname1= "1.0 deg. regional Colat. zones";
	string odddimname2 = "1.0 deg. regional Long. zones";

        // Add a loop to change the odddimnames to (normal)tempdimnames.
        for (std::vector < SDField * >::const_iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
            for (std::vector < Dimension * >::const_iterator j =
                (*i)->getDimensions ().begin ();
                j != (*i)->getDimensions ().end (); ++j) {
                if (odddimname1 == (*j)->name)
                    (*j)->name = tempdimname1;
                if (odddimname2 == (*j)->name)
                    (*j)->name = tempdimname2;
            }
            for (std::vector < Dimension * >::const_iterator j =
                (*i)->getCorrectedDimensions ().begin ();
                j != (*i)->getCorrectedDimensions ().end (); ++j) {
                if (odddimname1 == (*j)->name)
                    (*j)->name = tempdimname1;
                if (odddimname2 == (*j)->name)
                    (*j)->name = tempdimname2;

            }
        }
    }    
}

// Prepare the CER_ZAVG case. This is the zonal average case.
// Only latitude is needed.
void
File::PrepareCERZAVG ()
throw (Exception)
{

    std::string tempdimname3 = "1.0 deg. zonal colat. zones";
    std::string tempdimname4 = "1.0 deg. zonal long. zones";
    int tempdimsize3 = 180;
    int tempdimsize4 = 1;
    File *file = this;

    SDField *latitudez = new SDField ();

    latitudez->name = "latitudez";
    latitudez->rank = 1;
    latitudez->type = DFNT_FLOAT32;
    latitudez->fieldtype = 1;
    latitudez->newname = latitudez->name;


    Dimension *dim = new Dimension (tempdimname3, tempdimsize3, 0);
    latitudez->dims.push_back (dim);

    dim = new Dimension (tempdimname3, tempdimsize3, 0);
    latitudez->correcteddims.push_back (dim);

    file->sd->sdfields.push_back (latitudez);
    SDField *longitudez = new SDField ();

    longitudez->name = "longitudez";
    longitudez->rank = 1;
    longitudez->type = DFNT_FLOAT32;
    longitudez->fieldtype = 2;
    longitudez->newname = longitudez->name;

    dim = new Dimension (tempdimname4, tempdimsize4, 0);
    longitudez->dims.push_back (dim);

    dim = new Dimension (tempdimname4, tempdimsize4, 0);
    longitudez->correcteddims.push_back (dim);

    file->sd->sdfields.push_back (longitudez);
    file->sd->nonmisscvdimnamelist.insert (tempdimname3);
    file->sd->nonmisscvdimnamelist.insert (tempdimname4);

}

// Prepare the "Latitude" and "Longitude" for the MODISARNSS case.
// This file has Latitude and Longitude. The only thing it needs 
// to change is to assure the dimension names of the field names the same
// as the lat and lon.
void
File::PrepareMODISARNSS ()
throw (Exception)
{

    std::set < std::string > tempfulldimnamelist;
    std::pair < std::set < std::string >::iterator, bool > ret;

    std::map < int, std::string > tempsizedimnamelist;

    File *file = this;

    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
        if ((*i)->getName () == "Latitude")
            (*i)->fieldtype = 1;
        if ((*i)->getName () == "Longitude") {
            (*i)->fieldtype = 2;

            // Fields associate with lat/lon use different dimension names;
            // To be consistent with other code, use size-dim map to change 
            // fields that have the same size as lat/lon to hold the same dimension names.
            for (std::vector < Dimension * >::const_iterator j =
                (*i)->getCorrectedDimensions ().begin ();
                j != (*i)->getCorrectedDimensions ().end (); ++j) {
                tempsizedimnamelist[(*j)->getSize ()] = (*j)->getName ();
                file->sd->nonmisscvdimnamelist.insert ((*j)->getName ());
            }
        }
    }

    for (std::vector < SDField * >::const_iterator i =
        file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
        for (std::vector < Dimension * >::const_iterator j =
            (*i)->getCorrectedDimensions ().begin ();
            j != (*i)->getCorrectedDimensions ().end (); ++j) {

            // Need to change those dimension names to be the same as lat/lon 
            // so that a coordinate variable dimension name map can be built.
            if ((*i)->fieldtype == 0) {
                if ((tempsizedimnamelist.find ((*j)->getSize ())) !=
                    tempsizedimnamelist.end ())
                    (*j)->name = tempsizedimnamelist[(*j)->getSize ()];
            }
        }
    }
}


// For all other cases not listed above. What we did here is very limited.
// We only consider the field to be a "third-dimension" coordinate variable
// when dimensional scale is applied.
void
File::PrepareOTHERHDF ()
throw (Exception)
{

    std::set < std::string > tempfulldimnamelist;
    std::pair < std::set < std::string >::iterator, bool > ret;
    File *file = this;

    // I need to trimm MERRA data field and dim. names according to NASA's request.
    // Currently the field name includes the full path(/EOSGRID/Data Fields/PLE);
    // the dimension name is something
    // like XDim::EOSGRID, which are from the original HDF-EOS2 files. 
    // I need to trim them. Field name PLE, Dimension name: XDim.
    // KY 2012-7-2

    bool merra_is_eos2 = false;
    size_t found_forward_slash = file->path.find_last_of("/");
    if ((found_forward_slash != string::npos) && 
        (((file->path).substr(found_forward_slash+1).compare(0,5,"MERRA"))==0)){

        for (std::vector < Attribute * >::const_iterator i =
            file->sd->getAttributes ().begin ();
            i != file->sd->getAttributes ().end (); ++i) {

            // CHeck if this MERRA file is an HDF-EOS2 or not.
            if(((*i)->getName().compare(0, 14, "StructMetadata" )== 0) ||
                ((*i)->getName().compare(0, 14, "structmetadata" )== 0)) {
                merra_is_eos2 = true;
                break;
            }

        }
    }

    if( true == merra_is_eos2) {
        vector <string> noneos_newnamelist; 

        // 1. Remove EOSGRID from the added-non-EOS field names(XDim:EOSGRID to XDim)
        for (std::vector < SDField * >::const_iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
            (*i)->special_product_fullpath = (*i)->newname;
            // "EOSGRID" inside variable and dim. names needs to be trimmed out. KY 7-2-2012
            string EOSGRIDstring=":EOSGRID";
            size_t found = ((*i)->name).rfind(EOSGRIDstring);

            if (found !=string::npos && (((*i)->name).size() == (found + EOSGRIDstring.size()))) {
                    
                (*i)->newname = (*i)->name.substr(0,found);
                noneos_newnamelist.push_back((*i)->newname);
            }
            else
                (*i)->newname = (*i)->name;
        }

        // 2. Make the redundant and clashing CVs such as XDim to XDim_EOS etc.
        // I don't want to remove these fields since a variable like Time is different than TIME
        // So still keep it in case it is useful for some users.

        for (std::vector < SDField * >::const_iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
                        
            for(vector<string>::const_iterator j = 
                noneos_newnamelist.begin(); j !=noneos_newnamelist.end();++j) {

                if ((*i)->newname == (*j) && (*i)->name == (*j)) {
                    // Make XDim to XDim_EOS so that we don't have two "XDim".
                    (*i)->newname = (*i)->newname +"_EOS";
                }
            } 
        }

        // 3. Handle Dimension scales
        // 3.1 Change the dimension names for coordinate variables.
        map<string,string> dimname_to_newdimname;
        for (std::vector < SDField * >::const_iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
            for (std::vector < Dimension * >::const_iterator j =
                (*i)->getCorrectedDimensions ().begin ();
                j != (*i)->getCorrectedDimensions ().end (); ++j) {
                // Find the dimension scale
                if ((*j)->getType () != 0) {
                    if ((*i)->name == (*j)->getName () && (*i)->getRank () == 1){
                        (*i)->fieldtype = 3;
                        (*i)->is_dim_scale = true;
                        (*j)->name = (*i)->newname;
                        // Build up the map from the original name to the new name, Note (*i)->name is the original
                        // dimension name. 
                        HDFCFUtil::insert_map(dimname_to_newdimname,(*i)->name,(*j)->name);
                    }
                    file->sd->nonmisscvdimnamelist.insert ((*j)->name);
                }
            }
        }

        // 3.2 Change the dimension names for data variables.
        map<string,string>::iterator itmap;
        for (std::vector < SDField * >::const_iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {

            if (0 == (*i)->fieldtype) {
                for (std::vector < Dimension * >::const_iterator j =
                    (*i)->getCorrectedDimensions ().begin ();
                    j != (*i)->getCorrectedDimensions ().end (); ++j) {
                    itmap = dimname_to_newdimname.find((*j)->name);
                    if (itmap == dimname_to_newdimname.end()) 
                         throw2("Cannot find the corresponding new dim. name for dim. name ",(*j)->name);
                    else
                        (*j)->name = (*itmap).second;

                }
            }
        }
    }
    else {
            
        for (std::vector < SDField * >::const_iterator i =
            file->sd->sdfields.begin (); i != file->sd->sdfields.end () && (false == this->OTHERHDF_Has_Dim_NoScale_Field); ++i) {
            for (std::vector < Dimension * >::const_iterator j =
                (*i)->getCorrectedDimensions ().begin ();
                j != (*i)->getCorrectedDimensions ().end () && (false == this->OTHERHDF_Has_Dim_NoScale_Field); ++j) {

                if ((*j)->getType () != 0) {
                    if ((*i)->name == (*j)->getName () && (*i)->getRank () == 1)
                        (*i)->fieldtype = 3;
                    file->sd->nonmisscvdimnamelist.insert ((*j)->getName ());
                }
                else {
                    this->OTHERHDF_Has_Dim_NoScale_Field = true;
                }
	    }
	}

        // For OTHERHDF cases, currently if we find that there are "no dimension scale" dimensions, we will NOT generate any "cooridnates" attributes.
        // That means "nonmisscvdimnamelist" should be cleared if OTHERHDF_Has_Dim_NoScale_Field is true. 

        if (true == this->OTHERHDF_Has_Dim_NoScale_Field) 
            file->sd->nonmisscvdimnamelist.clear();
    }
}
