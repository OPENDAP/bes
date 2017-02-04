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

#include "ValuesElement.h"

#include "BaseType.h"
#include "Array.h"
#include "Byte.h"
#include "Float32.h"
#include "Float64.h"
#include "Grid.h"
#include "Int16.h"
#include "Int32.h"
#include "Sequence.h"
#include "Str.h"
#include "Structure.h"
#include "UInt16.h"
#include "UInt32.h"
#include "Url.h"

#include "NCMLDebug.h"
#include "NCMLParser.h"
#include "NCMLUtil.h"
#include <sstream>
#include "VariableElement.h"

using namespace libdap;

namespace ncml_module {
const string ValuesElement::_sTypeName = "values";
const vector<string> ValuesElement::_sValidAttributes = getValidAttributes();

ValuesElement::ValuesElement() :
    RCObjectInterface(), NCMLElement(0), _start(""), _increment(""), _separator(""), _gotContent(false), _tokens()
{
    _tokens.reserve(256);
}

ValuesElement::ValuesElement(const ValuesElement& proto) :
    RCObjectInterface(), NCMLElement(proto)
{
    _start = proto._start;
    _increment = proto._increment;
    _separator = proto._separator;
    _gotContent = proto._gotContent;
    _tokens = proto._tokens;
}

ValuesElement::~ValuesElement()
{
    _tokens.resize(0);
}

const string&
ValuesElement::getTypeName() const
{
    return _sTypeName;
}

ValuesElement*
ValuesElement::clone() const
{
    return new ValuesElement(*this);
}

void ValuesElement::setAttributes(const XMLAttributeMap& attrs)
{
    validateAttributes(attrs, _sValidAttributes);

    _start = attrs.getValueForLocalNameOrDefault("start");
    _increment = attrs.getValueForLocalNameOrDefault("increment");
    _separator = attrs.getValueForLocalNameOrDefault("separator", ""); // empty means "not specified" and becoems whitesoace for all but string

    // Validate them... if _start is specified, then _increment must be as well!
    if (!_start.empty() && _increment.empty()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "values element=" + toString() + " had a start attribute without a corresponding increment attribute!");
    }
    if (_start.empty() && !_increment.empty()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "values element=" + toString() + " had an increment attribute without a corresponding start attribute!");
    }
}

void ValuesElement::handleBegin()
{
    VALID_PTR(_parser);
    NCMLParser& p = *_parser;

    BESDEBUG("ncml",
        "ValuesElement::handleBegin called with element=" << toString() << " at scope=" << p.getScopeString() << endl);

    // First, make sure we're in a current parse start for this elemnent
    if (!p.isScopeVariable()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Got values element while not parsing a variable!  values=" + toString() + " at scope="
                + p.getTypedScopeString());
    }

    // Make sure we're the first values element on the variable or there is a problem!
    const VariableElement* pVarElt = getContainingVariableElement(p);
    VALID_PTR(pVarElt);
    if (pVarElt->checkGotValues()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Got a values element when one was already specified for this variable=" + pVarElt->toString()
                + " at scope=" + p.getScopeString());
    }

    // If we got a start and increment, we are supposed to auto-generate values for the variable
    if (shouldAutoGenerateValues()) {
        BaseType* pVar = p.getCurrentVariable();
        NCML_ASSERT_MSG(pVar, "ValuesElement::handleBegin(): Expected non-null p.getCurrentVariable()!");
        autogenerateAndSetVariableValues(p, *pVar);
    }
    // else we'll expect content

    // We zero this out here in 'begin'; load it up with raw text in 'handlerContent'
    // and parse it in 'end'. jhrg 10/12/11
    _accumulated_content.resize(0);
}

