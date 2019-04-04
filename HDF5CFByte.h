// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2016 The HDF Group, Inc. and OPeNDAP, Inc.
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

////////////////////////////////////////////////////////////////////////////////
/// \file HDF5CFByte.h
/// \brief This class provides a way to map HDF5 byte to DAP byte for the CF option
///
/// In the future, this may be merged with the default option.
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////



#ifndef _HDF5CFBYTE_H
#define _HDF5CFBYTE_H

// STL includes
#include <string>

// DODS includes
#include <Byte.h>


class HDF5CFByte:public libdap::Byte {

    private:
        string filename;
    public:
        HDF5CFByte(const std::string & n, const std::string &d);
        HDF5CFByte(const std::string & n, const std::string &d,const std::string &d_f);
        virtual ~ HDF5CFByte();
        virtual libdap::BaseType *ptr_duplicate();
        virtual bool read();
};

#endif                          // _HDF5CFBYTE_H

