//////////////////////////////////////////////////////////////////////////////
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

#include <BaseType.h> // libdap
#include <BESDapResponse.h> // bes

#include "AggMemberDataset.h" // agg_util
#include "AggMemberDatasetDDSWrapper.h" // agg_util
#include "AggMemberDatasetSharedDDSWrapper.h" // agg_util
#include "AggMemberDatasetUsingLocationRef.h" // agg_util
#include "AggregationElement.h"
#include "DimensionElement.h"
#include "NetcdfElement.h"
#include "NCMLDebug.h"
#include "NCMLParser.h"
#include "NCMLUtil.h"
#include "VariableElement.h"

#include <assert.h>
#include <sstream>

using namespace std;
using libdap::DDS;
using namespace agg_util;

namespace ncml_module {

const string NetcdfElement::_sTypeName = "netcdf";
const vector<string> NetcdfElement::_sValidAttributes = getValidAttributes();

NetcdfElement::NetcdfElement() :
    NCMLElement(0), _location(""), _id(""), _title(""), _ncoords(""), _enhance(""), _addRecords(""), _coordValue(""), _fmrcDefinition(
        ""), _gotMetadataDirective(false), _weOwnResponse(false), _loaded(false), _response(0), _aggregation(0), _parentAgg(
        0), _dimensions(), _variableValidator(this)
{
}

NetcdfElement::NetcdfElement(const NetcdfElement& proto) :
    RCObjectInterface(), DDSAccessInterface(), DDSAccessRCInterface(), NCMLElement(proto), _location(proto._location), _id(
        proto._id), _title(proto._title), _ncoords(proto._ncoords), _enhance(proto._enhance), _addRecords(
        proto._addRecords), _coordValue(proto._coordValue), _fmrcDefinition(proto._fmrcDefinition), _gotMetadataDirective(
        false), _weOwnResponse(false), _loaded(false), _response(0), _aggregation(0), _parentAgg(0) // we can't really set this to the proto one or we break an invariant...
        , _dimensions(), _variableValidator(this) // start it empty rather than copy to avoid ref counting errors...
{
    // we can't copy the proto response object...  I'd say just don't allow this.
    // if it's needed later, we can implement a proper full clone on a DDS, but the
    // current one is buggy.
    if (proto._response) {
        THROW_NCML_INTERNAL_ERROR("Can't clone() a NetcdfElement that contains a response!");
    }

    // Yuck clone the whole aggregation.
    if (proto._aggregation.get()) {
        setChildAggregation(proto._aggregation.get()->clone());
    }

    // Deep copy the dimension table so they don't side effect each other...
    vector<DimensionElement*>::const_iterator endIt = proto._dimensions.end();
    vector<DimensionElement*>::const_iterator it;
    for (it = proto._dimensions.begin(); it != endIt; ++it) {
        DimensionElement* elt = *it;
        addDimension(elt->clone());
    }
}

NetcdfElement::~NetcdfElement()
{
    BESDEBUG("ncml:memory", "~NetcdfElement called...");
    // Only if its ours do we nuke it.
    if (_weOwnResponse) {
        SAFE_DELETE(_response);
    }
    _response = 0; // but null it in all cases.
    _parentAgg = 0;
    clearDimensions();

    // _aggregation dtor will take care of the ref itself.
}

const string&
NetcdfElement::getTypeName() const
{
    return _sTypeName;
}

NetcdfElement*
NetcdfElement::clone() const
{
    return new NetcdfElement(*this);
}

void NetcdfElement::setAttributes(const XMLAttributeMap& attrs)
{
    // Make sure they exist in the schema, even if we don't support them.
    validateAttributes(attrs, _sValidAttributes);

    // set them
    _location = attrs.getValueForLocalNameOrDefault("location");
    _id = attrs.getValueForLocalNameOrDefault("id");
    _title = attrs.getValueForLocalNameOrDefault("title");
    _enhance = attrs.getValueForLocalNameOrDefault("enhance");
    _addRecords = attrs.getValueForLocalNameOrDefault("addRecords");
    // Aggregation children only below!
    _ncoords = attrs.getValueForLocalNameOrDefault("ncoords");
    _coordValue = attrs.getValueForLocalNameOrDefault("coordValue");
    _fmrcDefinition = attrs.getValueForLocalNameOrDefault("fmrcDefinition");

    throwOnUnsupportedAttributes();
}

void NetcdfElement::handleBegin()
{
    BESDEBUG("ncml", "NetcdfElement::handleBegin on " << toString() << endl);
    NCMLParser& p = *_parser;

    // Make sure that we are in an AggregationElement if we're not root!
    if (p.getRootDataset() && !p.isScopeAggregation()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Got a nested <netcdf> element which was NOT a direct child of an <aggregation>!");
    }

    // Tell the parser we got a new current dataset.
    // If this is the root, it also needs to set up our response!!
    p.pushCurrentDataset(this);

    // Make sure the attributes that are set are valid for context
    // that we just pushed.
    validateAttributeContextOrThrow();
}

void NetcdfElement::handleContent(const string& content)
{
    if (!NCMLUtil::isAllWhitespace(content)) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Got non-whitespace for element content and didn't expect it.  Element=" + toString() + " content=\""
                + content + "\"");
    }
}

