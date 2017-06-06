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
#include "VariableElement.h"

#include <Array.h>
#include <BaseType.h>
#include <Structure.h>
#include <dods-limits.h>
#include <ctype.h>
#include "DimensionElement.h"
#include <memory>
#include "MyBaseTypeFactory.h"
#include "NCMLBaseArray.h"
#include "NCMLDebug.h"
#include "NCMLParser.h"
#include "NCMLUtil.h"
#include "NetcdfElement.h"
#include "RenamedArrayWrapper.h"
#include <sstream>

using namespace libdap;
using std::vector;
using std::auto_ptr;

namespace ncml_module {

const string VariableElement::_sTypeName = "variable";
const vector<string> VariableElement::_sValidAttributes = getValidAttributes();

VariableElement::VariableElement() :
    RCObjectInterface(), NCMLElement(0), _name(""), _type(""), _shape(""), _orgName(""), _shapeTokens(), _pNewlyCreatedVar(
        0), _gotValues(false)
{
}

VariableElement::VariableElement(const VariableElement& proto) :
    RCObjectInterface(), NCMLElement(proto)
{
    _name = proto._name;
    _type = proto._type;
    _shape = proto._shape;
    _orgName = proto._orgName;
    _shapeTokens = proto._shapeTokens;
    _pNewlyCreatedVar = 0; // not safe to copy the proto one, so pretend we didn't.
    _gotValues = false; // to be consistent with previosu line
}

VariableElement::~VariableElement()
{
    // help clear up memory
    _shapeTokens.resize(0);
    _shapeTokens.clear();
}

const string&
VariableElement::getTypeName() const
{
    return _sTypeName;
}

VariableElement*
VariableElement::clone() const
{
    return new VariableElement(*this);
}

void VariableElement::setAttributes(const XMLAttributeMap& attrs)
{
    validateAttributes(attrs, _sValidAttributes);

    _name = attrs.getValueForLocalNameOrDefault("name", "");
    _type = attrs.getValueForLocalNameOrDefault("type", "");
    _shape = attrs.getValueForLocalNameOrDefault("shape", "");
    _orgName = attrs.getValueForLocalNameOrDefault("orgName", "");
}

void VariableElement::handleBegin()
{
    VALID_PTR(_parser);
    processBegin(*_parser);
}

void VariableElement::handleContent(const string& content)
{
    // Variables cannot have content like attribute.  It must be within a <values> element.
    if (!NCMLUtil::isAllWhitespace(content)) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Got non-whitespace for element content and didn't expect it.  "
                "Element=" + toString() + " content=\"" + content + "\"");
    }
}

void VariableElement::handleEnd()
{
    processEnd(*_parser);
}

string VariableElement::toString() const
{
    return "<" + _sTypeName + " name=\"" + _name + "\"" + " type=\"" + _type + "\""
        + ((!_shape.empty()) ? (" shape=\"" + _shape + "\"") : (""))
        + ((!_orgName.empty()) ? (" orgName=\"" + _orgName + "\"") : ("")) + ">";
}

bool VariableElement::isNewVariable() const
{
    return _pNewlyCreatedVar;
}

bool VariableElement::checkGotValues() const
{
    return _gotValues;
}

void VariableElement::setGotValues()
{
    _gotValues = true;
}

////////////////// NON PUBLIC IMPLEMENTATION

void VariableElement::processBegin(NCMLParser& p)
{
    BESDEBUG("ncml", "VariableElement::handleBegin called for " << toString() << endl);

    if (!p.withinNetcdf()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Got element " + toString() + " while not in <netcdf> node!");
    }

    // Can only specify variable globally or within a composite variable now.
    if (!(p.isScopeGlobal() || p.isScopeCompositeVariable())) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Got <variable> element while not within a <netcdf> or within variable container.  scope="
                + p.getScopeString());
    }

    // If a request to rename the variable
    if (!_orgName.empty()) {
        processRenameVariable(p);
    }
    else // Otherwise see if it's an existing or new variable _name at scope of p
    {
        // Lookup the variable in whatever the parser's current variable container is
        // this could be the DDS top level or a container (constructor) variable.
        BaseType* pVar = p.getVariableInCurrentVariableContainer(_name);
        if (!pVar) {
            processNewVariable(p);
        }
        else {
            processExistingVariable(p, pVar);
        }
    }
}

