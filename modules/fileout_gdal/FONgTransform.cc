// FONgTransform.cc

// This file is part of BES GDAL File Out Module

// Copyright (c) 2012 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

#include "config.h"

#include <cstdlib>

#include <gdal.h>
#include <gdal_priv.h>

#include <DDS.h>
#include <ConstraintEvaluator.h>

#include <Structure.h>
#include <Array.h>
#include <Grid.h>
#include <util.h>

#include <BESDebug.h>
#include <BESInternalError.h>

#include "FONgTransform.h"

#include "FONgType.h"

using namespace std;
using namespace libdap;

/** @brief Constructor that creates transformation object from the specified
 * DataDDS object to the specified file
 *
 * @param dds DataDDS object that contains the data structure, attributes
 * and data
 * @param dhi The data interface containing information about the current
 * request
 * @param localfile GeoTiff to create and write the information to
 * @throws BESInternalError
 */
FONgTransform::FONgTransform(DDS *dds, ConstraintEvaluator &/*evaluator*/, const string &localfile) :
    d_dest(0), d_dds(dds), d_localfile(localfile), d_geo_transform_set(false), d_width(0.0), d_height(0.0), d_top(0.0), d_left(
        0.0), d_bottom(0.0), d_right(0.0), d_no_data(0.0), d_no_data_type(none), d_num_bands(0)
{
    if (localfile.empty()) throw BESInternalError("Empty local file name passed to constructor", __FILE__, __LINE__);
}

/** @brief Destructor
 *
 * Cleans up any temporary data created during the transformation
 */
FONgTransform::~FONgTransform()
{
    vector<FONgType *>::iterator i = d_fong_vars.begin();
    vector<FONgType *>::iterator e = d_fong_vars.end();
    while (i != e) {
        delete (*i++);
    }
}

/** @brief can this DAP type be turned into a GeoTiff or JP2 file?
 *
 */
static bool is_convertable_type(const BaseType *b)
{
    switch (b->type()) {
    case dods_grid_c:
        return true;

        // TODO Add support for Array (?)
    case dods_array_c:
    default:
        return false;
    }
}

/** @brief creates a FONg object for the given DAP object
 *
 * @param v The DAP object to convert
 * @returns The FONg object created via the DAP object
 * @throws BESInternalError if the DAP object is not an expected type
 */
static FONgType *convert(BaseType *v)
{
    switch (v->type()) {
    case dods_grid_c:
        return new FONgType(static_cast<Grid*>(v));

    default:
        throw BESInternalError("file out GeoTiff, unable to write unknown variable type", __FILE__, __LINE__);
    }
}

/** @breif scale the values for a better looking result
 *
 * Often datasets use very small (or less often, very large) values
 * to indicate 'no data' or 'missing data'. The GDAL library scales
 * data so that the entire range of data values are represented in
 * a grayscale image. When the no data value is very small this
 * skews the mean of the values to some very small number. This code
 * builds a histogram of the data and then computes a new 'no data'
 * value based on the smallest (or largest) value that is greater
 * than (or less than) the no data value.
 *
 * @note The initial no data value is determined be looking at attributes and
 * is done by FONgType::extract_coordinates().
 * @note It's an error to call this if no_data_type() is 'none'.
 *
 * @param data The data values to fiddle
 */
void FONgTransform::m_scale_data(double *data)
{
    // It is an error to call this if no_data_type() is 'none'
    set<double> hist;
    for (int i = 0; i < width() * height(); ++i)
        hist.insert(data[i]);

    BESDEBUG("fong3", "Hist count: " << hist.size() << endl);

    if (no_data_type() == negative && hist.size() > 1) {
        // Values are sorted by 'weak' <, so this is the smallest value
        // and ++i is the next smallest value. Assume no_data is the
        // smallest value in the data set and ++i is the smallest actual
        // data value. Reset the no_data value to be 1.0 < the smallest
        // actual value. This makes for a good grayscale photometric
        // GeoTiff w/o changing the actual data values.
        set<double>::iterator i = hist.begin();
        double smallest = *(++i);
        if (fabs(smallest + no_data()) > 1) {
            smallest -= 1.0;

            BESDEBUG("fong3", "New no_data value: " << smallest << endl);

            for (int i = 0; i < width() * height(); ++i) {
                if (data[i] <= no_data()) {
                    data[i] = smallest;
                }
            }
        }
    }
    else {
        set<double>::reverse_iterator r = hist.rbegin();
        double biggest = *(++r);
        if (fabs(no_data() - biggest) > 1) {
            biggest += 1.0;

            BESDEBUG("fong3", "New no_data value: " << biggest << endl);

            for (int i = 0; i < width() * height(); ++i) {
                if (data[i] >= no_data()) {
                    data[i] = biggest;
                }
            }
        }
    }
}

