
// This file is part of the GDAL OPeNDAP Adapter

// Copyright (c) 2004 OPeNDAP, Inc.
// Author: Frank Warmerdam <warmerdam@pobox.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

//#define DODS_DEBUG 1

#include <iostream>
#include <sstream>
#include <string>

#include <gdal.h>

#include <DDS.h>
#include <DAS.h>
#include <debug.h>

#include "GDALTypes.h"

using namespace libdap;

/************************************************************************/
/*                          read_descriptors()                          */
/************************************************************************/

void gdal_read_dataset_variables(DDS *dds, GDALDatasetH &hDS, const string &filename)
{
/* -------------------------------------------------------------------- */
/*      Create the basic matrix for each band.                          */
/* -------------------------------------------------------------------- */
    GDALDataType eBufType;

    for( int iBand = 0; iBand < GDALGetRasterCount( hDS ); iBand++ )
    {
        DBG(cerr << "In dgal_dds.cc  iBand" << endl);

        GDALRasterBandH hBand = GDALGetRasterBand( hDS, iBand+1 );

        ostringstream oss;
        oss << "band_" << iBand+1;

        eBufType = GDALGetRasterDataType( hBand );

        BaseType *bt;
        switch( GDALGetRasterDataType( hBand ) )
        {
          case GDT_Byte:
            bt = new Byte( oss.str() );
            break;

          case GDT_UInt16:
            bt = new UInt16( oss.str() );
            break;

          case GDT_Int16:
            bt = new Int16( oss.str() );
            break;

          case GDT_UInt32:
            bt = new UInt32( oss.str() );
            break;

          case GDT_Int32:
            bt = new Int32( oss.str() );
            break;

          case GDT_Float32:
            bt = new Float32( oss.str() );
            break;

          case GDT_Float64:
            bt = new Float64( oss.str() );
            break;

          case GDT_CFloat32:
          case GDT_CFloat64:
          case GDT_CInt16:
          case GDT_CInt32:
          default:
            // TODO eventually we need to preserve complex info
            bt = new Float64( oss.str() );
            eBufType = GDT_Float64;
            break;
        }

/* -------------------------------------------------------------------- */
/*      Create a grid to hold the raster.                               */
/* -------------------------------------------------------------------- */
        Grid *grid;
        grid = new GDALGrid( filename, oss.str(), hBand, eBufType );

/* -------------------------------------------------------------------- */
/*      Make into an Array for the raster data with appropriate         */
/*      dimensions.                                                     */
/* -------------------------------------------------------------------- */
        Array *ar;
        // A 'feature' of Array is that it copies the variable passed to
        // its ctor. To get around that, pass null and use add_var_nocopy().
        // Modified for the DAP4 response; switched to this new ctor.
        ar = new GDALArray( oss.str(), 0, filename, hBand, eBufType );
        ar->add_var_nocopy( bt );
        ar->append_dim( GDALGetRasterYSize( hDS ), "northing" );
        ar->append_dim( GDALGetRasterXSize( hDS ), "easting" );

        grid->add_var_nocopy( ar, libdap::array );

/* -------------------------------------------------------------------- */
/*      Add the dimension map arrays.                                   */
/* -------------------------------------------------------------------- */
        bt = new GDALFloat64( "northing" );
        ar = new GDALArray( "northing", 0, filename, hBand, eBufType );
        ar->add_var_nocopy( bt );
        ar->append_dim( GDALGetRasterYSize( hDS ), "northing" );

        grid->add_var_nocopy( ar, maps );

        bt = new GDALFloat64( "easting" );
        ar = new GDALArray( "easting", 0, filename, hBand, eBufType );
        ar->add_var_nocopy( bt );
        ar->append_dim( GDALGetRasterXSize( hDS ), "easting" );

        grid->add_var_nocopy( ar, maps );

        DBG(cerr << "Type of grid: " << typeid(grid).name() << endl);

        dds->add_var_nocopy( grid );
    }
}

/**
 * Read the data array of a DAP2 Grid. This is called by both GDALGrid::read()
 * (for a DAP2 response) and GDALArray::read() when we're building a DAP4
 * data response. The result is that the Array object holds data that can then
 * be serialized.
 *
 * @note I split up the original GDALGrid::read() code that Frank Warmerdam
 * wrote into two functions because I needed to be able to read the data when
 * building DAP4 responses. There is no Grid type in DAP4.
 *
 * @see read_map_array()
 *
 * @param array
 */
