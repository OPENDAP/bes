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

#include <iostream>
#include <sstream>
#include <string>

#include <gdal.h>
#include <cpl_string.h>

//#define DODS_DEBUG 1

#include <Byte.h>
#include <UInt16.h>
#include <Int16.h>
#include <UInt32.h>
#include <Int32.h>
#include <Float32.h>
#include <Float64.h>

#include <DDS.h>
#include <DAS.h>
#include <BaseTypeFactory.h>
#include <debug.h>

#include "GDALTypes.h"

using namespace libdap;

/************************************************************************/
/*                        attach_str_attr_item()                        */
/*                                                                      */
/*      Add a string attribute item to target container with            */
/*      appropriate quoting and escaping.                               */
/************************************************************************/

static void attach_str_attr_item(AttrTable *parent_table, const char *pszKey, const char *pszValue)
{
    //string oQuotedValue;
    char *pszEscapedText = CPLEscapeString(pszValue, -1,
    CPLES_BackslashQuotable);

    parent_table->append_attr(pszKey, "String", pszEscapedText /*oQuotedValue*/);

    CPLFree(pszEscapedText);
}

/************************************************************************/
/*                         translate_metadata()                         */
/*                                                                      */
/*      Turn a list of metadata name/value pairs into DAS into and      */
/*      attach it to the passed container.                              */
/************************************************************************/

static void translate_metadata(char **md, AttrTable *parent_table)
{
    AttrTable *md_table;
    int i;

    md_table = parent_table->append_container(string("Metadata"));

    for (i = 0; md != NULL && md[i] != NULL; i++) {
        const char *pszValue;
        char *pszKey = NULL;

        pszValue = CPLParseNameValue(md[i], &pszKey);

        attach_str_attr_item(md_table, pszKey, pszValue);

        CPLFree(pszKey);
    }
}

static void build_global_attributes(const GDALDatasetH& hDS, AttrTable* attr_table)
{
    /* -------------------------------------------------------------------- */
    /*      Geotransform                                                    */
    /* -------------------------------------------------------------------- */
    double adfGeoTransform[6];
    if (GDALGetGeoTransform(hDS, adfGeoTransform) == CE_None
        && (adfGeoTransform[0] != 0.0 || adfGeoTransform[1] != 1.0 || adfGeoTransform[2] != 0.0
            || adfGeoTransform[3] != 0.0 || adfGeoTransform[4] != 0.0 || fabs(adfGeoTransform[5]) != 1.0)) {

        char szGeoTransform[200];
        double dfMaxX, dfMinX, dfMaxY, dfMinY;
        int nXSize = GDALGetRasterXSize(hDS);
        int nYSize = GDALGetRasterYSize(hDS);

        dfMaxX =
            MAX(MAX(adfGeoTransform[0], adfGeoTransform[0] + adfGeoTransform[1] * nXSize),
                MAX(adfGeoTransform[0] + adfGeoTransform[2] * nYSize, adfGeoTransform[0] + adfGeoTransform[2] * nYSize + adfGeoTransform[1] * nXSize));
        dfMinX =
            MIN(MIN(adfGeoTransform[0], adfGeoTransform[0] + adfGeoTransform[1] * nXSize),
                MIN(adfGeoTransform[0] + adfGeoTransform[2] * nYSize, adfGeoTransform[0] + adfGeoTransform[2] * nYSize + adfGeoTransform[1] * nXSize));
        dfMaxY =
            MAX(MAX(adfGeoTransform[3], adfGeoTransform[3] + adfGeoTransform[4] * nXSize),
                MAX(adfGeoTransform[3] + adfGeoTransform[5] * nYSize, adfGeoTransform[3] + adfGeoTransform[5] * nYSize + adfGeoTransform[4] * nXSize));
        dfMinY =
            MIN(MIN(adfGeoTransform[3], adfGeoTransform[3] + adfGeoTransform[4] * nXSize),
                MIN(adfGeoTransform[3] + adfGeoTransform[5] * nYSize, adfGeoTransform[3] + adfGeoTransform[5] * nYSize + adfGeoTransform[4] * nXSize));

        attr_table->append_attr("Northernmost_Northing", "Float64", CPLSPrintf("%.16g", dfMaxY));
        attr_table->append_attr("Southernmost_Northing", "Float64", CPLSPrintf("%.16g", dfMinY));
        attr_table->append_attr("Easternmost_Easting", "Float64", CPLSPrintf("%.16g", dfMaxX));
        attr_table->append_attr("Westernmost_Northing", "Float64", CPLSPrintf("%.16g", dfMinX));

        snprintf(szGeoTransform, 200, "%.16g %.16g %.16g %.16g %.16g %.16g", adfGeoTransform[0], adfGeoTransform[1],
            adfGeoTransform[2], adfGeoTransform[3], adfGeoTransform[4], adfGeoTransform[5]);

        attach_str_attr_item(attr_table, "GeoTransform", szGeoTransform);
    }

    /* -------------------------------------------------------------------- */
    /*      Metadata.                                                       */
    /* -------------------------------------------------------------------- */
    char** md;
    md = GDALGetMetadata(hDS, NULL);
    if (md != NULL) translate_metadata(md, attr_table);

    /* -------------------------------------------------------------------- */
    /*      SRS                                                             */
    /* -------------------------------------------------------------------- */
    const char* pszWKT = GDALGetProjectionRef(hDS);
    if (pszWKT != NULL && strlen(pszWKT) > 0) attach_str_attr_item(attr_table, "spatial_ref", pszWKT);
}

