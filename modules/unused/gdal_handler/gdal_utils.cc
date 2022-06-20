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

#include <libdap/Byte.h>
#include <libdap/UInt16.h>
#include <libdap/Int16.h>
#include <libdap/UInt32.h>
#include <libdap/Int32.h>
#include <libdap/Float32.h>
#include <libdap/Float64.h>

#include <libdap/DDS.h>
#include <libdap/DAS.h>
#include <libdap/BaseTypeFactory.h>

#include <libdap/DMR.h>
#include <libdap/D4Group.h>
#include <libdap/D4Dimensions.h>
#include <libdap/D4Maps.h>
#include <libdap/D4Attributes.h>
#include <libdap/D4BaseTypeFactory.h>
#include <libdap/debug.h>

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

/**
 * @brief Build the global attributes for this data set.
 *
 * @param hDS
 * @param attr_table A value-result parameter
 */
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
#if 0
        // Gareth Williams pointed out this typo. jhrg 9/26/19
        attr_table->append_attr("Westernmost_Northing", "Float64", CPLSPrintf("%.16g", dfMinX));
#endif
        attr_table->append_attr("Westernmost_Easting", "Float64", CPLSPrintf("%.16g", dfMinX));

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

/**
 * @brief Build the attributes for a given band/variable.
 *
 * @param hDS
 * @param band_attr a value-result parameter
 * @param iBand
 */
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
    }
}

/**
 * Build the DDS object for this dataset.
 *
 * @note The GDAL data model doc: http://www.gdal.org/gdal_datamodel.html
 *
 * @param dds A value-result parameter
 * @param hDS
 * @param filename
 * @param include_attrs 
 */
void gdal_read_dataset_variables(DDS *dds, const GDALDatasetH &hDS, const string &filename,bool include_attrs)
{
    // Load in to global attributes
    if(true == include_attrs) {
        AttrTable *global_attr = dds->get_attr_table().append_container("GLOBAL");
        build_global_attributes(hDS, global_attr);
    }

    /* -------------------------------------------------------------------- */
    /*      Create the basic matrix for each band.                          */
    /* -------------------------------------------------------------------- */
    BaseTypeFactory factory;    // Use this to build new scalar variables
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
        ar = new GDALArray("northing", 0, filename, GDT_Float64/* eBufType */, iBand + 1);
        ar->add_var_nocopy(bt);
        ar->append_dim(GDALGetRasterYSize(hDS), "northing");

        grid->add_var_nocopy(ar, maps);

        //bt = new GDALFloat64( "easting" );
        bt = factory.NewFloat64("easting");
        ar = new GDALArray("easting", 0, filename, GDT_Float64/* eBufType */, iBand + 1);
        ar->add_var_nocopy(bt);
        ar->append_dim(GDALGetRasterXSize(hDS), "easting");

        grid->add_var_nocopy(ar, maps);

        DBG(cerr << "Type of grid: " << typeid(grid).name() << endl);

        // Add attributes to the Grid
        if(true == include_attrs) {
            AttrTable &band_attr = grid->get_attr_table();
            build_variable_attributes(hDS, &band_attr, iBand);
        }

        dds->add_var_nocopy(grid);
    }
}

/**
 * @brief Build the DMR
 *
 * This function builds the DMR without first building a DDS. It does use
 * the AttrTable object, though, to cut down on duplication of code in the
 * functions that build attributes.
 *
 * @note The GDAL data model doc: http://www.gdal.org/gdal_datamodel.html
 *
 * @param dmr A value-result parameter
 * @param hDS
 * @param filename
 */
