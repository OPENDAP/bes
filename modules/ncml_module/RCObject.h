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
#ifndef __AGG_UTIL__REF_COUNTED_OBJECT_H__
#define __AGG_UTIL__REF_COUNTED_OBJECT_H__

#include "RCObjectInterface.h" // interface super

#include <list>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace agg_util {
class RCOBjectPool;
class RCObject;

typedef std::set<RCObject*> RCObjectSet;

/**
 * A monitoring "pool" class for created RCObject's which allows us to
 * forcibly delete any RCOBject's regardless of their ref counts
 * when we know we are done with them, say after an exception.
 *
 * @TODO For now, this won't be a real pool, where objects are pre-allocated
 * ahead of time and reused, etc.  Support for this sort of thing will
 * require a template class containing a vector of reusable objects
 * for each type we need to factory up.  At that point this will serve
 * as a base class for the concrete pool.
 */
class RCObjectPool {
    friend class RCObject;

public:
    /** Create an empty pool */
    RCObjectPool();

    /** Forcibly delete all remaining objects in pool, regardless of ref count */
    virtual ~RCObjectPool();

    /** @return whether the pool is currently monitoring the object
     * or not.
     */
    bool contains(RCObject* pObj) const;

    /** Add the object to the pool uniquely.
     * When the pool is destroyed, pObj will be destroyed
     * as well, regardless of its count, if it is still live.
     */
    void add(RCObject* pObj);

    /** Remove the object from the pool, but don't delete it
     * @param pObj object to remove from the auto delete pool. */
    void remove(RCObject* pObj)
    {
        release(pObj, false);
    }

    /**
     * Tell the pool that the object's count is 0 and it can be released to be
     * deleted or potentially reused again later.  Users should not call this.
     * @param pObj the object to remove from the pool.
     * @param shouldDelete whether this call should call delete on pObj, or just remove it.
     */
    void release(RCObject* pObj, bool shouldDelete = true);

protected:
    /** Call delete on all objects remaining in _liveObjects and clear it out.
     * After call, _liveObjects.empty().
     */
    void deleteAllObjects();

private:

    // A set of the live, monitored objects.
    // Lookups are log(N) and entries unique, as required.
    // All entries in this list will be delete'd in the dtor.
    RCObjectSet _liveObjects;
};
// class RCObjectPool

/**
 * Interface for registering callbacks to the RCObject for
 * when the usecount hits 0 but before the deallocate
 * functionality is performed.
 */
class UseCountHitZeroCB {
public:
    UseCountHitZeroCB()
    {
    }
    virtual ~UseCountHitZeroCB()
    {
    }
    virtual void executeUseCountHitZeroCB(RCObject* pAboutToDie) = 0;
};

/**
 * @brief A base class for a simple reference counted object.
 *
 * Use as a base class for objects that need to delete themselves
 * when their reference count goes to 0.
 *
 * When a strong reference to the object is required, the
 * caller uses ref().  When the reference needs to be released,
 * unref() is called.  p->unref() should be considered potentially
 * identical to delete p; since it can cause the object to be deleted.
 * The pointer should NOT be used after an unref() unless it was known
 * to be preceded by a ref(), or unless the count is checked prior to unref()
 * and found to be > 1.
 *
 * A new RCObject has a count of 0, and will only be destroyed automatically
 * if the count goes from 1 back to 0, so the caller is in charge of it unless the first
 * ref() call.  Be careful storing these in std::auto_ptr!  Instead, use a
 * RCPtr(new RCObject()) in place of auto_ptr for hanging onto
 * an RCOBject* in a local variable before possible early exit.
 *
 * @see RCPtr which can be used as a temporary
 * reference similar to std::auto_ptr<T>, but which uses the
 * reference counting system to safely handle a RCObject
 * as a temporary in a location where an exception might cause it to be
 * leaked.  This is especially useful when the object is removed from
 * a refcounted container but safely needs to be used locally before destruction.
 *
 * @note This class influenced by Scott Meyers and Open Inventor ref counting stuff.
 *
 * @note I'd much rather use boost::shared_ptr and boost::weak_ptr
 * for this stuff since they can be used in STL containers and
 * are thread-safe, but adding a boost dependency for just shared_ptr seems like overkill now.
 * NOTE: shared_ptr is now in C++ TR1, implemented by gcc 4.x.  Can we assume it?
 *
 * @TODO Consider adding a pointer to an abstract MemoryPool or what have you
 * so that a Factory can implement the interface and these objects can be stored
 * in a list as well as returned from factory.  That way the factory can forcibly
 * clear all dangling references from the pool in its dtor in the face of exception
 * unwind or programmer ref counting error.
 */