static void build_variable_attributes(const GDALDatasetH &hDS, AttrTable *band_attr, const int iBand)
{
    /* -------------------------------------------------------------------- */
    /*      Offset.                                                         */
    /* -------------------------------------------------------------------- */
    int bSuccess;
    double dfValue;
    char szValue[128];
    GDALRasterBandH hBand = GDALGetRasterBand(hDS, iBand + 1);

    dfValue = GDALGetRasterOffset(hBand, &bSuccess);
    if (bSuccess) {
        snprintf(szValue, 128, "%.16g", dfValue);
        band_attr->append_attr("add_offset", "Float64", szValue);
    }

    /* -------------------------------------------------------------------- */
    /*      Scale                                                           */
    /* -------------------------------------------------------------------- */
    dfValue = GDALGetRasterScale(hBand, &bSuccess);
    if (bSuccess) {
        snprintf(szValue, 128, "%.16g", dfValue);
        band_attr->append_attr("scale_factor", "Float64", szValue);
    }

    /* -------------------------------------------------------------------- */
    /*      nodata/missing_value                                            */
    /* -------------------------------------------------------------------- */
    dfValue = GDALGetRasterNoDataValue(hBand, &bSuccess);
    if (bSuccess) {
        snprintf(szValue, 128, "%.16g", dfValue);
        band_attr->append_attr("missing_value", "Float64", szValue);
    }

    /* -------------------------------------------------------------------- */
    /*      Description.                                                    */
    /* -------------------------------------------------------------------- */
    if (GDALGetDescription(hBand) != NULL && strlen(GDALGetDescription(hBand)) > 0) {
        attach_str_attr_item(band_attr, "Description", GDALGetDescription(hBand));
    }

    /* -------------------------------------------------------------------- */
    /*      PhotometricInterpretation.                                      */
    /* -------------------------------------------------------------------- */
    if (GDALGetRasterColorInterpretation(hBand) != GCI_Undefined) {
        attach_str_attr_item(band_attr, "PhotometricInterpretation",
            GDALGetColorInterpretationName(GDALGetRasterColorInterpretation(hBand)));
    }

    /* -------------------------------------------------------------------- */
    /*      Band Metadata.                                                  */
    /* -------------------------------------------------------------------- */
    char **md = GDALGetMetadata(hBand, NULL);
    if (md != NULL) translate_metadata(md, band_attr);

    /* -------------------------------------------------------------------- */
    /*      Colormap.                                                       */
    /* -------------------------------------------------------------------- */
    GDALColorTableH hCT;

    hCT = GDALGetRasterColorTable(hBand);
    if (hCT != NULL) {
        AttrTable *ct_attr;
        int iColor, nColorCount = GDALGetColorEntryCount(hCT);

        ct_attr = band_attr->append_container(string("Colormap"));

        for (iColor = 0; iColor < nColorCount; iColor++) {
            GDALColorEntry sRGB;
            AttrTable *color_attr;

            color_attr = ct_attr->append_container(string(CPLSPrintf("color_%d", iColor)));

            GDALGetColorEntryAsRGB(hCT, iColor, &sRGB);

            color_attr->append_attr("red", "byte", CPLSPrintf("%d", sRGB.c1));
            color_attr->append_attr("green", "byte", CPLSPrintf("%d", sRGB.c2));
            color_attr->append_attr("blue", "byte", CPLSPrintf("%d", sRGB.c3));
            color_attr->append_attr("alpha", "byte", CPLSPrintf("%d", sRGB.c4));
        }
    }
}

