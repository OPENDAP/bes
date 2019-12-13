//
// Created by ndp on 12/13/19.
//

#ifndef HYRAX_KVP_UTILS_H
#define HYRAX_KVP_UTILS_H
#include <fstream>
#include <map>
#include <vector>



void load_keys(   std::ifstream *keys_file, std::map<std::string, std::vector<std::string> > &keystore);
void load_keys(const std::string &keys_file_name, std::map<std::string, std::vector<std::string> > &keystore);



#endif //HYRAX_KVP_UTILS_H
