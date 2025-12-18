// -*- mode: c++; c-basic-offset:4 -*-
//
// FoDapCovJsonTransform.h
//
// This file is part of BES CovJSON File Out Module
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Corey Hemphill <hemphilc@oregonstate.edu>
// Author: River Hendriksen <hendriri@oregonstate.edu>
// Author: Riley Rimer <rrimer@oregonstate.edu>
//
// Adapted from the File Out JSON module implemented by Nathan Potter
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

#ifndef FODAPNJSONTRANSFORM_H_
#define FODAPNJSONTRANSFORM_H_

#include <string>
#include <vector>
#include <map>

#include <BESObj.h>

enum DSGType {
    UNSUPPORTED_DSG,SPOINT,POINTS,PROFILE
};
namespace libdap {
class BaseType;
class DDS;
class DMR;
class D4Group;
class Array;
}

class BESDataHandlerInterface;

/**
 * Used to transform a DDS into a CovJSON metadata or CovJSON data document.
 * The output is written to a local file whose name is passed as a parameter 
 * the constructor.
 */
class FoDapCovJsonTransform: public BESObj {
private:
    libdap::DDS *_dds;
    libdap::DMR *_dmr;
    std::string _returnAs;
    std::string _indent_increment = "  ";
    std::string atomicVals;
    std::string currDataType;
    std::string domainType = "Unknown";
    std::string coordRefType = "GeographicCRS";
    bool xExists = false;
    bool yExists = false;
    bool zExists = false;
    bool tExists = false;
    bool isParam = false;
    bool isAxis = false;
    bool canConvertToCovJson = false;

    struct Axis {
        std::string name;
        std::string values;
    };

    struct Parameter {
        std::string id;
        std::string name;
        std::string type;
        std::string dataType;
        std::string unit;
        std::string longName;
        std::string standardName;
        std::string shape;
        std::string values;
    };

    unsigned int axisCount = 0;
    std::vector<Axis *> axes;
    std::string axis_t_units;
    std::string axis_z_units;
    std::string axis_z_direction;
    std::string axis_z_standardName;
    
    unsigned int parameterCount = 0;
    std::vector<Parameter *> parameters;
    std::vector<int> shapeVals;

#if 0
    std::string axis_x_varname;
    std::string axis_y_varname;
    std::string axis_z_varname;
    std::string axis_t_varname;
#endif

    struct axisVar {
        int dim_size;
        std::string name;
        std::string dim_name;
        std::string bound_name;
    };
    axisVar axisVar_x;
    axisVar axisVar_y;
    axisVar axisVar_z;
    axisVar axisVar_t;

    std::vector<float> axisVar_x_bnd_val;
    std::vector<float> axisVar_y_bnd_val;
    std::vector<float> axisVar_z_bnd_val;
    std::vector<double> axisVar_t_bnd_val;

    std::vector<std::string>bnd_dim_names;
    std::vector<std::string>par_vars;

    bool is_simple_cf_geographic = false;
    bool is_dap2_grid = false;
    bool is_geo_dap2_grid = false;

    // Discrete Sampling Geometries 
    DSGType dsg_type = UNSUPPORTED_DSG;
    
    // TODO: add more error messages.
#if 0
    std::string err_msg;
    bool err_msg_set = false;
#endif

    bool is_cf_grid_mapping_var(libdap::BaseType *v) const;

    bool is_supported_vars_by_type(libdap::BaseType*v) const;
    void handle_axisVars_array(libdap::BaseType*v,axisVar & this_axisVar) ;
    void set_axisVar(libdap::BaseType*v,const string &val);
    bool is_simple_dsg(DSGType dsg);
    bool is_simple_dsg_common() const;
    DSGType is_single_point () const;
    DSGType is_point_series () const;
    DSGType is_single_profile () const;
    bool is_valid_single_point_par_var(libdap::BaseType*) const;
    bool is_fake_coor_vars(libdap::Array*) const;
    bool is_valid_array_dsg_par_var(libdap::Array*) const;
    bool is_valid_dsg_par_var(libdap::BaseType *);
    bool obtain_valid_dsg_par_vars(libdap::DDS *);
    bool check_update_simple_dsg(libdap::DDS *);