void NetcdfElement::handleEnd()
{
    BESDEBUG("ncml", "NetcdfElement::handleEnd called!" << endl);

    if (!_parser->isScopeNetcdf()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(), "Got close of <netcdf> node while not within one!");
    }

    // If we had an aggregation, we must tell it to finish any post-processing that
    // it needs to do to make the aggregations correct (like add Grid map vectors
    // for new dimensions)
    if (_aggregation.get()) {
        _aggregation->processParentDatasetComplete();
    }

    // At this point, any deferred new variables MUST have their values set
    // to preserve an invariant (and avoid cryptic internal error in libdap).
    // So validate any deferred new variables now:
    _variableValidator.validate(); // throws parse error if failure....

    // Tell the parser to close the current dataset and figure out what the new current one is!
    // We pass our ptr to make sure that we're the current one to avoid logic bugs!!
    _parser->popCurrentDataset(this);

    // We'll leave our element table around until we're destroyed since people are allowed to use it if they
    // maintained a ref to us....
}

string NetcdfElement::toString() const
{
    return "<" + _sTypeName + " " + "location=\"" + _location + "\""
        + // always print this one even in empty.
        printAttributeIfNotEmpty("id", _id) + printAttributeIfNotEmpty("title", _title)
        + printAttributeIfNotEmpty("enhance", _enhance) + printAttributeIfNotEmpty("addRecords", _addRecords)
        + printAttributeIfNotEmpty("ncoords", _ncoords) + printAttributeIfNotEmpty("coordValue", _coordValue)
        + printAttributeIfNotEmpty("fmrcDefinition", _fmrcDefinition) + ">";
}

const libdap::DDS*
NetcdfElement::getDDS() const
{
    return const_cast<NetcdfElement*>(this)->getDDS();
}

libdap::DDS*
NetcdfElement::getDDS()
{
    // lazy eval loading the dds
    if (!_loaded) {
        BESDEBUG("ncml", "Lazy loading DDX for location=" << location() << endl);
        loadLocation();
    }

    if (_response) {
        return NCMLUtil::getDDSFromEitherResponse(_response);
    }
    else {
        return 0;
    }
}

bool NetcdfElement::isValid() const
{
    // Right now we need these ptrs valid to be ready...
    // Technically handleBegin() sets the parser,
    // so we're not ready until after that has successfully completed.
    return _response && _parser;
}

unsigned int NetcdfElement::getNcoordsAsUnsignedInt() const
{
    NCML_ASSERT_MSG(hasNcoords(),
        "NetcdfElement::getNCoords(): called illegally when no ncoords attribute was specified!");
    unsigned int num = 0;
    if (!NCMLUtil::toUnsignedInt(_ncoords, num)) {
        THROW_NCML_PARSE_ERROR(line(), "A <netcdf> element has an invalid ncoords attribute set.  Bad value was:"
            "\"" + _ncoords + "\"");
    }
    return num;
}

void NetcdfElement::borrowResponseObject(BESDapResponse* pResponse)
{
    NCML_ASSERT_MSG(!_response, "_response object should be NULL for proper logic of borrowResponseObject call!");
    _response = pResponse;
    _weOwnResponse = false;
}

void NetcdfElement::unborrowResponseObject(BESDapResponse* pResponse)
{
    NCML_ASSERT_MSG(pResponse == _response,
        "NetcdfElement::unborrowResponseObject() called with a response we are not borrowing.");
    _response = 0;
}

void NetcdfElement::createResponseObject(DDSLoader::ResponseType type)
{
    if (_response) {
        THROW_NCML_INTERNAL_ERROR(
            "NetcdfElement::createResponseObject(): Called when we already had a _response!  Logic error!");
    }

    VALID_PTR(_parser);

    // Make a new response and store the raw ptr, noting that we need to delete it in dtor.
    std::auto_ptr<BESDapResponse> newResponse = _parser->getDDSLoader().makeResponseForType(type);
    VALID_PTR(newResponse.get());
    _response = newResponse.release();
    _weOwnResponse = true;
}