void ValuesElement::handleContent(const string& content)
{
    NCMLParser& p = *_parser;

    BESDEBUG("ncml", "ValuesElement::handleContent called for " << toString() << " with content=" << content << endl);

    // N.B. Technically, we're still in isScopeVariable() since we don't push values elements on the scopestack,
    // only the XML parser stack...

    // Make sure we don't get non-whitespace content for autogenerated values!
    if (shouldAutoGenerateValues() && !NCMLUtil::isAllWhitespace(content)) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Element: " + toString()
                + " specified a start and increment to autogenerate values but also illegally specified content!");
    }

    // There had better be one or we goofed!
    BaseType* pVar = p.getCurrentVariable();
    NCML_ASSERT_MSG(pVar, "ValuesElement::handleContent: got unexpected null getCurrentVariable() from parser!!");

    // Also, make sure the variable we plan to add values to is a new variable and not an existing one.
    // We do not support changing existing dataset values currently.
    const VariableElement* pVarElt = getContainingVariableElement(p);
    VALID_PTR(pVarElt);
    if (!pVarElt->isNewVariable()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "This version of the NCML Module cannot change the values of an existing variable! "
                "However, we got " + toString() + " element for variable=" + pVarElt->toString() + " at scope="
                + p.getScopeString());
    }

    // Ripped out this block; moved to 'handleEnd'. Just accumulate raw text here. jhrg 10/12/11
    _accumulated_content.append(content);

#if 0
    // Tokenize the values for all cases EXCEPT if it's a scalar string.
    // We'll make a special exception an assume the entire content is the token
    // to avoid accidental tokenization with whitespace, which is clearly not intended
    // by the NcML file author!
    if (pVar->is_simple_type() &&
        (pVar->type() == dods_str_c || pVar->type() == dods_url_c))
    {
        _tokens.resize(0);
        _tokens.push_back(string(content));
    }
    // Don't tokenize a char array either, since we want to read all the char's in.
    else if (pVar->is_vector_type() && getNCMLTypeForVariable(p) == "char")
    {
        NCMLUtil::tokenizeChars(content, _tokens); // tokenize with no separator so each char is token.
    }
    else if (pVar->is_vector_type() && getNCMLTypeForVariable(p) == "string")
    {
        string sep = ((_separator.empty())?(NCMLUtil::WHITESPACE):(_separator));
        NCMLUtil::tokenize(content, _tokens, sep);
    }
    else // for arrays of other values, use whitespace separation for default if not specified.
    {
        string sep = ((_separator.empty())?(NCMLUtil::WHITESPACE):(_separator));
        NCMLUtil::tokenize(content, _tokens, sep);
    }
#endif
#if 0
    setVariableValuesFromTokens(p, *pVar);
    _gotContent = true;
    setGotValuesOnOurVariableElement(p);
#endif
}

void ValuesElement::handleEnd()
{
    BESDEBUG("ncml", "ValuesElement::handleEnd called for " << toString() << endl);

    NCMLParser& p = *_parser;
    // There had better be one or we goofed!
    BaseType* pVar = p.getCurrentVariable();
    NCML_ASSERT_MSG(pVar, "ValuesElement::handleContent: got unexpected null getCurrentVariable() from parser!!");

    // I set _gotContent here because other methods depend on it.
    _gotContent = !_accumulated_content.empty();
#if 0
    if (!shouldAutoGenerateValues() && !_gotContent)
    {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Values element=" + toString() + " expected content for values but didn't get any!");
    }
#endif
    // Tokenize the values for all cases EXCEPT if it's a scalar string.
    // We'll make a special exception an assume the entire content is the token
    // to avoid accidental tokenization with whitespace, which is clearly not intended
    // by the NcML file author!
    if (pVar->is_simple_type() && (pVar->type() == dods_str_c || pVar->type() == dods_url_c)) {
        _tokens.resize(0);
        _tokens.push_back(string(_accumulated_content));
    }
    // Don't tokenize a char array either, since we want to read all the char's in.
    else if (pVar->is_vector_type() && getNCMLTypeForVariable(p) == "char") {
        NCMLUtil::tokenizeChars(_accumulated_content, _tokens); // tokenize with no separator so each char is token.
    }
    else if (pVar->is_vector_type() && getNCMLTypeForVariable(p) == "string") {
        string sep = ((_separator.empty()) ? (NCMLUtil::WHITESPACE) : (_separator));
        NCMLUtil::tokenize(_accumulated_content, _tokens, sep);
    }
    else // for arrays of other values, use whitespace separation for default if not specified.
    {
        string sep = ((_separator.empty()) ? (NCMLUtil::WHITESPACE) : (_separator));
        NCMLUtil::tokenize(_accumulated_content, _tokens, sep);
    }

    if (!shouldAutoGenerateValues()) {
        setVariableValuesFromTokens(p, *pVar);
        setGotValuesOnOurVariableElement(p);
    }

    // In the original version of this method, the 'if' before this was
    // if (!shouldAutoGenerateValues() && !_gotContent) and it throws/threw an
    // exception (I moved that code up in my modification of this method). But
    // dealWithEmptyStringValues() only does something when _gotContent is false,
    // so there's no way to get to call dealWithEmptyStringValues here. I'm
    // removing it and looking at the tests. jhrg 10/12/11
#if 0
    // if unspecified, string and url vars get set to empty string ""
    if (!shouldAutoGenerateValues())
    {
        dealWithEmptyStringValues();
    }
    // Otherwise, we're all good.
#endif
}

