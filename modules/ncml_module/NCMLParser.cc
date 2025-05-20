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
#include "NCMLParser.h"  // ncml_module

#include "AggregationElement.h"  // ncml_module
#include "AggregationUtil.h" // agg_util
#include <BESConstraintFuncs.h>
#include <BESDataDDSResponse.h>
#include <BESDDSResponse.h>
#include <BESDebug.h>
#include <BESStopWatch.h>
#include "DDSLoader.h" // ncml_module
#include "DimensionElement.h"  // ncml_module
#include <libdap/AttrTable.h> // libdap
#include <libdap/BaseType.h> // libdap
#include <libdap/DAS.h> // libdap
#include <libdap/DDS.h> // libdap
//#include <libdap/mime_util.h>
#include <libdap/Structure.h> // libdap
#include <map>
#include <memory>
#include "NCMLDebug.h" // ncml_module
#include "NCMLElement.h"  // ncml_module
#include "NCMLUtil.h"  // ncml_module
#include "NetcdfElement.h"  // ncml_module
#include "OtherXMLParser.h" // ncml_module
#include <libdap/parser.h> // libdap  for the type checking...
#include "SaxParserWrapper.h"  // ncml_module
#include <sstream>

// For extra debug spew for now.
#define DEBUG_NCML_PARSER_INTERNALS 1
#define MODULE "ncml"
#define prolog std::string("NCMLParser::").append(__func__).append("() - ")

using namespace agg_util;

