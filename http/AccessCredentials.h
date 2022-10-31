//
// Created by ndp on 2/12/20.
//

#ifndef HYRAX_GIT_ACCESSCREDENTIALS_H
#define HYRAX_GIT_ACCESSCREDENTIALS_H

#include <map>
#include <string>

namespace http {

class AccessCredentials {
public:
    // These are the string keys used to express the normative key names
    // for the credentials components.
    static const char *ID_KEY;
    static const char *KEY_KEY;
    static const char *REGION_KEY;
    static const char *URL_KEY;

private:
    std::map<std::string, std::string> d_kvp;
    std::string d_config_name;
    bool d_s3_tested = false;
    bool d_is_s3 = false;

public:
    AccessCredentials() = default;

    explicit AccessCredentials(std::string config_name) : d_config_name(std::move(config_name)) {}

    AccessCredentials(const AccessCredentials &ac) = default;

    virtual ~AccessCredentials() = default;

    virtual std::string get(const std::string &key);

    virtual void add(const std::string &key, const std::string &value);

    virtual bool is_s3_cred();

    std::string to_json() const;

    std::string name() const { return d_config_name; }

    void name(const std::string &name) { d_config_name = name; }
};

} // namespace http

#endif //HYRAX_GIT_ACCESSCREDENTIALS_H