    void check_update_simple_geo(libdap::DDS *dds,bool sendData);
    void check_update_simple_geo_dap4(libdap::D4Group *d4g);
    bool check_add_axis(libdap::Array *d_a, const std::string &, const std::vector<std::string> &, axisVar &, bool is_t_axis);
    void check_bounds(libdap::DDS *dds, std::map<std::string,std::string>& vname_b_name);
    void obtain_bound_values(libdap::DDS *dds, const axisVar& av, std::vector<float>& av_bnd_val,std::string &bnd_dim_name,bool);
    void obtain_bound_values(libdap::DDS *dds, const axisVar& av, std::vector<double>& av_bnd_val,std::string &bnd_dim_name,bool);
    libdap::Array *  obtain_bound_values_worker(libdap::DDS *dds, const std::string & bound_name, std::string &bound_dim_name);


    bool obtain_valid_vars(libdap::DDS *dds, short axis_var_z_count, short axis_var_t_count);
    bool obtain_valid_vars_dap4(libdap::D4Group *d4g, short axis_var_z_count, short axis_var_t_count);

    // Convert CF time to gregorian calendar.
    std::string cf_time_to_greg(long long time);
    void print_bound(std::ostream *strm, const std::vector<std::string> & t_bnd_val,const std::string & indent,bool is_t_axis) const;
    
    // Check DAP2 CF units
    bool check_geo_dap2_grid(libdap::DDS *dds, const vector<string> & dap2_grid_map_names) const;
    short check_cf_unit_attr(libdap::Array *d_a) const;
    /**
     * @brief Checks the spacial/temporal dimensions that we've obtained, if we've
     *    obtained any at all, can be used to convert to a CovJSON file. If x, y,
     *    and t exist, then we determine domainType based on the shape values and we
     *    return true. If x, y, and/or t don't exist, we simply return false
     *
     * @note see CovJSON domain type spec: https://covjson.org/domain-types/ for
     *    further details on determining domain type
     *
     * @returns true if can convert to CovJSON, false if cannot convert
     */
    // Note we add more accurate checks to see  whether covjson can be supported prior to this function.
    // Currently this function serves as a wrapper of these checks.
    bool canConvert();

    /**
     * @brief Gets a leaf's attribute metadata and stores them to private
     *   class variables to make them accessible for printing.
     *
     * Gets the current attribute values and stores the metadata the data in
     * the corresponding private class variables. Will logically search
     * for value names (ie "longitude") and store them as required.
     *
     * @note strm is included here for debugging purposes. Otherwise, there is no
     *   absolute need to require it as an argument. May remove strm as an arg if
     *   necessary.
     *
     * @note CoverageJSON specification for Temporal Reference Systems
     *   https://covjson.org/spec/#temporal-reference-systems
     *
     * @param ostrm Write the CovJSON to this stream (TEST/DEBUGGING)
     * @param attr_table Reference to an AttrTable containing Axis attribute values
     * @param name Name of a given parameter
     * @param axisRetrieved true if axis is retrieved, false if not
     * @param parameterRetrieved true if parameter is retrieved, false if not
     */
    void getAttributes(std::ostream *strm, libdap::AttrTable &attr_table, std::string name,
        bool *axisRetrieved, bool *parameterRetrieved);

    // Different types need to be handled differently. Eventually the getAttributes() listed above becomes
    // the wrapper of the functions declared below.
    void getAttributes_simple_cf_geographic_dsg(std::ostream *strm, libdap::AttrTable &attr_table, const std::string& name,
        bool *axisRetrieved, bool *parameterRetrieved);

    
   
    /**
     * @brief Attemps to sanitize the time origin string by reformatting and removing
     *    unnecessary words when appropriate
     *
     * @param timeOrigin the original, unclean, time origin value from the source DDS
     * 
     * @returns a sanitized version of the time origin timestamp string
     */
    // TODO: this function is not necessary. May be removed.
    string sanitizeTimeOriginString(std::string timeOrigin);

