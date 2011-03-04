/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
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
/// Copyright (C) 2010 The HDF Group
///
/// All rights reserved.

/////////////////////////////////////////////////////////////////////////////
#include <sstream>
#include <algorithm>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include "HDFSP.h"

using namespace HDFSP;
using namespace std;

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


using namespace HDFSP;
using namespace std;

struct delete_elem
{
	template < typename T > void operator () (T * ptr)
	{
		delete ptr;
	}
};


File::~File ()
{

	if (this->sdfd != -1) {

		if (sd != NULL)
			delete sd;

		SDend (this->sdfd);
	}

	if (this->fileid != -1) {

		for (std::vector < VDATA * >::const_iterator i = this->vds.begin ();
			 i != this->vds.end (); ++i) {
			delete *i;
		}
		Vend (this->fileid);
		Hclose (this->fileid);
	}
}

VDATA::~VDATA ()
{
	std::for_each (this->vdfields.begin (), this->vdfields.end (),
				   delete_elem ());
	std::for_each (this->attrs.begin (), this->attrs.end (), delete_elem ());

}

SD::~SD ()
{

	std::for_each (this->attrs.begin (), this->attrs.end (), delete_elem ());
	std::for_each (this->sdfields.begin (), this->sdfields.end (),
				   delete_elem ());

}

SDField::~SDField ()
{

	std::for_each (this->dims.begin (), this->dims.end (), delete_elem ());
	std::for_each (this->correcteddims.begin (), this->correcteddims.end (),
				   delete_elem ());

}

VDField::~VDField ()
{
}

Field::~Field ()
{
	std::for_each (this->attrs.begin (), this->attrs.end (), delete_elem ());
}

