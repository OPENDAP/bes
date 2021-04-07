/// @file
/// THis is the root class for the STAREmaster functionality. Derived
/// classes read different format input files and create sidecar files
/// for them.

// Ed Hartnett 4/7/21

#include "GeoFile.h"
//#include "SidecarFile.h"
#include <netcdf.h>

/** Construct a GeoFile.
 *
 * @return a GeoFile
 */
GeoFile::GeoFile()
{
    cout<<"GeoFile constructor\n";

    // Initialize values.
    num_index = 0;
    geo_num_i1 = NULL;
    geo_num_j1 = NULL;
    geo_cover1 = NULL;
    geo_num_cover_values1 = NULL;
    geo_lat1 = NULL;
    geo_lon1 = NULL;
    geo_index1 = NULL;
}

/** Destroy a GeoFile.
 *
 */
GeoFile::~GeoFile()
{
    cout<<"GeoFile destructor\n";

    // Free any allocated memory.
    if (geo_lat1)
    {
        for (int i = 0; i < num_index; i++)
            if (geo_lat1[i])
                free(geo_lat1[i]);
	free(geo_lat1);
    }

    if (geo_lon1)
    {
        for (int i = 0; i < num_index; i++)
            if (geo_lon1[i])
                free(geo_lon1[i]);
	free(geo_lon1);
    }
    
    if (geo_index1)
    {
        for (int i = 0; i < num_index; i++)
            if (geo_index1[i])
                free(geo_index1[i]);
	free(geo_index1);
    }
        
    if (geo_num_i1)
	free(geo_num_i1);
    if (geo_num_j1)
	free(geo_num_j1);

    for (int i = 0; i < num_cover; i++)
    {
	if (geo_cover1)
	    free(geo_cover1[i]);
	// if (cover1)
	//     free(cover1[i]);
    }

    if (geo_cover1)
	free(geo_cover1);
    if (geo_num_cover_values1)
	free(geo_num_cover_values1);
    //    if (cover1)
    //	free(cover1);

}

string
GeoFile::sidecarFileName(const string fileName)
{
    string sidecarFileName;
    
    // Is there a file extension?
    size_t f = fileName.rfind(".");
    if (f != string::npos)
	sidecarFileName = fileName.substr(0, f) + "_stare.nc";
    else
    {
        sidecarFileName = fileName;
	sidecarFileName.append("_stare.nc");
    }

    return sidecarFileName;
}

/**
 * Read a sidecare file.
 *
 * @param fileName Name of the sidecar file.
 * @param verbose Set to non-zero to enable verbose output for
 * debugging.
 * @param num_index Reference to an int that will get the number of
 * STARE indexes in the file.
 * @param stare_index_name Reference to a vector of string which hold
 * the names of the STARE index variables.
 * @param size_i vector with the sizes of I for each STARE index.
 * @param size_j vector with the sizes of J for each STARE index.
 * @param variables vector of strings with variables each STARE index
 * applies to.
 * @param ncid The ncid of the opened sidecar file.
 * @return 0 for success, error code otherwise.
 */
int
GeoFile::readSidecarFile_int(const std::string fileName, int verbose, int &num_index,
                             vector<string> &stare_index_name, vector<size_t> &size_i,
                             vector<size_t> &size_j, vector<string> &variables,
			     vector<int> &stare_varid, int &ncid)
{
    char title_in[NC_MAX_NAME + 1];
    int ndims, nvars;
    int ret;
    
    if (verbose) std::cout << "Reading sidecar file " << fileName << "\n";

    // Open the sidecar file.
    if ((ret = nc_open(fileName.c_str(), NC_NOWRITE, &ncid)))
        return ret;

    // Check the title attribute to make sure this is a sidecar file.
    if ((ret = nc_get_att_text(ncid, NC_GLOBAL, SSC_TITLE_NAME, title_in)))
        return ret;
    if (strncmp(title_in, SSC_TITLE, NC_MAX_NAME))
        return SSC_NOT_SIDECAR;

    // How many vars and dims?
    if ((ret = nc_inq(ncid, &ndims, &nvars, NULL, NULL)))
        return ret;

    // Find all variables that are STARE indexes.
    num_index = 0;
    for (int v = 0; v < nvars; v++)
    {
        char var_name[NC_MAX_NAME + 1];
        char long_name_in[NC_MAX_NAME + 1];
        nc_type xtype;
        int ndims, dimids[NDIM2], natts;
        size_t dimlen[NDIM2];

        // Learn about this var.
        if ((ret = nc_inq_var(ncid, v, var_name, &xtype, &ndims, dimids, &natts)))
            return ret;

        if (verbose) std::cout << "var " << var_name << " type " << xtype <<
                         " ndims " << ndims << "\n";

        // Get the long_name attribute value.
        if ((ret = nc_get_att_text(ncid, v, SSC_LONG_NAME, long_name_in)))
            continue;

        // If this is a STARE index, learn about it.
        if (!strncmp(long_name_in, SSC_INDEX_LONG_NAME, NC_MAX_NAME))
        {
            char variables_in[NC_MAX_NAME + 1];

	    // Save the varid.
	    stare_varid.push_back(v);

            // Find the length of the dimensions.
            if ((ret = nc_inq_dimlen(ncid, dimids[0], &dimlen[0])))
                return ret;
            if ((ret = nc_inq_dimlen(ncid, dimids[1], &dimlen[1])))
                return ret;
            
            // What variables does this STARE index apply to?
            if ((ret = nc_get_att_text(ncid, v, SSC_INDEX_VAR_ATT_NAME, variables_in)))
                return ret;
	    std::string var_list = variables_in;
	    variables.push_back(var_list);

            // Save the name of this STARE index variable.
            stare_index_name.push_back(var_name);

            // Save the dimensions of the STARE index var.
            size_i.push_back(dimlen[0]);
            size_j.push_back(dimlen[1]);

            // Keep count of how many STARE indexes we find in the file.
            num_index++;
            if (verbose)
                std::cout << "variable_in " << variables_in << "\n";
        }
    }

    return 0;
}