void gdal_read_dataset_variables(DMR *dmr, const GDALDatasetH &hDS, const string &filename)
{
    // Load in global attributes. Hack, use DAP2 attributes and transform.
    // This is easier than writing new D4Attributes code and not a heuristic
    // routine like the variable transforms or attribute to DDS merge. New
    // code for the D4Attributes would duplicate the AttrTable code.
    //
    // An extra AttrTable is needed because of oddities of its API but that
    // comes in handy since D4Attributes::transform_to_dap4() throws away
    // the top most AttrTable

    AttrTable *attr = new AttrTable();
    AttrTable *global_attr = attr->append_container("GLOBAL");

    build_global_attributes(hDS, global_attr);

    dmr->root()->attributes()->transform_to_dap4(*attr);
    delete attr; attr = 0;

    D4BaseTypeFactory factory;    // Use this to build new variables

    // Make the northing and easting shared dims for this gdal dataset.
    // For bands that have a different size than the overall Raster{X,Y}Size,
    // assume they are lower resolution bands. For these I'm going to simply
    // not include the shared dimensions for them. If this is a problem,
    // we can expand the set of dimensions later.
    //
    // See the GDAL data model doc (http://www.gdal.org/gdal_datamodel.html)
    // for info on why this seems correct. jhrg 6/2/17

    // Find the first band that is the same size as the whole raster dataset (i.e.,
    // the first band that is not at a reduced resolution). In the larger loop
    // below we only bind sdims to bands that match the overall raster size and
    // leave bands that are at a reduce resolution w/o shared dims.
    int sdim_band_num = 1;
    for (; sdim_band_num <= GDALGetRasterCount(hDS); ++sdim_band_num) {
        if (GDALGetRasterBandYSize(GDALGetRasterBand(hDS, sdim_band_num)) == GDALGetRasterYSize(hDS)
            && GDALGetRasterBandXSize(GDALGetRasterBand(hDS, sdim_band_num)) == GDALGetRasterXSize(hDS)) {
            break;
        }
    }

    // Make the two shared dims that will have the geo-referencing info
    const string northing = "northing";
    // Add the shared dim
    D4Dimension *northing_dim = new D4Dimension(northing, GDALGetRasterYSize(hDS));
    dmr->root()->dims()->add_dim_nocopy(northing_dim);
    // use the shared dim for the map
    Array *northing_map = new GDALArray(northing, 0, filename, GDT_Float64, sdim_band_num);
    northing_map->add_var_nocopy(factory.NewFloat64(northing));
    northing_map->append_dim(northing_dim);
    // add the map
    dmr->root()->add_var_nocopy(northing_map);   // Add the map array to the DMR/D4Group

    const string easting = "easting";
    D4Dimension *easting_dim = new D4Dimension(easting, GDALGetRasterXSize(hDS));
    dmr->root()->dims()->add_dim_nocopy(easting_dim);
    Array *easting_map = new GDALArray(easting, 0, filename, GDT_Float64, sdim_band_num);
    easting_map->add_var_nocopy(factory.NewFloat64(easting));
    easting_map->append_dim(easting_dim);
    dmr->root()->add_var_nocopy(easting_map);   // Add the map array to the DMR/D4Group

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
            // Replace this with an attribute about a missing/elided variable?
            bt = factory.NewFloat64(oss.str());
            eBufType = GDT_Float64;
            break;
        }

        // Make the array for this band and then associate the dimensions/maps
        // once they are made;
        Array *ar = new GDALArray(oss.str(), 0, filename, eBufType, iBand + 1);
        ar->add_var_nocopy(bt); // bt is from the above switch

        /* -------------------------------------------------------------------- */
        /*      Add the dimension map arrays.                                   */
        /* -------------------------------------------------------------------- */

        if (GDALGetRasterBandYSize(hBand) == GDALGetRasterYSize(hDS)
            && GDALGetRasterBandXSize(hBand) == GDALGetRasterXSize(hDS)) {
            // Use the shared dimensions
            ar->append_dim(northing_dim);
            ar->append_dim(easting_dim);

            // Bind the map to the array; FQNs for the maps (shared dims)
            ar->maps()->add_map(new D4Map(string("/") + northing, northing_map, ar));
            ar->maps()->add_map(new D4Map(string("/") + easting, easting_map, ar));
        }
        else {
            // Don't use the shared dims
            ar->append_dim(GDALGetRasterBandYSize(hBand), northing);
            ar->append_dim(GDALGetRasterBandXSize(hBand), easting);
        }

        // Add attributes, using the same hack as for the global attributes;
        // build the DAP2 AttrTable object and then transform to DAP4.
        AttrTable &band_attr = ar->get_attr_table();
        build_variable_attributes(hDS, &band_attr, iBand);
        ar->attributes()->transform_to_dap4(band_attr);

        dmr->root()->add_var_nocopy(ar);        // Add the array to the DMR
    }
}

/**
 * Read the data array of a DAP2 Grid. This is called by both GDALGrid::read()
 * (for a DAP2 response) and GDALArray::read() when we're building a DAP4
 * data response. The result is that the Array object holds data that can then
 * be serialized.
 *
 * @note This function reads data from the dataset (file). Its companion does
 * not.
 *
 * @note I split up the original GDALGrid::read() code that Frank Warmerdam
 * wrote into two functions because I needed to be able to read the data when
 * building DAP4 responses. There is no Grid type in DAP4.
 *
 * @see read_map_array()
 *
 * @param array Data are loaded into this GDALArray instance
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
    CPLErr eErr = GDALRasterIO(hBand, GF_Read, nWinXOff, nWinYOff, nWinXSize, nWinYSize, pData.data(), nBufXSize,
        nBufYSize, array->get_gdal_buf_type(), 0, 0);
    if (eErr != CE_None) throw Error("Error reading: " + array->name());

    array->val2buf(pData.data());
}

/**
 * Read one of the Map arrays. This uses the kludge that the DDS/DMR calls these
 * two arrays 'northing' and 'easting'.
 *
 * @note This function computes values for the DAP2 Maps or DAP4 shared dimensions;
 * it does not read the values from the dataset. And, it uses the band number,
 * even though for the current (6/1/17) version, there is only one set of Maps/SDims
 * and they are always the same size as the whole Raster. In the future this code
 * might support several sets of Maps/SDims that match not only the whole Raster by
 * any bands with reduced resolution. So I'm keeping the GDALGetRasterBandYSize(hBand),
 * etc., calls in the code.
 *
 * @see read_data_array()
 *
 * @param map Data are loaded into this libdap::Array instance.
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

    map->val2buf((void *) padfMap.data());
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

