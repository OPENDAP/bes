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
#include "DimensionElement.h"
#include "NCMLDebug.h"
#include "NCMLParser.h"
#include "NCMLUtil.h"
#include "NetcdfElement.h"
#include <sstream>
#include <Array.h>

using std::string;
using std::stringstream;

namespace ncml_module {
// the parse name of the element
const string DimensionElement::_sTypeName = "dimension";
const vector<string> DimensionElement::_sValidAttributes = getValidAttributes();

DimensionElement::DimensionElement() :
    NCMLElement(0), _length("0"), _orgName(""), _isUnlimited(""), _isShared(""), _isVariableLength(""), _dim()
{
}

DimensionElement::DimensionElement(const DimensionElement& proto) :
    RCObjectInterface(), NCMLElement(proto), _length(proto._length), _orgName(proto._orgName), _isUnlimited(
        proto._isUnlimited), _isShared(proto._isShared), _isVariableLength(proto._isVariableLength), _dim(proto._dim)
{
}

DimensionElement::DimensionElement(const agg_util::Dimension& dim) :
    NCMLElement(0), _length("0"), _orgName(""), _isUnlimited(""), _isShared(""), _isVariableLength(""), _dim(dim)
{
    // Set string to match the int size
    ostringstream oss;
    oss << dim.size;
    _length = oss.str();
}

DimensionElement::~DimensionElement()
{
}

const string& DimensionElement::getTypeName() const
{
    return _sTypeName;
}

DimensionElement*
DimensionElement::clone() const
{
    return new DimensionElement(*this);
}

void DimensionElement::setAttributes(const XMLAttributeMap& attrs)
{
    _dim.name = attrs.getValueForLocalNameOrDefault("name");
    _length = attrs.getValueForLocalNameOrDefault("length");
    _orgName = attrs.getValueForLocalNameOrDefault("orgName");
    _isUnlimited = attrs.getValueForLocalNameOrDefault("isUnlimited");
    _isShared = attrs.getValueForLocalNameOrDefault("isShared");
    _isVariableLength = attrs.getValueForLocalNameOrDefault("isVariableLength");

    // First check that we didn't get any typos...
    validateAttributes(attrs, _sValidAttributes);

    // Parse the size etc
    parseValidateAndCacheDimension();

//    // Final validation for things we implemented
//    validateOrThrow();
}

void DimensionElement::handleBegin()
{
    BESDEBUG("ncml", "DimensionElement::handleBegin called..." << endl);

    // Make sure we're placed at a valid parse location.
    // Direct child of <netcdf> only now since we dont handle <group>
    if (!_parser->isScopeNetcdf()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Got dimension element = " + toString()
                + " at an invalid parse location.  Expected it as a direct child of <netcdf> element only." + " scope="
                + _parser->getScopeString());
    }

    // This will be the scope we're to be added...
    NetcdfElement* dataset = _parser->getCurrentDataset();
    VALID_PTR(dataset);

    // Make sure the name is unique at this parse level or exception.
    const DimensionElement* pExistingDim = dataset->getDimensionInLocalScope(name());
    if (pExistingDim) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Tried to add dimension at " + toString() + " but a dimension with name=" + name()
                + " already exists in this scope=" + _parser->getScopeString());
    }

    // The dataset will maintain a strong reference to us while we're needed.
    dataset->addDimension(this);
    if(!_orgName.empty()){
        processRenameDimension(*_parser);
    }
}

void DimensionElement::processRenameDimension(NCMLParser& p)
{
    BESDEBUG("ncml",
        "DimensionElement::processRenameDimension() called on " + toString() << " at scope=" << p.getTypedScopeString() << endl);

    BESDEBUG("ncml", "Renaming dimension " << _orgName << " to " << name() << endl);
   // Loop over variables
    DDS* cDDS = p.getDDSForCurrentDataset();
    DDS::Vars_iter varit;
    for (varit = cDDS->var_begin(); varit != cDDS->var_end(); varit++) {
        Array* varArray = 0;
        if ((*varit)->type() == dods_array_c)
                    varArray = dynamic_cast<Array *>(*varit);
        Array::Dim_iter ait;
        // Loop over dimensions
        for (ait = varArray->dim_begin(); ait != varArray->dim_end(); ++ait) {
            if((*ait).name == name()){
                THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
                            "Renaming dimension failed for element=" + toString() + " since a dimension with name=" + (*ait).name
                                + " already exists at current parser scope=" + p.getScopeString());
            }
            if((*ait).name == _orgName){
                varArray->rename_dim(_orgName, name());
            }
        }
    }
}