string ValuesElement::toString() const
{
    return "<" + _sTypeName + " " + ((_start.empty()) ? ("") : ("start=\"" + _start + "\" "))
        + ((_increment.empty()) ? ("") : ("increment=\"" + _increment + "\" "))
        + ((_separator == NCMLUtil::WHITESPACE) ? ("") : ("separator=\"" + _separator + "\" ")) + ">";
}

void ValuesElement::validateStartAndIncrementForVariableTypeOrThrow(libdap::BaseType& /* pVar */) const
{
    // TODO IMPL ME
    // Look up the types...
}

template<class DAPType, typename ValueType>
void ValuesElement::setScalarValue(libdap::BaseType& var, const string& valueAsToken)
{
    // Make sure we got the right subclass type
    DAPType* pVar = dynamic_cast<DAPType*>(&var);
    NCML_ASSERT_MSG(pVar, "setScalarValue() got called with BaseType not matching the expected type.");

    // Parse the token using stringstream and the template ValueType.
    std::stringstream sis;
    sis.str(valueAsToken);
    ValueType value;
    sis >> value;
    if (sis.fail()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Setting array values failed to read the value token properly!  value was for var name=" + var.name()
                + " and the value token was " + valueAsToken);
    }

    // assuming it works, there should be a setValue(ValueType) on pVar now
    pVar->set_value(value);
}

/** We need to specialize for the case of a Byte since the
 * stringstream >> will just read a single char and not assume the
 * string is a decimal value in [0, 255].
 */
template<>
void ValuesElement::setScalarValue<Byte, dods_byte>(libdap::BaseType& var, const string& valueAsToken)
{
    Byte* pVar = dynamic_cast<Byte*>(&var);
    NCML_ASSERT_MSG(pVar, "setScalarValue() got called with BaseType not matching the expected type.");

    std::stringstream sis;
    sis.str(valueAsToken);
    dods_uint16 value; // read it as an unsigned short.
    sis >> value;
    if (sis.fail()) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Setting array values failed to read the value token properly!  value was for var name=" + var.name()
                + " and the value token was " + valueAsToken);
    }

    // then cast it for the set...
    pVar->set_value(static_cast<dods_byte>(value));
}

/** We need to specialize for the case of a ValueType of std::string as well since the
 * stringstream >> will just read a single TOKEN and not the whole string.  Grr.
 */
template<>
void ValuesElement::setScalarValue<Str, string>(libdap::BaseType& var, const string& valueAsToken)
{
    Str* pVar = dynamic_cast<Str*>(&var);
    NCML_ASSERT_MSG(pVar, "setScalarValue() got called with BaseType not matching the expected type.");
    pVar->set_value(valueAsToken);
}

/** We need to specialize for the case of a ValueType of std::string as well since the
 * stringstream >> will just read a single TOKEN and not the whole string.  Grr.
 */
template<>
void ValuesElement::setScalarValue<Url, string>(libdap::BaseType& var, const string& valueAsToken)
{
    Url* pVar = dynamic_cast<Url*>(&var);
    NCML_ASSERT_MSG(pVar, "setScalarValue() got called with BaseType not matching the expected type.");
    pVar->set_value(valueAsToken);
}

template<typename DAPType>
void ValuesElement::setVectorValues(libdap::Array* pArray, const vector<string>& valueTokens)
{
    VALID_PTR(pArray);
    // Make an array of values ready to read them all the tokens.
    vector<DAPType> values;
    values.reserve(valueTokens.size());

    int count = 0; // only to help error output msg
    vector<string>::const_iterator endIt = valueTokens.end();
    for (vector<string>::const_iterator it = valueTokens.begin(); it != endIt; ++it) {
        DAPType value;
        stringstream valueTokenAsStream;
        const std::string& token = *it;
        valueTokenAsStream.str(token);
        valueTokenAsStream >> value;
        if (valueTokenAsStream.fail()) {
            stringstream msg;
            msg << "Got fail() on parsing a value token for an Array name=" << pArray->name()
                << " for value token index " << count << " with token=" << (*it) << " for element " << toString();
            THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(), msg.str());
        }
        values.push_back(value);
        count++;
    }

    // Call the overloaded set with the parsed vector.
    pArray->set_value(values, values.size());
}

