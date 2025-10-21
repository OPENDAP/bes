// BESDapTransmit.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <libdap/DDS.h>
#include <libdap/DAS.h>
#include <libdap/DataDDS.h>
#include <libdap/ConstraintEvaluator.h>

#include <libdap/Error.h>

#include "BESDapTransmit.h"
#include "BESContainer.h"
#include "BESDapNames.h"
#include "BESDataNames.h"

#include "BESDASResponse.h"
#include "BESDDSResponse.h"
#include "BESDataDDSResponse.h"

#include "BESDMRResponse.h"

#include "BESContextManager.h"
#include "BESDapError.h"
#include "BESInternalFatalError.h"
#include "BESDebug.h"

#include "BESDapResponseBuilder.h"

using namespace libdap;
using namespace std;

#define MODULE "dap"
#define prolog std::string("DapTransmit::").append(__func__).append("() - ")

///////////////////////////////////////////////////////////////////////////////
// Local Helpers

// File local helper superclass for common exception handling
// for transmitting DAP responses.
class Sender
{
public:
    virtual ~Sender() = default;

    // The main call, non-virtual to force exception handling.
    // Subclasses will override send_internal private virtual.
    void send(BESResponseObject* obj, BESDataHandlerInterface & dhi)
    {
        string response_string = get_request_type();
        try {
            send_internal(obj, dhi);
        }
        catch (const InternalErr &e) {
            string err = "libdap error transmitting " + response_string + ": " + e.get_error_message();
            throw BESDapError(err, true, e.get_error_code(), (e.get_file().empty() ? __FILE__: e.get_file()),
                              (e.get_line() == 0 ? __LINE__: e.get_line()));
        }
        catch (const Error &e) {
            string err = "libdap error transmitting " + response_string + ": " + e.get_error_message();
            throw BESDapError(err, false, e.get_error_code(), (e.get_file().empty() ? __FILE__: e.get_file()),
                              (e.get_line() == 0 ? __LINE__: e.get_line()));
        }
        catch (const BESError &) {
            throw; // rethrow as is
        }
        catch (const std::exception &e) {
            string msg = "std::exception caught transmitting " + response_string + ": " + e.what()
                    + " (caught in BESDapTransmit).";
            throw BESInternalFatalError(msg, __FILE__, __LINE__);
        }
        catch (...) {
            string s = "unknown error caught transmitting " + response_string + ": ";
            throw BESInternalFatalError(s, __FILE__, __LINE__);
        }
    }

    // common code for subclasses
    bool get_print_mime() const
    {
        bool found = false;
        string protocol = BESContextManager::TheManager()->get_context("transmit_protocol", found);
        bool print_mime = false;
        if (found && protocol == "HTTP") {
            print_mime = true;
        }
        return print_mime;
    }

private:

    // Name of the request being sent, for debug
    virtual string get_request_type() const = 0;

    // Subclasses impl this for specialized behavior
    virtual void send_internal(BESResponseObject * obj, BESDataHandlerInterface & dhi) = 0;
};

class SendDAS: public Sender
{
private:
    string get_request_type() const override
    {
        return "DAS";
    }
    void send_internal(BESResponseObject * obj, BESDataHandlerInterface & dhi) override
    {
        BESDASResponse *bdas = dynamic_cast<BESDASResponse *>(obj);
        if (!bdas) {
            throw BESInternalError("cast error", __FILE__, __LINE__);
        }

        DAS *das = bdas->get_das();
        dhi.first_container();
        bool print_mime = get_print_mime();

        BESDapResponseBuilder rb;
        rb.set_dataset_name(dhi.container->get_real_name());
        rb.send_das(dhi.get_output_stream(), *das, print_mime);

        //rb.send_das(dhi.get_output_stream(), DDS &dds, ConstraintEvaluator &eval, bool constrained, bool with_mime_headers)
    }
};

