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
#include "VariableAggElement.h"
#include "AggregationElement.h"
#include "NCMLDebug.h"
#include "NCMLParser.h"
#include "NCMLUtil.h"

namespace ncml_module {
const string VariableAggElement::_sTypeName = "variableAgg";
const vector<string> VariableAggElement::_sValidAttributes = getValidAttributes();

VariableAggElement::VariableAggElement() :
    RCObjectInterface(), NCMLElement(0), _name("")
{
}

VariableAggElement::VariableAggElement(const VariableAggElement& proto) :
    RCObjectInterface(), NCMLElement(proto), _name(proto._name)
{
}

VariableAggElement::~VariableAggElement()
{
    _name.clear();
}

const string&
VariableAggElement::getTypeName() const
{
    return _sTypeName;
}

VariableAggElement*
VariableAggElement::clone() const
{
    return new VariableAggElement(*this);
}

void VariableAggElement::setAttributes(const XMLAttributeMap& attrs)
{
    validateAttributes(attrs, _sValidAttributes);
    _name = attrs.getValueForLocalNameOrDefault("name", "");
}

void VariableAggElement::handleBegin()
{
    VALID_PTR(_parser);

    // Make sure the name is not empty or this is uselss.
    if (_name.empty()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Cannot have variableAgg@name empty! Scope=" + _parser->getScopeString());
    }

    // Also make sure we are the direct child of an aggregation or it's an error as well!
    if (!_parser->isScopeAggregation()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Got a variableAgg element not as a direct child of an aggregation!  elt=" + toString() + " at scope="
                + _parser->getScopeString());
    }

    AggregationElement& parentAgg = getParentAggregation();
    parentAgg.addAggregationVariable(_name);
    parentAgg.setVariableAggElement(); // let the agg know we're adding to it.
}

void VariableAggElement::handleEnd()
{
}

string VariableAggElement::toString() const
{
    return (string("<") + _sTypeName + printAttributeIfNotEmpty("name", _name) + "/>");
}

AggregationElement&
VariableAggElement::getParentAggregation() const
{
    AggregationElement* pAgg = dynamic_cast<AggregationElement*>(_parser->getCurrentElement());
    NCML_ASSERT_MSG(pAgg, "VariableAggElement::getParentAggregation(): "
        "Expected current top of stack was AggregationElement*, but it wasn't!  Logic error!");
    return *pAgg;
}

/////////////////// PRIVATE BELOW

vector<string> VariableAggElement::getValidAttributes()
{
    vector<string> validAttrs;
    validAttrs.reserve(1);
    validAttrs.push_back("name");
    return validAttrs;
}
}