/** Specialization to handle the fact that operator>> tokenizes itself,
 * but we just want to shove the ENTIRE token in there for each one.
 */
template<>
void ValuesElement::setVectorValues<string>(libdap::Array* pArray, const vector<string>& valueTokens)
{
    VALID_PTR(pArray);
    // Call it DIRECTLY with the given values, modulo const_cast
    vector<string>& values = const_cast<vector<string>&>(valueTokens);
    pArray->set_value(values, values.size());
}

/**
 * Special case call for parsing char since underlying type is same as byte but want to parse differently.
 */
void ValuesElement::parseAndSetCharValue(libdap::BaseType& var, const string& valueAsToken)
{
    Byte* pVar = dynamic_cast<Byte*>(&var);
    NCML_ASSERT_MSG(pVar, "setScalarValue() got called with BaseType not matching the expected type.");

    if (valueAsToken.size() != 1) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Parsing scalar char, expected single character but didnt get it.  value was for var name=" + var.name()
                + " and the value token was " + valueAsToken);
    }

    // Read the char and set it
    dods_byte val = valueAsToken.at(0);
    pVar->set_value(val);
}

void ValuesElement::parseAndSetCharValueArray(NCMLParser& /* p */, libdap::Array* pVecVar, const vector<string>& tokens)
{
    vector<dods_byte> values;
    for (unsigned int i = 0; i < tokens.size(); ++i) {
        values.push_back(static_cast<dods_byte>((tokens.at(i))[0]));
    }
    pVecVar->set_value(values, values.size());
}

void ValuesElement::setVariableValuesFromTokens(NCMLParser& p, libdap::BaseType& var)
{
    // It's an error to have <values> for a Structure variable!
    if (var.type() == dods_structure_c) {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Illegal to specify <values> element for a Structure type variable name=" + var.name() + " at scope="
                + p.getScopeString());
    }
    // First, make sure the dimensionality matches or we're doomed from the get-go
    if (var.is_simple_type()) {
        setScalarVariableValuesFromTokens(p, var);
    }
    else if (var.is_vector_type()) {
        setVectorVariableValuesFromTokens(p, var);
    }
    else {
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
            "Can't call ValuesElement::setVariableValuesFromTokens for constructor type now!! "
                "Variable named " + var.name() + " at scope=" + p.getScopeString());
    }
}

void ValuesElement::setScalarVariableValuesFromTokens(NCMLParser& p, libdap::BaseType& var)
{
    // OK, we have a scalar, so make sure there's exactly one token!
    if (_tokens.size() != 1) {
        stringstream msg;
        msg << "While setting scalar variable name=" << var.name()
            << " we expected exactly 1 value in content but found " << _tokens.size() << " tokens.";
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(), msg.str());
    }

    // OK, we have one token for a scalar.  Now make sure it's a valid value for the given type.
    // legacy "char" type (internally a byte) is specified differently, so don't check for that case.
    if (getNCMLTypeForVariable(p) != "char") {
        p.checkDataIsValidForCanonicalTypeOrThrow(var.type_name(), _tokens);
    }

    // Just one of em
    const string& valueToken = _tokens.at(0);

    // OK, now it gets pretty ugly, but we hid most of it behind a template...
    Type varType = var.type();
    switch (varType) {
    case dods_byte_c:
        // Special case depending on whether the underlying NcML was type-converted
        // from "char" or not.  If char, we parse as char, not numeric
        if (getNCMLTypeForVariable(p) == "char") {
            parseAndSetCharValue(var, valueToken);
        }
        else {
            setScalarValue<Byte, dods_byte>(var, valueToken);
        }
        break;

    case dods_int16_c:
        setScalarValue<Int16, dods_int16>(var, valueToken);
        break;

    case dods_uint16_c:
        setScalarValue<UInt16, dods_uint16>(var, valueToken);
        break;

    case dods_int32_c:
        setScalarValue<Int32, dods_int32>(var, valueToken);
        break;

    case dods_uint32_c:
        setScalarValue<UInt32, dods_uint32>(var, valueToken);
        break;

    case dods_float32_c:
        setScalarValue<Float32, dods_float32>(var, valueToken);
        break;

    case dods_float64_c:
        setScalarValue<Float64, dods_float64>(var, valueToken);
        break;

    case dods_str_c:
        setScalarValue<Str, string>(var, valueToken);
        break;

    case dods_url_c:
        setScalarValue<Url, string>(var, valueToken);
        break;

    default:
        THROW_NCML_INTERNAL_ERROR("Expected simple type but didn't find it!")
        ;
        break;
    }
}

