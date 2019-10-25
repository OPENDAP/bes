///////////////////////////////////////////////////////////////////////////////
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

#include "RCObject.h"

#include "BESDebug.h"
#include "NCMLDebug.h"
#include <algorithm> // std::find
#include <sstream>
#include <vector>

using std::endl;
using std::string;

namespace agg_util {

RCObject::RCObject(RCObjectPool* pool/*=0*/) :
    RCObjectInterface(), _count(0), _pool(pool), _preDeleteCallbacks()
{
    if (_pool) {
        _pool->add(this);
    }
}

RCObject::RCObject(const RCObject& proto) :
    RCObjectInterface(), _count(0) // new objects have no count, forget what the proto has!
        , _pool(proto._pool), _preDeleteCallbacks()
{
    if (_pool) {
        _pool->add(this);
    }
}

RCObject::~RCObject()
{
    // just to let us know its invalid
    _count = -1;

    NCML_ASSERT_MSG(_preDeleteCallbacks.empty(),
        "~RCObject() called with a non-empty listener list!");
}

int RCObject::ref() const
{
    ++_count;
    BESDEBUG("ncml:memory", "Ref count for " << printRCObject() << " is now: " << _count << endl);
    return _count;
}

int RCObject::unref() const
{
    int tmp = --_count; // need tmp since might delete and need _count valid at end
    if (tmp == 0) {
        // Semantic constness here as well..
        const_cast<RCObject*>(this)->executeAndClearPreDeleteCallbacks();
        if (_pool) {
            BESDEBUG("ncml:memory",
                "Releasing back to pool: Object ref count hit 0.  " << printRCObject() << " with toString() == " << toString() << endl);
            _pool->release(const_cast<RCObject*>(this));
        }
        else {
            BESDEBUG("ncml:memory",
                "Calling delete: Object ref count hit 0.  " << printRCObject() << " with toString() == " << toString() << endl);
            delete this;
        }
    }
    else {
        BESDEBUG("ncml:memory", "unref() called and: " << printRCObject() << endl);
    }
    return tmp;
}

int RCObject::getRefCount() const
{
    return _count;
}

void RCObject::removeFromPool() const
{
    if (_pool) {
        // remove will not delete it
        // and will clear _pool
        _pool->remove(const_cast<RCObject*>(this));
        NCML_ASSERT(!_pool);
    }
}

string RCObject::toString() const
{
    return printRCObject();
}

string RCObject::printRCObject() const
{
    std::ostringstream oss;
    oss << "RCObject@(" << reinterpret_cast<const void*>(this) << ") _count=" << _count << " numberDeleteListeners="
        << _preDeleteCallbacks.size();
    return oss.str();
}

void RCObject::addPreDeleteCB(UseCountHitZeroCB* pCB)
{
    if (pCB) {
        // unique add
        if (std::find(_preDeleteCallbacks.begin(), _preDeleteCallbacks.end(), pCB) == _preDeleteCallbacks.end()) {
            BESDEBUG("ncml:memory",
                "Adding WeakRCPtr listener: " << printRCObject() << " is getting listener: " << reinterpret_cast<const void*>(pCB) << endl);
            _preDeleteCallbacks.push_back(pCB);
            BESDEBUG("ncml:memory", "After listener add, obj is: " << printRCObject() << endl);
        }
    }
}

void RCObject::removePreDeleteCB(UseCountHitZeroCB* pCB)
{
    if (pCB) {
        BESDEBUG("ncml:memory",
            "Removing WeakRCPtr listener from: " << printRCObject() << " Removed listener: " << reinterpret_cast<const void*>(pCB) << endl);
        _preDeleteCallbacks.remove(pCB);
        BESDEBUG("ncml:mempory", "Object after remove listener is: " << printRCObject() << endl);
    }
}

void RCObject::executeAndClearPreDeleteCallbacks()
{
    // Since the callbacks might remove themselves
    // from the PreDeleteCBList, we can't use an
    // iterator.  Use the queue interface instead
    // and force the deletion of a node when it's used
    // to be sure the loop exits.
    while (!(_preDeleteCallbacks.empty())) {
        UseCountHitZeroCB* pCB = _preDeleteCallbacks.front();
        _preDeleteCallbacks.pop_front();
        if (pCB) {
            pCB->executeUseCountHitZeroCB(this);
        }
    }
    NCML_ASSERT(_preDeleteCallbacks.empty());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// RCObjectPool

RCObjectPool::RCObjectPool() :
    _liveObjects()
{
}

RCObjectPool::~RCObjectPool()
{
    deleteAllObjects();
}

bool RCObjectPool::contains(RCObject* pObj) const
{
    RCObjectSet::const_iterator foundIt = _liveObjects.find(pObj);
    return (foundIt != _liveObjects.end());
}

void RCObjectPool::add(RCObject* pObj)
{
    if (contains(pObj)) {
        throw string("Internal Pool Error: Object added twice!");
    }
    _liveObjects.insert(pObj);
    pObj->_pool = this;
}

void RCObjectPool::release(RCObject* pObj, bool shouldDelete/*=true*/)
{
    if (contains(pObj)) {
        _liveObjects.erase(pObj);
        pObj->_pool = 0;

        if (shouldDelete) {
            // Delete it for now...  If we decide to subclass and implement a real pool,
            // we'll want to push this onto a vector for reuse.
            BESDEBUG("ncml:memory",
                "RCObjectPool::release(): Calling delete on released object=" << pObj->printRCObject() << endl);
            delete pObj;
        }
        else {
            BESDEBUG("ncml:memory",
                "RCObjectPool::release(): Removing object, but not deleting it: " << pObj->printRCObject() << endl);
        }
    }
    else {
        BESDEBUG("ncml:memory", "ERROR: RCObjectPool::release() called on object not in pool!!  Ignoring!" << endl);
    }
}

void RCObjectPool::deleteAllObjects()
{
    BESDEBUG("ncml:memory", "RCObjectPool::deleteAllObjects() started...." << endl);
    RCObjectSet::iterator endIt = _liveObjects.end();
    RCObjectSet::iterator it = _liveObjects.begin();
    for (; it != endIt; ++it) {
        RCObject* pObj = *it;
        // Just in case, flush the predelete list to avoid dangling WeakRCPtr
        if (pObj) {
            pObj->executeAndClearPreDeleteCallbacks();
            BESDEBUG("ncml:memory", "Calling delete on RCObject=" << pObj->printRCObject() << endl);
            delete pObj;
        }
    }
    _liveObjects.clear();
    BESDEBUG("ncml:memory", "RCObjectPool::deleteAllObjects() complete!" << endl);
}

} // namespace agg_util
