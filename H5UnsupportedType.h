/*
 * H5UnsupportedType.h
 *
 *  Created on: Jan 31, 2012
 *      Author: jimg
 */

#ifndef H5UNSUPPORTEDTYPE_H_
#define H5UNSUPPORTEDTYPE_H_

#include <Error.h>

class H5UnsupportedType : public Error {
public:
    H5UnsupportedType() {}
    H5UnsupportedType(string msg) : Error(msg) {}
    virtual ~H5UnsupportedType() {}
};

#endif /* H5UNSUPPORTEDTYPE_H_ */
