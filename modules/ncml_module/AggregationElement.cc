///////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2009 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////

#include "config.h"

#include <sstream>
#include <fstream>
#include <sys/stat.h>

#include "AggregationElement.h"
#include "AggMemberDatasetUsingLocationRef.h" // agg_util
#include "AggMemberDatasetSharedDDSWrapper.h" // agg_util
#include "AggregationUtil.h" // agg_util
#include "ArrayAggregateOnOuterDimension.h" // agg_util
#include "ArrayJoinExistingAggregation.h" // agg_util
#include "GridAggregateOnOuterDimension.h" // agg_util
#include "GridJoinExistingAggregation.h" // agg_util
#include "AggMemberDatasetDimensionCache.h"

#include <libdap/AttrTable.h> // libdap
#include <libdap/Array.h> // libdap
#include <libdap/AttrTable.h> // libdap
#include "DDSAccessInterface.h" // agg_util
#include "Dimension.h" // agg_util
#include "DimensionElement.h"
#include <libdap/Grid.h> // libdap
#include "MyBaseTypeFactory.h"
#include "NCMLBaseArray.h"
#include "NCMLDebug.h"
#include "NCMLParser.h"
#include "NetcdfElement.h"
#include "ScanElement.h"
#include "BESDebug.h"
#include "BESStopWatch.h"

using agg_util::AggregationUtil;
using agg_util::AggMemberDataset;
using agg_util::AMDList;
using agg_util::ArrayAggregateOnOuterDimension;
using agg_util::GridAggregateOnOuterDimension;
using agg_util::ArrayJoinExistingAggregation;
using agg_util::GridJoinExistingAggregation;

using namespace std;

#define MODULE "ncml"
#define prolog string("AggregationElement::").append(__func__).append("() - ")

namespace ncml_module {
const string AggregationElement::_sTypeName = "aggregation";

const vector<string> AggregationElement::_sValidAttrs = getValidAttributes();

AggregationElement::AggregationElement() :
    NCMLElement(0), _type(""), _dimName(""), _recheckEvery(""), _parent(0), _datasets(), _scanners(), _aggVars(), _gotVariableAggElement(
        false), _wasAggregatedMapAddedForJoinExistingGrid(false), _coordinateAxisType("")
{
}

AggregationElement::AggregationElement(const AggregationElement& proto) :
    RCObjectInterface(), NCMLElement(proto), _type(proto._type), _dimName(proto._dimName), _recheckEvery(
        proto._recheckEvery), _parent(proto._parent) // my parent is the same too... is this safe without a true weak reference?
        , _datasets() // deep copy below
    , _scanners() // deep copy below
    , _aggVars(proto._aggVars), _gotVariableAggElement(false), _wasAggregatedMapAddedForJoinExistingGrid(false), _coordinateAxisType(
        "")
{
    // Deep copy all the datasets and add them to me...
    // This is potentially expensive in memory for large datasets, so let's tell someone.
    if (!proto._datasets.empty()) {
        BESDEBUG("ncml",
            "WARNING: AggregationElement copy ctor is deep copying all contained datasets!  This might be memory and time intensive!");
    }

    // Clone the actual members
    _datasets.reserve(proto._datasets.size());
    for (vector<NetcdfElement*>::const_iterator it = proto._datasets.begin(); it != proto._datasets.end(); ++it) {
        const NetcdfElement* elt = (*it);
        addChildDataset(elt->clone());
    }
    NCML_ASSERT(_datasets.size() == proto._datasets.size());

    _scanners.reserve(proto._scanners.size());
    for (vector<ScanElement*>::const_iterator it = proto._scanners.begin(); it != proto._scanners.end(); ++it) {
        const ScanElement* elt = (*it);
        addScanElement(elt->clone());
    }
    NCML_ASSERT(_scanners.size() == proto._scanners.size());
}

AggregationElement::~AggregationElement()
{
    BESDEBUG("ncml:memory", "~AggregationElement called...");
    _type = "";
    _dimName = "";
    _recheckEvery = "";
    _parent = 0;
    _wasAggregatedMapAddedForJoinExistingGrid = false;

    // Release strong references to the contained netcdfelements....
    while (!_datasets.empty()) {
        NetcdfElement* elt = _datasets.back();
        _datasets.pop_back();
        elt->unref(); // Will be deleted if the last strong reference
    }

    // And the scan elements
    while (!_scanners.empty()) {
        ScanElement* elt = _scanners.back();
        _scanners.pop_back();
        elt->unref(); // Will be deleted if the last strong reference
    }
}

const string&
AggregationElement::getTypeName() const
{
    return _sTypeName;
}

AggregationElement*
AggregationElement::clone() const
{
    return new AggregationElement(*this);
}

void AggregationElement::setAttributes(const XMLAttributeMap& attrs)
{
    _type = attrs.getValueForLocalNameOrDefault("type", "");
    _dimName = attrs.getValueForLocalNameOrDefault("dimName", "");
    _recheckEvery = attrs.getValueForLocalNameOrDefault("recheckEvery", "");

    // default is to print errors and throw which we want.
    validateAttributes(attrs, _sValidAttrs);
}

void AggregationElement::handleBegin()
{
    BES_STOPWATCH_START(MODULE, prolog + "Timing");

    NCML_ASSERT(!getParentDataset());

    // Check that the immediate parent element is netcdf since we cannot put an aggregation anywhere else.
    if (!_parser->isScopeNetcdf()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Got an <aggregation> = " + toString()
                + " at incorrect parse location.  They can only be direct children of <netcdf>.  Scope="
                + _parser->getScopeString());
    }

    NetcdfElement* dataset = _parser->getCurrentDataset();
    NCML_ASSERT_MSG(dataset,
        "We expected a non-noll current dataset while processing AggregationElement::handleBegin() for " + toString());
    // If the enclosing dataset already has an aggregation, this is a parse error.
    if (dataset->getChildAggregation()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Got <aggregation> = " + toString() + " but the enclosing dataset = " + dataset->toString()
                + " already had an aggregation set!  There can be only one!");
    }
    // Set me as the aggregation for the current dataset.
    // This will set my parent and also ref() me.
    dataset->setChildAggregation(this);
}

void AggregationElement::handleContent(const string& content)
{
    // Aggregations do not specify content!
    if (!NCMLUtil::isAllWhitespace(content)) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Got non-whitespace for content and didn't expect it.  Element=" + toString() + " content=\"" + content
                + "\"");
    }
}

void AggregationElement::handleEnd()
{
    BES_STOPWATCH_START(MODULE, prolog + "Timing");
    // Handle the actual processing!!
    BESDEBUG("ncml", "AggregationElement::handleEnd() - Processing the aggregation!!" << endl);

    if (isUnionAggregation()) {
        BESDEBUG("ncml2", "AggregationElement::handleEnd() - isUnionAggregation" << endl);
        processUnion();
    }
    else if (isJoinNewAggregation()) {
        BESDEBUG("ncml2", "AggregationElement::handleEnd() - isJoinNewAggregation" << endl);
        processJoinNew();
    }
    else if (isJoinExistingAggregation()) {
        BESDEBUG("ncml2", "AggregationElement::handleEnd() - isJoinExistingAggregation" << endl);
        processJoinExisting();
    }
    else if (_type == "forecastModelRunCollection" || _type == "forecastModelSingleRunCollection") {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Sorry, we do not implement the forecastModelRunCollection aggregations in this version of the NCML Module!");
    }
    else {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Unknown aggregation type=" + _type + " at scope=" + _parser->getScopeString());
    }
}

string AggregationElement::toString() const
{
    return "<" + _sTypeName + " type=\"" + _type + "\"" + printAttributeIfNotEmpty("dimName", _dimName)
        + printAttributeIfNotEmpty("recheckEvery", _recheckEvery) + ">";
}

bool AggregationElement::isJoinNewAggregation() const
{
    return (_type == "joinNew");
}