void ValuesElement::setVectorVariableValuesFromTokens(NCMLParser& p, libdap::BaseType& var)
{
    Array* pVecVar = dynamic_cast<Array*>(&var);
    NCML_ASSERT_MSG(pVecVar, "ValuesElement::setVectorVariableValuesFromTokens expect var"
        " to be castable to class Array but it wasn't!!");

    // Make sure the Array length matches the number of tokens.
    // Note that length() should be the product of dimension sizes since N-D arrays are flattened in row major order
    if (pVecVar->length() > 0 && static_cast<unsigned int>(pVecVar->length()) != _tokens.size()) {
        stringstream msg;
        msg << "Dimension mismatch!  Variable name=" << pVecVar->name() << " has dimension product="
            << pVecVar->length() << " but we got " << _tokens.size() << " values in the values element " << toString();
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(), msg.str());
    }

    // Make sure all the tokens are valid for the given datatype
    if (getNCMLTypeForVariable(p) != "char") {
        BaseType* pTemplate = var.var();
        VALID_PTR(pTemplate);
        p.checkDataIsValidForCanonicalTypeOrThrow(pTemplate->type_name(), _tokens);
    }

    // Finally, go and parse the values in for the given type and set them in the vector
    VALID_PTR(pVecVar->var());
    libdap::Type valType = pVecVar->var()->type();
    switch (valType) {
    case dods_byte_c:
        // Special case depending on whether the underlying NcML was type-converted
        // from "char" or not.  If so, read the ascii character not the numeric version of it.
        if (getNCMLTypeForVariable(p) == "char") {
            parseAndSetCharValueArray(p, pVecVar, _tokens);
        }
        else {
            setVectorValues<dods_byte>(pVecVar, _tokens);
        }
        break;

    case dods_int16_c:
        setVectorValues<dods_int16>(pVecVar, _tokens);
        break;

    case dods_uint16_c:
        setVectorValues<dods_uint16>(pVecVar, _tokens);
        break;

    case dods_int32_c:
        setVectorValues<dods_int32>(pVecVar, _tokens);
        break;

    case dods_uint32_c:
        setVectorValues<dods_uint32>(pVecVar, _tokens);
        break;

    case dods_float32_c:
        setVectorValues<dods_float32>(pVecVar, _tokens);
        break;

    case dods_float64_c:
        setVectorValues<dods_float64>(pVecVar, _tokens);
        break;

    case dods_str_c:
        setVectorValues<string>(pVecVar, _tokens);
        break;

    case dods_url_c:
        setVectorValues<string>(pVecVar, _tokens);
        break;

    default:
        THROW_NCML_INTERNAL_ERROR("Expected Vector template type was a simple type but didn't find it!")
        ;
        break;
    } // switch

}

template<typename DAPType>
void ValuesElement::generateAndSetVectorValues(NCMLParser& p, libdap::Array* pArray)
{
    // 1) Check that the start and increment values can be parsed into the appropriate type
    // 2) Find out the dimensionality
    // 3) Generate a vector of values with the dimensionality
    // 4) Set it on pArray

    DAPType start;
    {
        stringstream sis;
        sis.str(_start);
        sis >> start;
        if (sis.fail()) {
            THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
                "Failed to parse the values@start=" + _start + " for " + toString() + " at scope=" + p.getScopeString());
        }
    }

    DAPType increment;
    {
        stringstream sis;
        sis.str(_increment);
        sis >> increment;
        if (sis.fail()) {
            THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(),
                "Failed to parse the values@increment=" + _start + " for " + toString() + " at scope="
                    + p.getScopeString());
        }
    }

    int numPoints = pArray->length();
    NCML_ASSERT(numPoints >= 1);
    vector<DAPType> values;
    values.reserve(numPoints);
    DAPType x = start;
    values.push_back(x);
    for (int i = 1; i < numPoints; ++i) {
        x += increment;
        values.push_back(x);
    }
    NCML_ASSERT(values.size() == static_cast<unsigned int>(numPoints));
    pArray->set_value(values, values.size());
}