void read_data_array(GDALArray *array, GDALRasterBandH hBand, GDALDataType eBufType) {
	/* -------------------------------------------------------------------- */
	/*      Collect the x and y sampling values from the constraint.        */
	/* -------------------------------------------------------------------- */
	Array::Dim_iter p = array->dim_begin();
	int start = array->dimension_start(p, true);
	int stride = array->dimension_stride(p, true);
	int stop = array->dimension_stop(p, true);

    // Test for the case where a dimension has not been subset. jhrg 2/18/16
    if (array->dimension_size(p, true) == 0) { //default rows
        start = 0;
        stride = 1;
        stop = GDALGetRasterBandYSize(hBand) - 1;
    }

	p++;
	int start_2 = array->dimension_start(p, true);
	int stride_2 = array->dimension_stride(p, true);
	int stop_2 = array->dimension_stop(p, true);

	if (array->dimension_size(p, true) == 0) { //default columns
        start_2 = 0;
        stride_2 = 1;
        stop_2 = GDALGetRasterBandXSize(hBand) - 1;
    }

	/* -------------------------------------------------------------------- */
	/*      Build a window and buf size from this.                          */
	/* -------------------------------------------------------------------- */
	int nWinXOff, nWinYOff, nWinXSize, nWinYSize, nBufXSize, nBufYSize;

	nWinXOff = start_2;
	nWinYOff = start;
	nWinXSize = stop_2 + 1 - start_2;
	nWinYSize = stop + 1 - start;

	nBufXSize = (stop_2 - start_2) / stride_2 + 1;
	nBufYSize = (stop - start) / stride + 1;

	/* -------------------------------------------------------------------- */
	/*      Allocate buffer.                                                */
	/* -------------------------------------------------------------------- */
	int nPixelSize = GDALGetDataTypeSize(eBufType) / 8;
	vector<char> pData(nBufXSize * nBufYSize * nPixelSize);

	/* -------------------------------------------------------------------- */
	/*      Read request into buffer.                                       */
	/* -------------------------------------------------------------------- */
	CPLErr eErr = GDALRasterIO(hBand, GF_Read, nWinXOff, nWinYOff, nWinXSize, nWinYSize,
			&pData[0], nBufXSize, nBufYSize, eBufType, 0, 0);
	if (eErr != CE_None)
		throw Error("Error reading: " + array->name());

	array->val2buf(&pData[0]);
}

/**
 * Read one of the Map arrays. This uses the kludge that the DDS/DMR calls these
 * two arrays 'northing' and 'easting'.
 *
 * @param map
 * @param hBand
 * @param filename
 */
void read_map_array(Array *map, GDALRasterBandH hBand, string filename)
{
	Array::Dim_iter p = map->dim_begin();
	int start = map->dimension_start(p, true);
	int stride = map->dimension_stride(p, true);
	int stop = map->dimension_stop(p, true);

	if (start + stop + stride == 0) { //default rows
		start = 0;
		stride = 1;
		if (map->name() == "northing")
			stop = GDALGetRasterBandYSize(hBand) - 1;
		else if (map->name() == "easting")
			stop = GDALGetRasterBandXSize(hBand) - 1;
		else
			throw Error("Expected a map named 'northing' or 'easting' but got: " + map->name());
	}

	int nBufSize = (stop - start) / stride + 1;

	/* -------------------------------------------------------------------- */
	/*      Read or default the geotransform used to generate the           */
	/*      georeferencing maps.                                            */
	/* -------------------------------------------------------------------- */

	// Move this into the gdal_dds.cc code so that it store this in the
	// Grid or maybe in the GDALDDS instance? Then we can avoid a second
	// open/read operation on the file. jhrg
	GDALDatasetH hDS;

	hDS = GDALOpen(filename.c_str(), GA_ReadOnly);

	if (hDS == NULL) throw Error(string(CPLGetLastErrorMsg()));

	double adfGeoTransform[6];

	if (GDALGetGeoTransform(hDS, adfGeoTransform) != CE_None) {
		adfGeoTransform[0] = 0.0;
		adfGeoTransform[1] = 1.0;
		adfGeoTransform[2] = 0.0;
		adfGeoTransform[3] = 0.0;
		adfGeoTransform[4] = 0.0;
		adfGeoTransform[5] = 1.0;
	}

	GDALClose(hDS);

	/* -------------------------------------------------------------------- */
	/*      Set the map array.                                              */
	/* -------------------------------------------------------------------- */
	vector<double> padfMap(nBufSize);

	if (map->name() == "northing") {
		for (int i = 0, iLine = start; iLine <= stop; iLine += stride) {
			padfMap[i++] = adfGeoTransform[3] + adfGeoTransform[5] * iLine;
		}
	}
	else if (map->name() == "easting") {
		for (int i = 0, iPixel = start; iPixel <= stop; iPixel += stride) {
			padfMap[i++] = adfGeoTransform[0] + iPixel * adfGeoTransform[1];
		}
	}
	else
		throw Error("Expected a map named 'northing' or 'easting' but got: " + map->name());

	map->val2buf((void *) &padfMap[0]);
}
