//
// Created by James Gallagher on 3/5/25.
//

#ifndef IAWS_SDK_H
#define IAWS_SDK_H

#include <string>

namespace http {
/**
 * @brief Interface for the AWS_SDK class that accesses S3
 * This interface will enable the dependency injection pattern so
 * that we can write unit tests for code that makes use of AWS SDK
 * to read data, etc., from S3. jhrg 3/5/25
 */
class IAWS_SDK {
protected: // Let specializations assign to these. jhrg 3/6/25
    std::string d_aws_exception_name;
    std::string d_aws_exception_message;
    int d_http_status_code = 200;   // AWS calls only set HTTP status on error. jhrg 3/6/25

public:
    IAWS_SDK() = default;
    virtual ~IAWS_SDK() = default;
    IAWS_SDK(const IAWS_SDK&) = delete;
    IAWS_SDK& operator=(const IAWS_SDK&) = delete;
    IAWS_SDK(const IAWS_SDK&&) = delete;
    IAWS_SDK& operator=(const IAWS_SDK&&) = delete;

    std::string get_aws_exception_name() const { return d_aws_exception_name; }
    std::string get_aws_exception_message() const { return d_aws_exception_message; }
    int get_http_status_code() const { return d_http_status_code; }

    virtual void initialize(const std::string &region) = 0;
    virtual void initialize(const std::string &region, const std::string &aws_key, const std::string &aws_secret_key) = 0;

    virtual bool s3_head(const std::string &bucket, const std::string &key) = 0;
    virtual std::string s3_get_as_string(const std::string &bucket, const std::string &key) = 0;
    virtual bool s3_get_as_file(const std::string &bucket, const std::string &key, const std::string &filename) = 0;
};
}

#endif //IAWS_SDK_H
