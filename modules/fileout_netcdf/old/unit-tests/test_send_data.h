/*
 * test_send_data.h
 *
 *  Created on: Feb 3, 2013
 *      Author: jimg
 */

#ifndef TEST_SEND_DATA_H_
#define TEST_SEND_DATA_H_

void build_dods_response(DDS **dds, const string &file_name);
void build_netcdf_file(DDS **dds, const string &file_name);
void send_data(DDS **dds, ConstraintEvaluator &eval, BESDataHandlerInterface &dhi);

#endif /* TEST_SEND_DATA_H_ */
