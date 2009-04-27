#ifdef USE_HDFEOS2
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

namespace HDFEOS2
{

class Exception : public std::exception
{
public:
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
		// do not call constructor for each element
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

	const char * peek() const;
	virtual const char * get() = 0;
	virtual void drop() = 0;
	virtual int length() const = 0;

protected:
	bool valid;
	LightVector<char> data;
};

class SwathDimensionAdjustment;

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

class Field
{
public:
	virtual ~Field();

public:
	const std::string & getName() const { return this->name; }
	int32 getRank() const { return this->rank; }
	int32 getType() const { return this->type; }
	const std::vector<Dimension *> & getDimensions() const { return this->dims; }
	FieldData & getData() const { return *this->data; }
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

class Dataset
{
public:
	const std::string & getName() const { return this->name; }
	const std::vector<Dimension *> & getDimensions() const { return this->dims; }
	const std::vector<Field *> & getDataFields() const { return this->datafields; }
	const std::vector<Attribute *> & getAttributes() const { return this->attrs; }

protected:
	Dataset(const std::string &n)
		: datasetid(-1), name(n)
	{
	}

	virtual ~Dataset();

	void ReadDimensions(int32 (*entries)(int32, int32, int32 *), int32 (*inq)(int32, char *, int32 *), std::vector<Dimension *> &dims) throw(Exception);
	void ReadFields(int32 (*entries)(int32, int32, int32 *), int32 (*inq)(int32, char *, int32 *, int32 *), intn (*fldinfo)(int32, char *, int32 *, int32 *, int32 *, char *), intn (*readfld)(int32, char *, int32 *, int32 *, int32 *, VOIDP), intn (*getfill)(int32, char *, VOIDP), bool geofield, std::vector<Field *> &fields) throw(Exception);
	void ReadAttributes(int32 (*inq)(int32, char *, int32 *), intn (*attrinfo)(int32, char *, int32 *, int32 *), intn (*readattr)(int32, char *, VOIDP), std::vector<Attribute *> &attrs) throw(Exception);

protected:
	int32 datasetid;
	std::string name;
	std::vector<Dimension *> dims;
	std::vector<Field *> datafields;
	std::vector<Attribute *> attrs;
};

class GridDataset : public Dataset
{
public:
	class Info
	{
	public:
		int32 getX() const { return this->xdim; }
		int32 getY() const { return this->ydim; }
		const float64 * getUpLeft() const { return this->upleft; }
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
		int32 getCode() const { return this->code; }
		int32 getZone() const { return this->zone; }
		int32 getSphere() const { return this->sphere; }
		const float64 * getParam() const { return this->param; }
		int32 getPix() const { return this->pix; }
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

	class Calculated
	{
	public:
		const float64 * peekLongitude() const;
		const float64 * getLongitude() throw(Exception);
		const float64 * peekLatitude() const;
		const float64 * getLatitude() throw(Exception);
		void dropLongitudeLatitude();

		bool isYDimMajor() throw(Exception);
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

		void DetectMajorDimension() throw(Exception);
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
	static GridDataset * Read(int32 fd, const std::string &gridname) throw(Exception);
	virtual ~GridDataset();

	const Info & getInfo() const { return this->info; }
	const Projection & getProjection() const { return this->proj; }
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

class SwathDataset : public Dataset
{
public:
	class DimensionMap
	{
	public:
		const std::string & getGeoDimension() const { return this->geodim; }
		const std::string & getDataDimension() const { return this->datadim; }
		int32 getOffset() const { return this->offset; }
		int32 getIncrement() const { return this->increment; }

	protected:
		DimensionMap(const std::string &geodim, const std::string &datadim, int32 offset, int32 increment)
			: geodim(geodim), datadim(datadim), offset(offset), increment(increment)
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

	class IndexMap
	{
	public:
		const std::string & getGeoDimension() const { return this->geo; }
		const std::string & getDataDimension() const { return this->data; }
		const LightVector<int32> & getIndices() const { return this->indices; }

	protected:
		std::string geo;
		std::string data;
		LightVector<int32> indices;

		friend class SwathDataset;
	};

public:
	static SwathDataset * Read(int32 fd, const std::string &swathname) throw(Exception);
	virtual ~SwathDataset();

	const std::vector<DimensionMap *> & getDimensionMaps() const { return this->dimmaps; }
	const std::vector<IndexMap *> & getIndexMaps() const { return this->indexmaps; }
	const std::vector<Field *> & getGeoFields() const { return this->geofields; }
	const std::vector<std::pair<Field *, std::string> > & getAdjustedGeoFields() const { return this->adjustedgeofields; }

	void GetAssociatedGeoFields(const Field &datafield, std::vector<std::pair<std::string, std::string> > &associated) const;

private:
	SwathDataset(const std::string &name)
		: Dataset(name)
	{
	}

	void OverrideGeoFields();

	void ReadDimensionMaps(std::vector<DimensionMap *> &dimmaps) throw(Exception);
	void ReadIndexMaps(std::vector<IndexMap *> &indexmaps) throw(Exception);

protected:
	std::vector<DimensionMap *> dimmaps;
	std::vector<IndexMap *> indexmaps;
	mutable std::vector<Field *> allgeofields;

	std::vector<Field *> geofields;
	mutable std::vector<std::pair<Field *, std::string> > adjustedgeofields;
	std::vector<Field *> replacedgeofields;

	friend class SwathDimensionAdjustment;
	friend class File;
};

class PointDataset : public Dataset
{
public:
	static PointDataset * Read(int32 fd, const std::string &pointname) throw(Exception);
	virtual ~PointDataset();

private:
	PointDataset(const std::string &name)
		: Dataset(name)
	{
	}
};

class UnadjustedFieldData : public FieldData
{
private:
	UnadjustedFieldData(int32 id, const std::string &name, int32 len, intn (*readfld)(int32, char *, int32 *, int32 *, int32 *, VOIDP))
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
	AdjustedFieldData(const SwathDataset *swath, const Field *basegeofield, const std::vector<Map> &dimmaps, const std::vector<Dimension *> &datadims);

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
		int leftpoint;
		int rightpoint;
		int leftdist;
		int rightdist;
	};

	void Adjust(std::vector<AdjustInfo> &adjustinfo);
	void FinishOnePoint(const std::vector<AdjustInfo> &adjustinfo);
	T AccumulatePivots(const std::vector<AdjustInfo> &adjustinfo, std::vector<int> &pivots);
	int GetGeoIndexFromDataIndexLE(int dataindex, const Map &dimmap);
	int GetGeoIndexFromDataIndex(int dataindex, const Map &dimmap);
	int GetDataIndexFromGeoIndex(int geoindex, const Map &dimmap);

	friend class SwathDimensionAdjustment;
};

class File
{
public:
	static File * Read(const char *path) throw(Exception);
	static File * ReadAndAdjust(const char *path) throw(Exception);

	virtual ~File();

	const std::string & getPath() const { return this->path; }
	const std::vector<GridDataset *> & getGrids() const { return this->grids; }
	const std::vector<SwathDataset *> & getSwaths() const { return this->swaths; }
	const std::vector<PointDataset *> & getPoints() const { return this->points; }

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
	static void Split(const char *s, int len, char sep, std::vector<std::string> &names);
	static void Split(const char *sz, char sep, std::vector<std::string> &names);
	static bool ReadNamelist(const char *path, int32 (*inq)(char *, char *, int32 *), std::vector<std::string> &names);
};

}
#endif
// vim:ts=4:sw=4:sts=4


#endif