    /**
     * @brief Writes a CovJSON representation of the DDS to the passed stream. Data
     *   is sent if the sendData flag is true. Otherwise, only metadata is sent.
     *
     * @note w10 sees the world in terms of leaves and nodes. Leaves have data, nodes
     *   have other nodes and leaves.
     *
     * @param strm Write to this output stream
     * @param dds Pointer to a DDS vector, which contains both w10n leaves and nodes
     * @param indent Indent the output so humans can make sense of it
     * @param sendData true: send data; false: send metadata
     * @param testOverride true: print to stream regardless of whether the file can
     *    be converted to CoverageJSON (for testing purposes) false: run canConvert
     *    function to determine if the source DDS can be converted to CovJSON
     */
    void transform(std::ostream *strm, libdap::DDS *dds, std::string indent, bool sendData, bool testOverride);

    void transform(std::ostream *strm, libdap::DMR *dmr, const std::string& indent, bool sendData, bool testOverride);
    
    /**
     * @brief  Write the CovJSON representation of the passed BaseType instance. If the
     *   parameter sendData is true then include the data. Otherwise, only metadata is
     *   sent.
     *
     * @note Structure, Grid, and Sequence cases are not implemented at this time.
     *
     * @param strm Write to this output stream
     * @param bt Pointer to a BaseType vector containing values and/or attributes
     * @param indent Indent the output so humans can make sense of it
     * @param sendData true: send data; false: send metadata
     */
    void transform(std::ostream *strm, libdap::BaseType *bt, std::string indent, bool sendData);
    
    /**
     * @brief Contructs a new set of leaves and nodes derived from a previous set of nodes.
     *
     * Contructs a new set of leaves and nodes derived from a previous set of nodes, then
     * runs the transformRangesWorker() on the new set of leaves so that the range values
     * can be printed to the CovJSON output stream.
     *
     * @note The new derived nodes vector is not used for anything at this time.
     *
     * @note DAP Constructor types are semantically equivalent to a w10n node type so they
     *   must be represented as a collection of child nodes and leaves.
     *
     * @param strm Write to this output stream
     * @param cnstrctr a new collection of child nodes and leaves
     * @param indent Indent the output so humans can make sense of it
     * @param sendData true: send data; false: send metadata
     */
    void transform(std::ostream *strm, libdap::Constructor *cnstrctr, std::string indent, bool sendData);
    
    /**
     * @brief  Write the CovJSON representation of the passed DAP Array instance,
     *   which had better be one of the atomic DAP types. If the parameter sendData
     *   is true then include the data.
     *
     * @param strm Write to this output stream
     * @param cnstrctr a pointer to an Array containing atomic type variables
     * @param indent Indent the output so humans can make sense of it
     * @param sendData true: send data; false: send metadata
     */
    void transform(std::ostream *strm, libdap::Array *a, std::string indent, bool sendData);

    /**
     * @brief  Write the CovJSON representation of the passed BaseType instance,
     *   which had better be one of the atomic DAP types. If the parameter sendData
     *   is true then include the data.
     *
     * @note @TODO need to handle these types appropriately - make proper printing
     *
     * @param a pointer to a BaseType vector containing atomic type variables
     * @param indent Indent the output so humans can make sense of it
     * @param sendData true: send data; false: send metadata
     */
    void transformAtomic(ostream *strm, libdap::BaseType *bt, const std::string& indent, bool sendData);
#if 0
    //void transformAtomic(libdap::BaseType *bt, std::string indent, bool sendData);
#endif
    
    /**
     * @brief Worker method allows us to recursively traverse an Node's variable
     *   contents and any child nodes will be traversed as well.
     *
     * @param strm Write to this output stream
     * @param leaves Pointer to a vector of BaseTypes, which represent w10n leaves
     * @param nodes Pointer to a vector of BaseTypes, which represent w10n nodes
     * @param indent Indent the output so humans can make sense of it
     * @param sendData true: send data; false: send metadata
     */
    void transformNodeWorker(std::ostream *strm, vector<libdap::BaseType *> leaves, vector<libdap::BaseType *> nodes,
        string indent, bool sendData);

