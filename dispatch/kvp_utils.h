//
// Created by ndp on 12/13/19.
//

#ifndef HYRAX_KVP_UTILS_H
#define HYRAX_KVP_UTILS_H
#include <fstream>
#include <map>
#include <set>
#include <vector>


namespace kvp {


void load_keys(
        std::ifstream *keys_file,
        std::map<std::string,
        std::vector<std::string> > &keystore);

void load_keys(
        std::set<std::string> &loaded_kvp_files,
        const std::string &keys_file_name,
        std::map<std::string, std::vector<std::string> > &keystore);

void load_keys(
        const std::string &config_file,
        std::map <std::string, std::vector<std::string>>  &keystore);

bool break_pair(const char* b, std::string& key, std::string &value, bool &addto);

} // namespace kvp

#endif //HYRAX_KVP_UTILS_H
