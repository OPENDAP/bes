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

#ifndef __NCML_MODULE__SCOPE_STACK_H__
#define __NCML_MODULE__SCOPE_STACK_H__

#include <string>
#include <vector>

using namespace std;


/**
 * @brief Helper class for NCMLParser to maintain the current scope of the
 * parse.
 *
 * Basically, the full scope stack defines the namespace describing the location
 * of a variable or attribute in a DDS.  It can be used to generate the fully qualified
 * name of a variable or attribute within the DDS.
 *
 * For now, an empty scope stack will implicitly mean GLOBAL scope (DDS level, or what have you).
 * We won't actually push an entry for GLOBAL, it will just be assumed if empty().
 * We also won't put it in the scope strings since they are meant to refer to fully qualified names,
 * where an empty() name means global scope.
 *
 * @author mjohnson <m.johnson@opendap.org>
 */
namespace ncml_module
{
  class ScopeStack
  {
  public: // Inner classes
    /** The current scope is either global attribute table, within a variable's attribute table,
    * or within an attribute container. Additionally, we can be "within" an atomic attribute
    * if the <attribute>values here</attribute> form is used.
    */
    enum ScopeType { GLOBAL=0, VARIABLE_ATOMIC, VARIABLE_CONSTRUCTOR, ATTRIBUTE_ATOMIC, ATTRIBUTE_CONTAINER, NUM_SCOPE_TYPES};

    /**
      * Entry used in Scope class to maintain where we are within the DDS AttrTable hierarchy.
      * Each time we traverse down into containers, we'll
      * push one of these on a stack to keep track of where we
      * are in a parse for debugging.
    */
    struct Entry
    {
      Entry(ScopeType theType, const string& theName);
      Entry() : type(GLOBAL) , name("") {}

      string getTypedName() const
      {
        return name + toString(type);
      }

      static const string& toString(ScopeType theType)
      {
        return sTypeStrings[theType];
      }

      ScopeType type;
      string name;
      static const string sTypeStrings[NUM_SCOPE_TYPES];
    };

    ///////////////////////////// METHODS
  public:

    ScopeStack();
    virtual ~ScopeStack();

    void clear();
    void push(const string& name, ScopeType type) { push(Entry(type, name)); }
    void pop();
    const Entry& top() const;

    ScopeType topType() const { return top().type; }
    const string& topName() const { return top().name; }

    /** If there are no entries pushed.  If empty(), we assume
     * isCurrentScope(GLOBAL) is true.
    */
    bool empty() const;

    /** How many things are on the stack. This is useful for knowing if an
     * atomic attribute is being added at the top level of the DAS/DDS (which
     * DAP2 does not allow, but DAP4 will).
     * @return The number of items on the stack.
     */
    int size() const;

    string getFullyQualifiedName() const { return getScopeString(); }

    /**
     * Return a fully qualifed name for the scope, such as "" for global scope or
     * "MetaData.Info.Name" for an attribute container, etc.
    */
    string getScopeString() const;

    /**
     * Similar to getScopeString(), but appends the type of the
     * scope to the name in the form "Name<TYPE>" for better debugging.
     * For example, "MetaData<Attribute_Container>.Info<Attribute_Container>.Name<Attribute_Atomic>"
     * gives more information about the context.
     * @return
     */
    string getTypedScopeString() const;

    /** Is the current scope of the given type?
     * Note that isCurrentScope(GLOBAL) == empty().
     * In other words, an empty stack implicitly means global scope.
     * */
    bool isCurrentScope(ScopeType type) const;

  private: // Method

    void push(const Entry& entry);

    /////////////////////////// DATA REP
  private:
    vector<Entry> _scope;
  };
}

#endif /* __NCML_MODULE__SCOPE_STACK_H__ */