    /**
     * @brief Worker method prints the Coverage to stream, which
     *   includes domain, axes, and references, parameters, and ranges
     *
     * @param strm Write to this output stream
     * @param indent Indent the output so humans can make sense of it
     */
    void printCoverage(std::ostream *strm, std::string indent);

    /**
     * @brief Worker method prints the Domain to stream, which
     *   includes domainType, axes, and references
     *
     * @param strm Write to this output stream
     * @param indent Indent the output so humans can make sense of it
     * 
     * @output:
     *   "domain" : {
     *     "type" : "Domain",
     *     "domainType" : "Grid",
     *     "axes": {
     *       "x" : { "values": [-10,-5,0] },
     *       "y" : { "values": [40,50] },
     *       "z" : { "values": [ 5] },
     *       "t" : { "values": ["2010-01-01T00:12:20Z"] }
     *     },
     *     "referencing": [{
     *       "coordinates": ["y","x","z"],
     *       "system": {
     *         "type": "GeographicCRS",
     *         "id": "http://www.opengis.net/def/crs/EPSG/0/4979"
     *       }
     *     }, 
     *     {
     *       "coordinates": ["t"],
     *       "system": {
     *         "type": "TemporalRS",
     *         "calendar": "Gregorian"
     *       }
     *     }]
     *   },
     */
    void printDomain(std::ostream *strm, std::string indent);
    
    /**
     * @brief Worker method prints the Coverage Axes to stream
     *
     * @param strm Write to this output stream
     * @param indent Indent the output so humans can make sense of it
     *
     * @output:
     *   "axes": {
     *     "x" : { "values": [ -10,-5,0 ] },
     *     "y" : { "values": [ 40, 50 ] },
     *     "z" : { "values": [ 5 ] },
     *     "t" : { "values": [ "2010-01-01T00:12:20Z" ] }
     *   },
     */
    void printAxes(std::ostream *strm, std::string indent);

    /**
     * @brief Worker method prints the Coverage domain reference to stream
     *
     * @param strm Write to this output stream
     * @param indent Indent the output so humans can make sense of it
     * 
     * @output:
     *  "referencing": [{
     *    "coordinates": [ "y", "x", "z" ],
     *    "system": {
     *      "type": "GeographicCRS",
     *      "id": "http://www.opengis.net/def/crs/EPSG/0/4979"
     *    }
     *  }, {
     *    "coordinates": ["t"],
     *    "system": {
     *      "type": "TemporalRS",
     *      "calendar": "Gregorian"
     *    }
     *  }] 
     */
    void printReference(std::ostream *strm, std::string indent);
    
    /**
     * @brief Worker method prints the Coverage Parameters to stream
     *
     * @param strm Write to this output stream
     * @param indent Indent the output so humans can make sense of it
     * 
     * @output:
     *   "parameters" : {
     *     "ICEC": {
     *       "type" : "Parameter",
     *       "description": {
     *         "en": "Sea Ice concentration (ice=1;no ice=0)"
     *       },
     *       "unit" : {
     *         "label": {
     *           "en": "Ratio"
     *         },
     *         "symbol": {
     *           "value": "1",
     *           "type": "http://www.opengis.net/def/uom/UCUM/"
     *         }
     *       },
     *       "observedProperty" : {
     *         "id" : "http://vocab.nerc.ac.uk/standard_name/sea_ice_area_fraction/",
     *         "label" : {
     *           "en": "Sea Ice Concentration"
     *         }
     *       }
     *     }
     *   },
     */
    void printParameters(std::ostream *strm, std::string indent);
    
    /**
     * @brief Worker method prints the Coverage Ranges to stream
     *
     * @note CoverageJSON specification for N-dimensional Array Objects (NdArray)
     *    https://covjson.org/spec/#ndarray-objects
     *
     * @param strm Write to this output stream
     * @param indent Indent the output so humans can make sense of it
     * 
     * @output:
     *   "ranges" : {
     *     "ICEC" : {
     *       "type" : "NdArray",
     *       "dataType": "float",
     *       "axisNames": ["t","z","y","x"],
     *       "shape": [1, 1, 2, 3],
     *       "values" : [ 
     *         0.5, 0.6, 0.4, 0.6, 0.2, null
     *       ]
     *     }
     *   }
     */
    void printRanges(std::ostream *strm, std::string indent);

