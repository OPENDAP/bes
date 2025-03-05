//
// Created by James Gallagher on 3/5/25.
//

#ifndef IAWS_SDK_H
#define IAWS_SDK_H

#include <string>

/**
 * @brief Interface for the AWS_SDK class that accesses S3
 * This interface will enable the dependency injection pattern so
 * that we can write unit tests for code that makes use of AWS SDK
 * to read data, etc., from S3. jhrg 3/5/25
 */
class IAWS_SDK {
public:
    IAWS_SDK() = default;
    virtual ~IAWS_SDK() = default;
    IAWS_SDK(const IAWS_SDK&) = delete;
    IAWS_SDK& operator=(const IAWS_SDK&) = delete;
    IAWS_SDK(const IAWS_SDK&&) = delete;
    IAWS_SDK& operator=(const IAWS_SDK&&) = delete;

    virtual void init(const std::string &region) = 0;

    /**
     * @brief init method that defaults the AWS region to us-east-1
     */
    virtual void init() {
        init("us-east-1");
    }

    virtual bool s3_head(const std::string &bucket, const std::string &key) const = 0;
    virtual std::string s3_get_as_string(const std::string &bucket, const std::string &key) const = 0;
    virtual bool s3_get_as_file(const std::string &bucket, const std::string &key, const std::string &filename) const = 0;
};

#endif //IAWS_SDK_H
