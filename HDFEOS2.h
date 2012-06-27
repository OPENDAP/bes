/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf4 data handler for the OPeNDAP data server.
//
// Author:   Kent Yang,Choonghwan Lee <myang6@hdfgroup.org>
// Copyright (c) 2010 The HDF Group
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

// class that creates the unique name etc
#include "HE2CFNcML.h"
#include "HE2CFShortName.h"
#include "HE2CF.h"

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
///
///
/// @author Kent Yang, Choonghwan Lee <myang6@hdfgroup.org>
///
/// Copyright (C) 2010 The HDF Group
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

		virtual ~ Exception () throw ()
		{
		}

		virtual const char *what () const throw ()
		{
			return this->message.c_str ();
		}

		virtual bool getFileType ()
		{
			return this->isHDFEOS2;
		}

		virtual void setFileType (bool isHDFEOS2)
		{
			this->isHDFEOS2 = isHDFEOS2;
		}

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

	/// The base class of unadjusted FieldData and adjusted FieldData.
	class FieldData
	{
	protected:
		FieldData ()
		:valid (false)
		{
		}

	public:
		virtual ~ FieldData ()
		{
		}

		/// Peek the value, it will return NULL if this class doesn't hold
		/// the data.
		const char *peek () const;

		/// It will get the value even if the value is not in the FieldData
		/// buffer(LightVector). 
		virtual const char *get (int *offset, int *step, int *count, int nelms) =
			0;

		/// Release the buffer by resizing the LightVector. 
		virtual void drop () = 0;
		virtual int length () const = 0;
		virtual int dtypesize () const = 0;

	protected:
		bool valid;
		LightVector < char >data;
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
		:fieldtype (0), condenseddim (false), iscoard (false), ydimmajor (true),
		speciallon (false), specialcoard(false), specialformat (0), haveaddedfv (false),
		addedfv (-9999.0), dmap (false)
		{
		}
		virtual ~ Field ();

	public:

		/// Get the name of this field
		const std::string & getName () const
		{
			return this->name;
		}

		/// Get the name of this field for the third dimension field to match the special COARD request
		const std::string & getName_specialcoard () const
		{
			return this->oriname;
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

		const float getAddedFillValue () const
		{
			return this->addedfv;
		}
		// Add fill value
		void addFillValue (float fv)
		{
			addedfv = fv;
		}

		const bool haveAddedFillValue () const
		{
			return this->haveaddedfv;
		}
		// set the flag for the added FillValue
		void setAddedFillValue (bool havefv)
		{
			haveaddedfv = havefv;
		}

		const int getFieldType () const
		{
			return this->fieldtype;
		}

		/// Get the list of dimensions
		const std::vector < Dimension * >&getDimensions () const
		{
			return this->dims;
		}

		/// Return an instance of FieldData, which refers to this field.
		/// Then one can obtain the data array from this instance.
		FieldData & getData () const
		{
			return *this->data;
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

		/// Get special COARD flag that may change the field name  
		const bool getSpecialCoard () const
		{
			return this->specialcoard;
		}

		/// Set and get the special flag for adjustment
		void set_adjustment (int num_map)
		{
			need_adjustment = (num_map != 0);
		}
		bool get_adjustment ()
		{
			return need_adjustment;
		}

		/// Set field type: existing coordinate variable or added coordinate variable 
		// void set_fieldtype(int flag) { fieldtype = flag;}
	protected:
		std::string name;
		int32 rank;
		int32 type;

		std::vector < Dimension * >dims;
		std::vector < Dimension * >correcteddims;
		FieldData *data;
		std::vector < char >filler;

		std::string coordinates;
		std::string newname;

		std::string oriname;

		// This flag will specify the fieldtype.
		// 0 means this field is general field.
		// 1 means this field is lat.
		// 2 means this field is lon.
		// 3 means this field is other dimension variable.
		// 4 means this field is added other dimension variable with nature number.
		int fieldtype;
		bool condenseddim;
		bool iscoard;
		bool ydimmajor;
		bool speciallon;

		// To make IDV and Panoply work, the third dimension field name needs to
		// be different than the dimension name. So we have to remember the
		// original dimension field name when retrieving the third dimension data.
		// This flag is used to detect that.
		bool specialcoard;

		// This flag specifies the special latitude/longitude coordinate format
		// 0 means normal 
		// 1 means the coordinate is -180 to 180
		// 2 means the coordinate is default(0)
		int specialformat;

		std::string units;
		bool haveaddedfv;
		float addedfv;
		bool dmap;

		/// Add a special_flag to indicate if the field data needs to be adjusted(dimension map case)
		/// KY 2009-12-3
		bool need_adjustment;

		friend class Dataset;
		friend class SwathDimensionAdjustment;
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
		const std::string & getName () const
		{
			return this->name;
		}
		int32 getType () const
		{
			return this->type;
		}
		const std::vector < char >&getValue () const
		{
			return this->value;
		}

	protected:
		std::string name;
		int32 type;
		std::vector < char >value;

		friend class Dataset;
	};

	/// Base class of Grid and Swath Dataset. It provides public methods
	/// to obtain name, number of dimensions, number of data fields and
	/// attributes.
	class Dataset
	{
	public:
		const std::string & getName () const
		{
			return this->name;
		}
		const std::vector < Dimension * >&getDimensions () const
		{
			return this->dims;
		}
		const std::vector < Field * >&getDataFields () const
		{
			return this->datafields;
		}
		const std::vector < Attribute * >&getAttributes () const
		{
			return this->attrs;
		}

	protected:
		Dataset (const std::string & n)
		: datasetid (-1), name (n)
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

	protected:
		int32 datasetid;
		std::string name;
		std::vector < Dimension * >dims;
		std::vector < Field * >datafields;
		std::vector < Attribute * >attrs;
		std::map < std::string, std::string > dimcvarlist;

		std::map < std::string, std::string > ncvarnamelist;
		std::map < std::string, std::string > ndimnamelist;
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
			const float64 *peekLongitude () const;
			const float64 *getLongitude () throw (Exception);
			const float64 *peekLatitude () const;
			const float64 *getLatitude () throw (Exception);
			void dropLongitudeLatitude ();

			/// We follow C-major convention. Normally YDim is major.
			/// Sometimes it is not. .
			bool isYDimMajor () throw (Exception);
			/// The projection can be either 1-D or 2-D. For 1-D, the method
			/// returns true. Otherwise, return false.
			bool isOrthogonal () throw (Exception);

		protected:

			Calculated (const GridDataset * grid)
			: grid (grid), valid (false), ydimmajor (false), orthogonal (false)
			{
			}

			Calculated & operator= (const Calculated & victim)
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
			void DetectMajorDimension () throw (Exception);

			/// Find a field and check which dimension is major for this field. If Y dimension is major, return 1; if X dimension is major, return 0, otherwise throw exception. (LD -2012/01/16)
			int DetectFieldMajorDimension () throw (Exception);


			///  This method will detect if this projection is 1-D or 2-D.
			/// For 1-D, it is treated as "orthogonal".
			void DetectOrthogonality () throw (Exception);
			void ReadLongitudeLatitude () throw (Exception);

		protected:
			const GridDataset *grid;

			bool valid;

			LightVector < float64 > lons;
			LightVector < float64 > lats;
			bool ydimmajor;
			bool orthogonal;

			friend class GridDataset;
			friend class File;
		};

	public:
		/// Read all information regarding this Grid.
		static GridDataset *Read (int32 fd, const std::string & gridname)
		throw (Exception);

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
		Info info;
		Projection proj;
		mutable Calculated calculated;
		bool ownllflag;
		bool iscoard;
		Field *latfield;
		Field *lonfield;

		std::string dimxname;
		std::string dimyname;

		friend class File;

	};

	class File;

	/// This class retrieves and holds all information of an EOS swath.
	/// In addition to retrieve the general data field information.
	/// This class especially handles the calculation of longitude and
	/// latitude of EOS Swath. 
	/// Unlike retrieving latitude and longitude from EOS grid, extra efforts
	/// need to be made to retrieve geo-location fields when dimension map is
	/// used. 

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
			DimensionMap (const std::string & geodim, const std::string & datadim,
					int32 offset, int32 increment)
			: geodim (geodim), datadim (datadim), offset (offset),
			increment (increment)
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
		/// swath files.
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
		static SwathDataset *Read (int32 fd, const std::string & swathname)
		throw (Exception);

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
		int ReadDimensionMaps (std::vector < DimensionMap * >&dimmaps)
		throw (Exception);
		void ReadIndexMaps (std::vector < IndexMap * >&indexmaps)
		throw (Exception);

	protected:
		std::vector < DimensionMap * >dimmaps;
		std::vector < IndexMap * >indexmaps;

		std::set < std::string > nonmisscvdimlist;

		/// The geo-location fields that serves both as "input" and "output".
		/// Applications will obtain geo-location fields stored by this vector.
		/// Elements of this vector should be adjusted geo-location fields if
		/// OverrideGeoFields is called and a dimension map exists and is used.
		/// Otherwise, elements of this vector are unadjusted geo-location
		/// fields.
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
		static PointDataset *Read (int32 fd, const std::string & pointname)
		throw (Exception);
		virtual ~ PointDataset ();

	private:
		PointDataset (const std::string & name)
		: Dataset (name)
		{
		}
	};

	/// Interface for users to access elements of any original field data
	/// (including both data fields and geolocation fields)
	/// Please note that for Grid, there are no geo-location fields, so this
	/// class won't retrieve geo-location fields for Grid. 
	class UnadjustedFieldData:public FieldData
	{
	private:
		UnadjustedFieldData (int32 id, const std::string & name, int32 len,
				int typesize, intn (*readfld) (int32, char *,
						int32 *, int32 *,
						int32 *, VOIDP))
		: datasetid (id), fieldname (name), datatypesize (typesize), datalen (len),
		reader (readfld)
		{
		}

	public:
		virtual const char *get (int *offset, int *step, int *count, int nelms);
		virtual void drop ();
		virtual int length () const;
		virtual int dtypesize () const;

	protected:
		int32 datasetid;
		std::string fieldname;
		int datatypesize;
		int32 datalen;

		intn (*reader) (int32, char *, int32 *, int32 *, int32 *, VOIDP);

		friend class Dataset;
	};

	/// Interface for users to access elements of missing field data
	/// The data can be calculated or input by the user.
	/// The purpose is to provide  the missing third dimension
	/// coordinate variable data.

	class MissingFieldData:public FieldData
	{
	private:

	public:

		MissingFieldData (int rank, int typesize, int *dimsize,
				const LightVector < char >&inputdata)

		: rank (rank), datatypesize (typesize), dims (dimsize),
		inputdata (inputdata)
		{
		}

		virtual const char *get (int *offset, int *step, int *count, int nelms);
		virtual void drop ();
		virtual int length () const;
		virtual int dtypesize () const;

	protected:
		int rank;
		int datatypesize;
		int datalen;
		int *dims;
		LightVector < char >inputdata;

	};

	/// This class retrieves all information from an EOS file. It is a
	/// container for Grid, Swath and Point objects.
	class File
	{
	public:
		static File *Read (const char *path) throw (Exception);

		/// It will read all information from an EOS file with the adjusted
		/// geo-location fields of swath objects.
		static File *ReadAndAdjust (const char *path) throw (Exception);

		/// Read and prepare. This is the main method to make the DAP output CF-compliant.
		/// All dimension(coordinate variables) information need to be ready.
		/// All special arrangements need to be done in this step.

		void Prepare (const char *path, HE2CFShortName * sn,
				HE2CFShortName * sn_dim, HE2CFUniqName * un,
				HE2CFUniqName * un_dim) throw (Exception);
		bool getOneLatLon ()
		{
			return this->onelatlon;
		}

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

	protected:
		File (const char *path)
		: path (path), onelatlon (false), iscoard (false), gridfd (-1), swathfd (-1)
		{
		}

	protected:
		std::string path;
		std::vector < GridDataset * >grids;
		std::vector < SwathDataset * >swaths;
		std::vector < PointDataset * >points;

		/**
		 * Checker for dimension name clashing
		 * This function checks dimension name clashing.
		 * Basically, it gathers all dimension names of
		 * all grids and swaths and checks whether there is
		 * a collision or not. Note that, if llclash is true,
		 * some dimensions such as
		 * XDim and YDim are excluded for checking. -Eunsoo
		 * @param llclash If true, XDim and YDim are not checked.
		 * @return True if there is a dimension name clashing.
		 */
		bool check_dim_name_clashing (bool llclash) const;

		/**
		 * Checker for field name clashing
		 * Within each grid or swath, we check all fields and
		 * dimensions. If there is a dimension d such that
		 * there is no 1-D data field f whose dimension is d,
		 * we call all such dimensions NF-dimensions
		 * (which means NoField).
		 * We first check, for each grid of swath,
		 * whether there is a name collision between
		 * the field names and NF-dimension names.
		 * Then we check whether there is a collision among
		 * field names of all grids/swaths and NF-dimension names
		 * of them. - Eunsoo

		 * For swath,

		 I have to get Latitude and Longitude from geofield and
		 get their dims -> these should not be checked for
		 name clashing. ***

		 * If dimension map is used,
		 unexpanded dimensions should not be used.
		 also expanded dimensions should not be used.

		 * @param bUseDim If true, do as described. If false, we do not 
		 * 	generate NF dimesnsions, but treat all dimensions as NF-dimensions.
		 * @return True if there is a field name clashing.
		 */
		bool check_field_name_clashing (bool bUseDimNameMatching) const;

		// This is for the case that only one lat/lon
		// set is provided for multiple grid cases.
		//  The current case is that the user provides
		// the latitude and longitude under a special grid.
		// By default, the grid name is "location". This will
		// cover the AIRS grid case.
		bool iscoard;
		bool onelatlon;

		std::string gridname;
		std::string origlatname;
		std::string origlonname;
		std::string latname;
		std::string lonname;
		int llrank; // latitude and longitude rank
		int lltype; // latitude and longitude type
		bool llcondensed; // If 2-D lat and lon array  can be condensed to 1-D

		// Eunsoo
		/** 
		 * A grid's X-dimension can have different names: XDim, LatDim, etc.
		 * Y-dimension also has YDim, LonDim, etc.
		 * This function returns the name of X-dimension which is used in
		 * the given file.
		 * For better performance, we check the first grid or swath only.
		 */
		std::string get_geodim_x_name ();
		std::string get_geodim_y_name ();
		// Internal funcion and variables for the above functions.
		// These are not intended to be used outside the above functions.
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
		// Internal funcion and variables for the above functions.
		// These are not intended to be used outside the above functions.
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
		// Internal funcion and variables for the above function.
		// These are not intended to be used outside the above function.
		void _find_geogrid_name ();

		std::string _geogrid_name;
		static const char *_geogrid_names[];

	private:
		int32 gridfd;
		int32 swathfd;
	};

	struct Utility
	{

		/// From a string separated by a separator to a list of string,
		/// for example, split "ab,c" to {"ab","c"}
		static void Split (const char *s, int len, char sep,
				std::vector < std::string > &names);

		/// Assume sz is Null terminated string.
		static void Split (const char *sz, char sep,
				std::vector < std::string > &names);

		/// Call inquiry functions twice to get a list of strings. 
		static bool ReadNamelist (const char *path,
				int32 (*inq) (char *, char *, int32 *),
				std::vector < std::string > &names);

		/// Return number of objects for other purposes

	};

}
#endif

#endif