void VariableElement::processEnd(NCMLParser& p)
{
    BESDEBUG("ncml", "VariableElement::handleEnd called at scope:" << p.getScopeString() << endl);
    if (!p.isScopeVariable()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "VariableElement::handleEnd called when not parsing a variable element!  "
                "Scope=" + p.getTypedScopeString());
    }

    // We need to defer the setting of values until the dataset
    // is closed since that's when a joinNew aggregation new map will have all its
    // values to set, not before.
    // But we need to allow the user to specify
    // the metadata for the variable PRIOR to this happening, without specifying
    // values.  We need to catch the case of values not being set later...
    // To handle this, we push new variables WITHOUT values onto a request queue
    // in the containing dataset which will validate that they have gotten their values
    // set at the point where the containing dataset is closed, but after the aggregations
    // have run.
    if (isNewVariable() && !checkGotValues()) {
        BESDEBUG("ncml",
            "WARNING: at parse line: " << _parser->getParseLineNumber() << " the newly created variable=" << toString() << " did not have its values set!  We will assume this is a placeholder variable" " for an aggregation (such as the new outer dimension of a joinNew) and will" " defer checking that required values are set until the point when this " " netcdf element is closed..." " Scope=" << p.getScopeString() << endl);
        BESDEBUG("ncml",
            "Adding new variable name=" << _pNewlyCreatedVar->name() << " to the validation watch list for the closing of this netcdf." << endl);
        _parser->getCurrentDataset()->addVariableToValidateOnClose(_pNewlyCreatedVar, this);
    }

    NCML_ASSERT_MSG(p.getCurrentVariable(),
        "Error: VariableElement::handleEnd(): Expected non-null parser.getCurrentVariable()!");

    // Pop up the stack from this variable
    exitScope(p);

    BaseType* pVar = p.getCurrentVariable();
    BESDEBUG("ncml", "Variable scope now with name: " << ((pVar)?(pVar->name()):("<NULL>")) << endl);
}

void VariableElement::processExistingVariable(NCMLParser& p, BaseType* pVar)
{
    BESDEBUG("ncml",
        "VariableElement::processExistingVariable() called with name=" << _name << " at scope=" << p.getTypedScopeString() << endl);

    // If undefined, look it up
    if (!pVar) {
        pVar = p.getVariableInCurrentVariableContainer(_name);
    }

    // It better exist by now
    VALID_PTR(pVar);

    // Make sure the type matches.  NOTE:
    // We use "Structure" to mean Grid, Structure, Sequence!
    // Also type="" will match ANY type.
    // TODO This fails on Array as well due to NcML making arrays be a basic type with a non-zero dimension.
    // We're gonna ignore that until we allow addition of variables, but let's leave this check here for now
    if (!_type.empty() && !p.typeCheckDAPVariable(*pVar, p.convertNcmlTypeToCanonicalType(_type))) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Type Mismatch in variable element with name=" + _name + " at scope=" + p.getScopeString()
                + " Expected type=" + _type + " but found variable with type=" + pVar->type_name()
                + "  In order to match a variable of any type, please do not specify variable@type attribute in your NcML file.");
    }

    // Use this variable as the new scope until we get a handleEnd()
    enterScope(p, pVar);
}

#if 0

/**
 * I added this method as a place holder for repairing (potentially) broken behavior regarding the
 * the renmaing of libdap::Structre and libdap::Grid objects which I beleive is not curretnly being handled
 * correctly. The implementation is incomplete, so I am disabling the code  using a #if statement.
 * ndp - 08/21/2015
 */