// Retrieve all the information from an HDF file; this is the same approach
// as the way to handle HDF-EOS2 files.
File *
File::Read (const char *path, int32 myfileid)
throw (Exception)
{

	File *file = new File (path);

	int32 mysdid;
	if ((mysdid =
		 SDstart (const_cast < char *>(file->path.c_str ()),
				  DFACC_READ)) == -1) {
		delete file;

		throw2 ("SDstart", path);
	}

	// A strange compiling bug was found if we don't pass the file id to this fuction.
	// It will always give 0 number as the ID and the HDF4 library doesn't complain!! 
	// Will try dumplicating the problem and submit a bug report. KY 2010-7-14
	file->sdfd = mysdid;
	file->fileid = myfileid;

	// Handle SDS 
	file->sd = SD::Read (file->sdfd, file->fileid);

	// Handle vlone data here
	int status = Vstart (file->fileid);

	if (status == FAIL)
		throw2 ("Cannot start vdata/vgroup interface", path);

	int num_lone_vdata = VSlone (file->fileid, NULL, 0);

	if (num_lone_vdata == FAIL)
		throw2 ("Fail to obtain lone vdata number", path);

	// Currently the vdata name buffer has to be static allocated according to HDF4 reference manual. KY 2010-7-14
	char vdata_class[VSNAMELENMAX], vdata_name[VSNAMELENMAX];

	if (num_lone_vdata > 0) {

		// Since HDF4 is a C library, here we use the "C" way to handle the data, use "calloc" and "free".

		int32 *ref_array = (int32 *) calloc (num_lone_vdata, sizeof (int32));

		if (ref_array == NULL)
			throw1 ("no enough memory to allocate the buffer.");

		if (VSlone (file->fileid, ref_array, num_lone_vdata) == FAIL) {
			free (ref_array);
			throw2 ("cannot obtain lone vdata reference arrays", path);
		}

		for (int i = 0; i < num_lone_vdata; i++) {

			int32 vdata_id;

			vdata_id = VSattach (file->fileid, ref_array[i], "r");
			if (vdata_id == FAIL) {
				free (ref_array);
				throw2 ("Fail to attach Vdata", path);
			}

			status = VSgetclass (vdata_id, vdata_class);
			if (status == FAIL) {
				VSdetach (vdata_id);
				free (ref_array);
				throw2 ("Fail to obtain Vdata class", path);
			}

			if (VSgetname (vdata_id, vdata_name) == FAIL) {
				VSdetach (vdata_id);
				free (ref_array);
				throw3 ("Fail to obtain Vdata name", path, vdata_name);
			}

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
					free (ref_array);
					throw3 ("VSdetach failed ", "Vdata name ", vdata_name);
				}
			}

			else {

				VDATA *vdataobj = VDATA::Read (vdata_id, ref_array[i]);

				// We want to map fields of vdata with more than 10 records to DAP variables
				// and we need to add the path and vdata name to the new vdata field name
				if (!vdataobj->getTreatAsAttrFlag ()) {
					for (std::vector < VDField * >::const_iterator it_vdf =
						 vdataobj->getFields ().begin ();
						 it_vdf != vdataobj->getFields ().end (); it_vdf++) {
						(*it_vdf)->newname =
							"vdf_" + (*it_vdf)->name + "_" +
							vdataobj->newname;
					}
				}
				file->vds.push_back (vdataobj);

				// THe following code should be replaced by using the VDField member functions in the future
				// The code has largely overlapped with VDField member functions, but not for this release.
				// KY 2010-8-11
				// To know if the data product is CERES, we need to check Vdata CERE_metadata(CERE_META_NAME).
				//  One field name LOCALGRANULEID(CERE_META_FIELD_NAME) includes the product name. 

				if (!strncmp
					(vdata_name, CERE_META_NAME, strlen (CERE_META_NAME))) {

					char *fieldname;
					int num_field = VFnfields (vdata_id);

					if (num_field == FAIL) {
						free (ref_array);
						VSdetach (vdata_id);
						throw3 ("number of fields at Vdata ", vdata_name,
								" is -1");
					}

					for (int j = 0; j < num_field; j++) {

						fieldname = VFfieldname (vdata_id, j);
						if (fieldname == NULL) {
							free (ref_array);
							VSdetach (vdata_id);
							throw5 ("vdata ", vdata_name, " field index ", j,
									" field name is NULL.");
						}

						if (!strcmp (fieldname, CERE_META_FIELD_NAME)) {

							int32 fieldsize, nelms;

							fieldsize = VFfieldesize (vdata_id, j);
							if (fieldsize == FAIL) {
								free (ref_array);
								VSdetach (vdata_id);
								throw5 ("vdata ", vdata_name, " field ",
										fieldname, " size is wrong.");
							}

							nelms = VSelts (vdata_id);
							if (nelms == FAIL) {
								free (ref_array);
								VSdetach (vdata_id);
								throw5 ("vdata ", vdata_name,
										" number of field record ", nelms,
										"  is wrong.");
							}

							char *databuf =
								(char *) malloc (fieldsize * nelms);
							if (databuf == NULL) {
								free (ref_array);
								VSdetach (vdata_id);
								throw1
									("no enough memory to allocate buffer.");
							}
							if (VSseek (vdata_id, 0) == FAIL) {
								VSdetach (vdata_id);
								free (ref_array);
								free (databuf);
								throw5 ("vdata ", vdata_name, "field ",
										CERE_META_FIELD_NAME,
										" VSseek failed.");
							}
							if (VSsetfields (vdata_id, CERE_META_FIELD_NAME)
								== FAIL) {
								VSdetach (vdata_id);
								free (ref_array);
								free (databuf);
								throw5 ("vdata ", vdata_name, "field ",
										CERE_META_FIELD_NAME,
										" VSsetfields failed.");
							}

							if (VSread
								(vdata_id, (uint8 *) databuf, 1,
								 FULL_INTERLACE) == FAIL) {
								VSdetach (vdata_id);
								free (ref_array);
								free (databuf);
								throw5 ("vdata ", vdata_name, "field ",
										CERE_META_FIELD_NAME,
										" VSread failed.");
							}

							if (!strncmp
								(databuf, CER_AVG_NAME,
								 strlen (CER_AVG_NAME)))
								file->sptype = CER_AVG;
							else if (!strncmp
									 (databuf, CER_ES4_NAME,
									  strlen (CER_ES4_NAME)))
								file->sptype = CER_ES4;
							else if (!strncmp
									 (databuf, CER_CDAY_NAME,
									  strlen (CER_CDAY_NAME)))
								file->sptype = CER_CDAY;
							else if (!strncmp
									 (databuf, CER_CGEO_NAME,
									  strlen (CER_CGEO_NAME)))
								file->sptype = CER_CGEO;
							else if (!strncmp
									 (databuf, CER_SRB_NAME,
									  strlen (CER_SRB_NAME)))
								file->sptype = CER_SRB;
							else if (!strncmp
									 (databuf, CER_SYN_NAME,
									  strlen (CER_SYN_NAME)))
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
		free (ref_array);
	}

	return file;
}

// Retrieve all the information from the additional SDS objects of an HDF file; this is the same approach
// as the way to handle other HDF4 files.
File *
File::Read_Hybrid (const char *path, int32 myfileid)
throw (Exception)
{
	File *file = new File (path);

	int32 mysdid;
	if ((mysdid =
		 SDstart (const_cast < char *>(file->path.c_str ()),
				  DFACC_READ)) == -1) {
		delete file;
		throw2 ("SDstart", path);
	}
//cerr<<"SD id"<<mysdid <<endl;

	// A strange compiling bug was found if we don't pass the file id to this fuction.
	// It will always give 0 number as the ID and the HDF4 library doesn't complain!! 
	// Will try dumplicating the problem and submit a bug report. KY 2010-7-14
	file->sdfd = mysdid;
	file->fileid = myfileid;

        // Handle vlone data here
	int status = Vstart (file->fileid);

	if (status == FAIL)
		throw2 ("Cannot start vdata/vgroup interface", path);

	// Handle SDS 
	file->sd = SD::Read_Hybrid(file->sdfd, file->fileid);
//cerr <<"before the end of File Read_Hybrid "<<endl;
         return file;
}


//  This method will check if the HDF4 file is one of TRMM or OBPG products or MODISARNSS we supported.
void
File::CheckSDType ()
throw (Exception)
{

	// check the TRMM and MODARNSS/MYDARNSS cases
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

		if (metadataflag == 4)	// This is a very special MODIS product. It includes StructMetadata.0 but it is not an HDF-EOS2 file. 
			this->sptype = MODISARNSS;

		if (metadataflag == 2) {	// DATA_GRANULE is the TRMM "swath" name; geolocation is the TRMM "geolocation" field.

			for (std::vector < SDField * >::const_iterator i =
				 this->sd->getFields ().begin ();
				 i != this->sd->getFields ().end (); ++i) {
				if (((*i)->getName () == "geolocation")
					&& (*i)->getNewName ().find ("DATA_GRANULE") !=
					std::string::npos
					&& (*i)->getNewName ().find ("SwathData") !=
					std::string::npos && (*i)->getRank () == 3) {
					this->sptype = TRMML2;
					break;
				}
			}

			// This is for TRMM Level 3 3B42 and 3B43 data. The vgroup name is DATA_GRANULE.
			// At least one field is 1440*400 array. The information is obtained from 
			// http://disc.sci.gsfc.nasa.gov/additional/faq/precipitation_faq.shtml#lat_lon

			if (this->sptype == OTHERHDF) {
				for (std::vector < SDField * >::const_iterator i =
					 this->sd->getFields ().begin ();
					 i != this->sd->getFields ().end (); ++i) {
					if ((*i)->getNewName ().find ("DATA_GRANULE") !=
						std::string::npos) {
						int loncount = 0;
						int latcount = 0;

						for (std::vector < Dimension * >::const_iterator k =
							 (*i)->getDimensions ().begin ();
							 k != (*i)->getDimensions ().end (); ++k) {
							if ((*k)->getSize () == 1440)
								loncount++;

							if ((*k)->getSize () == 400)
								latcount++;
						}
						if (loncount == 1 && latcount == 1) {
							this->sptype = TRMML3;
							break;
						}
					}
				}
			}
		}
	}

	// Check the OBPG case
	// OBPG includes SeaWIFS,OCTS,CZCS,MODISA,MODIST
	// One attribute "Product Name" includes unique information for each product,
	// For example, SeaWIFS L2 data' "Product Name" is S2003006001857.L2_MLAC 
	// Since we only support Level 2 and Level 3m data, we just check if those characters exist inside the attribute.
	// The reason we cannot support L1A data is lat/lon consists of fill values.

	if (this->sptype == OTHERHDF) {

		int modisal2flag = 0;
		int modistl2flag = 0;
		int octsl2flag = 0;
		int seawifsl2flag = 0;
		int czcsl2flag = 0;

		int modisal3mflag = 0;
		int modistl3mflag = 0;
		int octsl3mflag = 0;
		int seawifsl3mflag = 0;
		int czcsl3mflag = 0;

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
							 ((attrvalue.find (".L1A", 0) !=
							   std::string::npos))))
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

	int32 status;
	int32 n_sds, n_sd_attrs;
	int index;
	int32 sds_id;
	int32 dim_sizes[H4_MAX_VAR_DIMS];
	int32 sds_rank, sds_dtype, n_sds_attrs;
	char sds_name[H4_MAX_NC_NAME];
	char dim_name[H4_MAX_NC_NAME];
	char attr_name[H4_MAX_NC_NAME];
	int32 dim_size, sds_ref, dim_type, num_dim_attrs;
	int32 attr_value_count;

	SD *sd = new SD (sdfd, fileid);

	if (SDfileinfo (sdfd, &n_sds, &n_sd_attrs) == FAIL)
		throw2 ("SDfileinfo failed ", sdfd);

	for (index = 0; index < n_sds; index++) {

		SDField *field = new SDField ();

		sds_id = SDselect (sdfd, index);
		if (sds_id == FAIL) {

			// We only need to close SDS ID. SD ID will be closed when the destructor is called.
			SDendaccess (sds_id);
			throw3 ("SDselect Failed ", "SDS index ", index);
		}

		sds_ref = SDidtoref (sds_id);
		if (sds_ref == FAIL) {
			SDendaccess (sds_id);
			throw3 ("Cannot obtain SDS reference number", " SDS ID is ",
					sds_id);
		}

		// Obtain object name, rank, size, field type and number of SDS attributes
		status = SDgetinfo (sds_id, sds_name, &field->rank, dim_sizes,
							&field->type, &n_sds_attrs);
		if (status == FAIL) {
			SDendaccess (sds_id);
			throw2 ("SDgetinfo failed ", sds_name);
		}
		string tempname (sds_name);

		field->name = tempname;
		field->newname = field->name;
		field->sdsref = sds_ref;
		sd->refindexlist[sds_ref] = index;

		// Handle dimensions with original dimension names
		for (int dimindex = 0; dimindex < field->rank; dimindex++) {
			int dimid = SDgetdimid (sds_id, dimindex);

			if (dimid == FAIL) {
				SDendaccess (sds_id);
				throw5 ("SDgetdimid failed ", "SDS name ", sds_name,
						"dim index= ", dimindex);
			}
			status =
				SDdiminfo (dimid, dim_name, &dim_size, &dim_type,
						   &num_dim_attrs);
			if (status == FAIL) {
				SDendaccess (sds_id);
				throw5 ("SDdiminfo failed ", "SDS name ", sds_name,
						"dim index= ", dimindex);
			}

			// No dimension attribute has been found in NASA files, so don't handle it now. KY 2010-06-08
			string dim_name_str (dim_name);

			// Since dim_size will be 0 if the dimension is unlimited dimension, so use dim_sizes instead
			Dimension *dim =
				new Dimension (dim_name_str, dim_sizes[dimindex], dim_type);
			field->dims.push_back (dim);
		}

		// Handle SDS attributes
		for (int attrindex = 0; attrindex < n_sds_attrs; attrindex++) {
			Attribute *attr = new Attribute ();

			status =
				SDattrinfo (sds_id, attrindex, attr_name, &attr->type,
							&attr_value_count);
			if (status == FAIL) {
				SDendaccess (sds_id);
				throw3 ("SDattrinfo failed ", "SDS name ", sds_name);
			}
			string tempname (attr_name);

			attr->name = tempname;
			attr->count = attr_value_count;
			attr->value.resize (attr_value_count * DFKNTsize (attr->type));
			if (SDreadattr (sds_id, attrindex, &attr->value[0]) == -1) {
				SDendaccess (sds_id);
				throw5 ("read SDS attribute failed ", "Field name ",
						field->name, " Attribute name ", attr->name);
			}
			field->attrs.push_back (attr);
		}
		SDendaccess (sds_id);

		sd->sdfields.push_back (field);
	}
	// Handle SD attributes
	for (int attrindex = 0; attrindex < n_sd_attrs; attrindex++) {
		Attribute *attr = new Attribute ();

		status =
			SDattrinfo (sdfd, attrindex, attr_name, &attr->type,
						&attr_value_count);
		if (status == FAIL)
			throw3 ("SDattrinfo failed ", "SD id ", sdfd);
		std::string tempname (attr_name);
		attr->name = tempname;
		attr->count = attr_value_count;
		attr->value.resize (attr_value_count * DFKNTsize (attr->type));
		if (SDreadattr (sdfd, attrindex, &attr->value[0]) == -1)
			throw3 ("Cannot read SD attribute", " Attribute name ",
					attr->name);
		sd->attrs.push_back (attr);
	}
	return sd;

}

// Read SDS information from the HDF4 file
SD *
SD::Read_Hybrid (int32 sdfd, int32 fileid)
throw (Exception)
{
	int32 status;
	int32 n_sds, n_sd_attrs;
	int index;
        int32 sds_index;
	int32 sd_id,sds_id;
	int32 dim_sizes[H4_MAX_VAR_DIMS];
	int32 sds_rank, sds_dtype, n_sds_attrs;
	char sds_name[H4_MAX_NC_NAME];
	char dim_name[H4_MAX_NC_NAME];
	char attr_name[H4_MAX_NC_NAME];
	int32 dim_size, sds_ref, dim_type, num_dim_attrs;
	int32 attr_value_count;

// TO OBTAIN the full path of the SDS objects 
       	intn status_n;				// returned status for functions returning an intn  
	int32 vgroup_id;			// returned status for functions returning an int32 
	int32 lone_vg_number,		// lone vgroup index 
	  num_of_lones = -1;		// number of lone vgroups 
	int32 *ref_array;			// buffer to hold the ref numbers of lone vgroups   
	uint16 name_len;
	int32 num_gobjects;
	int32 obj_ref, obj_tag;
	int i;
	int32 n_attrs;
	int32 cvgroup_id;

#if 0
	char *vdata_name = NULL;
	char *vdata_class = NULL;
#endif
	char vgroup_name[VGNAMELENMAX];
	char vgroup_class[VGNAMELENMAX];

	//std::string full_path;
	char *full_path;
	char *cfull_path;
// END OF EDITING

	SD *sd = new SD (sdfd, fileid);

	if (SDfileinfo (sdfd, &n_sds, &n_sd_attrs) == FAIL)
		throw2 ("SDfileinfo failed ", sdfd);

	for (index = 0; index < n_sds; index++) {

        	sds_id = SDselect (sdfd, index);
		if (sds_id == FAIL) {
			// We only need to close SDS ID. SD ID will be closed when the destructor is called.
			SDendaccess (sds_id);
			throw3 ("SDselect Failed ", "SDS index ", index);
		}

		sds_ref = SDidtoref (sds_id);
		if (sds_ref == FAIL) {
			SDendaccess (sds_id);
			throw3 ("Cannot obtain SDS reference number", " SDS ID is ",
					sds_id);
		}
//cerr <<"sds_ref "<<sds_ref <<endl;

                sd->sds_ref_list.push_back(sds_ref);

                SDendaccess(sds_id);
	}

// Now we need to obtain the sds reference numbers for SDS objects that are accessed as the HDF-EOS2 grid or swath.

	// No NASA HDF4 files have the vgroup that forms a ring; so ignore this case.
	// First, call Vlone with num_of_lones set to 0 to get the number of
	// lone vgroups in the file, but not to get their reference numbers.
//cerr<<"FILE ID"<<fileid <<endl;
	num_of_lones = Vlone (fileid, NULL, 0);
	if (num_of_lones == FAIL)
		throw3 ("Fail to obtain lone vgroup number", "file id is", fileid);
//cerr<<"num_of_lones= "<<num_of_lones <<endl;

	// if there are any lone vgroups,
	if (num_of_lones > 0) {

		// use the num_of_lones returned to allocate sufficient space for the
		// buffer ref_array to hold the reference numbers of all lone vgroups,
		ref_array = (int32 *) malloc (sizeof (int32) * num_of_lones);
		if (ref_array == NULL)
			throw1 ("no enough memory to allocate buffer.");

		// and call Vlone again to retrieve the reference numbers into
		// the buffer ref_array.
		num_of_lones = Vlone (fileid, ref_array, num_of_lones);
		if (num_of_lones == FAIL) {
			free (ref_array);
			throw3 ("Cannot obtain lone vgroup reference arrays ",
					"file id is ", fileid);
		}

		for (lone_vg_number = 0; lone_vg_number < num_of_lones;
			 lone_vg_number++) {
			// Attach to the current vgroup 
			vgroup_id = Vattach (fileid, ref_array[lone_vg_number], "r");
			if (vgroup_id == FAIL) {
				free (ref_array);
				throw3 ("Vattach failed ", "Reference number is ",
						ref_array[lone_vg_number]);
			}

			status = Vgetname (vgroup_id, vgroup_name);
			if (status == FAIL) {
				Vdetach (vgroup_id);
				free (ref_array);
				throw3 ("Vgetname failed ", "vgroup_id is ", vgroup_id);
			}

			status = Vgetclass (vgroup_id, vgroup_class);
			if (status == FAIL) {
				Vdetach (vgroup_id);
				free (ref_array);
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
				continue;
			}

			num_gobjects = Vntagrefs (vgroup_id);
			if (num_gobjects < 0) {
				Vdetach (vgroup_id);
				free (ref_array);
				throw3 ("Vntagrefs failed ", "vgroup_name is ", vgroup_name);
			}

			full_path = NULL;
			full_path = (char *) malloc (MAX_FULL_PATH_LEN);
			if (full_path == NULL) {
				Vdetach (vgroup_id);
				free (ref_array);
				throw1 ("No enough memory to allocate the buffer.");
			}

			strcpy (full_path, "/");
			strcat (full_path, vgroup_name);

			cfull_path = NULL;
			cfull_path = (char *) malloc (MAX_FULL_PATH_LEN);
			if (cfull_path == NULL) {
				Vdetach (vgroup_id);
				free (ref_array);
				free (full_path);
				throw1 ("No enough memory to allocate the buffer.");
			}
			strcpy (cfull_path, full_path);

			// Loop all vgroup objects
			for (i = 0; i < num_gobjects; i++) {
				if (Vgettagref (vgroup_id, i, &obj_tag, &obj_ref) == FAIL) {
					Vdetach (vgroup_id);
					free (ref_array);
					free (full_path);
					free (cfull_path);
					throw5 ("Vgettagref failed ", "vgroup_name is ",
							vgroup_name, " reference number is ", obj_ref);
				}

				if (Visvg (vgroup_id, obj_ref) == TRUE) {
					strcpy (full_path, cfull_path);
//cerr<<"before obtain_sds_path " <<endl;
					sd->obtain_sds_path (fileid, sd_id, full_path, obj_ref);
//cerr<<"after obtain_sds_path " <<endl;
				}
                                				// These are SDS objects
				else if (obj_tag == DFTAG_NDG || obj_tag == DFTAG_SDG
						 || obj_tag == DFTAG_SD) {

                                // Here we need to check if the SDS is an EOS object by checking
                                // if the the path includes "Data Fields" or "Geolocation Fields".
                                // If the object is an EOS object, we will remove the sds reference number from the list.

                                    string temp_str = string(full_path);
//cerr<<"temp_str "<<temp_str <<endl;
                                    if((temp_str.find("Data Fields") != std::string::npos)||
					(temp_str.find("Geolocation Fields") != std::string::npos))                                   
                                        sd->sds_ref_list.remove(obj_ref);

#if 0
					// We need to obtain the SDS index; also need to store the new name(object name + full_path).
					if (this->sd->refindexlist.find (obj_ref) !=
						sd->refindexlist.end ())
						this->sd->sdfields[this->sd->refindexlist[obj_ref]]->
							newname =
							this->sd->sdfields[this->sd->
											   refindexlist[obj_ref]]->name +
							full_path;
					else {
						Vdetach (vgroup_id);
						free (ref_array);
						free (full_path);
						free (cfull_path);
						throw3 ("SDS with the reference number ", obj_ref,
								" is not found");
					}
#endif
				}
                                else;
                            }
			free (full_path);
			free (cfull_path);

			status = Vdetach (vgroup_id);
			if (status == FAIL) {
				free (ref_array);
				throw3 ("Vdetach failed ", "vgroup_name is ", vgroup_name);
			}
		}						//end of the for loop
		if (ref_array != NULL)
			free (ref_array);
	}							// end of the if loop

// End of Kent editing
// Loop through the sds reference list; now the list should only include non-EOS SDS objects.
        for(std::list<int32>::iterator sds_ref_it = sd->sds_ref_list.begin(); sds_ref_it!=sd->sds_ref_list.end();++sds_ref_it) {
//cerr<<"the rest sds reference " <<*sds_ref_it <<endl;
           sds_index = SDreftoindex(sdfd,*sds_ref_it);
           if(sds_index == FAIL) throw3("SDreftoindex Failed ","SDS reference number ", *sds_ref_it);

           SDField *field = new SDField ();
           sds_id = SDselect (sdfd, sds_index);
       	   if (sds_id == FAIL) {
			// We only need to close SDS ID. SD ID will be closed when the destructor is called.
			SDendaccess (sds_id);
			throw3 ("SDselect Failed ", "SDS index ", sds_index);
	   }

           // Obtain object name, rank, size, field type and number of SDS attributes
	   status = SDgetinfo (sds_id, sds_name, &field->rank, dim_sizes,
							&field->type, &n_sds_attrs);
//cerr <<"sds_name "<<sds_name <<endl;
//cerr<<"sds field type "<<field->type <<endl;
          if (status == FAIL) {
			SDendaccess (sds_id);
			throw2 ("SDgetinfo failed ", sds_name);
		}
		string tempname (sds_name);
		field->name = tempname;
// Mark the non-EOS objects
                tempname = Utility::get_CF_string(tempname);
		field->newname = tempname+"_"+"NONEOS";
//cerr <<"sds_new_name "<<field->newname <<endl;
		field->sdsref = *sds_ref_it;
		sd->refindexlist[*sds_ref_it] = sds_index;

		// Handle dimensions with original dimension names
		for (int dimindex = 0; dimindex < field->rank; dimindex++) {
			int dimid = SDgetdimid (sds_id, dimindex);

			if (dimid == FAIL) {
				SDendaccess (sds_id);
				throw5 ("SDgetdimid failed ", "SDS name ", sds_name,
						"dim index= ", dimindex);
			}
			status =
				SDdiminfo (dimid, dim_name, &dim_size, &dim_type,
						   &num_dim_attrs);
			if (status == FAIL) {
				SDendaccess (sds_id);
				throw5 ("SDdiminfo failed ", "SDS name ", sds_name,
						"dim index= ", dimindex);
			}

			// No dimension attribute has been found in NASA files, so don't handle it now. KY 2010-06-08
			string dim_name_str (dim_name);

			// Since dim_size will be 0 if the dimension is unlimited dimension, so use dim_sizes instead
			Dimension *dim =
				new Dimension (dim_name_str, dim_sizes[dimindex], dim_type);
			field->dims.push_back (dim);

// The corrected dims are added simply for the consistency in hdfdesc.cc, it doesn't matter
// for the added SDSes at least for now. KY 2011-2-13
// However, some dimension names have special characters. We need to remove special characters.
// Since no coordinate attributes will be provided for these extra SDSes, we don't need to
// make the dimension names consistent with other dimension names. But we need to keep an eye
// on if the units of the extra SDSes are degrees_north or degrees_east. This will make the tools
// automatically treat them as latitude or longitude. Need to double check. KY 2011-2-17

                       string cfdimname =  Utility::get_CF_string(dim_name_str);
                       Dimension *correcteddim =
                                new Dimension (cfdimname, dim_sizes[dimindex], dim_type);
                        field->correcteddims.push_back (correcteddim);
                       
		}

		// Handle SDS attributes
		for (int attrindex = 0; attrindex < n_sds_attrs; attrindex++) {
			Attribute *attr = new Attribute ();

			status =
				SDattrinfo (sds_id, attrindex, attr_name, &attr->type,
							&attr_value_count);
			if (status == FAIL) {
				SDendaccess (sds_id);
				throw3 ("SDattrinfo failed ", "SDS name ", sds_name);
			}
			string tempname (attr_name);

			attr->name = tempname;
			attr->count = attr_value_count;
			attr->value.resize (attr_value_count * DFKNTsize (attr->type));
			if (SDreadattr (sds_id, attrindex, &attr->value[0]) == -1) {
				SDendaccess (sds_id);
				throw5 ("read SDS attribute failed ", "Field name ",
						field->name, " Attribute name ", attr->name);
			}
			field->attrs.push_back (attr);
		}
		SDendaccess (sds_id);

		sd->sdfields.push_back (field);
	
        }
//cerr<<"before the end of SD Read_Hybrid "<<endl;
	return sd;
}

// Retrieve Vdata information from the HDF4 file
VDATA *
VDATA::Read (int32 vdata_id, int32 obj_ref)
throw (Exception)
{

	int32 fieldsize;
	int32 fieldtype;
	int32 fieldorder;
	char *fieldname;
	char vdata_name[VSNAMELENMAX];

	VDATA *vdata = new VDATA (vdata_id, obj_ref);

	vdata->vdref = obj_ref;

	if (VSQueryname (vdata_id, vdata_name) == FAIL)
		throw3 ("VSQueryname failed ", "vdata id is ", vdata_id);

	string vdatanamestr (vdata_name);

	vdata->name = vdatanamestr;
	vdata->newname = vdata->name;
	int32 num_field = VFnfields (vdata_id);

	if (num_field == -1)
		throw3 ("For vdata, VFnfields failed. ", "vdata name is ",
				vdata->name);

	int32 num_record = VSelts (vdata_id);

	if (num_record == -1)
		throw3 ("For vdata, VSelts failed. ", "vdata name is ", vdata->name);

	// The reason to add this flag is if the number of record is too big, the DAS table is too huge to allow some clients to work.
	// Currently if the number of record is >=10; one vdata field is mapped to a DAP variable.
	// Otherwise, it is mapped to a DAP attribute.
	if (num_record <= 10)
		vdata->TreatAsAttrFlag = true;
	else
		vdata->TreatAsAttrFlag = false;

	// Loop through all fields and save information to a vector 
	for (int i = 0; i < num_field; i++) {

		VDField *field = new VDField ();

		fieldsize = VFfieldesize (vdata_id, i);
		if (fieldsize == FAIL)
			throw5 ("For vdata field, VFfieldsize failed. ", "vdata name is ",
					vdata->name, " index is ", i);

		fieldname = VFfieldname (vdata_id, i);
		if (fieldname == NULL)
			throw5 ("For vdata field, VFfieldname failed. ", "vdata name is ",
					vdata->name, " index is ", i);

		fieldtype = VFfieldtype (vdata_id, i);
		if (fieldtype == FAIL)
			throw5 ("For vdata field, VFfieldtype failed. ", "vdata name is ",
					vdata->name, " index is ", i);

		fieldorder = VFfieldorder (vdata_id, i);
		if (fieldorder == FAIL)
			throw5 ("For vdata field, VFfieldtype failed. ", "vdata name is ",
					vdata->name, " index is ", i);

		field->name = fieldname;
		field->newname = field->name;
		field->type = fieldtype;
		field->order = fieldorder;
		field->size = fieldsize;
		field->rank = 1;
		field->numrec = num_record;

		if (vdata->getTreatAsAttrFlag () && num_record > 0) {	// Currently we only save small size vdata to attributes

			field->value.resize (num_record * fieldsize);
			if (VSseek (vdata_id, 0) == FAIL)
				throw5 ("vdata ", vdata_name, "field ", fieldname,
						" VSseek failed.");

			if (VSsetfields (vdata_id, fieldname) == FAIL)
				throw3 ("vdata field ", fieldname, " VSsetfields failed.");

			if (VSread
				(vdata_id, (uint8 *) & field->value[0], num_record,
				 FULL_INTERLACE) == FAIL)
				throw3 ("vdata field ", fieldname, " VSread failed.");

		}
		// Read field attributes
		field->ReadAttributes (vdata_id, i);
		vdata->vdfields.push_back (field);
	}

	// Read Vdata attributes
	vdata->ReadAttributes (vdata_id);
	return vdata;

}

// Read Vdata attributes and save them into vectors
void
VDATA::ReadAttributes (int32 vdata_id)
throw (Exception)
{

	int32 nattrs;
	int32 attrsize;
	int32 status;

	nattrs = VSfnattrs (vdata_id, _HDF_VDATA);
	if (nattrs == FAIL)
		throw3 ("VSfnattrs failed ", "vdata id is ", vdata_id);

	if (nattrs > 0) {
		for (int i = 0; i < nattrs; i++) {

			Attribute *attr = new Attribute ();

			status =
				VSattrinfo (vdata_id, _HDF_VDATA, i, &attr->name[0],
							&attr->type, &attr->count, &attrsize);
			if (status == FAIL)
				throw5 ("VSattrinfo failed ", "vdata id is ", vdata_id,
						" attr index is ", i);
			attr->value.resize (attrsize);
			if (VSgetattr (vdata_id, _HDF_VDATA, i, &attr->value[0]) == FAIL)
				throw5 ("VSgetattr failed ", "vdata id is ", vdata_id,
						" attr index is ", i);
			attrs.push_back (attr);
		}
	}
}


// Retrive VD field attributes from the HDF4 file.
void
VDField::ReadAttributes (int32 vdata_id, int32 fieldindex)
throw (Exception)
{

	int32 nattrs;
	int32 attrsize;
	int32 status;

	nattrs = VSfnattrs (vdata_id, fieldindex);
	if (nattrs == FAIL)
		throw5 ("VSfnattrs failed ", "vdata id is ", vdata_id,
				"Field index is ", fieldindex);

	if (nattrs > 0) {

		for (int i = 0; i < nattrs; i++) {

			Attribute *attr = new Attribute ();

			status =
				VSattrinfo (vdata_id, fieldindex, i, &attr->name[0],
							&attr->type, &attr->count, &attrsize);

			if (status == FAIL)
				throw5 ("VSattrinfo failed ", "vdata field index ",
						fieldindex, " attr index is ", i);
			attr->value.resize (attrsize);
			if (VSgetattr (vdata_id, fieldindex, i, &attr->value[0]) == FAIL)
				throw5 ("VSgetattr failed ", "vdata field index is ",
						fieldindex, " attr index is ", i);
			attrs.push_back (attr);
		}
	}

}


// The following three codes are from HDFEOS2.cc. Keep them here just for convenience. 
// Consider combining them later.
void
Utility::Split (const char *s, int len, char sep,
				std::vector < std::string > &names)
{
	names.clear ();
	for (int i = 0, j = 0; j <= len; ++j) {
		if ((j == len && len) || s[j] == sep) {
			std::string elem (s + i, j - i);
			names.push_back (elem);
			i = j + 1;
			continue;
		}
	}
}

void
Utility::Split (const char *sz, char sep, std::vector < std::string > &names)
{
	Split (sz, (int) strlen (sz), sep, names);
}

bool
Utility::ReadNamelist (const char *path,
					   int32 (*inq) (char *, char *, int32 *),
					   std::vector < std::string > &names)
{
	char *fname = const_cast < char *>(path);
	int32 bufsize;
	int numobjs;

	if ((numobjs = inq (fname, NULL, &bufsize)) == -1)
		return false;
	if (numobjs > 0) {
		std::vector < char >buffer;

		buffer.resize (bufsize + 1);
		if (inq (fname, &buffer[0], &bufsize) == -1)
			return false;
		Split (&buffer[0], bufsize, ',', names);
	}
	return true;
}

string 
Utility::get_CF_string(string s) 
{
// Test if the character belongs to letters, numbers or '_'

// Always start with _ if the first character is not a letter
    string insertString(1,'_');

    if (isdigit(s[0]))
        s.insert(0,insertString);

    for(int i=0; i < s.length(); i++)
        if(!(isalnum( s[i]) || s[i]=='_'))
            s[i]='_';

    return s;

}

void 
SD::obtain_sds_path (int32 file_id, int32 sd_id, char *full_path, int32 pobj_ref)
throw (Exception)
{

	int32 vgroup_cid;
	int32 status;
	int i, num_gobjects;
	char cvgroup_name[VGNAMELENMAX];
	char vdata_name[VSNAMELENMAX];
	char vdata_class[VSNAMELENMAX];
	char sds_name[H4_MAX_NC_NAME];
	int32 sds_index;
	int32 vdata_id, sds_id;
	int32 obj_tag, obj_ref;
	int32 sds_rank, sds_dtype, n_attrs;
	char *cfull_path;
	int32 dimsizes[H4_MAX_VAR_DIMS];

	cfull_path = (char *) malloc (MAX_FULL_PATH_LEN);
	if (cfull_path == NULL)
		throw1 ("No enough memory to allocate the buffer");

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

	strcpy (cfull_path, full_path);
	strcat (cfull_path, "/");
	strcat (cfull_path, cvgroup_name);

	for (i = 0; i < num_gobjects; i++) {

		if (Vgettagref (vgroup_cid, i, &obj_tag, &obj_ref) == FAIL) {
			Vdetach (vgroup_cid);
			free (cfull_path);
			throw3 ("Vgettagref failed ", "object index is ", i);
		}

		if (Visvg (vgroup_cid, obj_ref) == TRUE) {
			strcpy (full_path, cfull_path);
			obtain_sds_path (file_id, sd_id, full_path, obj_ref);
		}
		else if (obj_tag == DFTAG_NDG || obj_tag == DFTAG_SDG
				 || obj_tag == DFTAG_SD) {
                     // Here we need to check if the SDS is an EOS object by checking
                     // if the the path includes "Data Fields" or "Geolocation Fields".
                     // If the object is an EOS object, we will remove it from the list.

                     string temp_str = string(cfull_path);
//cerr<<"temp_str inside obtain_sds_path "<<temp_str <<endl;
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
// This code is used to obtain the full path of SDS and vdata.
// Since it uses HDF4 library a lot, we keep the C style. KY 2010-7-13
void
File::InsertOrigFieldPath_ReadVgVdata ()
throw (Exception)
{
	   /************************* Variable declaration **************************/

	intn status_n;				// returned status for functions returning an intn  
	int32 status_32,			// returned status for functions returning an int32 
	  file_id, vgroup_id;
	int32 lone_vg_number,		// lone vgroup index 
	  num_of_lones = -1;		// number of lone vgroups 
	int32 *ref_array;			// buffer to hold the ref numbers of lone vgroups   
	uint16 name_len;
	int32 num_gobjects;
	int32 obj_ref, obj_tag;
	int i;
	int32 dim_sizes[H4_MAX_VAR_DIMS];
	int32 sds_rank, sds_dtype, n_attrs;

	int32 cvgroup_id;

	char vdata_name[VSNAMELENMAX];
	char vdata_class[VSNAMELENMAX];

#if 0
	char *vdata_name = NULL;
	char *vdata_class = NULL;
#endif
	char vgroup_name[VGNAMELENMAX];
	char vgroup_class[VGNAMELENMAX];

	//std::string full_path;
	char *full_path;
	char *cfull_path;
	int32 sds_index;
	int32 vdata_id, sds_id;
	int32 sd_id;

	file_id = this->fileid;
	sd_id = this->sdfd;

	// No NASA HDF4 files have the vgroup that forms a ring; so ignore this case.
	// First, call Vlone with num_of_lones set to 0 to get the number of
	// lone vgroups in the file, but not to get their reference numbers.
	num_of_lones = Vlone (file_id, NULL, 0);
	if (num_of_lones == FAIL)
		throw3 ("Fail to obtain lone vgroup number", "file id is", file_id);

	// if there are any lone vgroups,
	if (num_of_lones > 0) {
		// use the num_of_lones returned to allocate sufficient space for the
		// buffer ref_array to hold the reference numbers of all lone vgroups,
		ref_array = (int32 *) malloc (sizeof (int32) * num_of_lones);
		if (ref_array == NULL)
			throw1 ("no enough memory to allocate buffer.");

		// and call Vlone again to retrieve the reference numbers into
		// the buffer ref_array.
		num_of_lones = Vlone (file_id, ref_array, num_of_lones);
		if (num_of_lones == FAIL) {
			free (ref_array);
			throw3 ("Cannot obtain lone vgroup reference arrays ",
					"file id is ", file_id);
		}

		for (lone_vg_number = 0; lone_vg_number < num_of_lones;
			 lone_vg_number++) {
			// Attach to the current vgroup 
			vgroup_id = Vattach (file_id, ref_array[lone_vg_number], "r");
			if (vgroup_id == FAIL) {
				free (ref_array);
				throw3 ("Vattach failed ", "Reference number is ",
						ref_array[lone_vg_number]);
			}

			status_32 = Vgetname (vgroup_id, vgroup_name);
			if (status_32 == FAIL) {
				Vdetach (vgroup_id);
				free (ref_array);
				throw3 ("Vgetname failed ", "vgroup_id is ", vgroup_id);
			}

			status_32 = Vgetclass (vgroup_id, vgroup_class);
			if (status_32 == FAIL) {
				Vdetach (vgroup_id);
				free (ref_array);
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
				continue;
			}

			num_gobjects = Vntagrefs (vgroup_id);
			if (num_gobjects < 0) {
				Vdetach (vgroup_id);
				free (ref_array);
				throw3 ("Vntagrefs failed ", "vgroup_name is ", vgroup_name);
			}

			full_path = NULL;
			full_path = (char *) malloc (MAX_FULL_PATH_LEN);
			if (full_path == NULL) {
				Vdetach (vgroup_id);
				free (ref_array);
				throw1 ("No enough memory to allocate the buffer.");
			}

			strcpy (full_path, "/");
			strcat (full_path, vgroup_name);

			cfull_path = NULL;
			cfull_path = (char *) malloc (MAX_FULL_PATH_LEN);
			if (cfull_path == NULL) {
				Vdetach (vgroup_id);
				free (ref_array);
				free (full_path);
				throw1 ("No enough memory to allocate the buffer.");
			}
			strcpy (cfull_path, full_path);

			// Loop all vgroup objects
			for (i = 0; i < num_gobjects; i++) {
				if (Vgettagref (vgroup_id, i, &obj_tag, &obj_ref) == FAIL) {
					Vdetach (vgroup_id);
					free (ref_array);
					free (full_path);
					free (cfull_path);
					throw5 ("Vgettagref failed ", "vgroup_name is ",
							vgroup_name, " reference number is ", obj_ref);
				}

				if (Visvg (vgroup_id, obj_ref) == TRUE) {
					strcpy (full_path, cfull_path);
					obtain_path (file_id, sd_id, full_path, obj_ref);
				}

				else if (Visvs (vgroup_id, obj_ref)) {

					vdata_id = VSattach (file_id, obj_ref, "r");
					if (vdata_id == FAIL) {
						Vdetach (vgroup_id);
						free (ref_array);
						free (full_path);
						free (cfull_path);
						throw5 ("VSattach failed ", "vgroup_name is ",
								vgroup_name, " reference number is ",
								obj_ref);
					}
					status_32 = VSgetname (vdata_id, vdata_name);
					if (status_32 == FAIL) {
						Vdetach (vgroup_id);
						free (ref_array);
						free (full_path);
						free (cfull_path);
						throw5 ("VSgetclass failed ", "vgroup_name is ",
								vgroup_name, " reference number is ",
								obj_ref);
					}

					status_32 = VSgetclass (vdata_id, vdata_class);
					if (status_32 == FAIL) {
						Vdetach (vgroup_id);
						free (ref_array);
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
// Since currently the information has be preserved by the handler,
// so we don't have to handle this. It needs to be reported to the
// HDF4 developer. 2010-6-25 ky
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

						status_32 = VSdetach (vdata_id);
						if (status_32 == FAIL) {
							Vdetach (vgroup_id);
							free (ref_array);
							free (full_path);
							free (cfull_path);
							throw3 ("VSdetach failed ",
									"Vdata is under vgroup ", vgroup_name);
						}

					}
					else {

						VDATA *vdataobj = VDATA::Read (vdata_id, obj_ref);

						vdataobj->newname = vdataobj->name + full_path;

						// We want to map fields of vdata with more than 10 records to DAP variables
						// and we need to add the path and vdata name to the new vdata field name
						if (!vdataobj->getTreatAsAttrFlag ()) {
							for (std::vector <
								 VDField * >::const_iterator it_vdf =
								 vdataobj->getFields ().begin ();
								 it_vdf != vdataobj->getFields ().end ();
								 it_vdf++) {

								(*it_vdf)->newname =
									"vdf_" + (*it_vdf)->name + "_" +
									vdataobj->newname;
							}
						}
						this->vds.push_back (vdataobj);

						status_32 = VSdetach (vdata_id);
						if (status_32 == FAIL) {
							Vdetach (vgroup_id);
							free (ref_array);
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
						this->sd->sdfields[this->sd->refindexlist[obj_ref]]->
							newname =
							this->sd->sdfields[this->sd->
											   refindexlist[obj_ref]]->name +
							full_path;
					else {
						Vdetach (vgroup_id);
						free (ref_array);
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

			status_32 = Vdetach (vgroup_id);
			if (status_32 == FAIL) {
				free (ref_array);
				throw3 ("Vdetach failed ", "vgroup_name is ", vgroup_name);
			}
		}						//end of the for loop
		if (ref_array != NULL)
			free (ref_array);
	}							// end of the if loop

}

// This fuction is called recursively to obtain the full path of an HDF4 object.
void
File::obtain_path (int32 file_id, int32 sd_id, char *full_path,
				   int32 pobj_ref)
throw (Exception)
{

	int32 vgroup_cid;
	int32 status;
	int i, num_gobjects;
	char cvgroup_name[VGNAMELENMAX];
	char vdata_name[VSNAMELENMAX];
	char vdata_class[VSNAMELENMAX];
	char sds_name[H4_MAX_NC_NAME];
	int32 sds_index;
	int32 vdata_id, sds_id;
	int32 obj_tag, obj_ref;
	int32 sds_rank, sds_dtype, n_attrs;
	char *cfull_path;
	int32 dimsizes[H4_MAX_VAR_DIMS];

	cfull_path = (char *) malloc (MAX_FULL_PATH_LEN);
	if (cfull_path == NULL)
		throw1 ("No enough memory to allocate the buffer");

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

	strcpy (cfull_path, full_path);
	strcat (cfull_path, "/");
	strcat (cfull_path, cvgroup_name);

	for (i = 0; i < num_gobjects; i++) {

		if (Vgettagref (vgroup_cid, i, &obj_tag, &obj_ref) == FAIL) {
			Vdetach (vgroup_cid);
			free (cfull_path);
			throw3 ("Vgettagref failed ", "object index is ", i);
		}

		if (Visvg (vgroup_cid, obj_ref) == TRUE) {
			strcpy (full_path, cfull_path);
			obtain_path (file_id, sd_id, full_path, obj_ref);
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

			if (VSisattr (vdata_id) != TRUE) {
				if (strncmp
					(vdata_class, _HDF_CHK_TBL_CLASS,
					 strlen (_HDF_CHK_TBL_CLASS))) {

					VDATA *vdataobj = VDATA::Read (vdata_id, obj_ref);

					vdataobj->newname = vdataobj->name + cfull_path;
					// We want to map fields of vdata with more than 10 records to DAP variables
					// and we need to add the path and vdata name to the new vdata field name
					if (!vdataobj->getTreatAsAttrFlag ()) {
						for (std::vector <
							 VDField * >::const_iterator it_vdf =
							 vdataobj->getFields ().begin ();
							 it_vdf != vdataobj->getFields ().end ();
							 it_vdf++) {

							(*it_vdf)->newname =
								"vdf_" + (*it_vdf)->name + "_" +
								vdataobj->newname;
						}
					}
					this->vds.push_back (vdataobj);
				}
			}
			status = VSdetach (vdata_id);
			if (status == FAIL)
				throw3 ("VSdetach failed ", "object index is ", i);
		}
		else if (obj_tag == DFTAG_NDG || obj_tag == DFTAG_SDG
				 || obj_tag == DFTAG_SD) {
			if (this->sd->refindexlist.find (obj_ref) !=
				this->sd->refindexlist.end ())
				this->sd->sdfields[this->sd->refindexlist[obj_ref]]->newname =
					this->sd->sdfields[this->sd->refindexlist[obj_ref]]->
					name + cfull_path;
			else {
				Vdetach (vgroup_cid);
				free (cfull_path);
				throw3 ("SDS with the reference number ", obj_ref,
						" is not found");;
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

// This is the main function that make the HDF SDS objects follow the CF convention.
void
File::Prepare (HE2CFShortName * sn, HE2CFShortName * sn_dim,
			   HE2CFUniqName * un, HE2CFUniqName * un_dim)
throw (Exception)
{


	File *file = this;

	// 1. Obtain the original SDS and Vdata path,
	// Start with the lone vgroup they belong to and add the path
	file->InsertOrigFieldPath_ReadVgVdata ();


	// 2. Check the SDS special type(CERES special type has been checked at the Read function)
	file->CheckSDType ();

// 3. Build Dimension name list
	// We have to assume that NASA HDF4 SDSs provide unique dimension names under each vgroup

	// Find unique dimension name list
	// Build a map from unique dimension name list to the original dimension name list
	// Don't count fakeDim ......
	// Based on the new dimension name list, we will build a coordinate field for each dimension 
	// for each product we support. If dimension scale data are found, that dimension scale data will
	// be retrieved.
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

	// Sequeeze "fakeDim" names according to fakeDim size. For example, if fakeDim1, fakeDim3, fakeDim5 all shares the same size,
	// we use one name(fakeDim1) to be the dimension name. This will reduce the number of fakeDim names. 

	for (std::vector < SDField * >::const_iterator i =
		 file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
		for (std::vector < Dimension * >::const_iterator j =
			 (*i)->getCorrectedDimensions ().begin ();
			 j != (*i)->getCorrectedDimensions ().end (); ++j) {
			if ((*j)->getName ().find ("fakeDim") != std::string::npos) {
//cerr<<"origal fakedimension name list "<<(*j)->getName() <<endl;
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

	// 4. Prepare the "coordinate variable" list for each special NASA HDF product
	switch (file->sptype) {
	case TRMML2:
		{
			file->PrepareTRMML2 ();
			break;
		}
	case TRMML3:
		{
			file->PrepareTRMML3 ();
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

#if 0
// Leave this for the time being.
// 5. Build Dimension name list
	// We have to assume that NASA HDF4 SDSs provide unique dimension names under each vgroup

	// Find unique dimension name list
	// Build a map from unique dimension name list to the original dimension name list
	// Don't count fakeDim ......
	// Based on the new dimension name list, we will build a coordinate field for each dimension 
	// for each product we support. If dimension scale data are found, that dimension scale data will
	// be retrieved.
	// The unique dimension name is the dimension name plus the full path
	// We should build a map to obtain the final coordinate fields of each field
	std::string tempdimname;
	std::pair < std::set < std::string >::iterator, bool > ret;
	std::string temppath;
	for (std::vector < SDField * >::const_iterator i =
		 file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
		temppath = (*i)->newname.substr ((*i)->name.size ());
		for (std::vector < Dimension * >::const_iterator j =
			 (*i)->getDimensions ().begin ();
			 j != (*i)->getDimensions ().end (); ++j) {
			if (file->sptype != OTHERHDF)
				tempdimname = (*j)->getName ();
			else
				tempdimname = (*j)->getName () + temppath;

			Dimension *dim =
				new Dimension (tempdimname, (*j)->getSize (),
							   (*j)->getType ());
			(*i)->correcteddims.push_back (dim);
		  if (tempdimname.find ("fakeDim") == std::string:npos) {
				ret = file->sd->fulldimnamelist.insert (tempdimname);
				// NEED TO CREATE ANOTHER LIST that  DIMTYPE is not 0 for handling dimension scale.
				// Map from unique dimension name to the original dimension name
				if (ret.second == true)
					file->sd->n1dimnamelist[tempdimname] = (*j);
			}
		}
	}
#endif


	// 5. Create the new dimension name set and the dimension name to size map.

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

	// 6. Adding the missing coordinate variables based on the corrected dimension name list
	// For some CERES products, there are so many vgroups, so there are potentially many missing fields.


	// Go through the n1dimnamelist and check the map dimcvarlist; if no dimcvarlist[dimname], then this dimension namelist must be a missing field
	// Create the missing field and insert the missing field to the SDField list. 

	for (std::map < std::string, int32 >::const_iterator i =
		 file->sd->n1dimnamelist.begin ();
		 i != file->sd->n1dimnamelist.end (); ++i) {

		if (file->sd->nonmisscvdimnamelist.find ((*i).first) == file->sd->nonmisscvdimnamelist.end ()) {	// Create a missing Z-dimension field  

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

	// We will create the final unique dimension name list(erasing special characters etc.) 
	std::string temp1name, temp2name;

	bool shorteningname = false;

	// There are many fields in CERES data(a few hundred) and the full name(with the additional path)
	// is very long. It causes Java clients choken since Java clients append names in the URL
	// To improve the performance and to make Java clients access the data, use short-name for
	// the fields. KY 2010-7-13
	if (file->sptype == CER_ES4 || file->sptype == CER_SRB
		|| file->sptype == CER_CDAY || file->sptype == CER_CGEO
		|| file->sptype == CER_SYN || file->sptype == CER_ZAVG
		|| file->sptype == CER_AVG)
		sn->set_short_name_on ();

	for (std::set < std::string >::const_iterator i =
		 file->sd->fulldimnamelist.begin ();
		 i != file->sd->fulldimnamelist.end (); ++i) {
		temp1name = (*i);
		temp1name = sn_dim->get_short_string (temp1name, &shorteningname);
		file->sd->n2dimnamelist[(*i)] = temp1name;
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
			else {				//When the dimension name is fakeDim***, we will ignore. this dimension will not have the corresponding coordinate variable. 
				throw5 ("This dimension with the name ", (*j)->name,
						"and the field name ", (*i)->name,
						" is not found in the dimension list.");
			}
		}
	}

	// 7. Check if having nameclashing for the  SDS field list; 
	bool nameclashflag = false;

	std::set < std::string > fullobjnameset;
	for (int i = 0; i < file->sd->sdfields.size (); ++i) {
		ret = fullobjnameset.insert (file->sd->sdfields[i]->newname);
		if (ret.second == false) {
			nameclashflag = true;
			break;
		}
	}


	// 8. Check the special characters and change those characters to _ for field namelist
	//    Also create dimension name to coordinate variable name list

	shorteningname = false;
	std::string tempdim1name, tempdim2name;

	bool COARDFLAG = false;

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
				temp1name = (*i)->newname;
				temp1name = sn->get_short_string (temp1name, &shorteningname);
				if (!shorteningname && nameclashflag)
					temp2name = un->get_uniq_string (temp1name);
				else
					temp2name = temp1name;

				// This is the final name in DDS
				(*i)->newname = temp2name;


				if ((*i)->getRank () > 2)
					throw3 ("the lat/lon rank should NOT be greater than 2",
							(*i)->name, (*i)->getRank ());
				else if ((*i)->getRank () == 2) {	// Each lat/lon must be 2-D under the same group.
					for (std::vector < Dimension * >::const_iterator j =
						 (*i)->getCorrectedDimensions ().begin ();
						 j != (*i)->getCorrectedDimensions ().end (); ++j) {
						tempmapit =
							file->sd->dimcvarlist.find ((*j)->getName ());
						if (tempmapit == file->sd->dimcvarlist.end ()) {
							file->sd->dimcvarlist[(*j)->name] = (*i)->newname;
							break;
						}
					}
				}

				else {			// When rank = 1, must follow COARD conventions. Here we don't check name clashing for the performance
					// reason, the chance of clashing is very,very rare.
					(*i)->newname =
						(*i)->getCorrectedDimensions ()[0]->getName ();
					file->sd->dimcvarlist[(*i)->getCorrectedDimensions ()[0]->
										  getName ()] = (*i)->newname;
					COARDFLAG = true;

				}
			}
		}
		else {
			temp1name = (*i)->newname;
			temp1name = sn->get_short_string (temp1name, &shorteningname);
			if (!shorteningname && nameclashflag)
				temp2name = un->get_uniq_string (temp1name);
			else
				temp2name = temp1name;

			// This is the final name in DDS
			(*i)->newname = temp2name;
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
			if ((*i)->fieldtype != 1 && (*i)->fieldtype != 2) {	// "Missing" coordinate variables or coordinate variables having dimensional scale data
				temp1name = (*i)->newname;
				temp1name = sn->get_short_string (temp1name, &shorteningname);
				if (!shorteningname && nameclashflag)
					temp2name = un->get_uniq_string (temp1name);
				else
					temp2name = temp1name;

				// This is the final name in DDS
				(*i)->newname = temp2name;

				if ((*i)->getRank () > 1)
					throw3 ("The lat/lon rank should be 1", (*i)->name,
							(*i)->getRank ());

                             // The current OTHERHDF case we support(MERRA and SDS dimension scale)
                             // follow COARDS conventions. Panoply fail to display the data,
                             // if we just follow CF conventions. So following COARD. KY-2011-3-4
				if (COARDFLAG || file->sptype == OTHERHDF)	//  Follow COARD Conventions
					(*i)->newname =
						(*i)->getCorrectedDimensions ()[0]->getName ();
				else
					(*i)->newname =
						(*i)->getCorrectedDimensions ()[0]->getName () + "_d";
				file->sd->dimcvarlist[(*i)->getCorrectedDimensions ()[0]->
									  getName ()] = (*i)->newname;
			}
		}
	}


	// 9. Generate "coordinates " attribute
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
							"must have corresponding coordinate variable.");
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

	// 10. Handle vdata, only need to check name clashings and special characters for vdata names 
	// No need to check name clashings for vdata fields, but need to check special characters

	// Check name clashings, we also need to include SDS field names.
	nameclashflag = false;
	fullobjnameset.clear ();
	for (int i = 0; i < file->sd->sdfields.size (); ++i) {
		ret = fullobjnameset.insert (file->sd->sdfields[i]->newname);
	}

	for (std::vector < VDATA * >::const_iterator i = file->vds.begin ();
		 i != file->vds.end (); ++i) {
		for (std::vector < VDField * >::const_iterator j =
			 (*i)->getFields ().begin (); j != (*i)->getFields ().end ();
			 ++j) {
			ret = fullobjnameset.insert ((*j)->newname);
			if (ret.second == false) {
				nameclashflag = true;
				break;
			}
		}
	}
	// Shorteningname flag will be determined by the get_short_string function
	shorteningname = false;
	for (std::vector < VDATA * >::const_iterator i = file->vds.begin ();
		 i != file->vds.end (); ++i) {
		temp1name = (*i)->newname;
		temp1name = sn->get_short_string (temp1name, &shorteningname);

		if (!shorteningname && nameclashflag)
			temp2name = un->get_uniq_string (temp1name);
		else
			temp2name = temp1name;
		// This is the final name in DDS for this vdata
		(*i)->newname = temp2name;

		// We also need to handle special characters for vdata field names
		for (std::vector < VDField * >::const_iterator j =
			 (*i)->vdfields.begin (); j != (*i)->vdfields.end (); ++j) {
			temp1name = (*j)->newname;
			temp1name = sn->get_short_string (temp1name, &shorteningname);
			if (!shorteningname && nameclashflag)
				temp2name = un->get_uniq_string (temp1name);
			else
				temp2name = temp1name;
			// This is the final name in DDS for this vdata
			(*j)->newname = temp2name;
		}
	}

}

/// Special method to prepare TRMM Level 2 latitude and longitude information.
/// Latitude and longitude are stored in one array(geolocation). Need to separate.
void
File::PrepareTRMML2 ()
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
			latitude->sdsref = (*i)->sdsref;
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
			longitude->sdsref = (*i)->sdsref;
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
						(*k)->name = tempdimsizenamelist[(*k)->getSize ()];	// Change the dimension name
				}
			}
		}
	}

	file->sd->sdfields.push_back (latitude);
	file->sd->sdfields.push_back (longitude);

	// 3. Remove the geolocation field from the field list
	SDField *origeo;

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
	delete (origeo);



	// 4. Create the <dimname,coordinate variable> map from the corresponding dimension names to the latitude and the longitude
	file->sd->nonmisscvdimnamelist.insert (tempnewdimname1);
	file->sd->nonmisscvdimnamelist.insert (tempnewdimname2);

}

// Prepare TRMM Level 3, no lat/lon are in the original HDF4 file. Need to provide them.
void
File::PrepareTRMML3 ()
throw (Exception)
{

	std::string tempdimname1, tempdimname2;
	std::string tempnewdimname1, tempnewdimname2;
	int32 tempdimsize1, tempdimsize2;
	int latflag = 0;
	int lonflag = 0;

	std::string temppath;
	SDField *latitude;
	SDField *longitude;
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
				if ((*k)->getSize () == 1440 && (*k)->getType () == 0) {	//No dimension scale

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
					//                std::string uniquedimname = (*k)->getName() +temppath;
					//                tempnewdimname1 = uniquedimname;
					//               dim = new Dimension(uniquedimname,(*k)->getSize(),(*i)->getType());
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
			break;				// For this case, a field that needs lon and lot must exist
		// Need to reset the flag to avoid false alarm. For TRMM L3 we can handle, a field that has dimension 
		// which size is 400 and 1440 must exist in the file.
		latflag = 0;
		lonflag = 0;
	}

	if (latflag != 1 || lonflag != 1)
		throw5 ("Either latitude or longitude doesn't exist.", "lat. flag= ",
				latflag, "lon. flag= ", lonflag);
	file->sd->sdfields.push_back (latitude);
	file->sd->sdfields.push_back (longitude);


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

	int pixels_per_scan_line;

	std::string pixels_per_scan_line_name = "Pixels per Scan Line";
	std::string number_pixels_control_points =
		"Number of Pixel Control Points";
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

// 1. Obtain the global attribute for calculating latitude 
// and longitude. 
//   std::string lon_step = "Longitude Step";
//   std::string lat_step = "Latitude Step";
	std::string num_lat_name = "Number of Lines";
	std::string num_lon_name = "Number of Columns";
//   std::string swlat   = "SW Point Latitude";
//   std::string swlon   = "SW Point Longitude";
	int32 num_lat, num_lon;

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
	dim = new Dimension (num_lat_name, num_lat, 0);
	latitude->dims.push_back (dim);

	// Add the corrected dimension name only to be consistent with general handling of other cases.
	dim = new Dimension (num_lat_name, num_lat, 0);
	latitude->correcteddims.push_back (dim);

	// The dimension names of the SDS are fakeDim, so need to change them to dimension names of latitude and longitude
	for (std::vector < SDField * >::const_iterator i =
		 file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
		if ((*i)->getRank () != 2)
			throw3 ("The lat/lon rank must be 2", (*i)->getName (),
					(*i)->getRank ());
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
	int eraseflag = 0;

	std::vector < SDField * >::iterator beerasedit;

	SDField *beerased;

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
			else {				//remove the redundant Colatitude field
				delete (*i);
				file->sd->sdfields.erase (i);
				// When erasing the iterator, the iterator will automatically go to the next element, so we need to go back 1 in order not to miss the next element.
				i--;
			}
		}

		else if (((*i)->getName ()).find ("Longitude") != std::string::npos) {
			if (!lonflag) {
				lonflag = true;
				(*i)->fieldtype = 2;
				tempcvarname2 = (*i)->getName ();
			}
			else {				//remove the redundant Longitude field
				delete (*i);
				file->sd->sdfields.erase (i);
				// When erasing the iterator, the iterator will automatically go to the next element, so we need to go back 1 in order not to miss the next element.
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
	int tempdimsize1, tempdimsize2;

	std::string tempcvarname1, tempcvarname2;
	std::string temppath;
	std::set < std::string > tempdimnameset;
	std::pair < std::set < std::string >::iterator, bool > tempsetit;
	bool eraseflag = false;
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

			if (cvflag) {		// change the dimension from 3-D to 1-D.
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
			else {				//delete this element from the vector and erase it.
				delete (*i);
				file->sd->sdfields.erase (i);

				// When erasing the iterator, the iterator will automatically go to the next element, so we need to go back 1 in order not to miss the next element.
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
			else {				//delete this element from the vector and erase it.
				delete (*i);
				file->sd->sdfields.erase (i);
				// When erasing the iterator, the iterator will automatically go to the next element, so we need to go back 1 in order not to miss the next element.
				i--;
			}
		}

	}
}


// CERES SAVG and CERES ISCCP-IDAY cases.
// We need provide nested CERES grid 2-D lat/lon.
// The lat and lon should be calculated following:
// http://eosweb.larc.nasa.gov/PRODOCS/ceres/SRBAVG/Quality_Summaries/srbavg_ed2d/nestedgrid.html
// The dimension names and sizes are set according to the studies of these files.
void
File::PrepareCERSAVGID ()
throw (Exception)
{

	bool colatflag = false;
	bool lonflag = false;

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

			// Need to change those dimension names to be the same as lat/lon so that a coordinate variable dimension name map can be built.
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

	for (std::vector < SDField * >::const_iterator i =
		 file->sd->sdfields.begin (); i != file->sd->sdfields.end (); ++i) {
		for (std::vector < Dimension * >::const_iterator j =
			 (*i)->getCorrectedDimensions ().begin ();
			 j != (*i)->getCorrectedDimensions ().end (); ++j) {

			if ((*j)->getType () != 0) {
				if ((*i)->name == (*j)->getName () && (*i)->getRank () == 1)
					(*i)->fieldtype = 3;
				file->sd->nonmisscvdimnamelist.insert ((*j)->getName ());
			}
		}
	}
}