/** @brief Build the geotransform array needed by GDAL
 *
 * This code uses values gleaned by FONgType:extract_coordinates()
 * to build up the six value array of transform parameters that
 * GDAL needs to set the geographic transform.
 *
 * @note This method returns a pointer to a class field; don't delete
 * it.
 *
 * @return A pointer to the six element array of parameters (doubles)
 */
double *FONgTransform::geo_transform()
{

    BESDEBUG("fong3",
        "left: " << d_left << ", top: " << d_top << ", right: " << d_right << ", bottom: " << d_bottom << endl);
    BESDEBUG("fong3", "width: " << d_width << ", height: " << d_height << endl);

    d_gt[0] = d_left; // The leftmost 'x' value, which is longitude
    d_gt[3] = d_top;  // The topmost 'y' value, which is latitude

    // We assume data w/o any rotation (a north-up image)
    d_gt[2] = 0.0;
    d_gt[4] = 0.0;

    // Compute the lower left values. Note that wehn GDAL builds the geotiff
    // output dataset, it correctly inverts the image when the source data has
    // inverted latitude values.
    d_gt[1] = (d_right - d_left) / d_width; // width in pixels; top and bottom in lat
    d_gt[5] = (d_bottom - d_top) / d_height;

    BESDEBUG("fong3",
        "gt[0]: " << d_gt[0] << ", gt[1]: " << d_gt[1] << ", gt[2]: " << d_gt[2] << ", gt[3]: " << d_gt[3] << ", gt[4]: " << d_gt[4] << ", gt[5]: " << d_gt[5] << endl);

    return d_gt;
}

/** @brief Is this effectively a two-dimensional variable?
 *
 * Look at the Grid's Array and see if it is either a 2D array
 * or if it has been subset so that all but two dimensions are
 * removed. Assume that users know what they are doing.
 *
 * @note This code returns false for anything other than a
 * Grid.
 *
 * @return True if this is a 2D array, false otherwise.
 */
bool FONgTransform::effectively_two_D(FONgType *fbtp)
{
    if (fbtp->type() == dods_grid_c) {
        Grid *g = static_cast<FONgType*>(fbtp)->grid();

        if (g->get_array()->dimensions() == 2) return true;

        // count the dimensions with sizes other than 1
        Array *a = g->get_array();
        int dims = 0;
        for (Array::Dim_iter d = a->dim_begin(); d != a->dim_end(); ++d) {
            if (a->dimension_size(d, true) > 1) ++dims;
        }

        return dims == 2;
    }

    return false;
}

static void build_delegate(BaseType *btp, FONgTransform &t)
{
    if (btp->send_p() && is_convertable_type(btp)) {
        BESDEBUG("fong3", "converting " << btp->name() << endl);

        // Build the delegate
        FONgType *fb = convert(btp);

        // Get the information needed for the transform.
        // Note that FONgType::extract_coordinates() also pushes the
        // new FONgType instance onto the FONgTransform's vector of
        // delagate variable objects.
        fb->extract_coordinates(t);
    }
}

// Helper function to descend into Structures looking for Grids (and Arrays
// someday).
static void find_vars_helper(Structure *s, FONgTransform &t)
{
    Structure::Vars_iter vi = s->var_begin();
    while (vi != s->var_end()) {
        if ((*vi)->send_p() && is_convertable_type(*vi)) {
            build_delegate(*vi, t);
        }
        else if ((*vi)->type() == dods_structure_c) {
            find_vars_helper(static_cast<Structure*>(*vi), t);
        }

        ++vi;
    }
}

// Helper function to scan the DDS top-level for Grids, ...
// Note that FONgType::extract_coordinates() sets a bunch of
// values in the FONgType instance _and_ this instance of
// FONgTransform. One of these is 'num_bands()'. For GeoTiff,
// num_bands() must be 1. This is tested in transform().
static void find_vars(DDS *dds, FONgTransform &t)
{
    DDS::Vars_iter vi = dds->var_begin();
    while (vi != dds->var_end()) {
        BESDEBUG("fong3", "looking at: " << (*vi)->name() << " and it is/isn't selected: " << (*vi)->send_p() << endl);
        if ((*vi)->send_p() && is_convertable_type(*vi)) {
            BESDEBUG("fong3", "converting " << (*vi)->name() << endl);
            build_delegate(*vi, t);
        }
        else if ((*vi)->type() == dods_structure_c) {
            find_vars_helper(static_cast<Structure*>(*vi), t);
        }

        ++vi;
    }
}

/** @brief Transforms the variables of the DataDDS to a GeoTiff file.
 *
 * Scan the DDS of the dataset and find the Grids that have been projected.
 * Try to render their content as a GeoTiff. The result is a N-band GeoTiff
 * file.
 */