void DimensionElement::handleContent(const string& content)
{
    // BESDEBUG("ncml", "DimensionElement::handleContent called...");
    if (!NCMLUtil::isAllWhitespace(content)) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Got illegal (non-whitespace) content in element " + toString());
    }
}

void DimensionElement::handleEnd()
{
    // BESDEBUG("ncml", "DimensionElement::handleEnd called...");
}

string DimensionElement::toString() const
{
    string ret = "<" + _sTypeName + " ";
    ret += NCMLElement::printAttributeIfNotEmpty("name", name());
    ret += NCMLElement::printAttributeIfNotEmpty("length", _length);
    ret += NCMLElement::printAttributeIfNotEmpty("isShared", _isShared);
    ret += NCMLElement::printAttributeIfNotEmpty("isVariableLength", _isVariableLength);
    ret += NCMLElement::printAttributeIfNotEmpty("isUnlimited", _isUnlimited);
    ret += NCMLElement::printAttributeIfNotEmpty("orgName", _orgName);
    ret += " >";
    return ret;
}

bool DimensionElement::checkDimensionsMatch(const DimensionElement& rhs) const
{
    return ((this->name() == rhs.name()) && (this->getSize() == rhs.getSize()));
}

const string&
DimensionElement::name() const
{
    return _dim.name;
}

unsigned int DimensionElement::getLengthNumeric() const
{
    return _dim.size;
}

unsigned int DimensionElement::getSize() const
{
    return getLengthNumeric();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// PRIVATE IMPL
#if 0
void DimensionElement::parseAndCacheDimension()
{
    stringstream sis;
    if(!_length.empty()){
        sis.str(_length);
        sis >> _dim.size;
        if (sis.fail()) {
            THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
                "Element " + toString() + " failed to parse the length attribute into a proper unsigned int!");
        }
    }

    // @TODO set the _dim.isSizeConstant from the isVariableLength, etc once we know how to use them for aggs
    _dim.isSizeConstant = true;

    if (_isShared == "true") {
        _dim.isShared = true;
    }
    else if (_isShared == "false") {
        _dim.isShared = false;
    }
    else if (!_isShared.empty()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(), "dimension@isShared did not have value in {true,false}.");
    }

}

void DimensionElement::validateOrThrow()
{
    // Perhaps we want to warn in BESDEBUG rather than error, but I'd rather be explicit for now.
    if (!_isShared.empty() || !_isUnlimited.empty() || !_isVariableLength.empty()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Dimension element " + toString() + " has unexpected unimplemented attributes. "
                "This version of the module only handles name, orgName and length.");
    }else if(!_length.empty() && !_orgName.empty()){
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
                    "Dimension element " + toString() + " has unexpected attributes orgName or length.");
    }
}
#endif

void DimensionElement::parseValidateAndCacheDimension()
{
    // There is no name
    if (_dim.name.empty()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
             "Dimension element " + toString() + "Can't have an empty name.");
    }
    // Perhaps we want to warn in BESDEBUG rather than error, but I'd rather be explicit for now.
    else if (!_isShared.empty() || !_isUnlimited.empty() || !_isVariableLength.empty()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Dimension element " + toString() + " has unexpected unimplemented attributes. "
                "This version of the module only handles name, orgName and length.");
    }
    // There is only name
    else if (_length.empty() && _orgName.empty()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
             "Dimension element " + toString() + " has no expected length or orgName value for renaming.");
    }
    // There are length and orgName together
    else if(!_length.empty() && !_orgName.empty()){
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
              "Dimension element " + toString() + " has attributes orgName and length together.");
    }

    stringstream sis;
    if(!_length.empty()){
        sis.str(_length);
        sis >> _dim.size;
        if (sis.fail()) {
            THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
                "Element " + toString() + " failed to parse the length attribute into a proper unsigned int!");
        }
    }

    // @TODO set the _dim.isSizeConstant from the isVariableLength, etc once we know how to use them for aggs
    _dim.isSizeConstant = true;

    if (_isShared == "true") {
        _dim.isShared = true;
    }
    else if (_isShared == "false") {
        _dim.isShared = false;
    }
    else if (!_isShared.empty()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(), "dimension@isShared did not have value in {true,false}.");
    }

}


vector<string> DimensionElement::getValidAttributes()
{
    vector<string> validAttrs;
    validAttrs.reserve(10);
    validAttrs.push_back("name");
    validAttrs.push_back("length");
    validAttrs.push_back("isUnlimited");
    validAttrs.push_back("isVariableLength");
    validAttrs.push_back("isShared");
    validAttrs.push_back("orgName");
    return validAttrs;
}
}
