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
#ifndef __NCML_MODULE_DDSLOADER_H__
#define __NCML_MODULE_DDSLOADER_H__

#include <memory>
#include <string>

class BESDataHandlerInterface;
class BESContainer;
class BESContainerStorage;
class BESDapResponse;
class BESResponseObject;

/**
 @brief Helper class for temporarily hijacking an existing dhi to load a DDX response
 for one particular file.

 The dhi will hijacked at the start of the load() call and restored before the
 DDX is returned.

 For exception safety, if this object's dtor is called while the dhi is hijacked
 or the response hasn't been relinquished, it will restore the dhi and clean memory, so the
 caller can guarantee resources will be in their original (ctor time) state if this object
 is destroyed via an exception stack unwind, etc.

 TODO see if there's another way to load these files without hijacking an existing dhi or
 if we want to refer to remote BES's rather than the local.  If we do this,
 this class will become an interface class with the various concrete subclasses for
 doing local vs. remote loads, etc.

 @author mjohnson <m.johnson@opendap.org>
 */
namespace agg_util {

class DDSLoader {
private:

    // The dhi to use for the loading, passed in on creation.
    // Rep Invariant: the dhi state is the same on call exits as it was on call entry.
    BESDataHandlerInterface& _dhi;

    // whether we have actually hijacked the dhi, so restore knows.
    bool _hijacked;

    // The file we are laoding if we're hijacked
    std::string _filename;

    // Remember the store so we can pull the location out in restoreDHI on exception as well.
    BESContainerStorage* _store;

    // DHI state we hijack, for putting back on exception.
    std::string _containerSymbol;
    std::string _origAction;
    std::string _origActionName;
    BESContainer* _origContainer;
    BESResponseObject* _origResponse;

    // A counter we use to generate a "class-unique" symbol for containers internally.
    // Incremented by getNextContainerName().
    static long _gensymID;

public:

    /** For telling the loader what type of BESDapResponse to load and return.
     * It can handle a DDX load or a DataDDS load.  The returned BesDapResponse will
     * be of the proper subclass. */
    enum ResponseType {
        eRT_RequestDDX = 0, eRT_RequestDataDDS, eRT_Num
    };

    /**
     * @brief Create a loader that will hijack dhi on a load call, then restore it's state.
     *
     * @param dhi DHI to hijack during load, needs to be a valid object for the scope of this object's life.
     */
    DDSLoader(BESDataHandlerInterface &dhi);

    DDSLoader(const DDSLoader& proto);
    DDSLoader& operator=(const DDSLoader&);

    /**
     * @brief Dtor restores the state of dhi
     * Restores the state of the dhi to what it was when object if it is still hijacked (i.e. exception on load())
     * Destroys the BESDDSResponse if non-null, which is also the case on failed load() call.
     */
    virtual ~DDSLoader();

    /**
     * Return a reference to the dhi we are using.  A little dangerous
     * to bare this rep, so be careful in its usage!
     * @return
     */
    BESDataHandlerInterface& getDHI() const
    {
        return _dhi;
    }
#if 0
    /**
     * @brief Load and return a new DDX or DataDDS structure for the local dataset referred to by location.
     *
     * Ownership of the response object passes to the caller via auto_ptr.
     *
     * On exception, the dhi will be restored when this is destructed, or the user
     * call directly call cleanup() to ensure this if they catch the exception and need the
     * dhi restored immediately.
     *
     * @param location the filename of the local file
     * @param type whether to load DDX or DataDDS.
     *
     * @return a response object containing the new loaded response object.
     *         It will be either a BESDDSResponse or BESDataDDSResponse depending on \c type.
     *
     * @exception if the underlying location cannot be loaded.
     */
    std::auto_ptr<BESDapResponse> load(const std::string& location, ResponseType type);
#endif
    /** @brief Load a DDX or DataDDS response into the given pResponse object, which must be non-null.
     *
     *  Similar to load(), but the caller passes in the response object to fill rather than wanting a new one.
     *
     *  If type == eRT_RequestDDX, pResponse MUST have type BESDDSReponse.
     *  If type == eRT_RequestDataDDS, pResponse MUST have type BESDataDDSResponse.
     *
     *  The location is loaded in pResponse based on the type of response requested.
     *
     *  @param location the file to load
     *  @param type the response type requested, must match type of pResponse
     *  @param pResponse the response object to fill in, which must match the request type.
     *
     *  @see load()
     *
     * @exception If there is a problem loading the location.
     */
    void loadInto(const std::string& location, ResponseType type, BESDapResponse* pResponse);

    /**
     * @brief restore dhi to clean state
     *
     * Ensures that the resources and dhi are all in the state they were at construction time.
     * Probably not needed by users, but in case they want to catch an exception
     * and then retry or something
     */
    void cleanup();

    //////////////////////// Public Class Methods ////////////////////////////////////////////////////////////////

    /** Make a new response object for the requested type. */
    static std::auto_ptr<BESDapResponse> makeResponseForType(ResponseType type);

    /** Convert the type into the action in BESResponseNames.h for the type.
     *  @param type the response type
     *  @return either DDX_RESPONSE or DATA_RESPONSE
     */
    static std::string getActionForType(ResponseType type);

    /** Convert the type in the action name in BESResponseNames.h
     * @param type the response type
     * @return either DDX_RESPONSE_STR or DATA_RESPONSE_STR
     */
    static std::string getActionNameForType(ResponseType type);

    /** Return whether the given response's type matches the given ResposneType.
     *  If type==eRT_RequestDDX, pResponse must be BESDDSResponse
     *  If type==eRT_RequeastDataDDS, pResponse must be BESDataDDSResponse
     */
    static bool checkResponseIsValidType(ResponseType type, BESDapResponse* pResponse);

private:

    /**
     *  Remember the current state of the _dhi we plan to change.
     *  @see restoreDHI()
     */
    void snapshotDHI();

    /**
     * if (_hijacked), put back all the _dhi state we hijacked. Inverse of snapshotDHI().
     * At end of call, (_hijacked == false)
     */
    void restoreDHI();

    /**
     * Add a new symbol to the BESContainerStorageList singleton.
     */
    BESContainer* addNewContainerToStorage();

    /** Remove the symbol we added in addNewContainerToStorage if it's there.
     * Used in dtor, can't throw */
    void removeContainerFromStorage();

    /** Make sure we clean up anything we've touched.
     * On exit, everything should be in the same state as construction.
     */
    void ensureClean();

    /** Increment _gensymID and use it to generate a container name
     * string unique to the class and return it.
     */
    static std::string getNextContainerName();

};
// class DDSLoader
}// namespace ncml_module

#endif /* __NCML_MODULE_DDSLOADER_H__ */
