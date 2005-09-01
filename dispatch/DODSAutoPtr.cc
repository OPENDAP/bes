// DODSAutoPtr.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSAutoPtr.h"

template <class T>
DODSAutoPtr<T>::DODSAutoPtr( T* pointed, bool v )
{
    p = pointed;
    _is_vector = v;
}

template <class T>
DODSAutoPtr<T>::~DODSAutoPtr()
{
    if( _is_vector ) 
	delete [] p; 
    else 
	delete p;
    p = 0;
}

template <class T>
T*
DODSAutoPtr<T>::set( T *pointed, bool v )
{
    T* temp = p;
    p = pointed;
    _is_vector = v;
    return temp;
}

template <class T>
T*
DODSAutoPtr<T>::get() const
{
    return p;
}

template <class T>
T*
DODSAutoPtr<T>::operator ->() const
{
    return p;
}

template <class T>
T&
DODSAutoPtr<T>::operator *() const
{
    return *p;
}

template <class T>
T*
DODSAutoPtr<T>::release()
{
    T* old = p;
    p = 0;
    return old;
}

template <class T>
void
DODSAutoPtr<T>::reset()
{
    if( _is_vector ) 
	delete [] p; 
    else 
	delete p;
    p = 0;
}

// $Log: DODSAutoPtr.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