/**
 * Build a DAP2 DAS object for this dataset. This function enables the handler
 * to build just the DAS without the overhead of building the DDS. It uses
 * helper functions the the DDS builder can use as well.
 *
 * @note A design trade-off is that we have some extra code for the 'just a DAS'
 * case. We could code this handler so that it only builds DDS objects (for DAP2,
 * of course) and returns the DAS from that. But, that will make the DAS more
 * intensive to build and might require changes elsewhere in the BES.
 *
 * @param das A value-result parameter
 * @param hDS
 */
void gdal_read_dataset_attributes(DAS &das, const GDALDatasetH &hDS)
{
    AttrTable *attr_table = das.add_table(string("GLOBAL"), new AttrTable);

    build_global_attributes(hDS, attr_table);

    /* ==================================================================== */
    /*      Generation info for bands.                                      */
    /* ==================================================================== */
    for (int iBand = 0; iBand < GDALGetRasterCount(hDS); iBand++) {
        char szName[128];
        AttrTable *band_attr;

        /* -------------------------------------------------------------------- */
        /*      Create container named after the band.                          */
        /* -------------------------------------------------------------------- */
        snprintf(szName, 128, "band_%d", iBand + 1);
        band_attr = das.add_table(string(szName), new AttrTable);

        build_variable_attributes(hDS, band_attr, iBand);
#if 0
        /* -------------------------------------------------------------------- */
        /*      Offset.                                                         */
        /* -------------------------------------------------------------------- */
        int bSuccess;
        double dfValue;
        char szValue[128];
        GDALRasterBandH hBand = GDALGetRasterBand( hDS, iBand+1 );

        dfValue = GDALGetRasterOffset( hBand, &bSuccess );
        if( bSuccess )
        {
            snprintf( szValue, 128, "%.16g", dfValue );
            band_attr->append_attr( "add_offset", "Float64", szValue );
        }

        /* -------------------------------------------------------------------- */
        /*      Scale                                                           */
        /* -------------------------------------------------------------------- */
        dfValue = GDALGetRasterScale( hBand, &bSuccess );
        if( bSuccess )
        {
            snprintf( szValue, 128, "%.16g", dfValue );
            band_attr->append_attr( "scale_factor", "Float64", szValue );
        }

        /* -------------------------------------------------------------------- */
        /*      nodata/missing_value                                            */
        /* -------------------------------------------------------------------- */
        dfValue = GDALGetRasterNoDataValue( hBand, &bSuccess );
        if( bSuccess )
        {
            snprintf( szValue, 128, "%.16g", dfValue );
            band_attr->append_attr( "missing_value", "Float64", szValue );
        }

        /* -------------------------------------------------------------------- */
        /*      Description.                                                    */
        /* -------------------------------------------------------------------- */
        if( GDALGetDescription( hBand ) != NULL
            && strlen(GDALGetDescription( hBand )) > 0 )
        {
            attach_str_attr_item( band_attr,
                "Description",
                GDALGetDescription( hBand ) );
        }

        /* -------------------------------------------------------------------- */
        /*      PhotometricInterpretation.                                      */
        /* -------------------------------------------------------------------- */
        if( GDALGetRasterColorInterpretation( hBand ) != GCI_Undefined )
        {
            attach_str_attr_item(
                band_attr, "PhotometricInterpretation",
                GDALGetColorInterpretationName(
                    GDALGetRasterColorInterpretation(hBand) ) );
        }

        /* -------------------------------------------------------------------- */
        /*      Band Metadata.                                                  */
        /* -------------------------------------------------------------------- */
        char **md = GDALGetMetadata( hBand, NULL );
        if( md != NULL )
        translate_metadata( md, band_attr );

        /* -------------------------------------------------------------------- */
        /*      Colormap.                                                       */
        /* -------------------------------------------------------------------- */
        GDALColorTableH hCT;

        hCT = GDALGetRasterColorTable( hBand );
        if( hCT != NULL )
        {
            AttrTable *ct_attr;
            int iColor, nColorCount = GDALGetColorEntryCount( hCT );

            ct_attr = band_attr->append_container( string( "Colormap" ) );

            for( iColor = 0; iColor < nColorCount; iColor++ )
            {
                GDALColorEntry sRGB;
                AttrTable *color_attr;

                color_attr = ct_attr->append_container(
                    string( CPLSPrintf( "color_%d", iColor ) ) );

                GDALGetColorEntryAsRGB( hCT, iColor, &sRGB );

                color_attr->append_attr( "red", "byte",
                    CPLSPrintf( "%d", sRGB.c1 ) );
                color_attr->append_attr( "green", "byte",
                    CPLSPrintf( "%d", sRGB.c2 ) );
                color_attr->append_attr( "blue", "byte",
                    CPLSPrintf( "%d", sRGB.c3 ) );
                color_attr->append_attr( "alpha", "byte",
                    CPLSPrintf( "%d", sRGB.c4 ) );
            }
        }
#endif
    }
}