void FONgTransform::transform_to_geotiff()
{
    // Scan the entire DDS looking for variables that have been 'projected' and
    // build the delegate objects for them.
    find_vars(d_dds, *this);

    for (int i = 0; i < num_bands(); ++i)
        if (!effectively_two_D(var(i)))
            throw Error(
                "GeoTiff responses can consist of two-dimensional variables only; use constraints to reduce the size of Grids as needed.");

#if 0
    GDALAllRegister();
    CPLSetErrorHandler(CPLQuietErrorHandler);
#endif

    GDALDriver *Driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (Driver == NULL) throw Error("Could not get the GTiff driver from/for GDAL: " + string(CPLGetLastErrorMsg()));

    char **Metadata = Driver->GetMetadata();
    if (!CSLFetchBoolean(Metadata, GDAL_DCAP_CREATE, FALSE)) throw Error("Could not make output format.");

    BESDEBUG("fong3", "num_bands: " << num_bands() << "." << endl);

    // NB: Changing PHOTOMETIC to MINISWHITE doesn't seem to have any visible affect,
    // although the resulting files differ. jhrg 11/21/12
    char **options = NULL;
    options = CSLSetNameValue(options, "PHOTOMETRIC", "MINISBLACK"); // The default for GDAL
    d_dest = Driver->Create(d_localfile.c_str(), width(), height(), num_bands(), GDT_Float64, options);
    if (!d_dest) throw Error("Could not create the geotiff dataset: " + string(CPLGetLastErrorMsg()));

    d_dest->SetGeoTransform(geo_transform());
    // Take the mapping data from the first variable
    // var(0)->get_projection(d_dds, d_dest);

    BESDEBUG("fong3", "Made new temp file and set georeferencing (" << num_bands() << " vars)." << endl);

    bool projection_set = false;
    string wkt = "";
    for (int i = 0; i < num_bands(); ++i) {
        FONgType *fbtp = var(i);

        if (!projection_set) {
            wkt = fbtp->get_projection(d_dds);
            if (d_dest->SetProjection(wkt.c_str()) != CPLE_None)
                throw Error("Could not set the projection: " + string(CPLGetLastErrorMsg()));
            projection_set = true;
        }
        else {
            string wkt_i = fbtp->get_projection(d_dds);
            if (wkt_i != wkt)
                throw Error("In building a multiband response, different bands had different projection information.");
        }
#if 0
        d_dest->AddBand(GDT_Float64, 0);
        GDALRasterBand *band = d_dest->GetRasterBand(d_dest->GetRasterCount());
        if (!band)
        throw Error("Could not get the " + long_to_string(i) + "th band: " + string(CPLGetLastErrorMsg()));
#endif
        GDALRasterBand *band = d_dest->GetRasterBand(i + 1);
        if (!band)
            throw Error("Could not get the " + long_to_string(i + 1) + "th band: " + string(CPLGetLastErrorMsg()));

        try {
            // TODO We can read any of the basic DAP2 types and let RasterIO convert it to any other type.
            double *data = fbtp->get_data();

            // hack the values; because the missing value used with many datasets
            // is often really small it'll skew the mapping of values to the grayscale
            // that GDAL performs. Move the no_data values to something closer to the
            // other values in the dataset.
            BESDEBUG("fong3", "no_data_type(): " << no_data_type() << endl);
            if (no_data_type() != none) m_scale_data(data);

            BESDEBUG("fong3", "calling band->RasterIO" << endl);
            CPLErr error = band->RasterIO(GF_Write, 0, 0, width(), height(), data, width(), height(), GDT_Float64, 0,
                0);
            delete[] data;

            if (error != CPLE_None)
                throw Error(
                    "Could not write data for band: " + long_to_string(i + 1) + ": " + string(CPLGetLastErrorMsg()));
        }
        catch (...) {
            GDALClose(d_dest);
            throw;
        }
    }

    GDALClose(d_dest);
}

/** @brief Transforms the variables of the DataDDS to a JPEG2000 file.
 *
 * Scan the DDS of the dataset and find the Grids that have been projected.
 * Try to render their content as a JPEG2000. The result is a N-band JPEG2000
 * file.
 *
 * @note This method is a copy of the transform_to_geotiff() method and most
 * of the code here could be factored out. However, the real utility of this
 * method will be in its ability to write GML for a GMLJP2 response... 12/14/12
 *
 * @note Since the available GMLJP2 drivers for GDAL only support making
 * files using the CreateCopy() mathod, we make a MEM dataset, load it with data
 * and then use that to make GMLJP2 file.
 */
