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
#ifndef __NCML_MODULE__VALUESELEMENT_H__
#define __NCML_MODULE__VALUESELEMENT_H__

#include "NCMLElement.h"
#include "NCMLDebug.h"
#include <sstream>
#include <string>
#include <vector>

namespace libdap {
class Array;
class BaseType;
}

namespace ncml_module {
class NCMLParser;
class VariableElement;

class ValuesElement: public NCMLElement {
private:
    ValuesElement& operator=(const ValuesElement& rhs); // disallow

public:
    static const std::string _sTypeName;
    static const std::vector<std::string> _sValidAttributes;

    ValuesElement();
    ValuesElement(const ValuesElement& proto);
    virtual ~ValuesElement();
    virtual const std::string& getTypeName() const;
    virtual ValuesElement* clone() const; // override clone with more specific subclass
    virtual void setAttributes(const XMLAttributeMap& attrs);
    virtual void handleBegin();
    virtual void handleContent(const std::string& content);
    virtual void handleEnd();
    virtual std::string toString() const;

private:
    // Methods

    /** @return true if we have a non-empty start and/or increment.
     * This means that we DO NOT expect non-whitespace content
     * and that we need to auto-generate the values for the given variable!
     */
    bool shouldAutoGenerateValues() const
    {
        return ((!_start.empty()) && (!_increment.empty()));
    }

    /** @brief Name says it all
     *  If _start or _increment cannot be successfully parsed into the type of pVar,
     *  then throw a parse error!
     *  @exception BESSyntaxUserError if _start or _increment are not valid numeric strings for pVar type.
     */
    void validateStartAndIncrementForVariableTypeOrThrow(libdap::BaseType& var) const;

    /** Assumes _tokens has tokenized values, go and set the variable pVar
     * with these values as appropriate for the type.
     *
     * @param p the parser state
     * @param var the variable these values are being set into
     *
     * Note: the number of tokens MUST be valid for the given type or exception!
     * @exception BESSyntaxUserError if the number of values specified does not match
     *                  the dimensionality of the pVar
     */
    void setVariableValuesFromTokens(NCMLParser& p, libdap::BaseType& var);

    /** @brief Helper for setVariableValuesFromTokens() that is called if var is a
     * simple type.
     *
     * Assumes: _tokens contains the tokenized value to put into var.
     * Assumes: _tokens.size() == 1 (or exception is throw)
     * Assumes: var.is_simple_type()
     */
    void setScalarVariableValuesFromTokens(NCMLParser& p, libdap::BaseType& var);

    /** @brief Helper for setVariableValuesFromTokens() that is called if var is a
     * vector type.
     *
     * Set the values in the array from the _tokens for the given underlying type
     * of the Vector variable var.
     *
     * @param p the parser to effect
     * @param var the variable to set values into, must be castable to class Vector
     *
     * Assumes: _tokens contains the tokenized values to put into var.
     * Assumes: _tokens.size() == the dimension of the Array (ie var) (or exception is throw)
     * Assumes: var.is_vector_type() so we can cast it to class Vector.
     */
    void setVectorVariableValuesFromTokens(NCMLParser& p, libdap::BaseType& var);

    /** @brief Template to parse the start and increment attributes and generate values for them.
     *  Generate the correct number of points using _start and _increment for the given DAPType.
     *  Use these to set the value on pArray
     *
     *  @param p the parser start to use
     *  @param pArray the Array variable to generate values for.
     *
     *  @exception if the start or increment cannot be parsed correctly.
     */
    template<typename DAPType> void generateAndSetVectorValues(NCMLParser& p, libdap::Array* pArray);

    /** @brief Autogenerate uniform interval numeric values from _start and _increment into variable
     *
     * @param p the parser state to use
     * @param var the variable to set the values for
     */
    void autogenerateAndSetVariableValues(NCMLParser& p, libdap::BaseType& var);

    /** A parameterized set function for all the DAP simple types (Byte, UInt32, etc...) and
     * their ValueType in DAPType::setValue(ValueType val)
     *
     * @typename DAPType: the subclass of simple type of BaseType
     * @typename ValueType: the internal simple type stored in DAPType, the arg type to DAPType::setValue()
     *
     * @param var the simple type var (scalar) of subclass type DAPType
     * @param valueAsToken the unparsed token version of the value.  Will be read using streams.
     */
    template<class DAPType, typename ValueType> void setScalarValue(libdap::BaseType& var, const std::string& valueAsToken);

    /** Parameterized set function to parse an array of value tokens for a given DAP type and then set the values into
     * an Array variable.
     *
     * @typename DAPType  the underlying data type of the Array
     * @param pArray the Array to set the values for
     * @param valueTokens the tokenized vector of value tokens from which to parse the actual values of DAPType
     *
     * ASSUMES: the number of tokens matches the length of the Vector super.
     * ASSUMES: pArray->dimensions() == 1!  This ONLY works for 1D arrays now!
     */
    template<typename DAPType> void setVectorValues(libdap::Array* pArray, const std::vector<std::string>& valueTokens);

    /** Special case for parsing char's instead of bytes. */
    void parseAndSetCharValue(libdap::BaseType& var, const std::string& valueAsToken);

    /** Parse the first token in tokens as if it were an array of char's
     * and store it into the pVecVar, which is assumed to be an Array<Byte>
     * @exception if these assumptions are not true
     */
    void parseAndSetCharValueArray(NCMLParser& p, libdap::Array* pVecVar, const std::vector<std::string>& tokens);

    /**
     * Figure out the NcML type of <variable> element we are within by walking up the parse stack of p
     * This should work for Arrays as well as simple types!
     */
    std::string getNCMLTypeForVariable(NCMLParser& p) const;

    /** Get the VariableElement we are contained within in the parse */
    const VariableElement* getContainingVariableElement(NCMLParser& p) const;

    /** Call setGotValues() on the containing variable element */
    void setGotValuesOnOurVariableElement(NCMLParser& p);

    /** If a var of type string or url is specified, we need to explicitly push an
     * empty string since the parser won't actually make the call.
     */
    void dealWithEmptyStringValues();

    static std::vector<std::string> getValidAttributes();

private:
    // Data Rep
    std::string _start;
    std::string _increment;
    std::string _separator; // defaults to whitespace

    // If we got handleContent successfully!
    bool _gotContent;

    //TODO add comment
    std::string _accumulated_content;
    // Temp to tokenize the content on handleContent()
    std::vector<std::string> _tokens;
};

}

#endif /* __NCML_MODULE__VALUESELEMENT_H__ */
