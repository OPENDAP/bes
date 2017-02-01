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
#ifndef __NCML_MODULE__VARIABLE_ELEMENT_H__
#define __NCML_MODULE__VARIABLE_ELEMENT_H__

#include "NCMLElement.h"

namespace libdap {
class BaseType;
}

namespace ncml_module {

/**
 * @brief Concrete class for NcML <variable> element
 *
 * This class handles the processing of <variable> elements
 * in the NcML.
 *
 * The class handles all processing of variables, including
 * setting lexical scope in the parser to the variable's attribute table,
 * renaming variables, and creating new variables.
 *
 * _isNewVariable specifies if the variable was created new in this NcML parse, or
 *    was just a lexical scope.
 * _gotValues specifies if the variable had a contained <values> element processed on it yet.
 *
 * On handleEnd(), if _isNewVariable && !_gotValues, a parse error is thrown.
 *
 * It is also an error to specify a second <values> element if _gotValues.
 */
class VariableElement: public NCMLElement {
private:
    VariableElement& operator=(const VariableElement& rhs); // disallow

public:
    static const string _sTypeName;
    static const vector<string> _sValidAttributes;

    VariableElement();
    VariableElement(const VariableElement& proto);
    virtual ~VariableElement();
    virtual const string& getTypeName() const;
    virtual VariableElement* clone() const; // override clone with more specific subclass
    virtual void setAttributes(const XMLAttributeMap& attrs);
    virtual void handleBegin();
    virtual void handleContent(const string& content);
    virtual void handleEnd();
    virtual string toString() const;

    const std::string name() const
    {
        return _name;
    }

    const std::string type() const
    {
        return _type;
    }

    const std::string shape() const
    {
        return _shape;
    }

    const std::string orgName() const
    {
        return _orgName;
    }

    /** @return whether this variable was created and added as a new variable in this parse */
    bool isNewVariable() const;

    /** @return whether we got a values element set to us yet, used to make sure new
     * variables get values once and only once.
     */
    bool checkGotValues() const;

    /** Called once we set the values from ValuesElement so we are aware. */
    void setGotValues();

private:

    /**
     * Handle the beginning of the variable element.
     * If _type=="" we assume we just want to match any type and traverse down.
     * If _type is specified, we assume we want to typecheck.
     * Pushes the variable onto the scope stack.
     * The parser's attribute table becomes this variable's table.
     */
    void processBegin(NCMLParser& p);

    /**
     * Handle exiting the scope of the variable.
     * Pops the current attribute table of the parser up to the
     * parent of this variable's table.
     * The p.getCurrentVariable() will also become the parent of the
     * current variable, or NULL if top-level variable.
     */
    void processEnd(NCMLParser& p);

    /** process this variable as one that already exists at the current parser scope
     * within the current dataset.  Essentially this means making the variable be
     * the new current scope of the parser for subsequent other variable or attribute calls.
     * @param p the parser to effect
     * @param pVar the existing variable in current scope, already looked up.  If null, we look it up from _name.
     */
    void processExistingVariable(NCMLParser& p, libdap::BaseType* pVar);

    /**
     * If _orgName is specified, this function is called from processBegin().
     * If a variable named _orgName exists at current parser scope,
     * the variable is renamed to _name.
     *
     * @param p the parser to effect
     *
     * @exception BESSyntaxUserError if _orgName does not exist at current scope
     * @exception BESSyntaxUserError if _name already DOES exist at current scope
     *
     * Assumes: !_orgName.empty()
     * On exit, the scope of p will be the renamed variable.
     */
#if 0
    // Disabled for now. See comment in the .cc file ndp - 08/12/2015
    void processRenameVariableDataWorker(NCMLParser& p, libdap::BaseType* pOrgVar);
#endif
    void processRenameVariable(NCMLParser& p);

    /**
     * Called from processBegin() if a variable with _name does NOT exist at
     * current parser scope of p.
     *
     * A new variable of type _type is created at the current parse scope.
     * This new variable will be the new scope of p on exit.
     *
     * If !_shape.empty(), then the created variable is a DAP Array of
     * type _type.  _shape will be tokenized with whitespace separators
     * in order to resolve the dimensions of the Array.  _shape can contain
     * either integers or mnemonic references to previously declared <dimension>'s
     * which must exist at the current parse scope of p.
     */
    void processNewVariable(NCMLParser& p);

