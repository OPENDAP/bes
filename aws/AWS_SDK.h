// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)

// Copyright (c) 2025 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
//

// Created by James Gallagher on 3/4/25.

#ifndef AWS_SDK_H
#define AWS_SDK_H

#include <string>

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>

#include "IAWS_SDK.h"

namespace bes
{
    class AWS_SDK : public IAWS_SDK
    {
        Aws::S3::S3Client d_s3_client;
        bool d_is_s3_client_initialized = false;
        static Aws::SDKOptions options;

        void throw_if_s3_client_uninitialized() const; // throws BESInternalFatalError

        static Aws::S3::S3Client get_s3_client(const std::string &region, const std::string &aws_key,
                                               const std::string &aws_secret_key);

        friend class AWS_SDK_Test;

    public:
        AWS_SDK() = default;
        ~AWS_SDK() override = default;
        AWS_SDK(const AWS_SDK &) = delete;
        AWS_SDK &operator=(const AWS_SDK &) = delete;
        AWS_SDK(const AWS_SDK &&) = delete;
        AWS_SDK &operator=(const AWS_SDK &&) = delete;

        static void aws_library_initialize()
        {
            Aws::InitAPI(options); // Must only be called once, as per AWS SDK
        }

        static void aws_library_shutdown()
        {
            Aws::ShutdownAPI(options);
        }

        void initialize_s3_client(const std::string &region, const std::string &aws_key, const std::string &aws_secret_key) override
        {
            d_s3_client = get_s3_client(region, aws_key, aws_secret_key);
            d_is_s3_client_initialized = true;
        }

        bool s3_head_exists(const std::string &bucket, const std::string &key) override;
        std::string s3_get_as_string(const std::string &bucket, const std::string &key) override;
        bool s3_get_as_file(const std::string &bucket, const std::string &key, const std::string &filename) override;

        Aws::String s3_generate_presigned_object_url(const Aws::String &bucket_name,
                                                     const Aws::String &key,
                                                     uint64_t expiration_seconds) override;
    };
}
#endif // AWS_SDK_H
