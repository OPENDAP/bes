//
// Created by James Gallagher on 9/30/25.
//

#include <cppunit/extensions/HelperMacros.h>

// Project/BES includes
#include "DmrppRequestHandler.h"
#include "BESResponseHandler.h"
#include "BESDMRResponse.h"
#include "BESDDSResponse.h"
#include "BESDataDDSResponse.h"
#include "BESDASResponse.h"
#include "BESVersionInfo.h"
#include "BESInfo.h"
#include "BESContainer.h"
#include "BESDataHandlerInterface.h"

#include <libdap/DMR.h>
#include <libdap/DDS.h>
#include <libdap/DAS.h>

#include <memory>
#include <map>
#include <string>

using namespace std;
using namespace dmrpp;
//using namespace bes;

class DmrppRequestHandlerTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(DmrppRequestHandlerTest);
    CPPUNIT_TEST(testCtorAndDestructor_NoThrow);
    CPPUNIT_TEST(testBuildDMR_FromStringContainer_Succeeds);
    CPPUNIT_TEST(testBuildDAP4Data_SetsNetcdf4Flags);
    CPPUNIT_TEST(testBuildDDS_FromDMRPP_Succeeds);
    CPPUNIT_TEST(testBuildDAS_FromDMRPP_Succeeds);
    CPPUNIT_TEST(testBuildVers_Succeeds);
    //CPPUNIT_TEST(testBuildHelp_Succeeds);
    CPPUNIT_TEST_SUITE_END();

public:
// ---------- Minimal test doubles ----------

// A tiny, in-memory container that returns DMR++ XML as a string
class FakeStringContainer : public BESContainer {
    string d_sym;
    string d_real;
    string d_attrs; // when "as-string", handler treats access() as the DMR++ xml content
    string d_payload; // the XML itself

public:
    FakeStringContainer(string sym, string real_name, string xml_payload, bool asString = true)
            : d_sym(std::move(sym)), d_real(std::move(real_name)), d_attrs(asString ? "as-string" : ""), d_payload(std::move(xml_payload)) {}

    string get_symbolic_name() const  { return d_sym; }
    string get_real_name() const  { return d_real; }
    string get_attributes() const  { return d_attrs; }
    string access() override { return d_payload; }
};

// A “file path” container that points at a local temporary file with the XML.
// (Not used by default; here for completeness.)
class FakePathContainer : public BESContainer {
    string d_sym;
    string d_real_path;
public:
    FakePathContainer(string sym, string real_path)
            : d_sym(std::move(sym)), d_real_path(std::move(real_path)) {}

    string get_symbolic_name() const  { return d_sym; }
    string get_real_name() const  { return d_real_path; }
    string get_attributes() const  { return ""; }
    string access() override { return d_real_path; }
};

// Response handler that simply wraps the object we want to expose to the handler.
template <class RespT>
class PassThruResponseHandler : public BESResponseHandler {
    unique_ptr<RespT> obj;
public:
    explicit PassThruResponseHandler(unique_ptr<RespT> r) : obj(std::move(r)) {}
    ~PassThruResponseHandler() override = default;

    BESResponseObject* get_response_object() override { return obj.get(); }
    const BESResponseObject* get_response_object() const { return obj.get(); }
};

// ---------- Helpers ----------

// Minimal valid DMR++ with one int variable.
// NOTE: namespace and elements match what DMZ expects.
static const char* kTinyDmrpp = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<dmrpp xmlns="http://opendap.org/ns/DAP/4.0#dmrpp">
  <Dataset name="tiny" dapVersion="4.0" xmlns:dap="http://xml.opendap.org/ns/DAP/4.0#">
    <dap:Int32 name="x">
      <dap:Attribute name="_FillValue" type="Int32"><dap:value>-999</dap:value></dap:Attribute>
    </dap:Int32>
  </Dataset>
</dmrpp>
)XML";

// Build a canned BESDataHandlerInterface with the provided container & response handler.
template <class RespT>
static BESDataHandlerInterface makeDHI(BESContainer* c, unique_ptr<RespT> resp)
{
    auto rh = new PassThruResponseHandler<RespT>(std::move(resp));
    BESDataHandlerInterface dhi;
    dhi.response_handler.reset(rh);
    dhi.container = c;
    return dhi;
}

// ---------- Test Fixture ----------

void setUp() {
    // Nothing global; ctor of the handler will init libcurl and caches as configured.
}

void tearDown() {
    // Ensure any globals are torn down by destroying handlers inside each test.
}

// ---------- Tests ----------

void testCtorAndDestructor_NoThrow() {
    CPPUNIT_ASSERT_NO_THROW({
                                    DmrppRequestHandler h("dmrpp");
                            });
    // If destructor leaked or double-freed curl handle pool, we’d crash here or in subsequent tests.
}