class RCObject: public virtual RCObjectInterface // abstract interface
{
    friend class RCObjectPool;

    typedef std::list<UseCountHitZeroCB*> PreDeleteCBList;

private:
    RCObject& operator=(const RCObject& rhs); //disallow

public:

    /** If the pool is given, the object will be released back to the pool when its count hits 0,
     * otherwise it will be deleted.
     */
    RCObject(RCObjectPool* pool = 0);

    /** Copy ctor: Starts count at 0 and adds us to the proto's pool
     * if it exists.
     */
    RCObject(const RCObject& proto);

    virtual ~RCObject();

    /** Increase the reference count by one.
     * const since we do not consider the ref count part of the semantic constness of the rep */
    virtual int ref() const;

    /** Decrease the reference count by one.  If it goes from 1 to 0,
     * delete this and this is no longer valid.
     * @return the new ref count.  If it is 0, the caller knows the
     * object was deleted.
     *
     * It is illegal to unref() an object with a count of 0.  We don't
     * throw to allow use in dtors, so the caller is assumed not to do it!
     *
     * const since the reference count is not part of the semantic constness of the rep
     */
    virtual int unref() const;

    /** Get the current reference count */
    virtual int getRefCount() const;

    /** If the object is in an auto-delete pool,
     * remove it from the pool and force it to only delete
     * when it's ref count goes to 0.
     * Useful when we desire a particular object stay around
     * outside of the pool's lifetime.
     */
    virtual void removeFromPool() const;

    /** Just prints the count and address  */
    virtual std::string toString() const;

public:
    // to workaround template friend issues.  Not designed for public consumption.

    /** Add uniquely.  If it is added agan, the second time is ignored.  */
    void addPreDeleteCB(UseCountHitZeroCB* pCB);

    /** Remove it exists.  If not, this unchanged.  */
    void removePreDeleteCB(UseCountHitZeroCB* pCB);

private:
    // interface

    /** Same as toString(), just not virtual so we can always use it. */
    std::string printRCObject() const;

    /**
     * Go through the list of preDeleteCallbacks registered
     * with this object.  For each one, remove it from the
     * callback list, then call it.
     * On exit, the pre callback list is empty.
     * Note: callbacks may remove themselves as well,
     * though this function will forcibly remove them
     * before they get a chance to maintain valid iterator
     * state.
     */
    void executeAndClearPreDeleteCallbacks();

private:
    // data rep

    // The reference count... mutable since we want to ref count const objects as well,
    // and the count doesn't affect the semantic constness of the subclasses.
    mutable int _count;

    // If not null, the object is from the given pool and should be release()'d to the
    // pool when count hits 0, not deleted.  If null, it can be deleted.
    RCObjectPool* _pool;

    // Callback list for when the use count hits 0 but before deallocate
    PreDeleteCBList _preDeleteCallbacks;

};
// class RCObject

/** @brief A reference to an RCObject which automatically ref() and deref() on creation and destruction.
 *
 * Use this for temporary references to an RCObject* instead of std::auto_ptr to avoid leaks or double deletion.
 * It is templated to allow RCObject subclass specific pointers.
 *
 * For example,
 *
 * RCPtr<RCObject> obj = RCPtr<RCObject>(new RCObject());
 * // count is now 1.
 * // make a call to add to container that might throw exception.
 * // we assume the container will up the ref() itself on a successful addition.
 * addToContainer(obj.get());
 * // if previous line exceptions, ~RCPtr will unref() it back to 0, causing it to delete.
 * // if we get here, the object is safely in the container and has been ref() to 2,
 * // so ~RCPtr correctly drops it back to 1.
 *
 * @note We don't have a class for weak references,
 * so make sure to not generate reference loops with back pointers (circular ref graphs)
 * Back pointers should be raw pointers (until/unless we add weak references)
 * and ref() should _only_ be called when it's a strong reference!
 * (ie one that shares ownership).
 */
template<class T>
class RCPtr {
public:
    RCPtr(T* pRef = 0) :
        _obj(pRef)
    {
        init();
    }

    RCPtr(const RCPtr& from) :
        _obj(from._obj)
    {
        init();
    }

    ~RCPtr()
    {
        if (_obj) {
            _obj->unref();
            _obj = 0;
        }
    }

    RCPtr&
    operator=(const RCPtr& rhs)
    {
        if (rhs._obj != _obj) {
            RCObject* oldObj = _obj;
            _obj = rhs._obj;
            init();
            if (oldObj) {
                oldObj->unref();
            }
        }
        return *this;
    }