RCPtr<agg_util::AggMemberDataset> NetcdfElement::getAggMemberDataset() const
{
    // If not created yet, make a new
    // one in an RCPtr, store a weak ref,
    // and return it.
    RCPtr<AggMemberDataset> pAGM(0);
    if (_pDatasetWrapper.empty()) {
        if (_location.empty()) {
            // if the location is empty, we must assume we have a valid DDS
            // that has been created virtually or as a nested aggregation
            // We create a wrapper for the NetcdfElement in this case
            // using the ref-counted DDS accessor interface.
            const DDSAccessRCInterface* pDDSHolder = this;
            pAGM = RCPtr<AggMemberDataset>(new AggMemberDatasetSharedDDSWrapper(pDDSHolder));
        }
        else {
            pAGM = new AggMemberDatasetUsingLocationRef(_location, _parser->getDDSLoader());
        }

        VALID_PTR(pAGM.get());

        // Make a weak ref to it, semantically const
        const_cast<NetcdfElement*>(this)->_pDatasetWrapper = WeakRCPtr<AggMemberDataset>(pAGM.get());
    }

    // Either way the weak ref is valid here, so return a new RCPtr to it
    NCML_ASSERT(!_pDatasetWrapper.empty());
    return _pDatasetWrapper.lock();
}

const DimensionElement*
NetcdfElement::getDimensionInLocalScope(const string& name) const
{
    const DimensionElement* ret = 0;
    vector<DimensionElement*>::const_iterator endIt = _dimensions.end();
    for (vector<DimensionElement*>::const_iterator it = _dimensions.begin(); it != endIt; ++it) {
        const DimensionElement* pElt = *it;
        VALID_PTR(pElt);
        if (pElt->name() == name) {
            ret = pElt;
            break;
        }
    }
    return ret;
}

const DimensionElement*
NetcdfElement::getDimensionInFullScope(const string& name) const
{
    // Base case...
    const DimensionElement* elt = 0;
    elt = getDimensionInLocalScope(name);
    if (!elt) {
        NetcdfElement* parentDataset = getParentDataset();
        if (parentDataset) {
            elt = parentDataset->getDimensionInFullScope(name);
        }
    }
    return elt;
}

void NetcdfElement::addDimension(DimensionElement* dim)
{
    VALID_PTR(dim);
    if (getDimensionInLocalScope(dim->name())) {
        THROW_NCML_INTERNAL_ERROR(
            "NCMLParser::addDimension(): already found dimension with name while adding " + dim->toString());
    }

    _dimensions.push_back(dim);
    dim->ref(); // strong reference!

    BESDEBUG("ncml", "Added dimension to dataset.  Dimension Table is now: " << printDimensions() << endl);
}

string NetcdfElement::printDimensions() const
{
    string ret = "Dimensions = {\n";
    vector<DimensionElement*>::const_iterator endIt = _dimensions.end();
    vector<DimensionElement*>::const_iterator it;
    for (it = _dimensions.begin(); it != endIt; ++it) {
        ret += (*it)->toString() + "\n";
    }
    ret += "}";
    return ret;
}

void NetcdfElement::clearDimensions()
{
    while (!_dimensions.empty()) {
        DimensionElement* elt = _dimensions.back();
        elt->unref();
        _dimensions.pop_back();
    }
    _dimensions.resize(0);
}

const std::vector<DimensionElement*>&
NetcdfElement::getDimensionElements() const
{
    return _dimensions;
}

void NetcdfElement::setChildAggregation(AggregationElement* agg, bool throwIfExists/*=true*/)
{
    if (_aggregation.get() && throwIfExists) {
        THROW_NCML_INTERNAL_ERROR(
            "NetcdfElement::setAggregation:  We were called but we already contain a non-NULL aggregation!  Previous="
                + _aggregation->toString() + " and the new one is: " + agg->toString());
    }

    // Otherwise, we can just set it and rely on the smart pointer to do the right thing
    _aggregation = agg;  // this will implicitly convert the raw ptr to a smart ptr

    // Also set a weak reference to this as the parent of the aggregation for walking up the tree...
    _aggregation->setParentDataset(this);
}

NetcdfElement*
NetcdfElement::getParentDataset() const
{
    NetcdfElement* ret = 0;
    if (_parentAgg && _parentAgg->getParentDataset()) {
        ret = _parentAgg->getParentDataset();
    }
    return ret;
}

AggregationElement*
NetcdfElement::getParentAggregation() const
{
    return _parentAgg;
}

