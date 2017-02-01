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
#include <string>

#include <gdal.h>
#include <cpl_string.h>

#include <DAS.h>

using namespace libdap;

static void translate_metadata( char **md, AttrTable *parent_table );
static void attach_str_attr_item( AttrTable *parent_table, 
                                  const char *pszKey, const char *pszValue );

/************************************************************************/
/*                           read_variables()                           */
/************************************************************************/

void gdal_read_dataset_attributes( DAS &das, GDALDatasetH &hDS)
{
/* -------------------------------------------------------------------- */
/*      Create the dataset container.                                   */
/* -------------------------------------------------------------------- */
    AttrTable *attr_table;
    char **md;

    attr_table = das.add_table( string("GLOBAL"), new AttrTable );

/* -------------------------------------------------------------------- */
/*      Geotransform                                                    */
/* -------------------------------------------------------------------- */
    double adfGeoTransform[6];

    if( GDALGetGeoTransform( hDS, adfGeoTransform ) == CE_None 
        && (adfGeoTransform[0] != 0.0
            || adfGeoTransform[1] != 1.0
            || adfGeoTransform[2] != 0.0
            || adfGeoTransform[3] != 0.0
            || adfGeoTransform[4] != 0.0
            || fabs(adfGeoTransform[5]) != 1.0) )
    {
        char szGeoTransform[200];
        double dfMaxX, dfMinX, dfMaxY, dfMinY;
        int nXSize = GDALGetRasterXSize( hDS );
        int nYSize = GDALGetRasterYSize( hDS );
        
        dfMaxX = MAX(
            MAX(adfGeoTransform[0], 
                adfGeoTransform[0] + adfGeoTransform[1] * nXSize),
            MAX(adfGeoTransform[0] + adfGeoTransform[2] * nYSize, 
                adfGeoTransform[0] + adfGeoTransform[2] * nYSize 
                                   + adfGeoTransform[1] * nXSize));

        dfMinX = MIN(
            MIN(adfGeoTransform[0], 
                adfGeoTransform[0] + adfGeoTransform[1] * nXSize),
            MIN(adfGeoTransform[0] + adfGeoTransform[2] * nYSize, 
                adfGeoTransform[0] + adfGeoTransform[2] * nYSize 
                                   + adfGeoTransform[1] * nXSize));

        dfMaxY = MAX(
            MAX(adfGeoTransform[3], 
                adfGeoTransform[3] + adfGeoTransform[4] * nXSize),
            MAX(adfGeoTransform[3] + adfGeoTransform[5] * nYSize, 
                adfGeoTransform[3] + adfGeoTransform[5] * nYSize 
                                   + adfGeoTransform[4] * nXSize));
        
        dfMinY = MIN(
            MIN(adfGeoTransform[3], 
                adfGeoTransform[3] + adfGeoTransform[4] * nXSize),
            MIN(adfGeoTransform[3] + adfGeoTransform[5] * nYSize, 
                adfGeoTransform[3] + adfGeoTransform[5] * nYSize 
                                   + adfGeoTransform[4] * nXSize));

        attr_table->append_attr( 
            "Northernmost_Northing", "Float64", 
            CPLSPrintf( "%.16g", dfMaxY ) );
        attr_table->append_attr( 
            "Southernmost_Northing", "Float64", 
            CPLSPrintf( "%.16g", dfMinY ) );
        attr_table->append_attr( 
            "Easternmost_Easting", "Float64", 
            CPLSPrintf( "%.16g", dfMaxX ) );
        attr_table->append_attr( 
            "Westernmost_Northing", "Float64", 
            CPLSPrintf( "%.16g", dfMinX ) );
        
        snprintf( szGeoTransform, 200, "%.16g %.16g %.16g %.16g %.16g %.16g",
                 adfGeoTransform[0],
                 adfGeoTransform[1],
                 adfGeoTransform[2],
                 adfGeoTransform[3],
                 adfGeoTransform[4],
                 adfGeoTransform[5] );

        attach_str_attr_item( attr_table, "GeoTransform", szGeoTransform );
    }

/* -------------------------------------------------------------------- */
/*      Metadata.                                                       */
/* -------------------------------------------------------------------- */
    md = GDALGetMetadata( hDS, NULL );
    if( md != NULL )
        translate_metadata( md, attr_table );

/* -------------------------------------------------------------------- */
/*      SRS                                                             */
/* -------------------------------------------------------------------- */
    const char *pszWKT = GDALGetProjectionRef( hDS );

    if( pszWKT != NULL && strlen(pszWKT) > 0 )
        attach_str_attr_item( attr_table, "spatial_ref", pszWKT );

/* ==================================================================== */
/*      Generation info for bands.                                      */
/* ==================================================================== */
    for( int iBand = 0; iBand < GDALGetRasterCount( hDS ); iBand++ )
    {
        GDALRasterBandH hBand = GDALGetRasterBand( hDS, iBand+1 );
        char szName[128];
        AttrTable *band_attr;

/* -------------------------------------------------------------------- */
/*      Create container named after the band.                          */
/* -------------------------------------------------------------------- */
        snprintf( szName, 128, "band_%d", iBand+1 );
        band_attr = das.add_table( string(szName), new AttrTable );

/* -------------------------------------------------------------------- */
/*      Offset.                                                         */
/* -------------------------------------------------------------------- */
        int bSuccess;
        double dfValue;
        char szValue[128];

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
        md = GDALGetMetadata( hBand, NULL );
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
    }
}

/************************************************************************/
/*                         translate_metadata()                         */
/*                                                                      */
/*      Turn a list of metadata name/value pairs into DAS into and      */
/*      attach it to the passed container.                              */
/************************************************************************/

static void translate_metadata( char **md, AttrTable *parent_table )

{
    AttrTable *md_table;
    int i;

    md_table = parent_table->append_container( string("Metadata") );

    for( i = 0; md != NULL && md[i] != NULL; i++ )
    {
        const char *pszValue;
        char *pszKey = NULL;

        pszValue = CPLParseNameValue( md[i], &pszKey );

        attach_str_attr_item( md_table, pszKey, pszValue );

        CPLFree( pszKey );
    }
}

/************************************************************************/
/*                        attach_str_attr_item()                        */
/*                                                                      */
/*      Add a string attribute item to target container with            */
/*      appropriate quoting and escaping.                               */
/************************************************************************/

static void attach_str_attr_item( AttrTable *parent_table, 
                                  const char *pszKey, const char *pszValue )

{
    //string oQuotedValue;
    char *pszEscapedText = CPLEscapeString( pszValue, -1, 
                                            CPLES_BackslashQuotable );

    parent_table->append_attr( pszKey, "String", pszEscapedText /*oQuotedValue*/ );

    CPLFree( pszEscapedText );
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