/**
 * Build the DDS object for this dataset.
 * @param dds
 * @param hDS
 * @param filename
 */
void gdal_read_dataset_variables(DDS *dds, const GDALDatasetH &hDS, const string &filename)
{
    BaseTypeFactory factory;    // Use this to build new scalar variables

    /* -------------------------------------------------------------------- */
    /*      Create the basic matrix for each band.                          */
    /* -------------------------------------------------------------------- */
    GDALDataType eBufType;

    for (int iBand = 0; iBand < GDALGetRasterCount(hDS); iBand++) {
        DBG(cerr << "In dgal_dds.cc  iBand" << endl);

        GDALRasterBandH hBand = GDALGetRasterBand(hDS, iBand + 1);

        ostringstream oss;
        oss << "band_" << iBand + 1;

        eBufType = GDALGetRasterDataType(hBand);

        BaseType *bt;
        switch (GDALGetRasterDataType(hBand)) {
        case GDT_Byte:
            bt = factory.NewByte(oss.str());
            break;

        case GDT_UInt16:
            bt = factory.NewUInt16(oss.str());
            break;

        case GDT_Int16:
            bt = factory.NewInt16(oss.str());
            break;

        case GDT_UInt32:
            bt = factory.NewUInt32(oss.str());
            break;

        case GDT_Int32:
            bt = factory.NewInt32(oss.str());
            break;

        case GDT_Float32:
            bt = factory.NewFloat32(oss.str());
            break;

        case GDT_Float64:
            bt = factory.NewFloat64(oss.str());
            break;

        case GDT_CFloat32:
        case GDT_CFloat64:
        case GDT_CInt16:
        case GDT_CInt32:
        default:
            // TODO eventually we need to preserve complex info
            bt = factory.NewFloat64(oss.str());
            eBufType = GDT_Float64;
            break;
        }

        /* -------------------------------------------------------------------- */
        /*      Create a grid to hold the raster.                               */
        /* -------------------------------------------------------------------- */
        Grid *grid;
        grid = new GDALGrid(filename, oss.str());

        /* -------------------------------------------------------------------- */
        /*      Make into an Array for the raster data with appropriate         */
        /*      dimensions.                                                     */
        /* -------------------------------------------------------------------- */
        Array *ar;
        // A 'feature' of Array is that it copies the variable passed to
        // its ctor. To get around that, pass null and use add_var_nocopy().
        // Modified for the DAP4 response; switched to this new ctor.
        ar = new GDALArray(oss.str(), 0, filename, eBufType, iBand + 1);
        ar->add_var_nocopy(bt);
        ar->append_dim(GDALGetRasterYSize(hDS), "northing");
        ar->append_dim(GDALGetRasterXSize(hDS), "easting");

        grid->add_var_nocopy(ar, libdap::array);

        /* -------------------------------------------------------------------- */
        /*      Add the dimension map arrays.                                   */
        /* -------------------------------------------------------------------- */
        //bt = new GDALFloat64( "northing" );
        bt = factory.NewFloat64("northing");
        ar = new GDALArray("northing", 0, filename, eBufType, iBand + 1);
        ar->add_var_nocopy(bt);
        ar->append_dim(GDALGetRasterYSize(hDS), "northing");

        grid->add_var_nocopy(ar, maps);

        //bt = new GDALFloat64( "easting" );
        bt = factory.NewFloat64("easting");
        ar = new GDALArray("easting", 0, filename, eBufType, iBand + 1);
        ar->add_var_nocopy(bt);
        ar->append_dim(GDALGetRasterXSize(hDS), "easting");

        grid->add_var_nocopy(ar, maps);

        DBG(cerr << "Type of grid: " << typeid(grid).name() << endl);

        dds->add_var_nocopy(grid);
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
void read_data_array(GDALArray *array, const GDALRasterBandH &hBand)
{
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
    int nPixelSize = GDALGetDataTypeSize(array->get_gdal_buf_type()) / 8;
    vector<char> pData(nBufXSize * nBufYSize * nPixelSize);

    /* -------------------------------------------------------------------- */
    /*      Read request into buffer.                                       */
    /* -------------------------------------------------------------------- */
    CPLErr eErr = GDALRasterIO(hBand, GF_Read, nWinXOff, nWinYOff, nWinXSize, nWinYSize, &pData[0], nBufXSize,
        nBufYSize, array->get_gdal_buf_type(), 0, 0);
    if (eErr != CE_None) throw Error("Error reading: " + array->name());

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
void read_map_array(Array *map, const GDALRasterBandH &hBand, const GDALDatasetH &hDS)
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

    double adfGeoTransform[6];

    if (GDALGetGeoTransform(hDS, adfGeoTransform) != CE_None) {
        adfGeoTransform[0] = 0.0;
        adfGeoTransform[1] = 1.0;
        adfGeoTransform[2] = 0.0;
        adfGeoTransform[3] = 0.0;
        adfGeoTransform[4] = 0.0;
        adfGeoTransform[5] = 1.0;
    }

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

// $Log: gdal_das.cc,v $
// Revision 1.1  2004/10/19 20:38:28  warmerda
// New
//
// Revision 1.2  2004/10/15 18:06:45  warmerda
// Strip the extension off the filename.
//
// Revision 1.1  2004/10/04 14:29:29  warmerda
// New
//