void ValuesElement::autogenerateAndSetVariableValues(NCMLParser& p, BaseType& var)
{
    // First, make sure var is an Array or we don't know what to do
    libdap::Array* pArray = dynamic_cast<libdap::Array*>(&var);
    if (!pArray) {
        THROW_NCML_INTERNAL_ERROR(
            "ValuesElement::autogenerateAndSetVariableValues: expected variable of type libdap::Array but failed to cast it!");
    }

    setGotValuesOnOurVariableElement(p);

    // Next, find out the underlying type and use the class's template calls.
    libdap::BaseType* pTemplate = pArray->var();
    VALID_PTR(pTemplate);
    switch (pTemplate->type()) {
    case dods_byte_c:
        // This doesn't work for char!
        if (getNCMLTypeForVariable(p) == "char") {
            THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(), "Can't use values@start for non-numeric values!");
        }
        else // it works for bytes
        {
            generateAndSetVectorValues<dods_byte>(p, pArray);
        }
        break;

    case dods_int16_c:
        generateAndSetVectorValues<dods_int16>(p, pArray);
        break;

    case dods_uint16_c:
        generateAndSetVectorValues<dods_uint16>(p, pArray);
        break;

    case dods_int32_c:
        generateAndSetVectorValues<dods_int32>(p, pArray);
        break;

    case dods_uint32_c:
        generateAndSetVectorValues<dods_uint32>(p, pArray);
        break;

    case dods_float32_c:
        generateAndSetVectorValues<dods_float32>(p, pArray);
        break;

    case dods_float64_c:
        generateAndSetVectorValues<dods_float64>(p, pArray);
        break;

        // User error
    case dods_str_c:
    case dods_url_c:
        THROW_NCML_PARSE_ERROR(_parser->getParseLineNumber(), "Can't use values@start for non-numeric values!")
        ;
        break;

    default:
        THROW_NCML_INTERNAL_ERROR("Expected Vector template type was a simple type but didn't find it!")
        ;
        break;
    } // switch

}

std::string ValuesElement::getNCMLTypeForVariable(NCMLParser& p) const
{
    const VariableElement* pMyParent = getContainingVariableElement(p);
    VALID_PTR(pMyParent);
    return pMyParent->type();
}

const VariableElement*
ValuesElement::getContainingVariableElement(NCMLParser& p) const
{
    const VariableElement* ret = 0;

    // Get the parse stack for p and walk up it!
    NCMLParser::ElementStackConstIterator it;
    NCMLParser::ElementStackConstIterator endIt = p.getElementStackEnd();
    for (it = p.getElementStackBegin(); it != endIt; ++it) {
        const NCMLElement* pElt = *it;
        const VariableElement* pVarElt = dynamic_cast<const VariableElement*>(pElt);
        if (pVarElt) {
            ret = pVarElt;
            break;
        }
    }
    return ret;
}

void ValuesElement::setGotValuesOnOurVariableElement(NCMLParser& p)
{
    // Ugh, I know I shouldn't do this, but...
    VariableElement* pContainingVar = const_cast<VariableElement*>(getContainingVariableElement(p));
    VALID_PTR(pContainingVar);
    pContainingVar->setGotValues();
}

// I'm not sure I understand this method - I think it's use in handleEnd() is
// no longer needed given the changes I made there. jhrg 10/12/11
void ValuesElement::dealWithEmptyStringValues()
{
    // To reuse all the logic, we'll just explicitly call handleContent("")
    // which will push an empty string token for scalar string and url.
    if (!_gotContent) {
        handleContent("");
    }
}

vector<string> ValuesElement::getValidAttributes()
{
    vector<string> validAttrs;
    validAttrs.reserve(3);
    validAttrs.push_back("start");
    validAttrs.push_back("increment");
    validAttrs.push_back("separator");
    // we disallow npts since it was deprecated and we'll be used for new files.
    return validAttrs;
}

}