void FONgTransform::transform_to_jpeg2000()
{
    // Scan the entire DDS looking for variables that have been 'projected' and
    // build the delegate objects for them.
    find_vars(d_dds, *this);

    for (int i = 0; i < num_bands(); ++i)
        if (!effectively_two_D(var(i)))
            throw Error(
                "GeoTiff responses can consist of two-dimensional variables only; use constraints to reduce the size of Grids as needed.");

    GDALDriver *Driver = GetGDALDriverManager()->GetDriverByName("MEM");
    if (Driver == NULL) throw Error("Could not get the MEM driver from/for GDAL: " + string(CPLGetLastErrorMsg()));

    char **Metadata = Driver->GetMetadata();
    if (!CSLFetchBoolean(Metadata, GDAL_DCAP_CREATE, FALSE))
        throw Error("Driver JP2OpenJPEG does not support dataset creation.");

    // No creation options for a memory dataset
    // NB: This is where the type of the bands is set. JPEG2000 only supports integer types.
    d_dest = Driver->Create("in_memory_dataset", width(), height(), num_bands(), GDT_Int32, 0 /*options*/);
    if (!d_dest) throw Error("Could not create in-memory dataset: " + string(CPLGetLastErrorMsg()));

    d_dest->SetGeoTransform(geo_transform());

    BESDEBUG("fong3", "Made new temp file and set georeferencing (" << num_bands() << " vars)." << endl);

    bool projection_set = false;
    string wkt = "";
    for (int i = 0; i < num_bands(); ++i) {
        FONgType *fbtp = var(i);

        if (!projection_set) {
            wkt = fbtp->get_projection(d_dds);
            if (d_dest->SetProjection(wkt.c_str()) != CPLE_None)
                throw Error("Could not set the projection: " + string(CPLGetLastErrorMsg()));
            projection_set = true;
        }
        else {
            string wkt_i = fbtp->get_projection(d_dds);
            if (wkt_i != wkt)
                throw Error("In building a multiband response, different bands had different projection information.");
        }

        GDALRasterBand *band = d_dest->GetRasterBand(i + 1);
        if (!band)
            throw Error("Could not get the " + long_to_string(i + 1) + "th band: " + string(CPLGetLastErrorMsg()));

        try {
            // TODO We can read any of the basic DAP2 types and let RasterIO convert it to any other type.
            double *data = fbtp->get_data();

            // hack the values; because the missing value used with many datasets
            // is often really small it'll skew the mapping of values to the grayscale
            // that GDAL performs. Move the no_data values to something closer to the
            // other values in the dataset.
            BESDEBUG("fong3", "no_data_type(): " << no_data_type() << endl);
            if (no_data_type() != none) m_scale_data(data);

            BESDEBUG("fong3", "calling band->RasterIO" << endl);

            // NB: Here the 'type' value indicates the type of data in the buffer. The
            // type of the band is set above when the dataset is created.
            CPLErr error = band->RasterIO(GF_Write, 0, 0, width(), height(), data, width(), height(), GDT_Float64, 0,
                0);
            delete[] data;

            if (error != CPLE_None) throw Error(CPLGetLastErrorMsg());
        }
        catch (...) {
            GDALClose(d_dest);
            throw;
        }
    }

    // Now get the OpenJPEG driver and use CreateCopy() on the d_dest "MEM" dataset
    GDALDataset *jpeg_dst = 0;
    try {
        Driver = GetGDALDriverManager()->GetDriverByName("JP2OpenJPEG");
        if (Driver == NULL) throw Error("Could not get driver for JP2OpenJPEG: " + string(CPLGetLastErrorMsg()));

        // The JPEG2000 drivers only support CreateCopy()
        char **Metadata = Driver->GetMetadata();
        if (!CSLFetchBoolean(Metadata, GDAL_DCAP_CREATECOPY, FALSE))
        BESDEBUG("fong", "Driver JP2OpenJPEG does not support dataset creation via 'CreateCopy()'." << endl);
        //throw Error("Driver JP2OpenJPEG does not support dataset creation via 'CreateCopy()'.");

        char **options = NULL;
        options = CSLSetNameValue(options, "CODEC", "JP2");
        options = CSLSetNameValue(options, "GMLJP2", "YES");
        options = CSLSetNameValue(options, "GeoJP2", "NO");
        options = CSLSetNameValue(options, "QUALITY", "100"); // 25 is the default;
        options = CSLSetNameValue(options, "REVERSIBLE", "YES"); // lossy compression

        BESDEBUG("fong3", "Before JPEG2000 CreateCopy, number of bands: " << d_dest->GetRasterCount() << endl);

        jpeg_dst = Driver->CreateCopy(d_localfile.c_str(), d_dest, FALSE/*strict*/, options, NULL/*progress*/,
            NULL/*progress data*/);

        if (!jpeg_dst) throw Error("Could not create the JPEG200 dataset: " + string(CPLGetLastErrorMsg()));
    }
    catch (...) {
        GDALClose(d_dest);
        GDALClose(jpeg_dst);
        throw;
    }

    GDALClose(d_dest);
    GDALClose(jpeg_dst);
}
