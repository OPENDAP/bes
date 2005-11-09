// DODSAutoPtr.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

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