bool AggregationElement::isUnionAggregation() const
{
    return (_type == "union");
}

bool AggregationElement::isJoinExistingAggregation() const
{
    return (_type == "joinExisting");
}

void AggregationElement::addChildDataset(NetcdfElement* pDataset)
{
    VALID_PTR(pDataset);
    BESDEBUG("ncml", "AggregationElement: adding child dataset: " << pDataset->toString() << endl);

    // Add as a strong reference.
    pDataset->ref();
    _datasets.push_back(pDataset);

    // also set a weak reference to us as the parent
    pDataset->setParentAggregation(this);
}

void AggregationElement::addAggregationVariable(const string& name)
{
    if (isAggregationVariable(name)) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Tried to add an aggregation variable twice: name=" + name + " at scope=" + _parser->getScopeString());
    }
    else {
        _aggVars.push_back(name);
        BESDEBUG("ncml", "Added aggregation variable name=" + name << endl);
    }
}

bool AggregationElement::isAggregationVariable(const string& name) const
{
    bool ret = false;
    AggVarIter endIt = endAggVarIter();
    AggVarIter it = beginAggVarIter();
    for (; it != endIt; ++it) {
        if (name == *it) {
            ret = true;
            break;
        }
    }
    return ret;
}

string AggregationElement::printAggregationVariables() const
{
    string ret("{ ");
    AggVarIter endIt = endAggVarIter();
    AggVarIter it = beginAggVarIter();
    for (; it != endIt; ++it) {
        ret += *it;
        ret += " ";
    }
    ret += "}";
    return ret;
}

AggregationElement::AggVarIter AggregationElement::beginAggVarIter() const
{
    return _aggVars.begin();
}

AggregationElement::AggVarIter AggregationElement::endAggVarIter() const
{
    return _aggVars.end();
}

bool AggregationElement::gotVariableAggElement() const
{
    return _gotVariableAggElement;
}

void AggregationElement::setVariableAggElement()
{
    _gotVariableAggElement = true;
}

void AggregationElement::addScanElement(ScanElement* pScanner)
{
    VALID_PTR(pScanner);
    _scanners.push_back(pScanner);
    pScanner->ref(); // strong ref
    pScanner->setParent(this); // weak ref.
}