namespace ncml_module {

// From the DAP 2 guide....
static const unsigned int MAX_DAP_STRING_SIZE = 32767;

// Consider filling this with a compilation flag.
/* static */bool NCMLParser::sThrowExceptionOnUnknownElements = true;

// An attribute or variable with type "Structure" will match this string.
const string NCMLParser::STRUCTURE_TYPE("Structure");

// Just cuz I hate magic -1.  Used in _currentParseLine
static const int NO_CURRENT_PARSE_LINE_NUMBER = -1;

/////////////////////////////////////////////////////////////////////////////////////////////
//  Helper class.
AttrTableLazyPtr::AttrTableLazyPtr(const NCMLParser& parser, AttrTable* pAT/*=0*/) :
    _parser(parser), _pAttrTable(pAT), _loaded(pAT)
{
}

AttrTableLazyPtr::~AttrTableLazyPtr()
{
    _pAttrTable = 0;
    _loaded = false;
}

AttrTable*
AttrTableLazyPtr::get() const
{
    if (!_loaded) {
        const_cast<AttrTableLazyPtr*>(this)->loadAndSetAttrTable();
    }
    return _pAttrTable;
}

void AttrTableLazyPtr::set(AttrTable* pAT)
{
    _pAttrTable = pAT;
    if (pAT) {
        _loaded = true;
    }
    else {
        _loaded = false;
    }
}

void AttrTableLazyPtr::invalidate()
{
    // force it to load next get().
    _pAttrTable = 0;
    _loaded = false;
}

void AttrTableLazyPtr::loadAndSetAttrTable()
{
    set(0);
    NetcdfElement* pDataset = _parser.getCurrentDataset();
    if (pDataset) {
        // The lazy load actually occurs in here
        DDS* pDDS = pDataset->getDDS();
        if (pDDS) {
            set(&(pDDS->get_attr_table()));
            _loaded = true;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
////// Public

NCMLParser::NCMLParser(DDSLoader& loader) :
    _filename(""), _loader(loader), _responseType(DDSLoader::eRT_RequestDDX), _response(0), _rootDataset(0), _currentDataset(
        0), _pVar(0), _pCurrentTable(*this, 0), _elementStack(), _scope(), _namespaceStack(), _pOtherXMLParser(0), _currentParseLine(
        NO_CURRENT_PARSE_LINE_NUMBER)
{
    BESDEBUG(MODULE, prolog << "Created NCMLParser." << endl);
}

NCMLParser::~NCMLParser()
{
    // clean other stuff up
    cleanup();
}

unique_ptr<BESDapResponse> NCMLParser::parse(const string& ncmlFilename, DDSLoader::ResponseType responseType)
{
    // Parse into a newly created object.
    unique_ptr<BESDapResponse> response = DDSLoader::makeResponseForType(responseType);

    // Parse into the response.  We still got it in the unique_ptr in this scope, so we're safe
    // on exception since the unique_ptr in this func will cleanup the memory.
    parseInto(ncmlFilename, responseType, response.get());

    // Relinquish it to the caller
    return response;
}

void NCMLParser::parseInto(const string& ncmlFilename, DDSLoader::ResponseType responseType, BESDapResponse* response)
{
    BES_STOPWATCH_START(MODULE, prolog + "Timer");

    VALID_PTR(response);
    NCML_ASSERT_MSG(DDSLoader::checkResponseIsValidType(responseType, response),
        "NCMLParser::parseInto: got wrong response object for given type.");

    _responseType = responseType;
    _response = response;

    if (parsing()) {
        THROW_NCML_INTERNAL_ERROR("Illegal Operation: NCMLParser::parse called while already parsing!");
    }

    BESDEBUG(MODULE, prolog << "Beginning NcML parse of file=" << ncmlFilename << endl);

    // In case we care.
    _filename = ncmlFilename;

    // Invoke the libxml sax parser
    SaxParserWrapper parser(*this);

    parser.parse(ncmlFilename);

    // Prepare for a new parse, making sure it's all cleaned up (with the exception of the _ddsResponse
    // which where's about to send off)
    resetParseState();

    // we're done with it.
    _response = 0;
}

bool NCMLParser::parsing() const
{
    return !_filename.empty();
}

int NCMLParser::getParseLineNumber() const
{
    return _currentParseLine;
}

const XMLNamespaceStack&
NCMLParser::getXMLNamespaceStack() const
{
    return _namespaceStack;
}

void NCMLParser::onStartDocument()
{
    BESDEBUG(MODULE, prolog << "onStartDocument." << endl);
}

void NCMLParser::onEndDocument()
{
    BESDEBUG(MODULE, prolog << "onEndDocument." << endl);
}

void NCMLParser::onStartElement(const std::string& name, const XMLAttributeMap& attrs)
{
    // If we have a proxy set for OtherXML, pass calls there.
    if (isParsingOtherXML()) {
        VALID_PTR(_pOtherXMLParser);
        _pOtherXMLParser->onStartElement(name, attrs);
    }
    else // Otherwise do the standard NCML parse
    {
        processStartNCMLElement(name, attrs);
    }
}

// Local helper for below...
// Sees whether we are closing the element on top
// of the NCMLElement stack and that we're not parsing
// OtherXML, or if we are that its depth is now zero.
static bool shouldStopOtherXMLParse(NCMLElement* top, const string& closingElement, OtherXMLParser& rProxyParser)
{
    // If the stack top element name is the same as the element we are closing...
    // and the parse depth is 0, then we're done.
    // We MUST check the parse depth in case the other XML has an Attribute in it!
    // We want to be sure we're closing the right one.
    if (top->getTypeName() == closingElement && rProxyParser.getParseDepth() == 0) {
        return true;
    }
    else // we're not done.
    {
        return false;
    }
}

void NCMLParser::onEndElement(const std::string& name)
{
    NCMLElement* elt = getCurrentElement();
    VALID_PTR(elt);

    // First, handle the OtherXML proxy parsing case
    if (isParsingOtherXML()) {
        VALID_PTR(_pOtherXMLParser);
        // If we're closing the element that caused the OtherXML parse...
        if (shouldStopOtherXMLParse(elt, name, *_pOtherXMLParser)) {
            // Then we want to clear the proxy from this and
            // call the end on the top of the element stack.
            // We assume it has access to the OtherXML parser
            // and will use the data.
            _pOtherXMLParser = 0;
            processEndNCMLElement(name);
        }
        else {
            // Pass through to proxy
            _pOtherXMLParser->onEndElement(name);
        }
    }
    else // Do the regular NCMLElement call.
    {
        // Call the regular NCMLElement end element.
        processEndNCMLElement(name);
    }
}

void NCMLParser::onStartElementWithNamespace(const std::string& localname, const std::string& prefix,
    const std::string& uri, const XMLAttributeMap& attributes, const XMLNamespaceMap& namespaces)
{
    // If we have a proxy set for OtherXML, pass calls there.
    if (isParsingOtherXML()) {
        VALID_PTR(_pOtherXMLParser);
        _pOtherXMLParser->onStartElementWithNamespace(localname, prefix, uri, attributes, namespaces);
    }
    else // Otherwise do the standard NCML parse
         // but keep the namespaces on the stack.  We don't do this for OtherXML.
    {
        _namespaceStack.push(namespaces);
        processStartNCMLElement(localname, attributes);
    }
}

void NCMLParser::onEndElementWithNamespace(const std::string& localname, const std::string& prefix,
    const std::string& uri)
{
    NCMLElement* elt = getCurrentElement();
    VALID_PTR(elt);

    // First, handle the OtherXML proxy parsing case
    if (isParsingOtherXML()) {
        VALID_PTR(_pOtherXMLParser);
        // If we're closing the element that caused the OtherXML parse...
        if (shouldStopOtherXMLParse(elt, localname, *_pOtherXMLParser)) {
            // Then we want to clear the proxy from this and
            // call the end on the top of the element stack.
            // We assume it has access to the OtherXML parser
            // and will use the data.
            _pOtherXMLParser = 0;
            processEndNCMLElement(localname);
        }
        else {
            // Pass through to proxy
            _pOtherXMLParser->onEndElementWithNamespace(localname, prefix, uri);
        }
    }
    else // Do the regular NCMLElement call.
    {
        // Call the regular NCMLElement end element.
        processEndNCMLElement(localname);
        _namespaceStack.pop();
    }
}

void NCMLParser::onCharacters(const std::string& content)
{
    // If we're parsing OtherXML, send the call to the proxy.
    if (isParsingOtherXML()) {
        VALID_PTR(_pOtherXMLParser);
        _pOtherXMLParser->onCharacters(content);
    }
    else // Standard NCML parse
    {
        // If we got an element on the stack, hand it off.  Otherwise, do nothing.
        NCMLElement* elt = getCurrentElement();
        if (elt) {
            elt->handleContent(content);
        }
    }
}

void NCMLParser::onParseWarning(std::string msg)
{
    // TODO  We may want to make a flag for considering warnings errors as well.
    BESDEBUG(MODULE, prolog << "PARSE WARNING: LibXML msg={" << msg << "}.  Attempting to continue parse." << endl);
}

void NCMLParser::onParseError(std::string msg)
{
    // Pretty much have to give up on malformed XML.
    THROW_NCML_PARSE_ERROR(getParseLineNumber(), "libxml SAX2 parser error! msg={" + msg + "} Terminating parse!");
}

void NCMLParser::setParseLineNumber(int line)
{
    _currentParseLine = line;
    // BESDEBUG(MODULE, prolog << "******** Now parsing line: " << line << endl);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Non-public Implemenation

bool NCMLParser::isScopeAtomicAttribute() const
{
    return (!_scope.empty()) && (_scope.topType() == ScopeStack::ATTRIBUTE_ATOMIC);
}

bool NCMLParser::isScopeAttributeContainer() const
{
    return (!_scope.empty()) && (_scope.topType() == ScopeStack::ATTRIBUTE_CONTAINER);
}

bool NCMLParser::isScopeSimpleVariable() const
{
    return (!_scope.empty()) && (_scope.topType() == ScopeStack::VARIABLE_ATOMIC);
}

bool NCMLParser::isScopeCompositeVariable() const
{
    return (!_scope.empty()) && (_scope.topType() == ScopeStack::VARIABLE_CONSTRUCTOR);
}

bool NCMLParser::isScopeVariable() const
{
    return (isScopeSimpleVariable() || isScopeCompositeVariable());
}

bool NCMLParser::isScopeGlobal() const
{
    return withinNetcdf() && _scope.empty();
}

// TODO Clean up these next two calls with a parser state or something....
// Dynamic casting all the time isn't super fast or clean if not needed...
bool NCMLParser::isScopeNetcdf() const
{
    // see if the last thing parsed was <netcdf>
    return (!_elementStack.empty() && dynamic_cast<NetcdfElement*>(_elementStack.back()));
}

bool NCMLParser::isScopeAggregation() const
{
    // see if the last thing parsed was <netcdf>
    return (!_elementStack.empty() && dynamic_cast<AggregationElement*>(_elementStack.back()));
}

bool NCMLParser::withinNetcdf() const
{
    return _currentDataset != 0;
}

bool NCMLParser::withinVariable() const
{
    return withinNetcdf() && _pVar;
}

agg_util::DDSLoader&
NCMLParser::getDDSLoader() const
{
    return _loader;
}

NetcdfElement*
NCMLParser::getCurrentDataset() const
{
    return _currentDataset;
}

NetcdfElement*
NCMLParser::getRootDataset() const
{
    return _rootDataset;
}

DDS*
NCMLParser::getDDSForCurrentDataset() const
{
    NetcdfElement* dataset = getCurrentDataset();
    NCML_ASSERT_MSG(dataset, "getDDSForCurrentDataset() called when we're not processing a <netcdf> location!");
    return dataset->getDDS();
}

void NCMLParser::pushCurrentDataset(NetcdfElement* dataset)
{
    VALID_PTR(dataset);
    // The first one we get is the root  It's special!
    // We tell it to use the top level response object for the
    // parser, since that's what ultimately is returned
    // and we don't want the root making its own we need to copy.
    bool thisIsRoot = !_rootDataset;
    if (thisIsRoot) {
        _rootDataset = dataset;
        VALID_PTR(_response);
        _rootDataset->borrowResponseObject(_response);
    }
    else {
        addChildDatasetToCurrentDataset(dataset);
    }

    // Also invalidates the AttrTable so it gets cached again.
    setCurrentDataset(dataset);

    // TODO: What do we do with the scope stack for a nested dataset?!
}

void NCMLParser::popCurrentDataset(NetcdfElement* dataset)
{
    if (dataset && dataset != _currentDataset) {
        THROW_NCML_INTERNAL_ERROR(
            "NCMLParser::popCurrentDataset(): the dataset we expect on the top of the stack is not correct!");
    }

    dataset = getCurrentDataset();
    VALID_PTR(dataset);

    // If it's the root, we're done and need to clear up the state.
    if (dataset == _rootDataset) {
        _rootDataset->unborrowResponseObject(_response);
        _rootDataset = 0;
        setCurrentDataset(0);
    }
    else {
        // If it's not the root, it should have a parent, so go get it and make that the new current.
        NetcdfElement* parentDataset = dataset->getParentDataset();
        NCML_ASSERT_MSG(parentDataset, "NCMLParser::popCurrentDataset() got non-root dataset, but it had no parent!!");
        setCurrentDataset(parentDataset);
    }
}

void NCMLParser::setCurrentDataset(NetcdfElement* dataset)
{
    if (dataset) {
        // Make sure it's state is ready to go with operations before making it current
        NCML_ASSERT(dataset->isValid());
        _currentDataset = dataset;
        // We don't set the current attr table, rather it is lazy eval
        // from getCurrentAttrTable() only if called.  This call tells it to do that.
        _pCurrentTable.invalidate();

        // UNLESS it's the root dataset, which we want to force to load
        // since a passthrough file will generate an empty metadata set otherwise
        // since the table is never requested.
        if (_currentDataset == _rootDataset) {
            // Force it to cache so we actually laod the metadata for the root set.
            // Chidl sets are aggregations so we don't load those unless needed.
            _pCurrentTable.set(_pCurrentTable.get());
        }
    }
    else {
        BESDEBUG(MODULE, prolog << "NCMLParser::setCurrentDataset(): setting to NULL..." << endl);
        _currentDataset = 0;
        _pCurrentTable.invalidate();
    }
}

void NCMLParser::addChildDatasetToCurrentDataset(NetcdfElement* dataset)
{
    VALID_PTR(dataset);

    AggregationElement* agg = _currentDataset->getChildAggregation();
    if (!agg) {
        THROW_NCML_INTERNAL_ERROR(
            "NCMLParser::addChildDatasetToCurrentDataset(): current dataset has no aggregation element!  We can't add it!");
    }

    // This will add as strong ref to dataset from agg (child) and a weak to agg from dataset (parent)
    agg->addChildDataset(dataset);

    // Force the dataset to create an internal response object for the request type we're processing
    dataset->createResponseObject(_responseType);
}

bool NCMLParser::parsingDataRequest() const
{
    const BESDataDDSResponse* const pDataDDSResponse = dynamic_cast<const BESDataDDSResponse* const >(_response);
    return (pDataDDSResponse);
}

void NCMLParser::loadLocation(const std::string& location, agg_util::DDSLoader::ResponseType responseType,
    BESDapResponse* response)
{
    VALID_PTR(response);
    _loader.loadInto(location, responseType, response);
}

void NCMLParser::resetParseState()
{
    _filename = "";
    _pVar = 0;
    _pCurrentTable.set(0);

    _scope.clear();

    // Not that this matters...
    _responseType = DDSLoader::eRT_RequestDDX;

    // We never own the memory in this, so just clear it.
    _response = 0;

    // We don't own these either.
    _rootDataset = 0;
    _currentDataset = 0;

    // Cleanup any memory in the _elementStack
    clearElementStack();

    _namespaceStack.clear();

    // just in case
    _loader.cleanup();

    // In case we had one, null it.  The setter is in charge of the memory.
    _pOtherXMLParser = 0;
}

bool NCMLParser::isNameAlreadyUsedAtCurrentScope(const std::string& name)
{
    return (getVariableInCurrentVariableContainer(name) || attributeExistsAtCurrentScope(name));
}

BaseType*
NCMLParser::getVariableInCurrentVariableContainer(const string& name)
{
    return getVariableInContainer(name, _pVar);
}

BaseType*
NCMLParser::getVariableInContainer(const string& varName, BaseType* pContainer)
{
    // BaseType::btp_stack varContext;
    if (pContainer) {
        // @@@ Old code... recurses and uses dots as field separators... Not good.
        //return pContainer->var(varName, varContext);
        // It has to be a Constructor!
        Constructor* pCtor = dynamic_cast<Constructor*>(pContainer);
        if (!pCtor) {
            BESDEBUG(MODULE,
                "WARNING: NCMLParser::getVariableInContainer:  " "Expected a BaseType of subclass Constructor, but didn't get it!" << endl);
            return 0;
        }
        else {
            return agg_util::AggregationUtil::getVariableNoRecurse(*pCtor, varName);
        }
    }
    else {
        return getVariableInDDS(varName);
    }
}

// Not that this should take a fully qualified one too, but without a scoping operator (.) it will
// just search the top level variables.
BaseType*
NCMLParser::getVariableInDDS(const string& varName)
{
    // BaseType::btp_stack varContext;
    // return  getDDSForCurrentDataset()->var(varName, varContext);
    DDS* pDDS = getDDSForCurrentDataset();
    if (pDDS) {
        return agg_util::AggregationUtil::getVariableNoRecurse(*pDDS, varName);
    }
    else {
        return 0;
    }
}

void NCMLParser::addCopyOfVariableAtCurrentScope(BaseType& varTemplate)
{
    // make sure the name is free
    if (isNameAlreadyUsedAtCurrentScope(varTemplate.name())) {
        THROW_NCML_PARSE_ERROR(getParseLineNumber(), "NCMLParser::addNewVariableAtCurrentScope:"
            " Cannot add variable since a variable or attribute of the same name exists at current scope."
            " Name= " + varTemplate.name());
    }

    // Also an internal error if the caller tries it.
    if (!(isScopeCompositeVariable() || isScopeGlobal())) {
        THROW_NCML_INTERNAL_ERROR(
            "NCMLParser::addNewVariableAtCurrentScope: current scope not valid for adding variable.  Scope="
                + getTypedScopeString());
    }

    // OK, we know we can add it now.  But to what?
    if (_pVar) // Constructor variable
    {
        NCML_ASSERT_MSG(_pVar->is_constructor_type(), "Expected _pVar is a container type!");
        _pVar->add_var(&varTemplate);
    }
    else // Top level DDS for current dataset
    {
        BESDEBUG(MODULE,
            "Adding new variable to DDS top level.  Variable name=" << varTemplate.name() << " and typename=" << varTemplate.type_name() << endl);
        DDS* pDDS = getDDSForCurrentDataset();
        pDDS->add_var(&varTemplate);
    }
}

void NCMLParser::deleteVariableAtCurrentScope(const string& name)
{
    if (!(isScopeCompositeVariable() || isScopeGlobal())) {
        THROW_NCML_INTERNAL_ERROR(
            "NCMLParser::deleteVariableAtCurrentScope called when we do not have a variable container at current scope!");
    }

    if (_pVar) // In container?
    {
        // Given interfaces, unfortunately it needs to be a Structure or we can't do this operation.
        Structure* pVarContainer = dynamic_cast<Structure*>(_pVar);
        if (!pVarContainer) {
            THROW_NCML_PARSE_ERROR(getParseLineNumber(),
                "NCMLParser::deleteVariableAtCurrentScope called with _pVar not a "
                    "Structure class variable!  "
                    "We can only delete variables from top DDS or within a Structure now.  scope="
                    + getTypedScopeString());
        }
        // First, make sure it exists so we can warn if not.  The call fails silently.
        BaseType* pToBeNuked = pVarContainer->var(name);
        if (!pToBeNuked) {
            THROW_NCML_PARSE_ERROR(getParseLineNumber(),
                "Tried to remove variable from a Structure, but couldn't find the variable with name=" + name
                    + "at scope=" + getScopeString());
        }
        // Silently fails, so assume it worked.
        pVarContainer->del_var(name);
    }
    else // Global
    {
        // we better have a DDS if we get here!
        DDS* pDDS = getDDSForCurrentDataset();
        VALID_PTR(pDDS);
        pDDS->del_var(name);
    }
}

BaseType*
NCMLParser::getCurrentVariable() const
{
    return _pVar;
}

void NCMLParser::setCurrentVariable(BaseType* pVar)
{
    _pVar = pVar;
    if (pVar) // got a variable
    {
        setCurrentAttrTable(&(pVar->get_attr_table()));
    }
    else if (getDDSForCurrentDataset()) // null pvar but we have a dds, use global table
    {
        DDS* dds = getDDSForCurrentDataset();
        setCurrentAttrTable(&(dds->get_attr_table()));
    }
    else // just clear it out, no context
    {
        setCurrentAttrTable(0);
    }
}

bool NCMLParser::typeCheckDAPVariable(const BaseType& var, const string& expectedType)
{
    // Match all types.
    if (expectedType.empty()) {
        return true;
    }
    else {
        // If the type specifies a Structure, it better be a Constructor type.
        if (expectedType == STRUCTURE_TYPE) {
            // Calls like is_constructor_type really should be const...
            BaseType& varSemanticConst = const_cast<BaseType&>(var);
            return varSemanticConst.is_constructor_type();
        }
        else {
            return (var.type_name() == expectedType);
        }
    }
}

AttrTable*
NCMLParser::getCurrentAttrTable() const
{
    // will load the DDS of current dataset if required.
    // The end result of calling AttrTableLazyPtr::get() is that the NCMLParser
    // field '_pAttrTable' points to the DDS' AttrTable.
    return _pCurrentTable.get();
}

void NCMLParser::setCurrentAttrTable(AttrTable* pAT)
{
    _pCurrentTable.set(pAT);
}

AttrTable*
NCMLParser::getGlobalAttrTable() const
{
    AttrTable* pAT = 0;
    DDS* pDDS = getDDSForCurrentDataset();
    if (pDDS) {
        pAT = &(pDDS->get_attr_table());
    }
    return pAT;
}

bool NCMLParser::attributeExistsAtCurrentScope(const string& name) const
{
    // Lookup the given attribute in the current table.
    AttrTable::Attr_iter attr;
    bool foundIt = findAttribute(name, attr);
    return foundIt;
}

bool NCMLParser::findAttribute(const string& name, AttrTable::Attr_iter& attr) const
{
    AttrTable* pAT = getCurrentAttrTable();
    if (pAT) {
        attr = pAT->simple_find(name);
        return (attr != pAT->attr_end());
    }
    else {
        return false;
    }
}

int NCMLParser::tokenizeAttrValues(vector<string>& tokens, const string& values, const string& dapAttrTypeName,
    const string& separator)
{
    // Convert the type string into a DAP AttrType to be sure
    AttrType dapType = String_to_AttrType(dapAttrTypeName);
    if (dapType == Attr_unknown) {
        THROW_NCML_PARSE_ERROR(getParseLineNumber(),
            "Attempting to tokenize attribute value failed since"
                " we found an unknown internal DAP type=" + dapAttrTypeName
                + " for the current fully qualified attribute=" + _scope.getScopeString());
    }

    // If we're valid type, tokenize us according to type.
    int numTokens = tokenizeValuesForDAPType(tokens, values, dapType, separator);
    if (numTokens == 0 && ((dapType == Attr_string) || (dapType == Attr_url) || (dapType == Attr_other_xml))) {
        tokens.push_back(""); // 0 tokens will cause a problem later, so push empty string!
    }

    // Now type check the tokens are valid strings for the type.
    NCMLParser::checkDataIsValidForCanonicalTypeOrThrow(dapAttrTypeName, tokens);

#if DEBUG_NCML_PARSER_INTERNALS

    if (separator != NCMLUtil::WHITESPACE) {
        BESDEBUG(MODULE, prolog << "Got non-default separators for tokenize.  separator=\"" << separator << "\"" << endl);
    }

    string msg;
    for (unsigned int i = 0; i < tokens.size(); i++) {
        if (i > 0) {
            msg += ",";
        }
        msg += "\"";
        msg += tokens[i];
        msg += "\"";
    }
    BESDEBUG(MODULE, prolog << "Tokenize got " << numTokens << " tokens:\n" << msg << endl);

#endif // DEBUG_NCML_PARSER_INTERNALS

    return numTokens;
}

int NCMLParser::tokenizeValuesForDAPType(vector<string>& tokens, const string& values, AttrType dapType,
    const string& separator)
{
    tokens.resize(0);  // Start empty.
    int numTokens = 0;

    if (dapType == Attr_unknown) {
        // Do out best to recover....
        BESDEBUG(MODULE,
            "Warning: tokenizeValuesForDAPType() got unknown DAP type!  Attempting to continue..." << endl);
        tokens.push_back(values);
        numTokens = 1;
    }
    else if (dapType == Attr_container) {
        // Not supposed to have values, just push empty string....
        BESDEBUG(MODULE, prolog << "Warning: tokenizeValuesForDAPType() got container type, we should not have values!" << endl);
        tokens.push_back("");
        numTokens = 1;
    }
    else if (dapType == Attr_string) {
        // Don't use whitespace as default separator for strings.
        // If they explicitly set it, then fine.
        // We don't trim strings either.  All whitespace, trailing or leading, is left.
        numTokens = NCMLUtil::tokenize(values, tokens, separator);
    }
    else // For all other atomic types, do a split on separator
    {
        // Use whitespace as default if sep not set
        string sep = ((separator.empty()) ? (NCMLUtil::WHITESPACE) : (separator));
        numTokens = NCMLUtil::tokenize(values, tokens, sep);
        NCMLUtil::trimAll(tokens);
    }
    return numTokens;
}

////////////////////////////////////// Class Methods (Statics)

// Used below to convert NcML data type to a DAP data type.
typedef std::map<string, string> TypeConverter;

// If true, we allow the specification of a DAP scalar type
// in a location expecting an NcML type.
static const bool ALLOW_DAP_TYPES_AS_NCML_TYPES = true;

/*
 * Causes a small memory leak that shows up in Valgrind but is ignored as the leak does not as grow since
 * TypeConverter object is only allocated once per process. SBL 10.31.19
 *
 * Ncml DataType:
 <xsd:enumeration value="char"/>
 <xsd:enumeration value="byte"/>
 <xsd:enumeration value="short"/>
 <xsd:enumeration value="int"/>
 <xsd:enumeration value="long"/>
 <xsd:enumeration value="float"/>
 <xsd:enumeration value="double"/>
 <xsd:enumeration value="String"/>
 <xsd:enumeration value="string"/>
 <xsd:enumeration value="Structure"/>
 */
static TypeConverter* makeTypeConverter()
{
    TypeConverter* ptc = new TypeConverter();
    TypeConverter& tc = *ptc;
    // NcML to DAP conversions
    tc["char"] = "Byte"; // char is a C char, let's use a Byte and special parse it as a char not numeric
    tc["byte"] = "Int16"; // Since NcML byte's can be signed, we must promote them to not lose the sign bit.
    tc["short"] = "Int16";
    tc["int"] = "Int32";
    tc["long"] = "Int32"; // not sure of this one
    tc["float"] = "Float32";
    tc["double"] = "Float64";
    tc["string"] = "String"; // allow lower case.
    tc["String"] = "String";
    tc["Structure"] = "Structure";
    tc["structure"] = "Structure"; // allow lower case for this as well

    // If we allow DAP types to be specified directly,
    // then make them be passthroughs in the converter...
    if (ALLOW_DAP_TYPES_AS_NCML_TYPES) {
        tc["Byte"] = "Byte"; // DAP Byte can fit in Byte tho, unlike NcML "byte"!
        tc["Int16"] = "Int16";
        tc["UInt16"] = "UInt16";
        tc["Int32"] = "Int32";
        tc["UInt32"] = "UInt32";
        tc["Float32"] = "Float32";
        tc["Float64"] = "Float64";
        // allow both url cases due to old bug where "Url" is returned in dds rather then DAP2 spec "URL"
        tc["Url"] = "URL";
        tc["URL"] = "URL";
        tc["OtherXML"] = "OtherXML"; // Pass it through
    }

    return ptc;
}

// Singleton
static const TypeConverter& getTypeConverter()
{
    static TypeConverter* singleton = 0;
    if (!singleton) {
        singleton = makeTypeConverter();
    }
    return *singleton;
}

#if 0 // Unused right now... might be later, but I hate compiler warnings.
// Is the given type a DAP type?
static bool isDAPType(const string& type)
{
    return (String_to_AttrType(type) != Attr_unknown);
}
#endif // 0

/* static */
string NCMLParser::convertNcmlTypeToCanonicalType(const string& ncmlType)
{

#if 0
	// OLD WAY - Disallows attributes that do not specify type
	NCML_ASSERT_MSG(!daType.empty(), "Logic error: convertNcmlTypeToCanonicalType disallows empty() input.");
#endif

	// NEW WAY - If the attribute does not specify a type them the type is defaulted to "string"
	string daType = ncmlType;
	if(daType.empty())
		daType = "string";

    const TypeConverter& tc = getTypeConverter();
    TypeConverter::const_iterator it = tc.find(daType);

    if (it == tc.end()) {
        return ""; // error condition
    }
    else {
        return it->second;
    }
}

void NCMLParser::checkDataIsValidForCanonicalTypeOrThrow(const string& type, const vector<string>& tokens) const
{
    /*  Byte
     Int16
     UInt16
     Int32
     UInt32
     Float32
     Float64
     String
     URL
     OtherXML
     */
    bool valid = true;
    vector<string>::const_iterator it;
    vector<string>::const_iterator endIt = tokens.end();
    for (it = tokens.begin(); it != endIt; ++it) {
        if (type == "Byte") {
            valid &= check_byte(it->c_str());
        }
        else if (type == "Int16") {
            valid &= check_int16(it->c_str());
        }
        else if (type == "UInt16") {
            valid &= check_uint16(it->c_str());
        }
        else if (type == "Int32") {
            valid &= check_int32(it->c_str());
        }
        else if (type == "UInt32") {
            valid &= check_uint32(it->c_str());
        }
        else if (type == "Float32") {
            valid &= check_float32(it->c_str());
        }
        else if (type == "Float64") {
            valid &= check_float64(it->c_str());
        }
        // Doh!  The DAP2 specifies case as "URL" but internally libdap uses "Url"  Allow both...
        else if (type == "URL" || type == "Url" || type == "String") {
            // TODO the DAP call check_url is currently a noop.  do we want to check for well-formed URL?
            // This isn't an NcML type now, so straight up NcML users might enter URL as String anyway.
            valid &= (it->size() <= MAX_DAP_STRING_SIZE);
            if (!valid) {
                std::stringstream msg;
                msg << "Invalid Value: The " << type << " attribute value (not shown) exceeded max string length of "
                    << MAX_DAP_STRING_SIZE << " at scope=" << _scope.getScopeString() << endl;
                THROW_NCML_PARSE_ERROR(getParseLineNumber(), msg.str());
            }

            valid &= NCMLUtil::isAscii(*it);
            if (!valid) {
                THROW_NCML_PARSE_ERROR(getParseLineNumber(),
                    "Invalid Value: The " + type + " attribute value (not shown) has an invalid non-ascii character.");
            }
        }

        // For OtherXML, there's nothing to check so just say it's OK.
        // The SAX parser checks it for wellformedness already,
        // but ultimately it's just an arbitrary string...
        else if (type == "OtherXML") {
            valid &= true;
        }

        else {
            // We probably shouldn't get here, but...
            THROW_NCML_INTERNAL_ERROR("checkDataIsValidForCanonicalType() got unknown data type=" + type);
        }

        // Early throw so we know which token it was.
        if (!valid) {
            THROW_NCML_PARSE_ERROR(getParseLineNumber(),
                "Invalid Value given for type=" + type + " with value=" + (*it)
                    + " was invalidly formed or out of range" + _scope.getScopeString());
        }
    }
    // All is good if we get here.
}

void NCMLParser::clearAllAttrTables(DDS* dds)
{
    if (!dds) {
        return;
    }

    // Blow away the global attribute table.
    dds->get_attr_table().erase();

    // Hit all variables, recursing on containers.
    for (DDS::Vars_iter it = dds->var_begin(); it != dds->var_end(); ++it) {
        // this will clear not only *it's table, but it's children if it's composite.
        clearVariableMetadataRecursively(*it);
    }
}

void NCMLParser::clearVariableMetadataRecursively(BaseType* var)
{
    VALID_PTR(var);
    // clear the table
    var->get_attr_table().erase();

    if (var->is_constructor_type()) {
        Constructor *compositeVar = dynamic_cast<Constructor*>(var);
        if (!compositeVar) {
            THROW_NCML_INTERNAL_ERROR(
                "clearVariableMetadataRecursively: Unexpected cast error on dynamic_cast<Constructor*>");
        }
        for (Constructor::Vars_iter it = compositeVar->var_begin(); it != compositeVar->var_end(); ++it) {
            clearVariableMetadataRecursively(*it);
        }
    }
}

void NCMLParser::enterScope(const string& name, ScopeStack::ScopeType type)
{
    _scope.push(name, type);
    BESDEBUG(MODULE, prolog << "Entering scope: " << _scope.top().getTypedName() << endl);
    BESDEBUG(MODULE, prolog << "New scope=\"" << _scope.getScopeString() << "\"" << endl);
}

void NCMLParser::exitScope()
{
    NCML_ASSERT_MSG(!_scope.empty(), "Logic Error: Scope Stack Underflow!");
    BESDEBUG(MODULE, prolog << "Exiting scope " << _scope.top().getTypedName() << endl);
    _scope.pop();
    BESDEBUG(MODULE, prolog << "New scope=\"" << _scope.getScopeString() << "\"" << endl);
}

void NCMLParser::printScope() const
{
    BESDEBUG(MODULE, prolog << "Scope=\"" << _scope.getScopeString() << "\"" << endl);
}

string NCMLParser::getScopeString() const
{
    return _scope.getScopeString();
}

string NCMLParser::getTypedScopeString() const
{
    return _scope.getTypedScopeString();
}

int NCMLParser::getScopeDepth() const
{
    return _scope.size();
}
void NCMLParser::pushElement(NCMLElement* elt)
{
    VALID_PTR(elt);
    _elementStack.push_back(elt);
    elt->ref(); // up the count!
}

void NCMLParser::popElement()
{
    NCMLElement* elt = _elementStack.back();
    _elementStack.pop_back();

    // Keep the toString around if we plan to nuke him
    string infoOnDeletedDude = ((elt->getRefCount() == 1) ? (elt->toString()) : (string("")));

    // Drop the ref count.  If that forced a delete, print out the saved string.
    if (elt->unref() == 0) {
        BESDEBUG("ncml:memory",
            "NCMLParser::popElement: ref count hit 0 so we deleted element=" << infoOnDeletedDude << endl);
    }
}

NCMLElement*
NCMLParser::getCurrentElement() const
{
    if (_elementStack.empty()) {
        return 0;
    }
    else {
        return _elementStack.back();
    }
}

void NCMLParser::clearElementStack()
{
    while (!_elementStack.empty()) {
        NCMLElement* elt = _elementStack.back();
        _elementStack.pop_back();
        // unref() them... The Factory will take care of dangling memory...
        elt->unref();
    }
    _elementStack.resize(0);
}

void NCMLParser::processStartNCMLElement(const std::string& name, const XMLAttributeMap& attrs)
{
    // Store it in a shared ptr in case this function exceptions before we store it in the element stack.
    RCPtr<NCMLElement> elt = _elementFactory.makeElement(name, attrs, *this);

    // If we actually created an element of the given type name
    if (elt.get()) {
        elt->handleBegin();
        // tell the container to push the raw element, which will also ref() it on success
        // otherwise ~RCPtr will unref() to 0 and thus nuke it on exception.
        pushElement(elt.get());
    }
    else // Unknown element...
    {
        if (sThrowExceptionOnUnknownElements) {
            THROW_NCML_PARSE_ERROR(getParseLineNumber(),
                "Unknown element type=" + name + " found in NcML parse with scope=" + _scope.getScopeString());
        }
        else {
            BESDEBUG(MODULE, prolog << "Start of <" << name << "> element.  Element unsupported, ignoring." << endl);
        }
    }
}

void NCMLParser::processEndNCMLElement(const std::string& name)
{
    NCMLElement* elt = getCurrentElement();
    VALID_PTR(elt);

    // If it matches the one on the top of the stack, then process and pop.
    if (elt->getTypeName() == name) {
        elt->handleEnd();
        popElement(); // handles delete
    }
    else // the names don't match, so just ignore it.
    {
        BESDEBUG(MODULE, prolog << "End of <" << name << "> element unsupported currently, ignoring." << endl);
    }
}

const DimensionElement*
NCMLParser::getDimensionAtLexicalScope(const string& dimName) const
{
    const DimensionElement* ret = 0;
    if (getCurrentDataset()) {
        ret = getCurrentDataset()->getDimensionInFullScope(dimName);
    }
    return ret;
}

string NCMLParser::printAllDimensionsAtLexicalScope() const
{
    string ret("");
    NetcdfElement* dataset = getCurrentDataset();
    while (dataset) {
        ret += dataset->printDimensions();
        dataset = dataset->getParentDataset();
    }
    return ret;
}

void NCMLParser::enterOtherXMLParsingState(OtherXMLParser* pOtherXMLParser)
{
    BESDEBUG(MODULE, prolog << "Entering state for parsing OtherXML!" << endl);
    _pOtherXMLParser = pOtherXMLParser;
}

bool NCMLParser::isParsingOtherXML() const
{
    return _pOtherXMLParser;
}

void NCMLParser::cleanup()
{
    // The only memory we own is the _response, which is in an unique_ptr so will
    // either be returned to caller in parse() and cleared, or else
    // delete'd by our dtor via unique_ptr

    // All other objects point into _response temporarily, so nothing to destroy there.

    // Just for completeness.
    resetParseState();
}

} // namespace ncml_module