void VariableElement::processRenameVariableDataWorker(NCMLParser& p, BaseType* pOrgVar)
{

    if (pOrgVar->is_vector_type()) {
        // If the variable is an Array, we need to wrap it in a RenamedArrayWrapper
        // so that it finds its data correctly.
        // This will remove the old one and replace our wrapper under the new _name if it's an Array subclass!
        pOrgVar = replaceArrayIfNeeded(p, pOrgVar, _name);
    }
    else if (pOrgVar->is_constructor_type()) {
        // If the variable is a constructor then we are going to have to some special things so that any child variables
        // can be read after renaming

    }
    else if (pOrgVar->is_simple_type()) {
        // If it's a simple type then force it to read or we won't find the new name in the source
        // dataset when it comes time to serialize.
        pOrgVar->read();
    }

    // This is safe whether we converted it or not.  Rename!
    NCMLUtil::setVariableNameProperly(pOrgVar, _name);
}
#endif

void VariableElement::processRenameVariable(NCMLParser& p)
{
    BESDEBUG("ncml",
        "VariableElement::processRenameVariable() called on " + toString() << " at scope=" << p.getTypedScopeString() << endl);

    // First, make sure the data is valid
    NCML_ASSERT_MSG(!_name.empty(), "Can't have an empty variable@name if variable@orgName is specified!");
    NCML_ASSERT(!_orgName.empty()); // we shouldn't even call this if this is the case, but...

    // Lookup _orgName, which must exist or throw
    BESDEBUG("ncml", "Looking up the existence of a variable with name=" << _orgName << "..." <<endl);
    BaseType* pOrgVar = p.getVariableInCurrentVariableContainer(_orgName);
    if (!pOrgVar) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Renaming variable failed for element=" + toString() + " since no variable with orgName=" + _orgName
                + " exists at current parser scope=" + p.getScopeString());
    }
    BESDEBUG("ncml", "Found variable with name=" << _orgName << endl);

    // Lookup _name, which must NOT exist or throw
    BESDEBUG("ncml", "Making sure new name=" << _name << " does not exist at this scope already..." << endl);
    BaseType* pExisting = p.getVariableInCurrentVariableContainer(_name);
    if (pExisting) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Renaming variable failed for element=" + toString() + " since a variable with name=" + _name
                + " already exists at current parser scope=" + p.getScopeString());
    }
    BESDEBUG("ncml", "Success, new variable name is open at this scope." << endl);

    // Call set_name on the orgName variable.
    BESDEBUG("ncml", "Renaming variable " << _orgName << " to " << _name << endl);

    // If we are doing data, we need to handle some variables (Array)
    // specially since they might refer to underlying data by the new name
    if (p.parsingDataRequest()) {
        // If not an Array, force it to read or we won't find the new name in the file for HDF at least...
        if (!dynamic_cast<Array*>(pOrgVar)) {
            pOrgVar->read();
        }
        // If the variable is an Array, we need to wrap it in a RenamedArrayWrapper
        // so that it finds it data correctly.
        // This will remove the old one and replace our wrapper under the new _name if it's an Array subclass!
        pOrgVar = replaceArrayIfNeeded(p, pOrgVar, _name);

        // This is safe whether we converted it or not.  Rename!
        NCMLUtil::setVariableNameProperly(pOrgVar, _name);
    }
    else {
        // The above branch will reorder the output for the DataDDS case,
        // so we need to remove and readd even if we dont convert to preserve order!

        // BaseType::set_name fails for Vector (Array etc) subtypes since it doesn't
        // set the template's BaseType var's name as well.  This function does that until
        // a fix in libdap lets us call pOrgName->set_name(_name) directly.
        // pOrgVar->set_name(_name); // TODO  switch to this call when bug is fixed.
        //NCMLUtil::setVariableNameProperly(pOrgVar, _name);

        // Need to copy unfortunately, since delete will kill storage...
        auto_ptr<BaseType> pCopy = auto_ptr<BaseType>(pOrgVar->ptr_duplicate());
        NCMLUtil::setVariableNameProperly(pCopy.get(), _name);

        // Nuke the old
        p.deleteVariableAtCurrentScope(pOrgVar->name());

        // Add the new, which copies under the hood.  auto_ptr will clean pCopy.
        p.addCopyOfVariableAtCurrentScope(*pCopy);
    }

    // Make sure we find it under the new name
    BaseType* pRenamedVar = p.getVariableInCurrentVariableContainer(_name);
    NCML_ASSERT_MSG(pRenamedVar, "Renamed variable not found!  Logic error!");

    // Finally change the scope to the variable.
    enterScope(p, pRenamedVar);
    BESDEBUG("ncml", "Entering scope of the renamed variable.  Scope is now: " << p.getTypedScopeString() << endl);
}