    /**
     * @brief Prints the CoverageJSON file to stream via the print Coverage
     *    worker functions
     *
     * @param strm Write to this output stream
     * @param indent Indent the output so humans can make sense of it
     * @param testOverride true: print to stream regardless of whether the file can
     *    be converted to CoverageJSON (for testing purposes) false: run canConvert
     *    function to determine if the source DDS can be converted to CovJSON
     */
    void printCoverageJSON(std::ostream *strm, string indent, bool testOverride);

    /**
     * @brief Writes the CovJSON representation of the passed DAP Array of simple types.
     *   If the parameter "sendData" evaluates to true then data will also be sent.
     *
     * For each variable in the DDS, write out that variable and its
     * attributes as CovJSON. Each OPeNDAP data type translates into a
     * particular CovJSON type. Also write out any global attributes stored at the
     * top level of the DataDDS.
     *
     * @note If sendData is true but the DDS does not contain data, the result
     * is undefined.
     *
     * @param ostrm Write the CovJSON to this stream (for debugging purposes)
     * @param a Source data array - write out data or metadata from or about this array
     * @param indent Indent the output so humans can make sense of it
     * @param sendData true: send data; false: send metadata
     */
    template<typename T>
    void covjsonSimpleTypeArray(std::ostream *strm, libdap::Array *a, std::string indent, bool sendData);
    
    /**
     * @brief Writes the CovJSON representation of the passed DAP Array of string types.
     *   If the parameter "sendData" evaluates to true, then data will also be sent.
     *
     * For each variable in the DDS, write out that variable and its
     * attributes as CovJSON. Each OPeNDAP data type translates into a
     * particular CovJSON type. Also write out any global attributes stored at the
     * top level of the DataDDS.
     *
     * @note This version exists because of the differing type signatures of the
     *   libdap::Vector::value() methods for numeric and c++ string types.
     *
     * @note If sendData is true but the DDS does not contain data, the result
     *   is undefined.
     *
     * @param strm Write to this output stream
     * @param a Source data array - write out data or metadata from or about this array
     * @param indent Indent the output so humans can make sense of it
     * @param sendData true: send data; false: send metadata
     */
    void covjsonStringArray(std::ostream *strm, libdap::Array *a, std::string indent, bool sendData);

    /**
     * @brief Writes each of the variables in a given container to the CovJSON stream.
     *    For each variable in the DDS, write out that variable as CovJSON.
     *
     * @param ostrm Write the CovJSON to this output stream
     * @param values Source array of type T which we want to write to stream
     * @param indx variable for storing the current indexed value
     * @param shape a vector storing the shape's dimensional values
     * @param currentDim the current dimension we are printing values from
     *
     * @returns the most recently completed index
     */
    template<typename T>
    unsigned int covjsonSimpleTypeArrayWorker(std::ostream *strm, T *values, unsigned int indx,
        std::vector<unsigned int> *shape, unsigned int currentDim, bool is_axis_t_sgeo,libdap::Type a_type);

    /**
     * @brief Adds a new Axis
     *
     * @param name the new Axis name
     * @param values a string of values we want to write to stream
     *
     * @returns a new Axis in the axes vector
     */
    void addAxis(std::string name, std::string values);

    /**
     * @brief Adds a new Parameter
     *
     * @param id the new Parameter id
     * @param name the new Parameter name
     * @param type the new Parameter type
     * @param dataType the new Parameter dataType
     * @param unit the new Parameter unit
     * @param longName the new Parameter longName
     * @param standardName the new Parameter standardName
     * @param shape the new Parameter shape
     * @param values the new Parameter values
     *
     * @returns a new Parameter in the parameters vector
     */
    void addParameter(std::string id, std::string name, std::string type, std::string dataType, std::string unit,
        std::string longName, std::string standardName, std::string shape, std::string values);