class SendDDS: public Sender
{
private:
    string get_request_type() const override
    {
        return "DDS";
    }
    void send_internal(BESResponseObject * obj, BESDataHandlerInterface & dhi) override
    {
        BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *>(obj);
        if (!bdds) {
            throw BESInternalError("cast error", __FILE__, __LINE__);
        }

        DDS *dds = bdds->get_dds();
        ConstraintEvaluator & ce = bdds->get_ce();

        dhi.first_container();
        bool print_mime = get_print_mime();

        BESDapResponseBuilder rb;
        rb.set_dataset_name(dhi.container->get_real_name());
        rb.set_ce(dhi.data[POST_CONSTRAINT]);
        BESDEBUG(MODULE, prolog << "dhi.data[POST_CONSTRAINT]: " << dhi.data[POST_CONSTRAINT] << endl);
        rb.send_dds(dhi.get_output_stream(), &dds, ce, true, print_mime);
        bdds->set_dds(dds);
    }
};

class SendDataDDS: public Sender
{
private:
    string get_request_type() const override
    {
        return "DataDDS";
    }
    void send_internal(BESResponseObject * obj, BESDataHandlerInterface & dhi) override
    {
        BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(obj);
        if (!bdds) {
            throw BESInternalError("cast error", __FILE__, __LINE__);
        }

        DDS *dds = bdds->get_dds();
        ConstraintEvaluator & ce = bdds->get_ce();

        dhi.first_container();
        bool print_mime = get_print_mime();

        BESDapResponseBuilder rb;
        rb.set_dataset_name(dds->filename());
        rb.set_ce(dhi.data[POST_CONSTRAINT]);

        rb.set_async_accepted(dhi.data[ASYNC]);
        rb.set_store_result(dhi.data[STORE_RESULT]);

        BESDEBUG(MODULE, prolog << "dhi.data[POST_CONSTRAINT]: " << dhi.data[POST_CONSTRAINT] << endl);
        //rb.send_dap2_data(dhi.get_output_stream(), &dds, ce, print_mime);
        rb.send_dap2_data(dhi, &dds, ce, print_mime);
        bdds->set_dds(dds);
   }
};

class SendDDX: public Sender
{
private:
    string get_request_type() const override
    {
        return "DDX";
    }
    void send_internal(BESResponseObject * obj, BESDataHandlerInterface & dhi) override
    {
        BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *>(obj);
        if (!bdds) {
            throw BESInternalError("cast error", __FILE__, __LINE__);
        }

        DDS *dds = bdds->get_dds();
        ConstraintEvaluator & ce = bdds->get_ce();

        dhi.first_container();
        bool print_mime = get_print_mime();

        BESDapResponseBuilder rb;
        rb.set_dataset_name(dhi.container->get_real_name());
        rb.set_ce(dhi.data[POST_CONSTRAINT]);
        rb.send_ddx(dhi.get_output_stream(), &dds, ce, print_mime);
        bdds->set_dds(dds);
    }
};

class SendDMR: public Sender
{
private:
    string get_request_type() const override
    {
        return "DMR";
    }

    void send_internal(BESResponseObject * obj, BESDataHandlerInterface & dhi) override
    {
        BESDEBUG(MODULE, prolog << "SendDMR::send_internal() - BEGIN" << endl);

        BESDMRResponse *bdmr = dynamic_cast<BESDMRResponse *>(obj);
        if (!bdmr) throw BESInternalError("cast error", __FILE__, __LINE__);

        DMR *dmr = bdmr->get_dmr();

        BESDEBUG(MODULE, prolog << "SendDMR::send_internal() - dmr->request_xml_base(): '"<< dmr->request_xml_base() << endl);

        dhi.first_container();

        BESDapResponseBuilder rb;
        rb.set_dataset_name(dhi.container->get_real_name());

        rb.set_dap4ce(dhi.data[DAP4_CONSTRAINT]);
        rb.set_dap4function(dhi.data[DAP4_FUNCTION]);

        rb.set_async_accepted(dhi.data[ASYNC]);
        rb.set_store_result(dhi.data[STORE_RESULT]);

        rb.send_dmr(dhi.get_output_stream(), *dmr, get_print_mime());
        BESDEBUG(MODULE, prolog << "SendDMR::send_internal() - END" << endl);
    }
};

