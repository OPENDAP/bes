/*
 * DmrppUtil.h
 *
 *  Created on: Nov 22, 2016
 *      Author: jimg
 */

#ifndef MODULES_DMRPP_MODULE_DMRPPUTIL_H_
#define MODULES_DMRPP_MODULE_DMRPPUTIL_H_

#include <string>

typedef size_t (*curl_write_data)(void *buffer, size_t size, size_t nmemb, void *data);

void curl_read_bytes(const std::string &url, const std::string& range, curl_write_data write_data, void *user_data);

#endif /* MODULES_DMRPP_MODULE_DMRPPUTIL_H_ */
