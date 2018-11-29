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
#include "RemoveElement.h"
#include "NCMLDebug.h"
#include "NCMLParser.h"
#include "NCMLUtil.h"

#include "NetcdfElement.h"

using namespace libdap;

namespace ncml_module {
// The element name
const string RemoveElement::_sTypeName = "remove";
const vector<string> RemoveElement::_sValidAttributes = getValidAttributes();

RemoveElement::RemoveElement() :
    RCObjectInterface(), NCMLElement(0), _name(""), _type("")
{
}

RemoveElement::RemoveElement(const RemoveElement& proto) :
    RCObjectInterface(), NCMLElement(proto)
{
    _name = proto._name;
    _type = proto._type;
}

RemoveElement::~RemoveElement()
{
}

const string&
RemoveElement::getTypeName() const
{
    return _sTypeName;
}

RemoveElement*
RemoveElement::clone() const
{
    // We rely on the copy ctor here, so make sure it's valid
    RemoveElement* newElt = new RemoveElement(*this);
    return newElt;
}

void RemoveElement::setAttributes(const XMLAttributeMap& attrs)
{
    validateAttributes(attrs, _sValidAttributes);

    _name = attrs.getValueForLocalNameOrDefault("name");
    _type = attrs.getValueForLocalNameOrDefault("type");

    // We do other validation on the actual values later, so no need here.
}

void RemoveElement::handleBegin()
{
    VALID_PTR(_parser);
    processRemove(*_parser);
}

void RemoveElement::handleContent(const string& content)
{
    if (!NCMLUtil::isAllWhitespace(content)) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Got non-whitespace for element content and didn't expect it. "
                "Element=" + toString() + " content=\"" + content + "\"");
    }
}

void RemoveElement::handleEnd()
{
}

string RemoveElement::toString() const
{
    return "<" + _sTypeName + " " + "name=\"" + _name + "\" type=\"" + _type + "\" >";
}

/////////////////////////////////////////////////////////////////////////////////////////////
/// Non Public Implementation

void RemoveElement::processRemove(NCMLParser& p)
{
    if (!(_type.empty() || _type == "attribute" || _type == "variable" || _type == "dimension")) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Illegal type in remove element: type=" + _type
                + "  This version of the parser can only remove type=\"attribute\" or type=\"variable\".");
    }

    if (_type == "attribute") {
        processRemoveAttribute(p);
    }
    else if (_type == "variable") {
        processRemoveVariable(p);
    }
    else if (_type == "dimension") {
        processRemoveDimension(p);
    }
    else {
        THROW_NCML_INTERNAL_ERROR(
            toString() + " had type that wasn't attribute or variable.  We shouldn't be calling this if so.");
    }
}

void RemoveElement::processRemoveAttribute(NCMLParser& p)
{
    AttrTable::Attr_iter it;
    bool gotIt = p.findAttribute(_name, it);
    if (!gotIt) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "In remove element, could not find attribute to remove name=" + _name + " at the current scope="
                + p.getScopeString());
    }

    // Nuke it.  This call works with containers too, and will recursively delete the children.
    BESDEBUG("ncml", "Removing attribute name=" << _name << " at scope=" << p.getScopeString() << endl);
    AttrTable* pTab = p.getCurrentAttrTable();
    VALID_PTR(pTab);
    pTab->del_attr(_name);
}

void RemoveElement::processRemoveVariable(NCMLParser& p)
{
    BESDEBUG("ncml", "Removing variable name=" + _name + " at scope=" + p.getScopeString());

    // Remove the variable from the current container scope, either the dataset variable list or the current variable container.
    p.deleteVariableAtCurrentScope(_name);
}

void RemoveElement::processRemoveDimension(NCMLParser& p)
{
    BESDEBUG("ncml", "Removing dimension name=" + _name + " at scope=" + p.getScopeString() << endl);

    BaseType* pOrgVar = p.getCurrentVariable();
    // inside variable remove only dimension with name = _name
    if(pOrgVar){
        pOrgVar->set_send_p(true);
        pOrgVar->set_send_p(true);
        pOrgVar->read();
        Array*  varArray = dynamic_cast<Array *>(pOrgVar);
        RemoveElement::removeDimension(varArray, _name);
    }
    // outside variable remove all dimensions with name = _name
    // and remove variable with name = _name
    else
    {
        p.deleteVariableAtCurrentScope(_name);
        DDS* cDDS = p.getDDSForCurrentDataset();
        for (DDS::Vars_iter varit = cDDS->var_begin(); varit != cDDS->var_end(); varit++) {
            Array* varArray = 0;
            if ((*varit)->type() == dods_array_c){
                varArray = dynamic_cast<Array *>(*varit);
                RemoveElement::removeDimension(varArray, _name);
            }
        }
    }
}

void RemoveElement::removeDimension(Array* arr, string name)
{
    Array::Dim_iter ait;
    // Loop over dimensions
    for (ait = arr->dim_begin(); ait != arr->dim_end(); ++ait) {
        if((*ait).name == name){
            arr->rename_dim(name, "");
        }
    }
}

vector<string> RemoveElement::getValidAttributes()
{
    vector<string> validAttrs;
    validAttrs.reserve(2);
    validAttrs.push_back("name");
    validAttrs.push_back("type");
    return validAttrs;
}

} // ncml_module