    T&
    operator*() const
    {
        // caller is on their own if they deref this as null,
        // so should check with get() first.
        return *_obj;
    }

    T*
    operator->() const
    {
        return _obj;
    }

    T*
    get() const
    {
        return _obj;
    }

    /**
     * If not null, ref() the object and then return it.
     *
     * Useful for adding a reference to a
     * container, e.g.:
     *
     * RCPtr<T> myObj;
     * vector<T> myVecOfObj;
     * myVecOfObj.push_back(myObj.refAndGet());
     *
     * @return
     */
    T*
    refAndGet() const
    {
        if (_obj) {
            _obj->ref();
        }
        return _obj;
    }

private:
    void init()
    {
        if (_obj) {
            _obj->ref();
        }
    }

private:
    T* _obj;
};
// class RCPtr<T>

/** Exception class for all errors from WeakRCPtr<T> */
class BadWeakPtr: public std::runtime_error {
public:
    BadWeakPtr(const std::string& msg) :
        std::runtime_error(msg)
    {
    }

    virtual ~BadWeakPtr() throw ()
    {
    }
};
// class         BadWeakPtr

/**
 *  A variant of boost::weak_ptr that uses our intrusive RCObject counting.
 *
 *  WeakRCPtr<T> is used to refer weakly to a class T
 *  where T inherits from RCObject.
 *
 * NOTE: These are NOT thread-safe!!
 *
 *  By weak we mean that RCWeakPtr<T> does not
 *  change the use count of the wrapped object.
 *  Also it may transition from containing a valid ptr to containing NULL
 *  if the RCObject it wraps is deleted by its use count going to 0.
 *  Therefore, similarly to boost::weak_ptr,
 *  a lock() function is provided to return a new
 *  RCPtr<T> for the object which ups its ref count and therefore
 *  maintains the life of the object for the duration of the use of
 *  the ptr returned from lock().
 *
 *  get() is provided as access to the raw ptr BUT NOTE THAT IT IS NOT SAFE.
 *  The memory may go away during the use of the returned ptr, so lock() is
 *  the preferred method to get the resource.  get() is useful for checking
 *  for null of the wrapped ptr.
 * */
template<class T>
class WeakRCPtr: public UseCountHitZeroCB // can we private inherit this to avoid outside callers?
{

public:

    /** Default contains NULL */
    WeakRCPtr() :
        _pObj(0)
    {
    }

    explicit WeakRCPtr(RCPtr<T> src)
    {
        // Connect to the shared ptr by adding listener and storing raw.
        _pObj = src.get();
        addMeAsListener();
    }

    ~WeakRCPtr()
    {
        clear();
    }

    WeakRCPtr& operator=(const WeakRCPtr& r)
    {
        if (&r != this) {
            clear();
            _pObj = r._pObj;
            addMeAsListener();
        }
        return *this;
    }

    /** Will getting a lock() return a null? */
    bool expired() const
    {
        return empty();
    }

    /** Will getting a lock() return a null? */
    bool empty() const
    {
        return (!_pObj);
    }

    RCPtr<T> lock() const
    {
        // return a safe shared ptr to the wrapped resource
        // which will up it's count properly
        if (_pObj) {
            return RCPtr<T>(_pObj);
        }
        else {
            return RCPtr<T>(NULL);
        }
    }

    /** Remove any listener and NULL the wrapped pointer.
     * On Exit: expired()
     */
    void clear()
    {
        // Remove the listener, clear the ptr so it's like default ctor.
        removeMeAsListener();
        _pObj = NULL;
    }

public:
    // to avoid template friend issues, but not for public use!

    /**
     *  Listener callback from the RCObject to implement the interface.
     *  This is called when the wrapped RCObject ref count goes to 0
     *  so we can safely disconnect.
     */
    virtual void executeUseCountHitZeroCB(RCObject* pAboutToDie)
    {
        if (pAboutToDie != _pObj) {
            throw BadWeakPtr("executeUseCountHitZeroCB() called with mismatched raw pointers!");
        }
        clear();
    }

    ////////// Private Impl
private:

    void removeMeAsListener()
    {
        if (_pObj) {
            _pObj->removePreDeleteCB(this);
        }
    }

    void addMeAsListener()
    {
        if (_pObj) {
            _pObj->addPreDeleteCB(this);
        }
    }

private:
    // data rep

    // The underlying wrapped object, or NULL if uninit or expired.
    // NOTE: MUST be a subclass of RCObject
    T* _pObj;

};
// class WeakRCPtr<T>

}// namespace agg_util

#endif /* __AGG_UTIL__REF_COUNTED_OBJECT_H__ */
