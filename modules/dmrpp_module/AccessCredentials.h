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
    static const std::string ID_KEY;
    static const std::string KEY_KEY;
    static const std::string REGION_KEY;
    static const std::string BUCKET_KEY;
    static const std::string URL_KEY;
private:
    std::__1::map<std::string, std::string> kvp;
    bool s3_tested, is_s3;
    std::string d_config_name;
public:
    AccessCredentials()= default;
    AccessCredentials(std::string config_name){ d_config_name = config_name;}
    AccessCredentials(const AccessCredentials &ac) = default;

    std::string get(const std::string &key);
    void add(const std::string &key, const std::string &value);
    bool isS3Cred();
    std::string to_json();
    std::string name(){ return d_config_name; }
};

#include <string>
#include <vector>

#endif //HYRAX_GIT_ACCESSCREDENTIALS_H