void VariableElement::processNewVariable(NCMLParser& p)
{
    BESDEBUG("ncml", "Entered VariableElement::processNewVariable..." << endl);

    // ASSERT: We know the variable with name doesn't exist, or we wouldn't call this function.

    // Type cannot be empty for a new variable!!
    if (_type.empty()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Must have non-empty variable@type when creating new variable=" + toString());
    }

    // Convert the type to the canonical type...
    string type = p.convertNcmlTypeToCanonicalType(_type);
    if (_type.empty()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(), "Unknown type for new variable=" + toString());
    }

    // Tokenize the _shape string
    NCMLUtil::tokenize(_shape, _shapeTokens, NCMLUtil::WHITESPACE);

    // Add new variables of the given type...
    // On exit, each of these leave _parser->getCurrentVariable() with
    // the new variable that exists within the current dataset's DDS
    // at the current containing scope.
    if (_type == p.STRUCTURE_TYPE) {
        processNewStructure(p);
    }
    else if (_shape.empty()) // a scalar
    {
        processNewScalar(p, type);
    }
    else if (!_shape.empty()) {
        processNewArray(p, type);
    }
    else {
        THROW_NCML_INTERNAL_ERROR("UNIMPLEMENTED METHOD: Cannot create non-scalar Array types yet.");
    }

    // Keep track that it's new so we can error if we get values for non-new.
    // All the process new will have entered it into the dataset as scope, so
    // getCurrentVariable() on the parser is the actual one in the dataset always.
    _pNewlyCreatedVar = _parser->getCurrentVariable();
}

void VariableElement::processNewStructure(NCMLParser& p)
{
    // First, make sure we are at a parse scope that ALLOWS variables to be added!
    if (!(p.isScopeCompositeVariable() || p.isScopeGlobal())) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(), "Cannot add a new Structure variable at current scope!  "
            "TypedScope=" + p.getTypedScopeString());
    }

    auto_ptr<BaseType> pNewVar = MyBaseTypeFactory::makeVariable("Structure", _name);
    NCML_ASSERT_MSG(pNewVar.get(),
        "VariableElement::processNewStructure: factory failed to make a new Structure variable for name=" + _name);

    // Add the copy, let auto_ptr clean up
    p.addCopyOfVariableAtCurrentScope(*pNewVar);

    // Lookup the variable we just added since it is added as a copy!
    BaseType* pActualVar = p.getVariableInCurrentVariableContainer(_name);
    VALID_PTR(pActualVar);
    // Make sure the copy mechanism did the right thing so we don't delete the var we just added.
    NCML_ASSERT(pActualVar != pNewVar.get());

    // Make it be the scope for any incoming new attributes.
    enterScope(p, pActualVar);

    // Structures technically don't have values, but we check later that we set them, so say we're good.
    setGotValues();
}

void VariableElement::processNewScalar(NCMLParser&p, const string& dapType)
{
    addNewVariableAndEnterScope(p, dapType);
}