class SendDap4Data: public Sender
{
private:
    string get_request_type() const override
    {
        return "DAP4Data";
    }
    void send_internal(BESResponseObject * obj, BESDataHandlerInterface & dhi) override
    {
        BESDEBUG(MODULE, prolog << "SendDap4Data::send_internal() - BEGIN" << endl);
        // In DAP2 we made a special object for data - a child of DDS. That turned to make
        // some code harder to write, so this time I'll just use the DMR to hold data. jhrg
        // 10/31/13
        BESDMRResponse *bdmr = dynamic_cast<BESDMRResponse *>(obj);
        if (!bdmr) throw BESInternalError("cast error", __FILE__, __LINE__);

        DMR *dmr = bdmr->get_dmr();

        BESDEBUG(MODULE, prolog << "SendDap4Data::send_internal() -  dmr->request_xml_base(): '"<< dmr->request_xml_base() << endl);

        dhi.first_container();

        BESDapResponseBuilder rb;
        rb.set_dataset_name(dmr->filename());

        rb.set_dap4ce(dhi.data[DAP4_CONSTRAINT]);
        rb.set_dap4function(dhi.data[DAP4_FUNCTION]);

        rb.set_async_accepted(dhi.data[ASYNC]);
        rb.set_store_result(dhi.data[STORE_RESULT]);

        rb.send_dap4_data(dhi.get_output_stream(), *dmr, get_print_mime());
        BESDEBUG(MODULE, prolog << "SendDap4Data::send_internal() - END" << endl);
    }
};

///////////////////////////////////////////////////////////////////////////////
// Public Interface Impl

BESDapTransmit::BESDapTransmit() :
        BESTransmitter()
{
    add_method(DAS_SERVICE, BESDapTransmit::send_basic_das);
    add_method(DDS_SERVICE, BESDapTransmit::send_basic_dds);
    add_method(DDX_SERVICE, BESDapTransmit::send_basic_ddx);
    add_method(DATA_SERVICE, BESDapTransmit::send_basic_data);

    add_method(DMR_SERVICE, BESDapTransmit::send_basic_dmr);
    add_method(DAP4DATA_SERVICE, BESDapTransmit::send_basic_dap4data);
}

BESDapTransmit::~BESDapTransmit()
{
    remove_method(DAS_SERVICE);
    remove_method(DDS_SERVICE);
    remove_method(DDX_SERVICE);
    remove_method(DATA_SERVICE);

    remove_method(DMR_SERVICE);
    remove_method(DAP4DATA_SERVICE);
}

void BESDapTransmit::send_basic_das(BESResponseObject * obj, BESDataHandlerInterface & dhi)
{
    SendDAS sender;
    sender.send(obj, dhi);
}

void BESDapTransmit::send_basic_dds(BESResponseObject * obj, BESDataHandlerInterface & dhi)
{
    SendDDS sender;
    sender.send(obj, dhi);
}

void BESDapTransmit::send_basic_ddx(BESResponseObject * obj, BESDataHandlerInterface & dhi)
{
    SendDDX sender;
    sender.send(obj, dhi);
}

void BESDapTransmit::send_basic_data(BESResponseObject * obj, BESDataHandlerInterface & dhi)
{
    SendDataDDS sender;
    sender.send(obj, dhi);
}

void BESDapTransmit::send_basic_dmr(BESResponseObject * obj, BESDataHandlerInterface & dhi)
{
    SendDMR sender;
    sender.send(obj, dhi);
}

void BESDapTransmit::send_basic_dap4data(BESResponseObject * obj, BESDataHandlerInterface & dhi)
{
    SendDap4Data sender;
    sender.send(obj, dhi);
}