void testBuildDMR_FromStringContainer_Succeeds() {
    DmrppRequestHandler h("dmrpp");

    FakeStringContainer cont("sym", "tiny.dmrpp", kTinyDmrpp, /*asString*/true);

    auto resp = std::make_unique<BESDMRResponse>();
    auto dhi = makeDHI(&cont, std::move(resp));

    bool ok = DmrppRequestHandler::dap_build_dmr(dhi);
    CPPUNIT_ASSERT(ok);

    auto bdmr = dynamic_cast<BESDMRResponse*>(dhi.response_handler->get_response_object());
    CPPUNIT_ASSERT(bdmr);
    CPPUNIT_ASSERT(bdmr->get_dmr());
    // The handler sets name using libdap::name_path(); for as-string, name is derived from the "filename" it set.
    // We at least assert the DMR exists and contains a dataset name.
    CPPUNIT_ASSERT(!bdmr->get_dmr()->name().empty());
}

void testBuildDAP4Data_SetsNetcdf4Flags() {
    DmrppRequestHandler h("dmrpp");

    FakeStringContainer cont("sym", "tiny.dmrpp", kTinyDmrpp, true);

    auto resp = std::make_unique<BESDMRResponse>();
    auto dhi = makeDHI(&cont, std::move(resp));
    // Simulate a netcdf-4 request (as fileout netCDF would)
    dhi.data["return_command"] = "netcdf-4";

    // Precondition: reset static flags to a known state
    DmrppRequestHandler::is_netcdf4_classic_response = false;
    DmrppRequestHandler::is_netcdf4_enhanced_response = false;

    bool ok = DmrppRequestHandler::dap_build_dap4data(dhi);
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT(DmrppRequestHandler::is_netcdf4_enhanced_response);
    // “classic” stays false unless configured by key; we’re not toggling keys here.
    CPPUNIT_ASSERT(!DmrppRequestHandler::is_netcdf4_classic_response);

    auto bdmr = dynamic_cast<BESDMRResponse*>(dhi.response_handler->get_response_object());
    CPPUNIT_ASSERT(bdmr && bdmr->get_dmr());
}

void testBuildDDS_FromDMRPP_Succeeds() {
    DmrppRequestHandler h("dmrpp");

    FakeStringContainer cont("sym", "tiny.dmrpp", kTinyDmrpp, true);

    auto resp = std::make_unique<BESDDSResponse>();
    // Ask the response to include explicit container name so we can assert it was propagated.
    resp->set_explicit_containers(true);

    auto dhi = makeDHI(&cont, std::move(resp));

    bool ok = DmrppRequestHandler::dap_build_dds(dhi);
    CPPUNIT_ASSERT(ok);

    auto bdds = dynamic_cast<BESDDSResponse*>(dhi.response_handler->get_response_object());
    CPPUNIT_ASSERT(bdds);
    libdap::DDS* dds = bdds->get_dds();
    CPPUNIT_ASSERT(dds);

    // When explicit containers is true, handler sets the DDS container_name to the container's symbolic name.
    CPPUNIT_ASSERT_EQUAL(std::string("sym"), dds->container_name());
}

void testBuildDAS_FromDMRPP_Succeeds() {
    DmrppRequestHandler h("dmrpp");

    FakeStringContainer cont("sym", "tiny.dmrpp", kTinyDmrpp, true);

    auto resp = std::make_unique<BESDASResponse>();
    resp->set_explicit_containers(true);

    auto dhi = makeDHI(&cont, std::move(resp));

    bool ok = DmrppRequestHandler::dap_build_das(dhi);
    CPPUNIT_ASSERT(ok);

    auto bdas = dynamic_cast<BESDASResponse*>(dhi.response_handler->get_response_object());
    CPPUNIT_ASSERT(bdas);
    libdap::DAS* das = bdas->get_das();
    CPPUNIT_ASSERT(das);

    // Container name propagation
    CPPUNIT_ASSERT_EQUAL(std::string("sym"), das->container_name());
    // The minimal tiny DMR++ includes a _FillValue attribute; DAS should not be empty.
    CPPUNIT_ASSERT(!das->get_table().empty());
}

void testBuildVers_Succeeds() {
    DmrppRequestHandler h("dmrpp");

    // Vers/info don’t need a data container
    auto resp = std::make_unique<BESVersionInfo>();
    auto dhi = makeDHI<BESVersionInfo>(/*container*/nullptr, std::move(resp));

    bool ok = DmrppRequestHandler::dap_build_vers(dhi);
    CPPUNIT_ASSERT(ok);

    auto info = dynamic_cast<BESVersionInfo*>(dhi.response_handler->get_response_object());
    CPPUNIT_ASSERT(info);
    // Not deeply inspecting; just ensure something was added
    CPPUNIT_ASSERT(!info->get_modules().empty());
}

#if 0

    void testBuildHelp_Succeeds() {
    DmrppRequestHandler h("dmrpp");

    auto resp = std::make_unique<BESInfo>();
    auto dhi = makeDHI<BESInfo>(/*container*/nullptr, std::move(resp));

    bool ok = DmrppRequestHandler::dap_build_help(dhi);
    CPPUNIT_ASSERT(ok);

    auto info = dynamic_cast<BESInfo*>(dhi.response_handler->get_response_object());
    CPPUNIT_ASSERT(info);
    // Ensure the info buffer has some content
    std::ostringstream oss;
    info->print(oss);
    CPPUNIT_ASSERT(!oss.str().empty());
}

#endif
};
