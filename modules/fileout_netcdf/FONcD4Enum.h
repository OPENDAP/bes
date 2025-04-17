// FONcD4Enum.h

// This file is part of BES Netcdf File Out Module


#ifndef FONcD4Enum_h_
#define FONcD4Enum_h_ 

#include <libdap/D4Enum.h>
#include <libdap/D4EnumDefs.h>
#include <libdap/D4Group.h>
#include "FONcBaseType.h"

namespace libdap {
    class BaseType;
    class D4Enum;
}


/** @brief A DAP4 Enum with file out netcdf information included
 *
 * This class represents a DAP4 Enum with additional information
 * needed to write it out to a netcdf file. Includes a reference to the
 * actual DAP4 Enum being converted
 */
class FONcD4Enum : public FONcBaseType
{
private:
    libdap::D4Enum *			_f = nullptr;
    nc_type                             basetype = NC_NAT;
    int                                 nc_enum_type_id = NC_EBADTYPID;
public:
    FONcD4Enum( libdap::BaseType *b, nc_type d4_enum_basetype, int nc_type_id ) ;
    ~FONcD4Enum() override = default;

    void define(int ncid) override;
    void write( int ncid ) override;

    string name() override;
    
    void dump( ostream &strm ) const override;

} ;

#endif // FONcD4Enum_h_