void VariableElement::processNewArray(NCMLParser& p, const std::string& dapType)
{
    // For now, we can reuse the processNewScalar to make the variable and enter scope and all that.
    // Use the new template form for the Array so we get the NCMLArray<T> subclass that handles constraints.
    addNewVariableAndEnterScope(p, "Array<" + dapType + ">");

    // Now look up the added variable so we can set it's template and dimensionality.
    // it should be the current variable since we entered its scope above!
    Array* pNewVar = dynamic_cast<Array*>(p.getCurrentVariable());
    NCML_ASSERT_MSG(pNewVar,
        "VariableElement::processNewArray: Expected non-null getCurrentVariable() in parser but got NULL!");

    // Now make the template variable of the array entry type with the same name and add it
    auto_ptr<BaseType> pTemplateVar = MyBaseTypeFactory::makeVariable(dapType, _name);
    pNewVar->add_var(pTemplateVar.get());

    // For each dimension in the shape, append it to make an N-D array...
    for (unsigned int i = 0; i < _shapeTokens.size(); ++i) {
        unsigned int dim = getSizeForDimension(p, _shapeTokens.at(i));
        string dimName = ((isDimensionNumericConstant(_shapeTokens.at(i))) ? ("") : (_shapeTokens.at(i)));
        BESDEBUG("ncml",
            "Appending dimension name=\"" << dimName << "\" of size=" << dim << " to the Array name=" << pNewVar->name() << endl);
        pNewVar->append_dim(dim, dimName);
    }

    // Make sure the size of the flattened Array in memory (product of dimensions) is within the DAP2 limit...
    if (getProductOfDimensionSizes(p) > static_cast<unsigned int>(DODS_MAX_ARRAY)) // actually the call itself will throw...
        {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Product of dimension sizes for Array must be < (2^31-1).");
    }
}

#if 0 // old school copy method
libdap::BaseType*
VariableElement::replaceArrayIfNeeded(NCMLParser& p, libdap::BaseType* pOrgVar, const string& name)
{
    VALID_PTR(pOrgVar);
    Array* pOrgArray = dynamic_cast<libdap::Array*>(pOrgVar);
    if (!pOrgArray)
    {
        return pOrgVar;
    }

    BESDEBUG("ncml", "VariableElement::replaceArray if needed.  Renaming an Array means we need to convert it to NCMLArray." << endl);

    // Make a copy of it
    auto_ptr<NCMLBaseArray> pNewArray = NCMLBaseArray::createFromArray(*pOrgArray);
    VALID_PTR(pNewArray.get());

    // Remove the old one.
    p.deleteVariableAtCurrentScope(pOrgArray->name());

    // Make sure the new name is set.
    NCMLUtil::setVariableNameProperly(pNewArray.get(), name);

    // Add the new one.  Unfortunately this copies it under the libdap hood. ARGH!
    // So just use the get() and let the auto_ptr kill our copy.
    p.addCopyOfVariableAtCurrentScope(*(pNewArray.get()));

    return p.getVariableInCurrentVariableContainer(name);
}
#endif

libdap::BaseType*
VariableElement::replaceArrayIfNeeded(NCMLParser& p, libdap::BaseType* pOrgVar, const string& name)
{
    VALID_PTR(pOrgVar);
    Array* pOrgArray = dynamic_cast<libdap::Array*>(pOrgVar);
    if (!pOrgArray) {
        return pOrgVar;
    }

    BESDEBUG("ncml",
        "VariableElement::replaceArray if needed.  Renaming an Array means we need to wrap it with RenamedArrayWrapper!" << endl);

    // Must make a clone() since deleteVariableAtCurrentScope from container below will destroy pOrgArray!
    auto_ptr<RenamedArrayWrapper> pNewArray = auto_ptr<RenamedArrayWrapper>(
        new RenamedArrayWrapper(static_cast<Array*>(pOrgArray->ptr_duplicate())));
    p.deleteVariableAtCurrentScope(pOrgArray->name());

    // Make sure the new name is set.
    NCMLUtil::setVariableNameProperly(pNewArray.get(), name);

    // Add the new one.  Unfortunately this copies it under    the libdap hood. ARGH!
    // So just use the get() and let the auto_ptr kill our copy.
    p.addCopyOfVariableAtCurrentScope(*(pNewArray.get()));

    return p.getVariableInCurrentVariableContainer(name);
}

