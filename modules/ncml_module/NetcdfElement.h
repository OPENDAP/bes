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
#ifndef __NCML_MODULE__NETCDF_ELEMENT_H__
#define __NCML_MODULE__NETCDF_ELEMENT_H__

#include "AggMemberDataset.h" // agg_util
#include "DDSAccessInterface.h"
#include "DDSLoader.h"
#include "NCMLElement.h"

namespace libdap {
class BaseType;
class DDS;
}

class BESDapResponse;

namespace ncml_module {

class AggregationElement;
class DimensionElement;
class NCMLParser;
class VariableElement;

/**
 * @brief Concrete class for NcML <netcdf> element
 *
 * This element specifies the location attribute for the local
 * data file that we wrap and load into a DDX (DDS w/ AttrTable tree).
 *
 * We keep a ptr to our containing NCMLParser to help with
 * the differences needed if we are the root dataset element or not,
 * in particular that the response object is either passed in to us (root)
 * or loaded brandy new if we're the child of an aggregation.
 */
class NetcdfElement: public NCMLElement, // superclass
    public virtual agg_util::DDSAccessRCInterface // interface
{
private:
    NetcdfElement& operator=(const NetcdfElement& rhs); //disallow

public:
    static const string _sTypeName;
    static const vector<string> _sValidAttributes;

    NetcdfElement();
    NetcdfElement(const NetcdfElement& proto);
    virtual ~NetcdfElement();
    virtual const string& getTypeName() const;
    virtual NetcdfElement* clone() const; // override clone with more specific subclass
    virtual void setAttributes(const XMLAttributeMap& attrs);
    virtual void handleBegin();
    virtual void handleContent(const string& content);
    virtual void handleEnd();
    virtual string toString() const;

    // Accessors for attributes we deal with.
    // TODO Add these as we support aggregation attributes
    const string& location() const
    {
        return _location;
    }
    const string& id() const
    {
        return _id;
    }
    const string& title() const
    {
        return _title;
    }
    const string& coordValue() const
    {
        return _coordValue;
    }
    const string& ncoords() const
    {
        return _ncoords;
    }

    bool hasNcoords() const
    {
        return !_ncoords.empty();
    }

    /**
     * Get the ncoords() field as a valid size.
     * Throws: if !hasNCoords().
     * @return the parsed value of the ncoords() field.
     */
    unsigned int getNcoordsAsUnsignedInt() const;

    /**
     * @return whether this is initialized properly and ready to be used.
     * It should be true after handleBegin() if all is well.
     */
    bool isValid() const;

    /**
     * Return the DDS for this dataset, loading
     * it in if needed.  (semantically const
     * although the loaded DDS is cached).
     */
    virtual const libdap::DDS* getDDS() const;

    /** Non-const version to allow changes to the DDS.
     * Do NOT delete the return value!
     * */
    virtual libdap::DDS* getDDS();

    bool getProcessedMetadataDirective() const
    {
        return _gotMetadataDirective;
    }

    void setProcessedMetadataDirective()
    {
        _gotMetadataDirective = true;
    }

    /** Used by the NCMLParser to let us know to borrow the response
     * object and not own it.  Used for the root element only!
     * Nested datasets will create and own their own!
     */
    void borrowResponseObject(BESDapResponse* pResponse);

    /** Kind of superfluous, but tells this object to clear its reference
     * to pReponse, which had better match _response or we throw internal exception.
     */
    void unborrowResponseObject(BESDapResponse* pResponse);

    /**
     * Called if this is a member of an aggregation (i.e. not root)
     * to create a dynamic response object of the given type.
     * This call or borrowResponseObject() must be called before this is used.
     */
    void createResponseObject(agg_util::DDSLoader::ResponseType type);

    /**
     * Return a shared reference to the AggMemberDataset that encapsulates
     * this dataset.
     * If it doesn't exist in this instance yet, it is created and
     * stored in this (semantically const accessor)
     * If it does exist, a shared reference to the contained object is returned.
     */
    RCPtr<agg_util::AggMemberDataset> getAggMemberDataset() const;

    /**
     * @return the DimensionElement with the given name in the
     * dimension table for THIS NetcdfElement only (no traversing
     * up the tree is allowed).  If not found, NULL is returned.
     */
    const DimensionElement* getDimensionInLocalScope(const string& name) const;

    /**
     * @return the first DimensionElement with name found by checking
     * getDimensionInLocalScope() starting at this element and traversing upwards
     * to the enclosing scopes (in case we're in an aggregation).
     * Return NULL if not found.
     * @Note this allows dimensions to be lexically "shadowed".
     * @Note Ultimately, shared dimensions _cannot_ be shadowed, so we will need to
     * make sure of this when we handle shared dimensions.
     */
    const DimensionElement* getDimensionInFullScope(const string& name) const;

    /** Add the given element to this scope.
     * We maintain a strong reference, so the caller should
     * respect the RCObject count and not delete it on us!
     */
    void addDimension(DimensionElement* dim);

    /** "Print" out the dimensions to a string */
    string printDimensions() const;

    /** Clear the dimension table, releasing all strong references */
    void clearDimensions();

    /** Get the list of dimension elements local to only this dataset, not its enclosing scope.  */
    const std::vector<DimensionElement*>& getDimensionElements() const;

    /** Set our aggregation to the given agg.
     *
     * If there exists an aggregation already and !throwIfExists, agg will replace it,
     * which might cause the previous one to be deleted.
     *
     * If there exists one already and agg != NULL and throwIfExists, an exception will be thrown.
     *
     * If agg == NULL, it always removes the strong reference to the previous, regardless of throwIfExists.
     */
    void setChildAggregation(AggregationElement* agg, bool throwIfExists = true);

    /** Return the raw pointer (or NULL) to our contained aggregation.
     * Only guaranteed valid for the life of this object.  */
    AggregationElement* getChildAggregation() const;

    /** Return the next enclosing dataset for this, or NULL if we're the root.
     * Basically traverse upwards through any aggregation parent to get containing datset.
     */
    NetcdfElement* getParentDataset() const;

    /** @return the AggregationElement that contains me, or NULL if we're root. */
    AggregationElement* getParentAggregation() const;

    /** Set my parent AggregationElement to parent. This is a weak reference.  */
    void setParentAggregation(AggregationElement* parent);

#if 0 // not sure we need this yet
    /**
     * Assuming whitespace separated tokens, parse the tokens in _coordValue attribute
     * into the vector of values of the given type T.
     * If _coordValue.empty(), then values.size() == 0.
     * @param values the vector to parse the coordValue attribute into.
     * @return the number of values added (ie values.size() )
     * @exception will throw a parse error if the values cannot be parsed as the
     *  given type T.
     */
    template <typename T> int getCoordValueVector(vector<T>& values) const;
#endif

    /**
     * Parse the netcdf@coordValue attribute as a double.
     * If successful, put the value in val and return true.
     * If unsuccessful, val is unchanged and false is returned.
     * ASSUMES: there is only ONE token in the coordValue field.
     * TODO: look into loading multiple values later as needed.
     * @param val  output the parsed value here if possible
     * @return whether the parse was successful
     */
    bool getCoordValueAsDouble(double& val) const;

    /** Add the pNewvar created by pVE to this dataset's list of
     * variables to validate for having values set upon closing
     * (handleEnd() of this element).  All new variables are
     * required to have values at the time the dataset is closed
     * or a parse error is thrown at that time.
     *
     * @param pNewVar  the new variable to watch, used as a key to find pVE later
     * @param pVE the variable element that created the pNewVar, which contains whether
     *            the value of pNewVar has been set yet (since there's no direct way
     *            in BaseType to do that now).  pVE->ref() will be called to keep pVE
     *            around sicne it will go out of scope after this call.
     *            On the ~NetcdfElement, pVE->unref() will be called to undo this.
     */
    void addVariableToValidateOnClose(libdap::BaseType* pNewVar, VariableElement* pVE);

    /**
     * Lookup the VariableElement* associated with pVarToValidate via a previous
     * addVariableToValidateOnClose() and call pVE->setGotValues() on the
     * associated element so that it will be considered valid at handleEnd()
     * of this element.  Should be called when the values are set on pVarToValidate.
     * @param pVarToValidate  the variable which has had its deferred values set
     *                        to be used as a key to lookup the associated VariableElement
     *                        that created it.
     * @param removeEntry  if the entry is found, remove it from the list as well as setting gotvalues.
     */
    void setVariableGotValues(libdap::BaseType* pVarToValidate, bool removeEntry);

    /** If a VariableElement has been associated with a new var to validate,
     * return it.  If not, return null.
     * @param pNewVar  the libdap variable (key) to look up
     * @return the associated VariableElement for pNewVar, else null if not set with
     *        addVariableToValidate.
     */
    VariableElement* findVariableElementForLibdapVar(libdap::BaseType* pNewVar);

    /**
     * Compare the location fields of the two arguments and return true
     * if lhs.location() < rhs.location() in a lexicographic string sense.
     * Used for std::sort on vector<NetcdfElement*>
     * @see isCoordValueLexicographicallyLessThan()
     * @param pLHS the lefthandside of the less than must not be null!
     * @param pRHS the righthandside of the less than must not be null!
     * @return if pLHS->location() < pRHS->location() lexicographically.
     */
    static bool isLocationLexicographicallyLessThan(const NetcdfElement* pLHS, const NetcdfElement* pRHS);

    /**
     * Compare the coordvalue fields of the two arguments and return true
     * if lhs.coordValue() < rhs.coordValue() in a lexicographic string sense.
     * Used for std::sort on vector<NetcdfElement*>
     * @see isLocationLexicographicallyLessThan()
     * @param pLHS the lefthandside of the less than  Must not be null!
     * @param pRHS the righthandside of the less than  Must not be null!
     * @return if pLHS->coordValue() < pRHS->coordValue() lexicographically.
     */
    static bool isCoordValueLexicographicallyLessThan(const NetcdfElement* pLHS, const NetcdfElement* pRHS);

private:

    /** Ask the parser to load our location into our response object. */
    void loadLocation();

    /** Check the value of the attribute fields and if any are
     * !empty() that we don't support, throw a parse error to tell the author.
     */
    void throwOnUnsupportedAttributes();

    /**
     * Does a more context sensitive check of attributes,
     * like making sure ncoords is only specified on a child of
     * a joinExisting aggregation element.
     */
    bool validateAttributeContextOrThrow() const;

    static vector<string> getValidAttributes();

private:
    string _location;
    string _id;
    string _title;
    string _ncoords;
    string _enhance;
    string _addRecords;
    string _coordValue;
    string _fmrcDefinition;

    // Whether we got a metadata direction element { readMetadata | explicit } for this node yet.
    // Just used to check for more than one.
    bool _gotMetadataDirective;

    // Whether we own the memory for _response and need to destroy it in dtor or not.
    bool _weOwnResponse;

    // true after loadLocation has been called.
    bool _loaded;

    // Our response object
    // We OWN it if we're not the root element,
    // but the parser owns it if we are the root.
    // The parser sets _weOwnResponse correctly for our dtor.
    BESDapResponse* _response;

    // If non-null, a pointer to the singleton aggregation element for this dataset.
    // We use an RCPtr to automatically ref() it when we set it
    // and to unref() it in our dtor.
    RCPtr<AggregationElement> _aggregation;

    // If we are nested within an aggregation element,
    // this is a back ptr we can use to traverse upwards.
    // If it null if we're a root dataset.
    AggregationElement* _parentAgg;

    // A table of strong references to Dimensions valid for this element's lexical scope.
    // We use raw DimensionElement*, but ref() them upon adding them to the vector
    // and unref() them in the dtor.
    // We won't have that many, so a vector is more efficient than a map for this.
    std::vector<DimensionElement*> _dimensions;

    // Lazy evaluated, starts unassigned.
    // When getAggMemberDataset() is called, it is lazy-constructed
    // to weak ref the returned strong shared ptr (RCPtr).
    agg_util::WeakRCPtr<agg_util::AggMemberDataset> _pDatasetWrapper;

public:
    // inner classes can't be private?
    /**
     * Inner class for keeping track of new variables created within
     * the context of this dataset for which we do not get <values>
     * set up front.  This should really only happen in the case of
     * a "placeholder" array variable that acts as a coordinate variable
     * for a joinNew aggregation where the author desires setting the
     * metadata for the new dimension's map vector.
     *
     * Callers will access this via methods in NetcdfElement.
     *
     * If a new VariableElement is created and refers to a new
     * variable but which has not had its value set by the handleEnd()
     * of the VariableElement, it will be placed into an instance of this class
     * (a member variable of this).  The ref count of the element
     * will be increased on addition and will be decremented
     * when the VVV is destroyed (along
     * with the containing NetcdfElement).
     *
     * The AggregationElement processParentDatasetComplete() should
     * inform this NetcdfElement once it sets the values on an entry
     * in the VVV and the VVV instance will be updated to reflect
     * that entry as having been validated.
     *
     * VVV.validate() will be called in NetcdfElement::handleEnd()
     * AFTER the aggregations have had their processParentDatasetComplete()
     * called. If any entries in the VVV have NOT had their values set
     * by this point, the validate will throw a parse error explaining
     * the issue to maintain integrity of the libdap variables
     * (and avoid cryptic internal errors much later down the line).
     */
    class VariableValueValidator {
    public:
        VariableValueValidator(NetcdfElement* pParent);

        /**
         * Will decrement the ref count of all contained
         * VariableElement's
         * @return
         */
        ~VariableValueValidator();

        /** Add a validation entry for the given VariableElement and the actual variable that
         * it has created and added to the DDS.  pVE->ref() will be called to make sure
         * the element stays around after the parser has deref() it.
         * @param pNewVar the actual libdap variable that was created and is
         * currently in the DDS of this dataset (in _response).  Should not be null.
         * @param pVE  the VariableElement that created it.  pVE->checkGotValues() will
         * determine whether the entry has been validated.  pVE->ref() will be called to
         * up the ref count.  Should not be null.
         */
        void addVariableToValidate(libdap::BaseType* pNewVar, VariableElement* pVE);

        /** Remove an entry previously added under the key pVarToRemove with
         * addVariableToValidate.  Will unref() the VariableElement portion.
         * @param pVarToRemove
         */
        void removeVariableToValidate(libdap::BaseType* pVarToRemove);

        /** Lookup the VariableElement pVE associated with the given pVarToValidate
         * and call pVE->setGotValues() on it to validate it.
         * @param pVarToValidate a non-null variable that was entered with addVariableToValidate
         * @exception An internal error is thrown if pVarToValidate was not already added.
         */
        void setVariableGotValues(libdap::BaseType* pVarToValidate);

        /** Make sure all the entries has had their values set else throw a
         * parse error explaining which variable has not so the author
         * can fix the error.  On success return true.
         * @return whether all contained variables have values.
         */
        bool validate();

        /** If a VariableElement has been associated with a new var to validate,
         * return it.  If not, return null.
         * @param pNewVar  the libdap variable (key) to look up
         * @return the associated VariableElement for pNewVar, else null if not set with
         *        addVariableToValidate.
         */
        VariableElement* findVariableElementForLibdapVar(libdap::BaseType* pNewVar);

    private:
        // disallow implicit copies
        VariableValueValidator(const VariableValueValidator&);
        VariableValueValidator& operator=(const VariableValueValidator&);

    public:

        class VVVEntry {
        public:
            VVVEntry() :
                _pNewVar(0), _pVarElt(0)
            {
            }
            VVVEntry(libdap::BaseType* pBT, VariableElement* pVE) :
                _pNewVar(pBT), _pVarElt(pVE)
            {
            }
            void clear()
            {
                _pNewVar = 0;
                _pVarElt = 0;
            }
            libdap::BaseType* _pNewVar;
            VariableElement* _pVarElt;
        };

        /********************* helper functions *******************/

        /** Lookup the entry given the BaseType* it was added with and return
         * its address within _entries, else NULL if not found.
         * @param pVarToFind  the variable to lookup the entry for
         * @return the associated entry else NULL if not found.
         */
    private:
        VariableValueValidator::VVVEntry* findEntryByLibdapVar(libdap::BaseType* pVarToFind);

        // We don't expect too many entries, so a simple vector is the best way to go,
        // avoid overhead of maps, etc.
        vector<VVVEntry> _entries;
        NetcdfElement* _pParent;
    }; // class VariableValueValidator

private:
    // The actual instance we will poke from methods in NetcdfElement
    VariableValueValidator _variableValidator;
};

}

#endif /* __NCML_MODULE__NETCDF_ELEMENT_H__ */