void NetcdfElement::setParentAggregation(AggregationElement* parent)
{
    _parentAgg = parent;
}

AggregationElement*
NetcdfElement::getChildAggregation() const
{
    return _aggregation.get();
}

#if 0 // not sure we need this yet...
template <typename T>
int
NetcdfElement::getCoordValueVector(vector<T>& values) const
{
    // clear it out just in case.
    values.resize(0);

    // first, tokenize into strings
    vector<string> tokens;
    int numToks = NCMLUtil::tokenize(_coordValue, tokens, NCMLUtil::WHITESPACE);
    values.reserve(numToks);

    for (int i=0; i<numToks; ++i)
    {
        stringstream iss(tokens[i]);
        T val;
        iss >> val;
        if (iss.fail())
        {
            THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
                "NetcdfElement::getCoordValueVector(): "
                "Could not parse the token \"" + tokens[i] + "\""
                " into the required data type.");
        }
        else
        {
            values.push_back(val);
        }
    }
}
#endif

bool NetcdfElement::getCoordValueAsDouble(double& val) const
{
    if (_coordValue.empty()) {
        return false;
    }

    std::istringstream iss(_coordValue);
    double num;
    iss >> num;
    // eof() to make sure we parsed it all.  >> can stop early on malformedness.
    if (iss.fail() || !iss.eof()) {
        return false;
    }
    else {
        val = num;
        return true;
    }
}

void NetcdfElement::addVariableToValidateOnClose(libdap::BaseType* pNewVar, VariableElement* pVE)
{
    _variableValidator.addVariableToValidate(pNewVar, pVE);
}

void NetcdfElement::setVariableGotValues(libdap::BaseType* pVarToValidate, bool removeEntry)
{
    _variableValidator.setVariableGotValues(pVarToValidate);
    if (removeEntry) {
        _variableValidator.removeVariableToValidate(pVarToValidate);
    }
}

/********** Class Methods ***********/

bool NetcdfElement::isLocationLexicographicallyLessThan(const NetcdfElement* pLHS, const NetcdfElement* pRHS)
{
    // args should never be null, but let's catch the error in dev if we goof.
    // I don't want to use VALID_PTR here to avoid exception overhead in an internal
    // sort function.
    assert(pLHS);
    assert(pRHS);
    return (pLHS->location().compare(pRHS->location()) < 0);
}

bool NetcdfElement::isCoordValueLexicographicallyLessThan(const NetcdfElement* pLHS, const NetcdfElement* pRHS)
{
    // args should never be null, but let's catch the error in dev if we goof.
    // I don't want to use VALID_PTR here to avoid exception overhead in an internal
    // sort function.
    assert(pLHS);
    assert(pRHS);
    return (pLHS->coordValue().compare(pRHS->coordValue()) < 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Private Impl

void NetcdfElement::loadLocation()
{
    if (_location.empty()) {
        // nothing to load, so we're loaded I guess.
        _loaded = true;
        return;
    }

    // We better have one!  This gets created up front now.  It will be an empty DDS
    NCML_ASSERT_MSG(_response,
        "NetcdfElement::loadLocation(): Requires a valid _response via borrowResponseObject() or createResponseObject() prior to call!");

    // Use the loader to load the location
    // If not found, this call will throw an exception and we'll just unwind out.
    if (_parser) {
        _parser->loadLocation(_location, _parser->_responseType, _response);
        _loaded = true;
    }
}

void NetcdfElement::throwOnUnsupportedAttributes()
{
    const string msgStart = "NetcdfElement: unsupported attribute: ";
    const string msgEnd = " was declared.";
    if (!_enhance.empty()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(), msgStart + "enhance" + msgEnd);
    }
    if (!_addRecords.empty()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(), msgStart + "addRecords" + msgEnd);
    }

    // not until fmrc
    if (!_fmrcDefinition.empty()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(), msgStart + "fmrcDefinition" + msgEnd);
    }
}

bool NetcdfElement::validateAttributeContextOrThrow() const
{
    if (hasNcoords()) {
        AggregationElement* pParentAgg = getParentAggregation();
        if (!pParentAgg || !(pParentAgg->isJoinExistingAggregation())) {
            THROW_NCML_PARSE_ERROR(line(), "Cannot specify netcdf@ncoords attribute while not within a "
                "joinExisting aggregation!");
        }
    }
    return true;
}