void VariableElement::addNewVariableAndEnterScope(NCMLParser& p, const std::string& dapType)
{
    // First, make sure we are at a parse scope that ALLOWS variables to be added!
    if (!(p.isScopeCompositeVariable() || p.isScopeGlobal())) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Cannot add a new scalar variable at current scope!  TypedScope=" + p.getTypedScopeString());
    }

    // Destroy it no matter what sicne add_var copies it
    auto_ptr<BaseType> pNewVar = MyBaseTypeFactory::makeVariable(dapType, _name);
    NCML_ASSERT_MSG(pNewVar.get(),
        "VariableElement::addNewVariable: factory failed to make a new variable of type: " + dapType + " for element: "
            + toString());

    // Now that we have it, we need to add it to the parser at current scope
    // Internally, the add will copy the arg, not store it.
    p.addCopyOfVariableAtCurrentScope(*pNewVar);

    // Lookup the variable we just added since it is added as a copy!
    BaseType* pActualVar = p.getVariableInCurrentVariableContainer(_name);
    VALID_PTR(pActualVar);
    // Make sure the copy mechanism did the right thing so we don't delete the var we just added.
    NCML_ASSERT(pActualVar != pNewVar.get());

    // Make it be the scope for any incoming new attributes.
    enterScope(p, pActualVar);

}

void VariableElement::enterScope(NCMLParser& p, BaseType* pVar)
{
    VALID_PTR(pVar);

    // Add the proper variable scope to the stack
    if (pVar->is_constructor_type()) {
        p.enterScope(_name, ScopeStack::VARIABLE_CONSTRUCTOR);
    }
    else {
        p.enterScope(_name, ScopeStack::VARIABLE_ATOMIC);
    }

    // this sets the _pCurrentTable to the variable's table.
    p.setCurrentVariable(pVar);
}

void VariableElement::exitScope(NCMLParser& p)
{
    // Set the new variable container to the parent of the current.
    // This could be NULL if we're a top level variable, making the DDS the variable container.
    p.setCurrentVariable(p.getCurrentVariable()->get_parent());
    p.exitScope();
    p.printScope();
}

bool VariableElement::isDimensionNumericConstant(const std::string& dimToken) const
{
    // for now just test the first character is a number and assume it's a number
    return isdigit(dimToken.at(0));
}

unsigned int VariableElement::getSizeForDimension(NCMLParser& p, const std::string& dimToken) const
{
    unsigned int dim = 0;
    // First, if the first char is a number, then assume it's an explicit non-negative integer
    if (isDimensionNumericConstant(dimToken)) {
        stringstream token;
        token.str(dimToken);
        token >> dim;
        if (token.fail()) {
            THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
                "Trying to get the dimension size in shape=" + _shape + " for token " + dimToken
                    + " failed to parse the unsigned int!");
        }
    }
    else {
        const DimensionElement* pDim = p.getDimensionAtLexicalScope(dimToken);
        if (pDim) {
            return pDim->getLengthNumeric();
        }
        else {
            THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
                "Failed to find a dimension with name=" + dimToken + " for variable=" + toString()
                    + " with dimension table= " + p.printAllDimensionsAtLexicalScope() + " at scope="
                    + p.getScopeString());
        }
    }
    return dim;
}

unsigned int VariableElement::getProductOfDimensionSizes(NCMLParser& p) const
{
    // If no shape, then it's size 0 (scalar)
    if (_shape.empty()) {
        return 0;
    }

    // Otherwise compute it
    unsigned int product = 1;
    vector<string>::const_iterator endIt = _shapeTokens.end();
    vector<string>::const_iterator it;
    for (it = _shapeTokens.begin(); it != endIt; ++it) {
        const string& dimName = *it;
        unsigned int dimSize = getSizeForDimension(p, dimName); // might throw if not found...
        // if multiplying this in will cause over DODS_MAX_ARRAY, then error
        // Added test for product == 0. Coverity. jhrg 2/7/17
        if (product == 0 || dimSize > (DODS_MAX_ARRAY / product)) {
            THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
                "Product of dimension sizes exceeds the maximum DAP2 size of 2147483647 (2^31-1)!");
        }
        // otherwise, multiply it in
        product *= dimSize;
    }
    return product;
}

vector<string> VariableElement::getValidAttributes()
{
    vector<string> validAttrs;
    validAttrs.reserve(4);
    validAttrs.push_back("name");
    validAttrs.push_back("type");
    validAttrs.push_back("shape");
    validAttrs.push_back("orgName");
    return validAttrs;
}
}