/**
 * Read a sidecare file.
 *
 * @param fileName Name of the sidecar file.
 * @param verbose Set to non-zero to enable verbose output for
 * debugging.
 * @return 0 for success, error code otherwise.
 */
int
GeoFile::readSidecarFile(const std::string fileName, int verbose, int &ncid)
{
    int ret;
    
    if ((ret = readSidecarFile_int(fileName, verbose, num_index, stare_index_name,
				   size_i, size_j, variables, stare_varid, ncid)))
        return ret;
    return 0;
}

/**
 * Get STARE index for data varaible.
 *
 * @param ncid ID of the sidecar file.
 * @param verbose Set to non-zero to enable verbose output for
 * debugging.
 * @param varid A reference that gets the varid of the STARE index.
 * @return 0 for success, error code otherwise.
 */
int
GeoFile::getSTAREIndex(const std::string varName, int verbose, int ncid, int &varid,
		       size_t &my_size_i, size_t &my_size_j)
{
    if (verbose)
	cout << "getSTAREIndex called for " << varName << endl;
    
    // Check all of our STARE indexes.
    for (int v = 0; v < (int)variables.size(); v++)
    {
	string vars = variables.at(v);
	cout << vars << endl;

	// Is the desired variable listed in the vars string?
	if (vars.find(varName) != string::npos) {
	    cout << "found!" << endl;
	    varid = stare_varid.at(0);
	    my_size_i = size_i.at(0);
	    my_size_j = size_j.at(0);
	} 
    }
    return 0;
}

/**
 * Get STARE index for data varaible.
 *
 * @param ncid ID of the sidecar file.
 * @param verbose Set to non-zero to enable verbose output for
 * debugging.
 * @param varid A reference that gets the varid of the STARE index.
 * @return 0 for success, error code otherwise.
 */
int
GeoFile::getSTAREIndex_2(const std::string varName, int verbose, int ncid,
			 vector<unsigned long long> &values)
{
    size_t my_size_i, my_size_j;
    int varid;
    int ret;

    if (verbose)
	cout << "getSTAREIndex_2 called for " << varName << endl;
    
    // Check all of our STARE indexes.
    for (int v = 0; v < (int)variables.size(); v++)
    {
	string vars = variables.at(v);
	cout << vars << endl;

	// Is the desired variable listed in the vars string?
	if (vars.find(varName) != string::npos) {
	    cout << "found!" << endl;
	    varid = stare_varid.at(v);
	    my_size_i = size_i.at(v);
	    my_size_j = size_j.at(v);

	    // Copy the variables stare index data.
	    {
		unsigned long long *data;
		if (!(data = (unsigned long long *)malloc(my_size_i * my_size_j * sizeof(unsigned long long))))
		    return 99;
		if ((ret = nc_get_var(ncid, varid, data)))
		    return ret;
		values.insert(values.end(), &data[0], &data[my_size_i * my_size_j]);
		free(data);
	    }
	} 
    }
    return 0;
}

/**
 * Close sidecar file.
 *
 * @param ncid ID of the sidecar file.
 * @param verbose Set to non-zero to enable verbose output for
 * debugging.
 * @return 0 for success, error code otherwise.
 */
int
GeoFile::closeSidecarFile(int verbose, int ncid)
{
    int ret;
    
    if (verbose) std::cout << "Closing sidecar file with ncid " << ncid << "\n";
    if ((ret = nc_close(ncid)))
        return ret;
    
    return 0;
}


