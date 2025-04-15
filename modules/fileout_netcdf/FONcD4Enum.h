// FONcD4Enum.h

// This file is part of BES Netcdf File Out Module


#ifndef FONcD4Enum_h_
#define FONcD4Enum_h_ 1

#include <libdap/D4Enum.h>
#include <libdap/D4EnumDefs.h>
#include <libdap/D4Group.h>

namespace libdap {
    class BaseType;
    class D4Enum;
}

#include "FONcBaseType.h"

/** @brief A DAP4 Enum with file out netcdf information included
 *
 * This class represents a DAP4 Enum with additional information
 * needed to write it out to a netcdf file. Includes a reference to the
 * actual DAP4 Enum being converted
 */
class FONcD4Enum : public FONcBaseType
{
private:
    libdap::D4Enum *			_f ;
    nc_type                             basetype = NC_NAT;
    int                                 nc_enum_type_id = 0;
public:
    				FONcD4Enum( libdap::BaseType *b, nc_type d4_enum_basetype, int nc_type_id ) ;
    virtual			~FONcD4Enum() ;

    virtual void		define( int ncid ) ;
    virtual void		write( int ncid ) ;

    virtual string 		name() ;
    //virtual nc_type		type() ;
    nc_type                     obtain_basetype();
    

    virtual void                dump( ostream &strm ) const ;

} ;

#endif // FONcD4Enum_h_