    // FOR TESTING PURPOSES ------------------------------------------------------------------------------------
    void setAxesExistence(bool x, bool y, bool z, bool t) {
        this->xExists = x;
        this->yExists = y;
        this->zExists = z;
        this->tExists = t;
    }

    void setDomainType(int domainType) {
        this->domainType = domainType;
    }
    // ---------------------------------------------------------------------------------------------------------

public:
    /**
     * @brief Transforms each of the marked variables of the DDS to CovJSON
     *
     * For each variable in the DDS, write out that variable and its
     * attributes as CovJSON. Each OPeNDAP data type translates into a
     * particular CovJSON type. Also write out any global attributes stored at the
     * top level of the DataDDS.
     *
     * @param ostrm Write the CovJSON to this stream
     * @param sendData True if data should be sent, False to send only metadata.
     * @param testOverride true: print to stream regardless of whether the file can
     *    be converted to CoverageJSON (for testing purposes) false: run normally
     */
    virtual void transform(std::ostream &ostrm, bool sendData, bool testOverride);
    virtual void transform_dap4(std::ostream &ostrm, bool sendData, bool testOverride);

    /**
     * @brief Get the CovJSON encoding for a DDS
     *
     * Set up the CovJSON output transform object. This constructor builds
     * an object that will build a CovJSON encoding for a DDS. This class can
     * return both the entire DDS, including data, and a metadata-only
     * response.
     *
     * @note The 'transform' method is used to build the response and a
     *   bool flag is passed to it to select data or metadata. However, if
     *   that flag is true and the DDS does not already contain data, the
     *   result is undefined.
     *
     * @param dds DDS object
     * 
     * @throw BESInternalError if the DDS* is null or if localfile is empty.
     */
    explicit FoDapCovJsonTransform(libdap::DDS *dds);
    explicit FoDapCovJsonTransform(libdap::DMR *dmr);

    /**
     * @brief Destructs the FoDapCovJsonTransform object and frees all memory.
     */
    virtual ~FoDapCovJsonTransform()
    {
        for (std::vector<Axis *>::const_iterator i = axes.begin(); i != axes.end(); ++i)
            delete (*i);

        for (std::vector<Parameter *>::const_iterator i = parameters.begin(); i != parameters.end(); ++i)
            delete (*i);
    }

    /**
     * @brief Dumps information about this transformation object for debugging
     *   purposes
     *
     * Displays the pointer value of this instance plus instance data,
     * including all of the FoCovJson objects converted from DAP objects that are
     * to be sent to the netcdf file.
     *
     * @param strm C++ i/o stream to dump the information to
     */
    void dump(std::ostream &strm) const override;

    // FOR TESTING PURPOSES ------------------------------------------------------------------------------------
    virtual void addTestAxis(std::string name, std::string values) {
        addAxis(name, values);
    }

    virtual void addTestParameter(std::string id, std::string name, std::string type, std::string dataType, std::string unit,
            std::string longName, std::string standardName, std::string shape, std::string values) {
        addParameter(id, name, type, dataType, unit, longName, standardName, shape, values);
    }

    virtual void setTestAxesExistence(bool x, bool y, bool z, bool t) {
        setAxesExistence(x, y, z, t);
    }

    virtual void setTestDomainType(int domainType) {
        setDomainType(domainType);
    }

    virtual void printCoverage(std::ostream &ostrm, std::string indent) {
        printCoverage(&ostrm, indent);
    }

    virtual void printDomain(std::ostream &ostrm, std::string indent) {
        printDomain(&ostrm, indent);
    }

    virtual void printAxes(std::ostream &ostrm, std::string indent) {
        printAxes(&ostrm, indent);
    }

    virtual void printReference(std::ostream &ostrm, std::string indent) {
        printReference(&ostrm, indent);
    }

    virtual void printParameters(std::ostream &ostrm, std::string indent) {
        printParameters(&ostrm, indent);
    }

    virtual void printRanges(std::ostream &ostrm, std::string indent) {
        printRanges(&ostrm, indent);
    }
    // ---------------------------------------------------------------------------------------------------------
};

#endif /* FODAPCOVJSONTRANSFORM_H_ */