void AggregationElement::processParentDatasetComplete()
{
    BESDEBUG("ncml", "AggregationElement::processParentDatasetComplete() called..." << endl);

    if (_type == "joinNew") {
        processParentDatasetCompleteForJoinNew();
    }
    else if (_type == "joinExisting") {
        processParentDatasetCompleteForJoinExisting();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Private Impl

NetcdfElement*
AggregationElement::setParentDataset(NetcdfElement* parent)
{
    NetcdfElement* ret = getParentDataset();
    _parent = parent;
    return ret;
}

void AggregationElement::processUnion()
{
    BESDEBUG("ncml", "Processing a union aggregation..." << endl);

    // Merge all the dimensions...  For now, it is a parse error if a dimension
    // with the same name exists but has a different size.
    // Since DAP2 doesn't have dimensions, we can't do this in agg_util, but
    // have to do it here.
    mergeDimensions();

    // Merge the attributes and variables in all the DDS's into our parent DDS....
    vector<const DDS*> datasetsInOrder;
    // NOTE WELL: this will LOAD ALL DDX's, but there's no choice for union.
    // This doesn't load data, just the metadata!
    collectDatasetsInOrder(datasetsInOrder);
    DDS* pUnion = 0;
    if (getParentDataset()) {
        pUnion = getParentDataset()->getDDS();
    }
    AggregationUtil::performUnionAggregation(pUnion, datasetsInOrder);
}

void AggregationElement::processJoinNew()
{
    BES_STOPWATCH_START(MODULE, prolog + "Timing");

    // This will run any child <scan> elements to prepare them.
    processAnyScanElements();

    BESDEBUG("ncml",
        "AggregationElement::processJoinNew() - beginning joinNew on the following aggVars=" + printAggregationVariables() << endl);

    // Union the dimensions of the child sets so they're available
    BESDEBUG("ncml", "Merging dimensions from children into aggregated dataset..." << endl);
    mergeDimensions();

    // For now we will explicitly create the new dimension for lookups.
    unsigned int newDimSize = _datasets.size(); // ASSUMES we find an aggVar in EVERY dataset!
    getParentDataset()->addDimension(new DimensionElement(agg_util::Dimension(_dimName, newDimSize)));

    // We need at least one dataset, so warn.
    if (_datasets.empty()) {
        THROW_NCML_PARSE_ERROR(line(), "In joinNew aggregation we cannot have zero datasets specified!");
    }

    // This is where the output variables go
    DDS* pAggDDS = getParentDataset()->getDDS();
    // The first dataset acts as the template for the remainder
    DDS* pTemplateDDS = _datasets[0]->getDDS();
    NCML_ASSERT_MSG(pTemplateDDS, "AggregationElement::processJoinNew() - NULL template dataset!");

    // First, union the template's global attribute table into the output's table.
    AggregationUtil::unionAttrsInto(&(pAggDDS->get_attr_table()), pTemplateDDS->get_attr_table());

    // Then perform the aggregation for each variable...
    // TODO REFACTOR OPTIMIZE We loop on variables, not the datasets.
    // It might be more efficient to do all vars for each dataset
    vector<string>::const_iterator endIt = _aggVars.end();
    for (vector<string>::const_iterator it = _aggVars.begin(); it != endIt; ++it) {
        const string& varName = *it;
        BESDEBUG("ncml",
            "AggregationElement::processJoinNew() - Aggregating with joinNew on variable=" << varName << "..." << endl);
        processJoinNewOnAggVar(pAggDDS, varName, *pTemplateDDS);
    }

    // Union any non-aggregated variables from the template dataset into the aggregated dataset
    // Because we want the joinExistingaggregation to build up the Coordinate Variables (CVs)
    // in the order they are declared in the NCML file, we need to track the current position
    // where the last one was inserted. We can do that with a field in the AggregationUtil
    // class. Here we reset that field so that it starts at position 0. 12.13.11 jhrg
    AggregationUtil::resetCVInsertionPosition();

    // Union any non-aggregated variables from the template dataset into the aggregated dataset
    AggregationUtil::unionAllVariablesInto(pAggDDS, *pTemplateDDS, /*add_at_top = */true);
}

#if 0
// This function was used previously, but not now.
// Leaving it in case we need it, but commented out
// to deal with -werror compilation.

/* File local helper for next function */
static bool
doAllScannersSpecifyNCoords(const vector<ScanElement*>& scanners)
{
    bool success = true;
    for (vector<ScanElement*>::const_iterator it = scanners.begin();
        it != scanners.end();
        ++it)
    {
        VALID_PTR(*it);
        if ((*it)->ncoords().empty())
        {
            success = false;
            break;
        }
    }
    return success;
}
#endif // 0

void AggregationElement::processJoinExisting()
{
    BESDEBUG("ncml:2", "Called AggregationElement::processJoinExisting()...");

    // Merge any scans into _datasets
    processAnyScanElements();

    // We need at least one dataset or it's an error
    if (_datasets.empty()) {
        THROW_NCML_PARSE_ERROR(line(), "In joinExisting aggregation we cannot have zero datasets specified!");
    }

    // We need to know the size of the joinExisting dimension
    // for all granule datasets.
    // Make sure that we either get them from:
    // 1) ncoords specified
    // 2) Dimension cache file previously created
    // 3) Load them the slow way and cache the result
    AMDList granuleList;
    granuleList.reserve(_datasets.size());
    fillDimensionCacheForJoinExistingDimension(granuleList, _dimName);

    // Figure out the cardinality of the aggregated dimension
    // and add it into the parent dataset's scope for lookups.
    addNewDimensionForJoinExisting(granuleList);

    // Union any declared dimensions of the child sets so they're available,
    // but be carefuly to skip the join dimension since we already created it
    // new ourselves with the post-aggregation value!
    BESDEBUG("ncml:2", "Merging dimensions from children into aggregated dataset..." << endl);
    mergeDimensions(true, _dimName);

    // This is where the output variables go
    DDS* pAggDDS = getParentDataset()->getDDS();

    // The first dataset acts as the template
    DDS* pTemplateDDS = _datasets[0]->getDDS();
    NCML_ASSERT_MSG(pTemplateDDS, "AggregationElement::processJoinExisting(): NULL template dataset!");

    // First, union the template's global attribute table into the output's table.
    AggregationUtil::unionAttrsInto(&(pAggDDS->get_attr_table()), pTemplateDDS->get_attr_table());

    // Fills in the _aggVars list properly.
    decideWhichVariablesToJoinExist(*pTemplateDDS);

    // For each variable in the to-be-aggregated list, create the
    // aggregation variable in the output based on the granule list.
    vector<string>::const_iterator endIt = _aggVars.end();
    for (vector<string>::const_iterator it = _aggVars.begin(); it != endIt; ++it) {
        const string& varName = *it;
        BESDEBUG("ncml", "Aggregating with joinExisting on variable=" << varName << "..." << endl);
        processJoinExistingOnAggVar(pAggDDS, varName, *pTemplateDDS);
    }

    // Union in the remaining unaggregated variables from the template DDS
    // since they are likely to be coordinate variables.
    // Handle variableAgg properly.
    unionAddAllRequiredNonAggregatedVariablesFrom(*pTemplateDDS);
}

void AggregationElement::unionAddAllRequiredNonAggregatedVariablesFrom(const DDS& templateDDS)
{
    // Union any non-aggregated variables from the template dataset into the aggregated dataset
    // Because we want the joinExistingaggregation to build up the Coordinate Variables (CVs)
    // in the order they are declared in the NCML file, we need to track the current position
    // where the last one was inserted. We can do that with a field in the AggregationUtil
    // class. Here we reset that field so that it starts at position 0. 12.13.11 jhrg
    AggregationUtil::resetCVInsertionPosition();

    // If we didn't get a variable agg for a joinExisting, then union them all.
    if (isJoinExistingAggregation()) {
        if (!gotVariableAggElement()) {
            AggregationUtil::unionAllVariablesInto(getParentDataset()->getDDS(), templateDDS, /*add_at_top = */true);
        }
        else {
            // THROW ONLY IF A GRID since we need to implement the path that handles maps
        }
    } // if isJoinExistingAggregation

    else if (isJoinNewAggregation())
    // joinNew requires the list of vars, so for this one just union them all in as well.
    {
        AggregationUtil::unionAllVariablesInto(getParentDataset()->getDDS(), templateDDS, /*add_at_top = */true);
    }
}

void AggregationElement::decideWhichVariablesToJoinExist(const DDS& templateDDS)
{
    // If they were not specified by hand, then discover them.
    if (_aggVars.empty()) {
        BESDEBUG("ncml",
            "Searching the the template DDS for variables with outer " "dimension matching the join dimension = " << _dimName << " in order to add them to the aggregation output list." << endl);

        // the prototype (first dataset) will define the set of vars to be aggregated.
        // Note: the c.v. dim(dim) _must_ exist, either in all datasets or in the agg itself.
        vector<string> matchingVars;
        findVariablesWithOuterDimensionName(matchingVars, templateDDS, _dimName);
        for (vector<string>::const_iterator it = matchingVars.begin(); it != matchingVars.end(); ++it) {
            addAggregationVariable(*it);
        }
    }
    else // make sure the listed ones are valid
    {
        BESDEBUG("ncml",
            "joinExist aggregation had variableAgg specified...  " "Validating these variables have outer dimension named " << _dimName << endl);

        for (vector<string>::const_iterator it = _aggVars.begin(); it != _aggVars.end(); ++it) {
            BaseType* pVar = AggregationUtil::findVariableAtDDSTopLevel(templateDDS, *it);

            // First, it must exist!
            if (!pVar) {
                std::ostringstream msg;
                msg << "Error validating the variableAgg list.  The variable named " << *it
                    << " was not found in the top-level DDS!";
                THROW_NCML_PARSE_ERROR(line(), msg.str());
            }

            // Next see that it can be aggregated
            Array* pArray = AggregationUtil::getAsArrayIfPossible(pVar);
            if (!pArray) {
                std::ostringstream msg;
                msg << "The declared variableAgg aggregation variable named " << *it
                    << " was not of a type able to be aggregated!";
                THROW_NCML_PARSE_ERROR(line(), msg.str());
            }

            // Make sure the dimension name matches.
            if (pArray->dimension_name(pArray->dim_begin()) != _dimName) {
                std::ostringstream msg;
                msg << "The declared variableAgg variable named " << *it << " did not match the outer dimension name "
                    << _dimName << " for this joinExisting aggregation!";
                THROW_NCML_PARSE_ERROR(line(), msg.str());
            }

            // Otherwise, it's good, so let the log know.
            std::ostringstream msg;
            msg << "The variable named " << *it << " is a valid joinExisting variable.  Will be added to output.";
            BESDEBUG("ncml", msg.str() << endl);
        } // for loop over user-declared variableAgg list.
    }
}

//
void AggregationElement::fillDimensionCacheForJoinExistingDimension(AMDList& granuleList,
    const std::string& /* aggDimName */)
{
    // First, run down the dataset list (which has been expanded with scanners)
    // and create the AMD list for them.
    //    for each entry in _dataset
    vector<NetcdfElement*>::iterator endIt = _datasets.end();
    for (vector<NetcdfElement*>::iterator it = _datasets.begin(); it != endIt; ++it) {
        granuleList.push_back((*it)->getAggMemberDataset());
    }

    // Second, see if there is an ncoords for each of the datasets,
    // and if so, for each one add it to the cache in the AMD.
    if (doesFirstGranuleSpecifyNcoords()) {
        // If so, check they all do or it's a user error.
        if (!doAllGranulesSpecifyNcoords()) {
            THROW_NCML_PARSE_ERROR(-1, "In a joinExisting aggregation we found that the first "
                "granule specified an ncoords but not all of the others "
                "did.  Either all or none of them should have ncoords specified.");
        }
        // otherwise we're good, seed the cache from the ncoords
        else {
            seedDimensionCacheFromUserSpecs(granuleList);
        }
    }
    else // look for cached dimension file or load dimensionalities from granules
    {
        BES_STOPWATCH_START(MODULE, prolog + "LOAD_AGGREGATION_DIMENSIONS_CACHE");

    	agg_util::AggMemberDatasetDimensionCache *aggDimCache = agg_util::AggMemberDatasetDimensionCache::get_instance();

		AMDList::iterator endIt = granuleList.end();
		for (AMDList::iterator it = granuleList.begin(); it != endIt; ++it) {
			AggMemberDataset *amd = (*it).get();
			if(aggDimCache) {
				BESDEBUG("ncml", "AggregationElement::fillDimensionCacheForJoinExistingDimension() - Loading dimension cache for: " << (*it)->getLocation() << "..." << endl);
				aggDimCache->loadDimensionCache(amd);
			}
			else {
				BESDEBUG("ncml", "AggregationElement::fillDimensionCacheForJoinExistingDimension() - " <<
						"WARNING NcML Dimension Caching is not configured or is not working! Loading dimensions from DDS for dataset: " <<
						(*it)->getLocation() << "" << endl);
				amd->fillDimensionCacheByUsingDDS();
			}
		}
    }
}





bool AggregationElement::doesFirstGranuleSpecifyNcoords() const
{
    if (_datasets.size() > 0) {
        return _datasets.at(0)->hasNcoords();
    }
    else {
        return false;
    }
}

bool AggregationElement::doAllGranulesSpecifyNcoords() const
{
    bool success = true;
    vector<NetcdfElement*>::const_iterator endIt = _datasets.end();
    for (vector<NetcdfElement*>::const_iterator it = _datasets.begin(); it != endIt; ++it) {
        success = success && (*it)->hasNcoords();
        if (!success) {
            break;
        }
    }
    return success;
}

void AggregationElement::seedDimensionCacheFromUserSpecs(agg_util::AMDList& rGranuleList) const
{
    NCML_ASSERT(_datasets.size() == rGranuleList.size());

    vector<NetcdfElement*>::const_iterator datasetIt;
    AMDList::iterator amdIt;
    for (datasetIt = _datasets.begin(), amdIt = rGranuleList.begin(); datasetIt != _datasets.end();
        ++datasetIt, ++amdIt) {
        // Make sure the attribute exists or warn the author
        const NetcdfElement* pDataset = *datasetIt;
        if (!pDataset->hasNcoords()) {
            // This is an assumption of the
            THROW_NCML_INTERNAL_ERROR("Expected netcdf element member of a joinExisting "
                "aggregation to have the ncoords attribute specified "
                "but it did not.");
        }
        unsigned int ncoords = pDataset->getNcoordsAsUnsignedInt();
        RCPtr<AggMemberDataset> pAMD = *amdIt;
        VALID_PTR(pAMD.get());
        agg_util::Dimension dim;
        dim.name = _dimName;
        dim.size = ncoords;
        pAMD->setDimensionCacheFor(dim, true);

        NCML_ASSERT_MSG((pAMD->isDimensionCached(dim.name) && pAMD->getCachedDimensionSize(dim.name) == dim.size),
            "Dimension cache bug");
    }
    // make sure they stayed in sync
    NCML_ASSERT(amdIt == rGranuleList.end());
}


// For now, just count up the ncoords...
void AggregationElement::addNewDimensionForJoinExisting(const agg_util::AMDList& rGranuleList)
{
    // Sum up the cardinalities from AMD's
    unsigned int aggDimSize = 0;
    for (AMDList::const_iterator it = rGranuleList.begin(); it != rGranuleList.end(); ++it) {
        NCML_ASSERT((*it)->isDimensionCached(_dimName));
        aggDimSize += (*it)->getCachedDimensionSize(_dimName);
    }

    // Error if the dimension exists in the output local scope already
    NCML_ASSERT(getParentDataset());
    NCML_ASSERT_MSG(!(getParentDataset()->getDimensionInLocalScope(_dimName)),
        "AggregationElement::addNewDimensionForJoinExisting() found a dimension "
            "named " + _dimName + " already but did not expect it!");

    // Otherwise, create and add it in.
    getParentDataset()->addDimension(new DimensionElement(agg_util::Dimension(_dimName, aggDimSize)));

    // And tell the world at large
    ostringstream oss;
    oss << "Added joinExisting aggregation dimension "
        " name=" << _dimName << " with aggregated size= " << aggDimSize;
    BESDEBUG("ncml:2", oss.str());
}

void AggregationElement::findVariablesWithOuterDimensionName(vector<string>& oMatchingVars, const DDS& templateDDS,
    const string& outerDimName) const
{
    for (DDS::Vars_iter it = const_cast<DDS&>(templateDDS).var_begin(); it != const_cast<DDS&>(templateDDS).var_end();
        ++it) {
        Array* pArray = AggregationUtil::getAsArrayIfPossible(*it);
        // Only if it's an array or a grid data array
        if (pArray && outerDimName == pArray->dimension_name(pArray->dim_begin())) {
            oMatchingVars.push_back(pArray->name());
        }
    }
}

void AggregationElement::getParamsForJoinAggOnVariable(JoinAggParams* pOutParams, const DDS& /*aggOutputDDS*/,
    const std::string& varName, const DDS& templateDDS)
{
    VALID_PTR(pOutParams);

    // Look up the template variable.
    pOutParams->_pAggVarTemplate = AggregationUtil::getVariableNoRecurse(templateDDS, varName);
    if (!(pOutParams->_pAggVarTemplate)) {
        THROW_NCML_PARSE_ERROR(line(),
            " We could not find a template for the specified aggregation variable=" + varName
                + " so we cannot continue the aggregation.");
    }

    // Dimension must exist already
    const DimensionElement* pDim = getParentDataset()->getDimensionInLocalScope(_dimName);
    NCML_ASSERT_MSG(pDim, "Didn't find a DimensionElement with the aggregation dimName=" + _dimName);
    pOutParams->_pAggDim = &(pDim->getDimension());

#if 0
    // I don't follow the logic here. I think we should be able to add attributes to
    // variables that already exist. This may be intended to protect against removing
    // the variable on which the aggregation is performed 'over' (e.g., time) with a
    // different variable. But it has the affect of also prohibiting that addition of
    // an attribute on that variable. I'm removing it for now. jhrg 10/17/11

    // Be sure the name isn't taken in the output DDS.
    BaseType* pExists = AggregationUtil::getVariableNoRecurse(aggOutputDDS, varName);
    NCML_ASSERT_MSG(!pExists,
        "Failed since the name of the new variable to add (name="
        + varName
        + ") already exists in the "
        " output aggregation DDS!  What happened?!");
#endif

    // Get a vector of lazy loaders
    // We will transfer AGM ownership to the calls so do not need to delete them.
    collectAggMemberDatasets(pOutParams->_memberDatasets);
}

void AggregationElement::processJoinNewOnAggVar(DDS* pAggDDS, const std::string& varName, const DDS& templateDDS)
{
    BES_STOPWATCH_START(MODULE, prolog + "Timing");

    // Get the params we need to factory the actual aggregation subclass
    JoinAggParams joinAggParams;
    getParamsForJoinAggOnVariable(&joinAggParams, // output
        *pAggDDS, varName, templateDDS);

    // Factory out the proper subtype
    BaseType* pAggVarTemplate = joinAggParams._pAggVarTemplate;
    if (pAggVarTemplate->type() == dods_array_c) {
        processAggVarJoinNewForArray(*pAggDDS, *(static_cast<Array*>(pAggVarTemplate)), *(joinAggParams._pAggDim),
            joinAggParams._memberDatasets);
    }
    else if (pAggVarTemplate->type() == dods_grid_c) {
        processAggVarJoinNewForGrid(*pAggDDS, *(static_cast<Grid*>(pAggVarTemplate)), *(joinAggParams._pAggDim),
            joinAggParams._memberDatasets);
    }
    else {
        THROW_NCML_PARSE_ERROR(line(),
            "Got an aggregation variable not of type Array or Grid, but of: " + pAggVarTemplate->type_name()
                + " which we cannot aggregate!");
    }
    // Nothing else to do for this var until the call to processParentDataset() is complete.
}

void AggregationElement::processJoinExistingOnAggVar(DDS* pAggDDS, const std::string& varName, const DDS& templateDDS)
{

    BES_STOPWATCH_START(MODULE, prolog + "Timing");

    // Get the params we need to factory the actual aggregation subclass
    JoinAggParams joinAggParams;
    getParamsForJoinAggOnVariable(&joinAggParams, // output
        *pAggDDS, varName, templateDDS);

    // Factory out the proper subtype
    BaseType* pAggVarTemplate = joinAggParams._pAggVarTemplate;
    if (pAggVarTemplate->type() == dods_array_c) {
        processAggVarJoinExistingForArray(*pAggDDS, *(static_cast<Array*>(pAggVarTemplate)), *(joinAggParams._pAggDim),
            joinAggParams._memberDatasets);
    }
    else if (pAggVarTemplate->type() == dods_grid_c) {
        processAggVarJoinExistingForGrid(*pAggDDS, *(static_cast<Grid*>(pAggVarTemplate)), *(joinAggParams._pAggDim),
            joinAggParams._memberDatasets);
    }
    else {
        THROW_NCML_PARSE_ERROR(line(),
            "Got an aggregation variable not of type Array or Grid, but of: " + pAggVarTemplate->type_name()
                + " which we cannot aggregate!");
    }
    // Nothing else to do for this var until the call to processParentDataset() is complete.
}

void AggregationElement::processAggVarJoinNewForArray(DDS& aggDDS, const libdap::Array& arrayTemplate,
    const agg_util::Dimension& dim, const AMDList& memberDatasets)
{
    BES_STOPWATCH_START(MODULE, prolog + "Timing");

    // Use the basic array getter to read adn get from top level DDS.
    unique_ptr<agg_util::ArrayGetterInterface> arrayGetter(new agg_util::TopLevelArrayGetter());

    unique_ptr<ArrayAggregateOnOuterDimension> pAggArray(
        new ArrayAggregateOnOuterDimension(arrayTemplate, memberDatasets, std::move(arrayGetter), dim));

    // Make sure we xfer ownership of contained dumb ptr.
    NCML_ASSERT_MSG(!(arrayGetter.get()), "Expected unique_ptr owner xfer, failed!");

    // This will copy, unique_ptr will clear the prototype.
    // NOTE: add_var() makes a copy.
    // OPTIMIZE change to add_var_no_copy when it exists.
    BESDEBUG("ncml",
        "Adding new ArrayAggregateOnOuterDimension with name=" << arrayTemplate.name() << " to aggregated dataset!" << endl);

    // Replaced the copy version of DDS::add_var() with the nocopy version. This saves
    // a deep copy, but more importantly, is a workaround for a memory issue in the
    // ArrayAggregateOnOuterDimension or ArrayAggreagtionBase copy constructor, which
    // triggers a memory error deep in libdap::Array::Array(const Array&). See similar
    // changes below. This and related changes fix HYRAX-803. jhrg 8/3/18

    aggDDS.add_var_nocopy(pAggArray.release());
}

void AggregationElement::processAggVarJoinNewForGrid(DDS& aggDDS, const Grid& gridTemplate,
    const agg_util::Dimension& dim, const AMDList& memberDatasets)
{
    BES_STOPWATCH_START(MODULE, prolog + "Timing");

    unique_ptr<GridAggregateOnOuterDimension> pAggGrid(
        new GridAggregateOnOuterDimension(gridTemplate, dim, memberDatasets, _parser->getDDSLoader()));

    // This will copy, unique_ptr will clear the prototype.
    // OPTIMIZE change to add_var_no_copy when it exists.
    BESDEBUG("ncml",
        "Adding new GridAggregateOnOuterDimension with name=" << gridTemplate.name() << " to aggregated dataset!" << endl);

    aggDDS.add_var_nocopy(pAggGrid.release());
}

void AggregationElement::processAggVarJoinExistingForArray(DDS& aggDDS, const libdap::Array& arrayTemplate,
    const agg_util::Dimension& dim, const AMDList& memberDatasets)
{

    BES_STOPWATCH_START(MODULE, prolog + "Timing");

    // Use the basic array getter to read adn get from top level DDS.
    unique_ptr<agg_util::ArrayGetterInterface> arrayGetter(new agg_util::TopLevelArrayGetter());

    unique_ptr<ArrayJoinExistingAggregation> pAggArray(
        new ArrayJoinExistingAggregation(arrayTemplate, memberDatasets, std::move(arrayGetter),
            dim));

    // Make sure we xfer ownership of contained dumb ptr.
    NCML_ASSERT_MSG(!(arrayGetter.get()), "Expected unique_ptr owner xfer, failed!");

    // This will copy, unique_ptr will clear the prototype.
    // NOTE: add_var() makes a copy.
    // OPTIMIZE change to add_var_no_copy when it exists.
    BESDEBUG("ncml",
        "Adding new ArrayJoinExistingAggregation with name=" << arrayTemplate.name() << " to aggregated dataset!" << endl);

    aggDDS.add_var_nocopy(pAggArray.release());
}

void AggregationElement::processAggVarJoinExistingForGrid(DDS& aggDDS, const Grid& gridTemplate,
    const agg_util::Dimension& dim, const AMDList& memberDatasets)
{
    BES_STOPWATCH_START(MODULE, prolog + "Timing");

    unique_ptr<GridJoinExistingAggregation> pAggGrid(
        new GridJoinExistingAggregation(gridTemplate, memberDatasets, _parser->getDDSLoader(), dim));

    BESDEBUG("ncml",
        "Adding new GridJoinExistingAggregation with name=" << gridTemplate.name() << " to aggregated dataset!" << endl);

    aggDDS.add_var_nocopy(pAggGrid.release());
}

void AggregationElement::processParentDatasetCompleteForJoinNew()
{
    BES_STOPWATCH_START(MODULE, prolog + "Timing");

    NetcdfElement* pParentDataset = getParentDataset();
    VALID_PTR(pParentDataset);
    DDS* pParentDDS = pParentDataset->getDDS();
    VALID_PTR(pParentDDS);

    const DimensionElement* pDim = getParentDataset()->getDimensionInLocalScope(_dimName);
    NCML_ASSERT_MSG(pDim, "  AggregationElement::processParentDatasetCompleteForJoinNew(): "
        " didn't find a DimensionElement with the joinNew dimName=" + _dimName);
    const agg_util::Dimension& dim = pDim->getDimension();

    // See if there's an explicit or placeholder c.v. for this dimension name
    BaseType* pBT = AggregationUtil::getVariableNoRecurse(*pParentDDS, dim.name);
    Array* pCV = 0; // this will be a ptr to the actual (new or existing) c.v. in the *pParentDDS.

    // If name totally unused, we need to create a new c.v. and add it.
    if (!pBT) {
        pCV = createAndAddCoordinateVariableForNewDimension(*pParentDDS, dim);
        NCML_ASSERT_MSG(pCV, "processParentDatasetCompleteForJoinNew(): "
            "failed to create a new coordinate variable for dim=" + dim.name);
    }
    else // name exists: either it's explicit or deferred.
    {
        // See if the var we found with the dimension name is
        // in the deferred variable list for the parent dataset:
        VariableElement* pVarElt = pParentDataset->findVariableElementForLibdapVar(pBT);
        // If not, then we expect explicit values so just validate it's a proper c.v. for
        // the aggregation (the dim) and set pCV to it if so.
        if (!pVarElt) {
            // will throw if not valid since we send true.
            pCV = ensureVariableIsProperNewCoordinateVariable(pBT, dim, true);
            VALID_PTR(pCV);
        }
        else // it was deferred, need to do some special work...
        {
            pCV = processDeferredCoordinateVariable(pBT, dim);
            VALID_PTR(pCV);
        }
    }

    // OK, either pCV is valid or we've unwound out by this point.
    // If a coordinate axis type was specified, we need to add it now.
    //
    // This fiddles with the attribute for the CV. jhrg 10/17/11
    if (!_coordinateAxisType.empty()) {
        addCoordinateAxisType(*pCV, _coordinateAxisType);
    }

    // For each aggVar:
    //    If it's a Grid, add the coordinate variable as a new map vector.
    //    If it's an Array, do nothing -- we already added the CV as a sibling to the aggvar
    AggVarIter it;
    AggVarIter endIt = endAggVarIter();
    for (it = beginAggVarIter(); it != endIt; ++it) {
        const string& aggVar = *it;
        BaseType* pBT = AggregationUtil::getVariableNoRecurse(*pParentDDS, aggVar);
        GridAggregateOnOuterDimension* pGrid = dynamic_cast<GridAggregateOnOuterDimension*>(pBT);
        if (pGrid) {
            // Add the given map to the Grid as a copy
            pGrid->prepend_map(pCV, true);
        }
    }
}

void AggregationElement::processParentDatasetCompleteForJoinExisting()
{
    BES_STOPWATCH_START(MODULE, prolog + "Timing");

    NetcdfElement* pParentDataset = getParentDataset();
    VALID_PTR(pParentDataset);
    DDS* pAggDDS = pParentDataset->getDDS();
    VALID_PTR(pAggDDS);

    const DimensionElement* pDim = getParentDataset()->getDimensionInLocalScope(_dimName);
    NCML_ASSERT_MSG(pDim, " Didn't find a DimensionElement with the joinExisting dimName=" + _dimName);
    const agg_util::Dimension& dim = pDim->getDimension();

    // See if there's an explicit or placeholder c.v. for this dimension name
    BaseType* pDimNameVar = AggregationUtil::getVariableNoRecurse(*pAggDDS, dim.name);

    bool placeholderExists = false;
    Array* pCV = 0; // this will be a ptr to the actual (new or existing) c.v. in the *pParentDDS.
    // If the c.v. exists, then process it further.
    if (pDimNameVar) {
        // See if the var we found with the dimension name is
        // in the deferred variable list for the parent dataset:
        VariableElement* pVarElt = pParentDataset->findVariableElementForLibdapVar(pDimNameVar);
        // If not, then we expect explicit values so just validate it's a proper c.v. for
        // the aggregation (the dim) and set pCV to it if so.
        if (!pVarElt) {
            // will throw if not valid since we send true.
            pCV = ensureVariableIsProperNewCoordinateVariable(pDimNameVar, dim, true);
            VALID_PTR(pCV);
            placeholderExists = false;
        }
        else // it was deferred, need to do some special work below...
        {
            //pCV = processDeferredCoordinateVariable(pDimNameVar, dim);
            placeholderExists = true;
        }
    }

    // For the scope of the next loop, this will be filled
    // with a new aggregated map variable when we didn't find the first Grid
    // and then pCV will refer to it until the function end.
    // If created, it will be used as the map vector for all Grid's.
    unique_ptr<ArrayJoinExistingAggregation> pNewMap;

    // For each aggVar:
    //    If it's a Grid, add the coordinate variable as a new map vector
    //                    since we left it out in the actual Grid until aggregated.
    //    If it's an Array, do nothing
    auto endIt = endAggVarIter();
    for (auto it = beginAggVarIter(); it != endIt; ++it) {
        const string& aggVar = *it;
        BaseType* pAggVar = AggregationUtil::getVariableNoRecurse(*pAggDDS, aggVar);

        // HACK TODO clean this downcast later when we refactor this file.
        GridJoinExistingAggregation* pGrid = dynamic_cast<GridJoinExistingAggregation*>(pAggVar);
        if (pGrid) {
            // If we don't find it, but we're the first Grid, then assume it's in the Grid maps
            // and create it.  Will be reused by other Grid's.
            // We also do this if it was a placeholder since we need to replace it!
            if (!pCV || placeholderExists) {
                pNewMap = pGrid->makeAggregatedOuterMapVector();
                VALID_PTR(pNewMap.get());

                // If there was a placeholder, we need to
                // grab its metadata as a changeset and replace
                // the variable in the DDS with the new one.
                if (placeholderExists) {
                    processPlaceholderCoordinateVariableForJoinExisting(*pDimNameVar, pNewMap.get());
                }

                // this will make a copy, so the unique_ptr is ok.
                AggregationUtil::addOrReplaceVariableForName(pAggDDS, *(pNewMap.get()));

                // Use the new one as the coordinate variable for the maps below
                pCV = pNewMap.get();
            }

            // It MUST exist for a Grid since we have to add it for completeness.
            NCML_ASSERT_MSG(pCV, "Expected a coordinate variable since a Grid exists... what happened?");

            // Add the given map to the Grid as a copy
            pGrid->prepend_map(pCV, true);
        }
    }
}

void AggregationElement::processPlaceholderCoordinateVariableForJoinExisting(const libdap::BaseType& placeholderVar,
    libdap::Array* pNewVar)
{
    VALID_PTR(pNewVar);

    // Make sure the types of the placeholder scalar and created array match or the author goofed
    BaseType* pNewEltProto = pNewVar->var();
    VALID_PTR(pNewEltProto);
    if (placeholderVar.type() != pNewEltProto->type()) {
        THROW_NCML_PARSE_ERROR(line(),
            " We expected the type of the placeholder coordinate variable to be the same "
                " as that created by the aggregation.  Expected type=" + pNewEltProto->type_name()
                + +" but placeholder has type=" + placeholderVar.type_name()
                + "  Please make sure these match in the input file!");
    }

    // Pull the metadata into the new c.v. from the placeholder
    AggregationUtil::gatherMetadataChangesFrom(pNewVar, placeholderVar);

    // Let the validation know that we got values for the original value and to remove the entry
    // since we're about to delete the pointer to pBT!
    getParentDataset()->setVariableGotValues(const_cast<BaseType*>(&placeholderVar), true);
}

void AggregationElement::setAggregationVariableCoordinateAxisType(const std::string& cat)
{
    _coordinateAxisType = cat;
}

const std::string&
AggregationElement::getAggregationVariableCoordinateAxisType() const
{
    return _coordinateAxisType;
}

libdap::Array*
AggregationElement::ensureVariableIsProperNewCoordinateVariable(libdap::BaseType* pBT, const agg_util::Dimension& dim,
    bool throwOnInvalidCV) const
{
    VALID_PTR(pBT);
    Array* pArrRet = 0;

    // If 1D array with name == dim....
    if (AggregationUtil::couldBeCoordinateVariable(pBT)) {
        // Ensure the dimensionalities match
        Array* pArr = static_cast<Array*>(pBT);
        if (pArr->length() == static_cast<int>(dim.size)) {
            // OK, it's a valid return value.
            pArrRet = pArr;
        }
        else // Dimensionality mismatch, exception or return NULL.
        {
            ostringstream oss;
            oss << string("In the aggregation for dimension=") << dim.name
                << ": The coordinate variable we found does NOT have the same dimensionality as the"
                    "aggregated dimension!  We expected dimensionality=" << dim.size
                << " but the coordinate variable had dimensionality=" << pArr->length();
            BESDEBUG("ncml", oss.str() << endl);
            if (throwOnInvalidCV) {
                THROW_NCML_PARSE_ERROR(line(), oss.str());
            }
        }
    }

    else // Name exists, but not a coordinate variable, then exception or return null.
    {
        std::ostringstream msg;
        msg << "Aggregation found a variable matching aggregated dimension name=" << dim.name
            << " but it was not a coordinate variable.  "
                " It must be a 1D array whose dimension name is the same as its name. ";
        BESDEBUG("ncml", "AggregationElement::ensureVariableIsProperNewCoordinateVariable: " + msg.str() << endl);
        if (throwOnInvalidCV) {
            THROW_NCML_PARSE_ERROR(line(), msg.str())
        }
    }
    // Return valid Array or null on failures.
    return pArrRet;
}

libdap::Array*
AggregationElement::findMatchingCoordinateVariable(const DDS& dds, const agg_util::Dimension& dim,
    bool throwOnInvalidCV/*=true*/) const
{
    BaseType* pBT = AggregationUtil::getVariableNoRecurse(dds, dim.name);

    // Name doesn't exist, just NULL.  We'll have to create it from scratch
    if (!pBT) {
        return 0;
    }

    return ensureVariableIsProperNewCoordinateVariable(pBT, dim, throwOnInvalidCV);
}

/**
 *  We will:
 *    o Create the actual data for the coordinate variable as if there were
 *      no deferred variable at all
 *    o Ensure the type of placeholder elt and new var elts are the same or throw
 *    o Copy the metadata (AttrTable) in pBT into the new one
 *    o Remove pBT from the DDS since by definition it will be a scalar and not an Array type.
 *    o Add the newly created one to the dataset
 *    o Inform the dataset that the variables values are now valid.  This will REMOVE the entry
 *       since the object will be going away!!
 *    o Lookup the object ACTUALLY in the DDS and return it.
 */
libdap::Array*
AggregationElement::processDeferredCoordinateVariable(libdap::BaseType* pBT, const agg_util::Dimension& dim)
{
    VALID_PTR(pBT);

    BESDEBUG("ncml",
        "Processing the placeholder coordinate variable (no values) for the " "current aggregation to add placeholder metadata to the generated values..." << endl);

    // Generate the c.v. as if we had no placeholder since pBT will be a scalar (shape cannot
    // be defined on it by ncml spec defn).
    // @OPTIMIZE try to refactor this to avoid unnecessary copies.
    unique_ptr<Array> pNewArrCV = createCoordinateVariableForNewDimension(dim);
    NCML_ASSERT_MSG(pNewArrCV.get(), " createCoordinateVariableForNewDimension()"
        " returned null.");

    // Make sure the types of the placeholder scalar and created array match or the author goofed
    BaseType* pNewEltProto = pNewArrCV->var();
    VALID_PTR(pNewEltProto);
    if (pBT->type() != pNewEltProto->type()) {
        THROW_NCML_PARSE_ERROR(line(),
            " We expected the type of the placeholder coordinate variable to be the same "
                " as that created by the aggregation.  Expected type=" + pNewEltProto->type_name()
                + +" but placeholder has type=" + pBT->type_name()
                + "  Please make sure these match in the input file!");
    }

    // Let the validation know that we got values for the original value and to remove the entry
    // since we're about to delete the pointer to pBT!
    getParentDataset()->setVariableGotValues(pBT, true);

    // Copy the entire AttrTable tree (recursively) from the place holder into the new variable
    pNewArrCV->get_attr_table() = pBT->get_attr_table();

    // Delete the placeholder
    DDS* pDDS = getParentDataset()->getDDS();
    VALID_PTR(pDDS);
    pDDS->del_var(pBT->name());

    // Add the new one, which will copy it (argh! we need to fix this in libdap!)
    // OPTIMIZE  use non copy add when available.
    BESDEBUG("ncml", "Adding CV: " << pNewArrCV->name() << endl);
#if 0
    pDDS->add_var(pNewArrCV.get()); // use raw ptr for the copy.
#endif
    pDDS->add_var_nocopy(pNewArrCV.release());

    // Pull out the copy we just added and hand it back
    Array* pArrCV = static_cast<Array*>(AggregationUtil::getVariableNoRecurse(*pDDS, dim.name));
    VALID_PTR(pArrCV);
    return pArrCV;
}

unique_ptr<libdap::Array> AggregationElement::createCoordinateVariableForNewDimension(
    const agg_util::Dimension& dim) const
{
    // Get the netcdf@coordValue or use the netcdf@location (or auto generate if empty() ).
    NCML_ASSERT(_datasets.size() > 0);
    bool hasCoordValue = !(_datasets[0]->coordValue().empty());
    if (hasCoordValue) {
        return createCoordinateVariableForNewDimensionUsingCoordValue(dim);
    }
    else {
        return createCoordinateVariableForNewDimensionUsingLocation(dim);
    }
}

libdap::Array*
AggregationElement::createAndAddCoordinateVariableForNewDimension(DDS& dds, const agg_util::Dimension& dim)
{
    unique_ptr<libdap::Array> pNewCV = createCoordinateVariableForNewDimension(dim);

    // Make sure it did it
    NCML_ASSERT_MSG(pNewCV.get(),
        "AgregationElement::createCoordinateVariableForNewDimension() failed to create a coordinate variable!");

    // Add it to the DDS, which will make a copy
    // (change this when we add noncopy add_var to DDS)
    //
    // Fix. This will append the variable to the DDS; we need these CVs to be
    // prefixes to the Grids (so that old versions of the netCDF library will
    // recognize them). jhrg 10/17/11
    BESDEBUG("ncml2", "AggregationElement::createAndAddCoordinateVariableForNewDimension: " << pNewCV->name());
#if 0
    dds.add_var(pNewCV.get());
#else
    // This provides a way to remember where the last CV was inserted and adds
    // this one after it. That provides the behavior that all of the CVs are
    // added at the beginning of the DDS but in the order they appear in the NCML.
    // That will translate into a greater chance of success for users, I think ...
    //
    // See also similar code in AggregationUtil::addCopyOfVariableIfNameIsAvailable.
    // jhrg 10/17/11
    static int last_added = 0;
    DDS::Vars_iter pos = dds.var_begin();
    for (int i = 0; i < last_added; ++i)
        ++pos;

    dds.insert_var(pos, pNewCV.get());
    ++last_added;
#endif
    // Grab the copy back out and set to our expected result.
    Array* pCV = static_cast<Array*>(AggregationUtil::getVariableNoRecurse(dds, dim.name));

    NCML_ASSERT_MSG(pCV, "Logic Error: tried to add a new coordinate variable while processing joinNew"
        " but we couldn't locate it!");
    return pCV;
}

unique_ptr<libdap::Array> AggregationElement::createCoordinateVariableForNewDimensionUsingCoordValue(
    const agg_util::Dimension& dim) const
{
    NCML_ASSERT(_datasets.size() > 0);
    NCML_ASSERT_MSG(_datasets.size() == dim.size, "Logic error: Number of datasets doesn't match dimension!");
    // Use first dataset to define the proper type
    double doubleVal = 0;
    if (_datasets[0]->getCoordValueAsDouble(doubleVal)) {
        return createCoordinateVariableForNewDimensionUsingCoordValueAsDouble(dim);
    }
    else {
        return createCoordinateVariableForNewDimensionUsingCoordValueAsString(dim);
    }
}

unique_ptr<libdap::Array> AggregationElement::createCoordinateVariableForNewDimensionUsingCoordValueAsDouble(
    const agg_util::Dimension& dim) const
{
    vector<dods_float64> coords;
    coords.reserve(dim.size);
    double doubleVal = 0;
    // Use the index rather than iterator so we can use it in debug output...
    for (unsigned int i = 0; i < _datasets.size(); ++i) {
        const NetcdfElement* pDataset = _datasets[i];
        if (!pDataset->getCoordValueAsDouble(doubleVal)) {
            THROW_NCML_PARSE_ERROR(line(),
                "In creating joinNew coordinate variable from coordValue, expected a coordValue of type double"
                    " but failed!  coordValue=" + pDataset->coordValue() + " which was in the dataset location="
                    + pDataset->location() + " with title=\"" + pDataset->title() + "\"");
        }
        else // we got our value fine, so add it
        {
            coords.push_back(static_cast<dods_float64>(doubleVal));
        }
    }

    // If we got here, we have the array of coords.
    // So we need to make the proper array, fill it in, and return it.
    unique_ptr<Array> pNewCV = MyBaseTypeFactory::makeArrayTemplateVariable("Array<Float64>", dim.name, true);
    NCML_ASSERT_MSG(pNewCV.get(), "createCoordinateVariableForNewDimensionUsingCoordValueAsDouble: failed to create"
        " the new Array<Float64> for variable: " + dim.name);
    pNewCV->append_dim(dim.size, dim.name);
    pNewCV->set_value(coords, coords.size()); // this will set the length correctly.
    return pNewCV;
}

unique_ptr<libdap::Array> AggregationElement::createCoordinateVariableForNewDimensionUsingCoordValueAsString(
    const agg_util::Dimension& dim) const
{
    // I feel suitably dirty for cut and pasting this.
    vector<string> coords;
    coords.reserve(dim.size);
    for (unsigned int i = 0; i < _datasets.size(); ++i) {
        const NetcdfElement* pDataset = _datasets[i];
        if (pDataset->coordValue().empty()) {
            int parseLine = line();
            THROW_NCML_PARSE_ERROR(parseLine,
                "In creating joinNew coordinate variable from coordValue, expected a coordValue of type string"
                    " but it was empty! dataset location=" + pDataset->location() + " with title=\"" + pDataset->title()
                    + "\"");
        }
        else // we got our value fine, so add it
        {
            coords.push_back(pDataset->coordValue());
        }
    }
    // If we got here, we have the array of coords.
    // So we need to make the proper array, fill it in, and return it.
    unique_ptr<Array> pNewCV = MyBaseTypeFactory::makeArrayTemplateVariable("Array<String>", dim.name, true);
    NCML_ASSERT_MSG(pNewCV.get(), "createCoordinateVariableForNewDimensionUsingCoordValueAsString: failed to create"
        " the new Array<String> for variable: " + dim.name);
    pNewCV->append_dim(dim.size, dim.name);
    pNewCV->set_value(coords, coords.size()); // this will set the length correctly.
    return pNewCV;
}

unique_ptr<libdap::Array> AggregationElement::createCoordinateVariableForNewDimensionUsingLocation(
    const agg_util::Dimension& dim) const
{
    // I feel suitably dirty for cut and pasting this.
    vector<string> coords;
    coords.reserve(dim.size);
    for (unsigned int i = 0; i < _datasets.size(); ++i) {
        const NetcdfElement* pDataset = _datasets[i];
        string location("");
        if (pDataset->location().empty()) {
            std::ostringstream oss;
            oss << "Virtual_Dataset_" << i;
            location = oss.str();
        }
        else // we got our value fine, so add it
        {
            location = pDataset->location();
        }
        coords.push_back(location);
    }
    // If we got here, we have the array of coords.
    // So we need to make the proper array, fill it in, and return it.
    unique_ptr<Array> pNewCV = MyBaseTypeFactory::makeArrayTemplateVariable("Array<String>", dim.name, true);
    NCML_ASSERT_MSG(pNewCV.get(),
        "createCoordinateVariableForNewDimensionUsingCoordValueUsingLocation: failed to create"
            " the new Array<String> for variable: " + dim.name);

    pNewCV->append_dim(dim.size, dim.name);
    pNewCV->set_value(coords, coords.size());
    return pNewCV;
}

void AggregationElement::collectDatasetsInOrder(vector<const DDS*>& ddsList) const
{
    ddsList.resize(0);
    ddsList.reserve(_datasets.size());
    vector<NetcdfElement*>::const_iterator endIt = _datasets.end();
    vector<NetcdfElement*>::const_iterator it;
    for (it = _datasets.begin(); it != endIt; ++it) {
        const NetcdfElement* elt = *it;
        VALID_PTR(elt);
        const DDS* pDDS = elt->getDDS();
        VALID_PTR(pDDS);
        ddsList.push_back(pDDS);
    }
}

void AggregationElement::collectAggMemberDatasets(AMDList& rMemberDatasets) const
{
    rMemberDatasets.resize(0);
    rMemberDatasets.reserve(_datasets.size());

    for (vector<NetcdfElement*>::const_iterator it = _datasets.begin(); it != _datasets.end(); ++it) {
        VALID_PTR(*it);
        RCPtr<AggMemberDataset> pAGM((*it)->getAggMemberDataset());
        VALID_PTR(pAGM.get());

        // Push down the ncoords hint if it was given
        if (!((*it)->ncoords().empty()) && !_dimName.empty()) {
            if (!(pAGM->isDimensionCached(_dimName))) {
                unsigned int ncoords = (*it)->getNcoordsAsUnsignedInt();
                pAGM->setDimensionCacheFor(agg_util::Dimension(_dimName, ncoords), false);
            }
        }

        // don't need to ref(), the RCPtr copy ctor in the vector elt
        // takes care of it when we push_back()
        rMemberDatasets.push_back(pAGM);
    }
}

void AggregationElement::processAnyScanElements()
{
    if (_scanners.size() > 0) {
        BESDEBUG("ncml", "Started to process " << _scanners.size() << " scan elements..." << endl);
    }

    vector<ScanElement*>::iterator it;
    vector<ScanElement*>::iterator endIt = _scanners.end();
    vector<NetcdfElement*> scannedDatasets;
    for (it = _scanners.begin(); it != endIt; ++it) {
        BESDEBUG("ncml", "Processing scan element = " << (*it)->toString() << " ..." << endl);

        // Run the scanner to get the scanned datasets.
        // These will be sorted, so maintain order.
        (*it)->getDatasetList(scannedDatasets);

        // Add the datasets using the parser call to
        // set the data up correctly,
        // then unref() and remove them from the temp array
        vector<NetcdfElement*>::iterator datasetIt;
        vector<NetcdfElement*>::iterator datasetEndIt = scannedDatasets.end();
        for (datasetIt = scannedDatasets.begin(); datasetIt != datasetEndIt; ++datasetIt) {
            // this will ref() it and make sure we can load it.
            _parser->addChildDatasetToCurrentDataset(*datasetIt);
            // so we unref() it afterwards because we're dumping the temp array
            (*datasetIt)->unref();
        }
        // we're done with it and they're all unref().
        scannedDatasets.clear();
    }
}

void AggregationElement::mergeDimensions(bool checkDimensionMismatch/*=true*/, const std::string& dimToSkip/*=""*/)
{
    NetcdfElement* pParent = getParentDataset();
    // For each dataset in the children....
    vector<NetcdfElement*>::const_iterator datasetsEndIt = _datasets.end();
    vector<NetcdfElement*>::const_iterator datasetsIt;
    for (datasetsIt = _datasets.begin(); datasetsIt != datasetsEndIt; ++datasetsIt) {
        // Check each dimension in it compared to the parent
        const NetcdfElement* dataset = *datasetsIt;
        VALID_PTR(dataset);
        const vector<DimensionElement*>& dimensions = dataset->getDimensionElements();
        vector<DimensionElement*>::const_iterator dimEndIt = dimensions.end();
        vector<DimensionElement*>::const_iterator dimIt;
        for (dimIt = dimensions.begin(); dimIt != dimEndIt; ++dimIt) {
            const DimensionElement* pDim = *dimIt;
            VALID_PTR(pDim);
            // Skip if asked to do so
            if (!dimToSkip.empty() && (pDim->name() == dimToSkip)) {
                continue;
            }
            // Otherwise continue to look it up
            const DimensionElement* pUnionDim = pParent->getDimensionInLocalScope(pDim->name());
            if (pUnionDim) {
                // We'll check the dimensions match no matter what, but only warn unless we're told to check
                if (!pUnionDim->checkDimensionsMatch(*pDim)) {
                    string msg = string("The union aggregation already had a dimension=") + pUnionDim->toString()
                        + " but we found another with different cardinality: " + pDim->toString()
                        + " This is likely an error and could cause a later exception.";
                    BESDEBUG("ncml", "WARNING: " + msg);
                    if (checkDimensionMismatch) {
                        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
                            msg + " Scope=" + _parser->getScopeString());
                    }
                }
            }
            else // if not in the union already, we want to add it!
            {
                // this will up the ref count for it so when child dataset dies, we're good.
                BESDEBUG("ncml",
                    "Dimension name=" << pDim->name() << " was not found in the union yet, so adding it.  The full elt is: " << pDim->toString() << endl);
                pParent->addDimension(const_cast<DimensionElement*>(pDim));
            }
        }
    }
}

#define COORDINATE_AXIS_TYPE_ATTR "_CoordinateAxisType"
void AggregationElement::addCoordinateAxisType(libdap::Array& rCV, const std::string& cat)
{
    AttrTable& rAT = rCV.get_attr_table();
    AttrTable::Attr_iter foundIt = rAT.simple_find(COORDINATE_AXIS_TYPE_ATTR);
    // preexists, then delete it and we'll replace with the new
    if (foundIt != rAT.attr_end()) {
        rAT.del_attr(COORDINATE_AXIS_TYPE_ATTR);
    }

    BESDEBUG("ncml3",
        "Adding attribute to the aggregation variable " << rCV.name() << " Attr is " << COORDINATE_AXIS_TYPE_ATTR << " = " << cat << endl);

    // Either way, now we can add it.
    rAT.append_attr(COORDINATE_AXIS_TYPE_ATTR, "String", cat);
}

vector<string> AggregationElement::getValidAttributes()
{
    vector<string> attrs;
    attrs.push_back("type");
    attrs.push_back("dimName");
    attrs.push_back("recheckEvery");
    return attrs;
}


}

// namespace ncml_module
