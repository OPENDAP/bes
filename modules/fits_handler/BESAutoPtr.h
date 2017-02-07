// BESAutoPtr.h

// This file is part of fits_handler, a data handler for the OPeNDAP data
// server. 

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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef BESAutoPtr_h_
#define BESAutoPtr_h_ 1

template<class T>
class BESAutoPtr {
private:
	T* p;
	bool _is_vector;

	// disable copy constructor.
	template<class U> BESAutoPtr(BESAutoPtr<U> &) { }


	// disable overloaded = operator.
	template<class U> BESAutoPtr<T>& operator=(BESAutoPtr<U> &)
	{
		return *this;
	}

public:
	explicit BESAutoPtr(T* pointed = 0, bool v = false)
	{
		p = pointed;
		_is_vector = v;
	}

	~BESAutoPtr()
	{
		if (_is_vector)
			delete[] p;
		else
			delete p;
		p = 0;
	}

	T* set(T *pointed, bool v = false)
	{
		T* temp = p;
		p = pointed;
		_is_vector = v;
		return temp;
	}

	T* get() const
	{
		return p;
	}

	T* operator ->() const
	{
		return p;
	}

	T& operator *() const
	{
		return *p;
	}

	T* release()
	{
		T* old = p;
		p = 0;
		return old;
	}

	void reset()
	{
		if (_is_vector)
			delete[] p;
		else
			delete p;
		p = 0;
	}
};

#endif // BESAutoPtr_h_
