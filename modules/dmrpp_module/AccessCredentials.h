//
// Created by ndp on 2/12/20.
//

#ifndef HYRAX_GIT_ACCESSCREDENTIALS_H
#define HYRAX_GIT_ACCESSCREDENTIALS_H

#include <map>
#include <string>

class AccessCredentials {
public:
    // These are the string keys used to express the normative key names
    // for the credentials components.
#if 0
    static const  std::string ID_KEY;
    static const std::string KEY_KEY;
    static const std::string REGION_KEY;
    //static const std::string BUCKET_KEY;
    static const std::string URL_KEY;
#else
    static const char *ID_KEY;
    static const char *KEY_KEY;
    static const char *REGION_KEY;
    static const char *URL_KEY;
#endif
private:
    std::map<std::string, std::string> kvp;
    bool s3_tested, is_s3;
    std::string d_config_name;
public:
    AccessCredentials() = default;

    AccessCredentials(const std::string &config_name)
        :d_config_name(config_name) { }

    AccessCredentials(const AccessCredentials &ac) = default;

    virtual ~AccessCredentials() = default;

    virtual std::string get(const std::string &key);

    void add(const std::string &key, const std::string &value);

    virtual bool is_s3_cred();

    std::string to_json();

    std::string name() { return d_config_name; }

    void name(const std::string &name) { d_config_name = name; }
};

#include <string>
#include <vector>

#endif //HYRAX_GIT_ACCESSCREDENTIALS_H
