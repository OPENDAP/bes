// BESAutoPtr.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef BESAutoPtr_h_
#define BESAutoPtr_h_ 1

template <class T>
class BESAutoPtr
{
private:
    T* p;
    bool _is_vector;

    // disable copy constructor.
    template <class U> BESAutoPtr(BESAutoPtr<U> &){};

    // disable overloaded = operator.
    template <class U> BESAutoPtr<T>& operator= (BESAutoPtr<U> &){ return *this ; }

public:
    explicit BESAutoPtr(T* pointed=0, bool v=false) ;
    ~BESAutoPtr() ;

    T* set(T *pointed, bool v=false) ;
    T* get() const ;
    T* operator ->() const ;
    T& operator *() const ;
    T* release() ;
    void reset() ;
};

#endif // BESAutoPtr_h_