    /** @brief Create a new Structure variable at current scope.
     * ASSERT: this._type == "Structure"
     * On exit, the new variable will be the new current scope.
     */
    void processNewStructure(NCMLParser& p);

    /** @brief Create a new scalar variable of simple type at current scope.
     * ASSERT: On exit, the new variable will be the current scope.
     * @param p the parser to effect
     * @param dapType the internal simple DAP type (canonical type of _type) to create.
     */
    void processNewScalar(NCMLParser& p, const std::string& dapType);

    /** @brief Create a new Array of type dapType using the value from a nonempty _shape
     * @param p the parser to effect
     * @param dapType the internal DAP type for the array values
     *
     * The array size is gleaned from the _shape.  _shape can contain
     * white-space separated non-negative integers to specify the
     * dimensions of the array.
     *
     * We also allow the shape array to consist of white-space separated
     * tokens that will be assumed to be dimension references, which we can
     * look up and map to a size in the current dimension table.
     */
    void processNewArray(NCMLParser& p, const std::string& dapType);

    /** If pOrgVar is of type Array, then convert its data into an NCMLArray<T>
     * and replace pOrgVar in the current scope with the new NCMLArray under the
     * new name.  This will destroy pOrgVar.
     *
     * @return the new variable if it was replaced (since pOrgVar has been deleted)
     * else pOrgVar if untouched.
     */
    libdap::BaseType* replaceArrayIfNeeded(NCMLParser& p, libdap::BaseType* pOrgVar, const string& name);

    /** @brief Create a new variable of the given dapType and add it to the
     * current scope.  Then enter its scope.
     *
     * On exit, the scope of p will be within the new variable.
     * The variable is named _name.
     *
     * @param p the parser to effect
     * @param dapType the internal dap type to create, such as Array or Float32.
     */
    void addNewVariableAndEnterScope(NCMLParser& p, const std::string& dapType);

    /** @brief Tell the parser to use pVar as the current scope.
     * This also set's the current table to the pVar table.
     * pVar must not be null.*/
    void enterScope(NCMLParser& p, libdap::BaseType* pVar);

    /** Pop off this variable from the scope. */
    void exitScope(NCMLParser& p);

    /** @return if the dimToken is a numeric constant.
     * Otherwise, it's assumed to be a named dimensions.
     */
    bool isDimensionNumericConstant(const std::string& dimToken) const;
    /**
     * @return the size of the given dimension, either by parsing an int constant
     * or by looking up the dimToken as a named dimension in the parser.
     */
    unsigned int getSizeForDimension(NCMLParser& p, const std::string& dimToken) const;

    /** @return the product of the size of each dimension, which equates
     * to the total number of values for a multi-dimensional array.
     *
     * Note that this will be 0 for a scalar!!  In other words, a scalar is
     * considered 0 dimension, whereas an array with a single element is dimension 1
     * since they are represented differently though both contain technically a single value.
     *
     * DAP2 restricts the maximum of this value to be 2^32-1, which
     * happens to be the maximum for the unsigned int we return.
     *
     * @param p the parser whose dimension table we should use for named shape lookups.
     *
     * @exception BESInternalError: If this product is larger than 2^32-1, such that we
     * cannot return the correct value without overflow.
     */
    unsigned int getProductOfDimensionSizes(NCMLParser& p) const;

    static vector<string> getValidAttributes();

private:
    string _name;
    string _type;
    string _shape; // empty() => existing var (shape implicit) or if new var, then scalar (rank 0).
    string _orgName; // if !empty(), the name of existing variable we want to rename to _name

    // Ephemeral state below

    // tokenized version of _shape for dimensions
    vector<string> _shapeTokens;

    // if not null, this element created this var and it exists in the dds of the containing dataset
    libdap::BaseType* _pNewlyCreatedVar;

    // true once we get a valid <values> element with values in it.  Used for parse error checking
    bool _gotValues;
};

}

#endif /* __NCML_MODULE__VARIABLE_ELEMENT_H__ */
