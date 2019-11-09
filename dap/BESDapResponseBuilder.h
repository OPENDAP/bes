// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2011 OPeNDAP, Inc.
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

#ifndef _response_builder_h
#define _response_builder_h

#include <string>

#define DAP_PROTOCOL_VERSION "3.2"

#undef DAP2_STORED_RESULTS

class BESDapFunctionResponseCache;
class BESDataHandlerInterface;
class BESResponseObject;
class BRDRequestHandler;

namespace libdap {
    class ConstraintEvaluator;
    class DDS;
    class DAS;
}


/**
 * This class is used to build responses for/by the BES. This class replaces
 * DODSFilter (although DODSFilter is still included in the library, its use
 * is deprecated). and it does not have a provision for command line arguments.
 * @author jhrg 1/28/2011
 */

class BESDapResponseBuilder {
public:
    friend class ResponseBuilderTest;

protected:
	std::string d_dataset;          /// Name of the dataset/database
	std::string d_dap2ce;           /// DAP2 Constraint expression
	std::string d_dap4ce;           /// DAP4 Constraint expression
	std::string d_dap4function;     /// DAP4 Server Side Function expression
	std::string d_btp_func_ce;      /// The BTP functions, extracted from the CE
	int d_timeout;                  /// Response timeout after N seconds
	std::string d_default_protocol;	/// Version string for the library's default protocol version

	bool d_cancel_timeout_on_send;  /// Should a timeout be cancelled once transmission starts?

	/**
	 * Time, if any, that the client will wait for an async response.
	 * An empty string (length=0) means the client didn't supply an async parameter
	 */
	std::string d_async_accepted;

	/**
	 * If set (i.e. non-null) then the client has asked for the result to be stored
	 * for later retrieval. The value is the service URL used to construct
	 * the stored result access URL to be returned to the client.
	 * An empty string (length=0) means the client didn't supply a store_result async parameter
	 */
	std::string d_store_result;

	void initialize();

#ifdef DAP2_STORED_RESULTS
	bool store_dap2_result(ostream &out, libdap::DDS &dds, libdap::ConstraintEvaluator &eval);
#endif

	void send_dap4_data_using_ce(std::ostream &out, libdap::DMR &dmr, bool with_mime_headersr);

public:

	/** Make an empty instance. Use the set_*() methods to load with needed
	 values. You must call at least set_dataset_name() or be requesting
	 version information. */
	BESDapResponseBuilder(): d_dataset(""), d_dap2ce(""), d_dap4ce(""), d_dap4function(""),
	    d_btp_func_ce(""), d_timeout(0), d_default_protocol(DAP_PROTOCOL_VERSION),
	    d_cancel_timeout_on_send(false), d_async_accepted(""), d_store_result("")
	{
		initialize();
	}

	virtual ~BESDapResponseBuilder();

	virtual std::string get_ce() const;
	virtual void set_ce(std::string _ce);

	virtual std::string get_dap4ce() const;
	virtual void set_dap4ce(std::string _ce);

	virtual std::string get_dap4function() const;
	virtual void set_dap4function(std::string _func);

	virtual std::string get_store_result() const;
	virtual void set_store_result(std::string _sr);

	virtual std::string get_async_accepted() const;
	virtual void set_async_accepted(std::string _aa);

	virtual std::string get_btp_func_ce() const
	{
		return d_btp_func_ce;
	}
	virtual void set_btp_func_ce(std::string _ce)
	{
		d_btp_func_ce = _ce;
	}

	virtual std::string get_dataset_name() const;
	virtual void set_dataset_name(const std::string _dataset);

    /** @name DDS_timeout
     *  Old deprecated BESDapResponseBuilder timeout code. Do not use.
     *  @deprecated
     */
    ///@{
	void register_timeout() const;
	void set_timeout(int timeout = 0);
	int get_timeout() const;
	void timeout_on() const;
    void timeout_off();

    virtual void establish_timeout(std::ostream &stream) const;
    virtual void remove_timeout() const;
	///@}

	void conditional_timeout_cancel();

	virtual void split_ce(libdap::ConstraintEvaluator &eval, const std::string &expr = "");

	virtual void send_das(std::ostream &out, libdap::DAS &das, bool with_mime_headers = true) const;
	virtual void send_das(std::ostream &out, libdap::DDS **dds, libdap::ConstraintEvaluator &eval, bool constrained =
			false, bool with_mime_headers = true);

	virtual void send_dds(std::ostream &out, libdap::DDS **dds, libdap::ConstraintEvaluator &eval, bool constrained =
			false, bool with_mime_headers = true);

	virtual void serialize_dap2_data_dds(std::ostream &out, libdap::DDS **dds, libdap::ConstraintEvaluator &eval,
			bool ce_eval = true);
	virtual void send_dap2_data(std::ostream &data_stream, libdap::DDS **dds, libdap::ConstraintEvaluator &eval,
			bool with_mime_headers = true);
        virtual void send_dap2_data(BESDataHandlerInterface &dhi, libdap::DDS **dds, libdap::ConstraintEvaluator &eval,
			bool with_mime_headers = true);


	// Added jhrg 9/1/16
	virtual libdap::DDS *intern_dap2_data(BESResponseObject *obj, BESDataHandlerInterface &dhi);
	virtual libdap::DDS *process_dap2_dds(BESResponseObject *obj, BESDataHandlerInterface &dhi);

	// TODO jhrg 9/6/16
	//
	// virtual libdap::DMR *intern_dap4_data(BESResponseObject *obj, BESDataHandlerInterface &dhi);

	virtual void send_ddx(std::ostream &out, libdap::DDS **dds, libdap::ConstraintEvaluator &eval,
			bool with_mime_headers = true);

#ifdef	DAP2_STORED_RESULTS
	virtual void serialize_dap2_data_ddx(std::ostream &out, libdap::DDS **dds, libdap::ConstraintEvaluator & eval,
			const std::string &boundary, const std::string &start, bool ce_eval = true);
#endif
	virtual void send_dmr(std::ostream &out, libdap::DMR &dmr, bool with_mime_headers = true);

	virtual void send_dap4_data(std::ostream &out, libdap::DMR & dmr, bool with_mime_headers = true);

	virtual void serialize_dap4_data(std::ostream &out, libdap::DMR &dmr, bool with_mime_headers = true);

	virtual bool store_dap4_result(ostream &out, libdap::DMR &dmr);
};


#endif // _response_builder_h