vector<string> NetcdfElement::getValidAttributes()
{
    vector<string> validAttrs;
    validAttrs.push_back("schemaLocation");
    validAttrs.push_back("location");
    validAttrs.push_back("id");
    validAttrs.push_back("title");
    validAttrs.push_back("enhance");
    validAttrs.push_back("addRecords");
    // following only valid inside aggregation, will need to test for that later.
    validAttrs.push_back("ncoords");
    validAttrs.push_back("coordValue");
    validAttrs.push_back("fmrcDefinition");

    return validAttrs;
}

/********************  Inner Class Def of VariableValueValidator *********************/

typedef vector<NetcdfElement::VariableValueValidator::VVVEntry> VVVEntries;

NetcdfElement::VariableValueValidator::VariableValueValidator(NetcdfElement* pParent) :
    _entries(), _pParent(pParent)
{
}

NetcdfElement::VariableValueValidator::~VariableValueValidator()
{
    // For each entry, deref the VariableElement and clear it out.
    VVVEntries::iterator it;
    VVVEntries::iterator endIt = _entries.end();
    for (it = _entries.begin(); it != endIt; ++it) {
        VVVEntry& entry = *it;
        entry._pVarElt->unref();
        entry.clear();
    }
    _entries.resize(0);
}

void NetcdfElement::VariableValueValidator::addVariableToValidate(libdap::BaseType* pNewVar, VariableElement* pVE)
{
    VALID_PTR(pNewVar);
    VALID_PTR(pVE);
    {
        VVVEntry* pExisting = findEntryByLibdapVar(pNewVar);
        NCML_ASSERT_MSG(!pExisting,
            "NetcdfElement::VariableValueValidator::addVariableToValidate: var was already added!");
    }

    // Up the ref count to keep it around and add an entry for it
    pVE->ref();
    _entries.push_back(VVVEntry(pNewVar, pVE));
}

void NetcdfElement::VariableValueValidator::removeVariableToValidate(libdap::BaseType* pVarToRemove)
{
    for (unsigned int i = 0; i < _entries.size(); ++i) {
        if (_entries[i]._pNewVar == pVarToRemove) {
            _entries[i]._pVarElt->unref();
            _entries[i] = _entries[_entries.size() - 1];
            _entries.pop_back();
            break;
        }
    }
}

void NetcdfElement::VariableValueValidator::setVariableGotValues(libdap::BaseType* pVarToValidate)
{
    VALID_PTR(pVarToValidate);
    VVVEntry* pEntry = findEntryByLibdapVar(pVarToValidate);
    NCML_ASSERT_MSG(pEntry,
        "NetcdfElement::VariableValueValidator::setVariableGotValues: expected to find the var name="
            + pVarToValidate->name() + " but we did not!");
    pEntry->_pVarElt->setGotValues();
}

VariableElement*
NetcdfElement::findVariableElementForLibdapVar(libdap::BaseType* pNewVar)
{
    return _variableValidator.findVariableElementForLibdapVar(pNewVar);
}

bool NetcdfElement::VariableValueValidator::validate()
{
    VVVEntries::iterator it;
    VVVEntries::iterator endIt = _entries.end();
    for (it = _entries.begin(); it != endIt; ++it) {
        VVVEntry& entry = *it;
        // If the values was never set...
        if (!entry._pVarElt->checkGotValues()) {
            THROW_NCML_PARSE_ERROR(_pParent->line(),
                "On closing the <netcdf> element, we found a new variable name=" + entry._pNewVar->name()
                    + " that was added"
                        " to the dataset but which never had values set on it.  This is illegal!"
                        " Please make sure all variables in this dataset have values set on them"
                        " or that they are new coordinate variables for a joinNew aggregation.");
        }
    }
    return true;
}

NetcdfElement::VariableValueValidator::VVVEntry*
NetcdfElement::VariableValueValidator::findEntryByLibdapVar(libdap::BaseType* pVarToFind)
{
    VALID_PTR(pVarToFind);
    VVVEntry* pRetVal = 0;
    VVVEntries::iterator it;
    VVVEntries::iterator endIt = _entries.end();
    for (it = _entries.begin(); it != endIt; ++it) {
        VVVEntry& entry = *it;
        if (entry._pNewVar == pVarToFind) {
            pRetVal = &entry;
            break;
        }
    }
    return pRetVal;
}

VariableElement*
NetcdfElement::VariableValueValidator::findVariableElementForLibdapVar(libdap::BaseType* pVarToFind)
{
    VALID_PTR(pVarToFind);
    VVVEntry* pRetVal = findEntryByLibdapVar(pVarToFind);
    if (pRetVal) {
        return pRetVal->_pVarElt;
    }
    else {
        return 0;
    }
}

} // namespace ncml_module
