/// @file
/// THis is the root class for the STAREmaster functionality. Derived
/// classes read different format input files and create sidecar files
/// for them.
///
/// Ed Hartnett 4/7/21

#include <netcdf.h>

#include <BESDebug.h>

#include "GeoFile.h"

#define MODULE "geofile"

/**
 * @brief Strip away path info. Use in error messages.
 * @param path Full pathname, etc., to a file.
 * @return Just the last part of a pathname
 */
string
GeoFile::sanitize_pathname(string path) const
{
    string::size_type last_slash = path.find_last_of('/');
    ++last_slash;   // don't include the slash in the returned string
    return (last_slash == string::npos) ? path: path.substr(last_slash);
}

string
GeoFile::sidecar_filename(const string &file_name) const
{
    // Is there a file extension?
    size_t f = file_name.rfind(".");
    if (f != string::npos)
        return file_name.substr(0, f) + "_stare.nc";
    else {
        string sidecarFileName = file_name;
        return sidecarFileName.append("_stare.nc");
    }
}

/**
 * @brief Read a sidecar file.
 *
 * Open and read information (except the actual stare indices) from a STARE sidecar
 * file. Each set of stare indices and the variables geolocation information they
 * describe is recorded in the GeoFile instance.
 *
 * The caller is responsible for calling close_sidecar_file().
 *
 * @param fileName Full pathname of the sidecar file.
 * @param ncid Value-result parameter that returns the ncid of the open file
 * @return 0 for success, netCDF error code otherwise.
 */
int
GeoFile::read_sidecar_file(const string &fileName) {
    BESDEBUG(MODULE, "Reading sidecar file " << fileName << endl);

    // Open the sidecar file.
    int ret;
    if ((ret = nc_open(fileName.c_str(), NC_NOWRITE, &d_ncid)))
        return ret;

    // Check the title attribute to make sure this is a sidecar file.
    char title_in[NC_MAX_NAME + 1];
    if ((ret = nc_get_att_text(d_ncid, NC_GLOBAL, SSC_TITLE_NAME, title_in)))
        return ret;
    if (strncmp(title_in, SSC_TITLE, NC_MAX_NAME))
        return SSC_NOT_SIDECAR;

    // How many vars and dims in the index file?
    int ndims, nvars;
    if ((ret = nc_inq(d_ncid, &ndims, &nvars, NULL, NULL)))
        return ret;

    // Find all variables that are STARE indexes.
    d_num_index = 0;
    for (int v = 0; v < nvars; v++) {
        // Learn about this var.
        char var_name[NC_MAX_NAME + 1];
        nc_type xtype;
        int ndims, dimids[NDIM2], natts;
        if ((ret = nc_inq_var(d_ncid, v, var_name, &xtype, &ndims, dimids, &natts)))
            return ret;

        BESDEBUG(MODULE, "var " << var_name << " type " << xtype << " ndims " << ndims << endl);

        // Get the long_name attribute value.
        char long_name_in[NC_MAX_NAME + 1];
        if ((ret = nc_get_att_text(d_ncid, v, SSC_LONG_NAME, long_name_in)))
            continue;

        // If this is a STARE index, learn about it.
        if (!strncmp(long_name_in, SSC_INDEX_LONG_NAME, NC_MAX_NAME)) {
            // Save the varid.
            d_stare_varid.push_back(v);

            // Find the length of the dimensions.
            size_t dimlen[NDIM2];
            if ((ret = nc_inq_dimlen(d_ncid, dimids[0], &dimlen[0])))
                return ret;
            if ((ret = nc_inq_dimlen(d_ncid, dimids[1], &dimlen[1])))
                return ret;

            // What variables does this STARE index apply to?
            char variables_in[NC_MAX_NAME + 1];
            if ((ret = nc_get_att_text(d_ncid, v, SSC_INDEX_VAR_ATT_NAME, variables_in)))
                return ret;

            d_variables.emplace_back(string(variables_in));

            // Save the name of this STARE index variable.
            d_stare_index_name.emplace_back(var_name);

            // Save the dimensions of the STARE index var.
            d_size_i.push_back(dimlen[0]);
            d_size_j.push_back(dimlen[1]);

            // Keep count of how many STARE indexes we find in the file.
            d_num_index++;
            BESDEBUG(MODULE,  "variable_in " << variables_in << endl);
        }
    }

    return 0;
}

/**
 * Get the STARE indices for data variable.
 *
 * @param ncid ID of the open sidecar file.
 * @param var_name The name of the data variable.
 * @param values Value-result parameter; holds the returned stare indices.
 * @return 0 for success, NetCDF library error code otherwise.
 */
void
GeoFile::get_stare_indices(const std::string &variable_name, vector<unsigned long long> &values) {

    BESDEBUG(MODULE, "get_stare_indices called for '" << variable_name << "'" << endl);

    // Check all of the sets of STARE indices. 'variables[v]' lists all of the variables
    // that are indexed by stare_varid[v].
    for (unsigned long v = 0; v < d_variables.size(); ++v) {
        BESDEBUG(MODULE, "Looking st: '" << d_variables[v] << "'" << endl);

        // Is the desired variable listed in the variables[v] string?
        if (d_variables[v].find(variable_name) != string::npos) {
            BESDEBUG(MODULE, "found" << endl);

            values.resize(d_size_i[v] * d_size_j[v]);
            int status = nc_get_var(d_ncid, d_stare_varid[v], &values[0]);
            if (status != NC_NOERR)
                throw BESInternalError("Could not get STARE indices from the sidecar file "
                                   + sanitize_pathname(sidecar_filename(d_data_file_name))
                                   + " for " + variable_name + " - " + nc_strerror(status), __FILE__, __LINE__);
        }
    }
}

size_t GeoFile::get_variable_rows(string variable_name) const {
    for (unsigned long v = 0; v < d_variables.size(); ++v) {
        if (d_variables[v].find(variable_name) != string::npos) {
            return d_size_i[v];
        }
    }

    throw BESInternalError("Could not get row size from the sidecar file "
                           + sanitize_pathname(sidecar_filename(d_data_file_name))
                           + " for " + variable_name, __FILE__, __LINE__);
}

size_t GeoFile::get_variable_cols(string variable_name) const {
    for (unsigned long v = 0; v < d_variables.size(); ++v) {
        if (d_variables[v].find(variable_name) != string::npos) {
            return d_size_j[v];
        }
    }

    throw BESInternalError("Could not get column size from the sidecar file "
                           + sanitize_pathname(sidecar_filename(d_data_file_name))
                           + " for " + variable_name, __FILE__, __LINE__);
}

/**
 * Close sidecar file.
 *
 * @param ncid ID of the sidecar file.
 * @return 0 for success, error code otherwise.
 */
void
GeoFile::close_sidecar_file()
{
    BESDEBUG(MODULE, "Closing sidecar file with ncid " << d_ncid << endl);

    if (d_ncid != -1) {
        (void) nc_close(d_ncid);
        d_ncid = -1;
    }
}


