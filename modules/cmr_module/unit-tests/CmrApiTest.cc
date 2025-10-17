// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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

#include <memory>
#include <iostream>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <libdap/util.h>

#include <BESError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <BESCatalogList.h>
#include <TheBESKeys.h>
#include "test_config.h"

#include "CmrApi.h"
#include "CmrCatalog.h"
#include "CmrInternalError.h"
#include "JsonUtils.h"

#include "common/run_tests_cppunit.h"

using namespace std;

#define prolog std::string("CmrApiTest::").append(__func__).append("() - ")

namespace cmr {

class CmrApiTest: public CppUnit::TestFixture {

public:
    // Called once before everything gets tested
    CmrApiTest() = default;

    // Called at the end of the test
    ~CmrApiTest() override = default;

    // Called before each test
    void setUp() override {
        DBG(cerr << endl);
        DBG2(cerr << "setUp() - BEGIN" << endl);
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
        DBG2(cerr << "setUp() - Using BES configuration: " << bes_conf << endl);

        TheBESKeys::ConfigFile = bes_conf;

        DBG2(cerr << "setUp() - Adding catalog '"<< CMR_CATALOG_NAME << "'" << endl);
        BESCatalogList::TheCatalogList()->add_catalog(new cmr::CmrCatalog(CMR_CATALOG_NAME));

        if (debug2) show_file(bes_conf);
        DBG(cerr << "setUp() - END" << endl);
    }

    // Called after each test
    void tearDown()
    {
    }

    void get_years_test() {
        string collection_name = "C179003030-ORNL_DAAC";
        vector<string> expected = { "1984", "1985", "1986",
                "1987", "1988" };
        unsigned long  expected_size = 5;
        vector<string> years;
        try {
            CmrApi cmr;
            cmr.get_years(collection_name, years);
            BESDEBUG(MODULE, prolog << "Checking expected size ("<< expected_size << ") vs received size ( " << years.size() << ")" << endl);

            CPPUNIT_ASSERT(expected_size == years.size());

            stringstream msg;
            msg << prolog << "The collection '" << collection_name << "' spans "
                    << years.size() << " years: ";
            for (size_t i = 0; i < years.size(); i++) {
                if (i > 0)
                    msg << ", ";
                msg << years[i];
            }
            BESDEBUG(MODULE, msg.str() << endl);

            for (size_t i = 0; i < years.size(); i++) {
                msg.str(std::string());
                msg << prolog << "Checking:  expected: " << expected[i]
                        << " received: " << years[i];
                BESDEBUG(MODULE, msg.str() << endl);
                CPPUNIT_ASSERT(expected[i] == years[i]);
            }
        }
        catch (BESError &be) {
            string msg = "Caught BESError! Message: " + be.get_message();
            cerr << endl << msg << endl;
            CPPUNIT_ASSERT(!"Caught BESError");
        }
    }

    void get_months_test() {
        string collection_name = "C179003030-ORNL_DAAC";
        vector<string> expected = {
                "01",
                "02",
                "03",
                "04",
                "05",
                "06",
                "07",
                "08",
                "09",
                "10",
                "11",
                "12" };
        unsigned long  expected_size = 12;
        vector<string> months;
        try {
            CmrApi cmr;

            string year ="1985";

            cmr.get_months(collection_name, year, months);
            BESDEBUG(MODULE, prolog << "Checking expected size ("<< expected_size << ") vs received size (" << months.size() << ")" << endl);
            CPPUNIT_ASSERT(expected_size == months.size());

            stringstream msg;
            msg << prolog << "In the year " << year << " the collection '" << collection_name << "' spans "
                    << months.size() << " months: ";

            for (size_t i = 0; i < months.size(); i++) {
                if (i > 0)
                    msg << ", ";
                msg << months[i];
            }
            BESDEBUG(MODULE, msg.str() << endl);

            for (size_t i = 0; i < months.size(); i++) {
                msg.str(std::string());
                msg << prolog << "Checking:  expected: " << expected[i]
                        << " received: " << months[i];
                BESDEBUG(MODULE, msg.str() << endl);
                CPPUNIT_ASSERT(expected[i] == months[i]);
            }

        }
        catch (BESError &be) {
            string msg = "Caught BESError! Message: " + be.get_message();
            cerr << endl << msg << endl;
            CPPUNIT_ASSERT(!"Caught BESError");
        }
    }

    void get_days_test() {
        string collection_name = "C1276812863-GES_DISC";
        vector<string> expected = {
                "01","02","03","04","05","06","07","08","09","10",
                "11","12","13","14","15","16","17","18","19","20",
                "21","22","23","24","25","26","27","28","29","30",
                "31"
        };
        unsigned long  expected_size = 31;
        vector<string> days;
        try {
            CmrApi cmr;

            string year ="1985";
            string month = "03";

            cmr.get_days(collection_name, year, month, days);
            BESDEBUG(MODULE, prolog << "Checking expected size ("<< expected_size << ") vs received size (" << days.size() << ")" << endl);
            CPPUNIT_ASSERT(expected_size == days.size());

            stringstream msg;
            msg << prolog << "In the year " << year << ", month " << month << " the collection '" << collection_name << "' spans "
                    << days.size() << " days: ";
            for (size_t i = 0; i < days.size(); i++) {
                if (i > 0)
                    msg << ", ";
                msg << days[i];
            }
            BESDEBUG(MODULE, msg.str() << endl);

            for (size_t i = 0; i < days.size(); i++) {
                msg.str(std::string());
                msg << prolog << "Checking:  expected: " << expected[i]
                        << " received: " << days[i];
                BESDEBUG(MODULE, msg.str() << endl);
                CPPUNIT_ASSERT(expected[i] == days[i]);
            }
        }
        catch (BESError &be) {
            string msg = "Caught BESError! Message: " + be.get_message();
            cerr << endl << msg << endl;
            CPPUNIT_ASSERT(!"Caught BESError");
        }
    }

    void get_granule_ids_day_test() {
        string collection_name = "C1276812863-GES_DISC";

        vector<string> expected = {
                "G1277917089-GES_DISC"
        };

        unsigned long  expected_size = 1;
        vector<string> granules;
        try {
            CmrApi cmr;

            string year ="1985";
            string month = "03";
            string day = "13";

            cmr.get_granule_ids(collection_name, year, month, day, granules);
            BESDEBUG(MODULE, prolog << "Checking expected size ("<< expected_size << ") vs received size (" << granules.size() << ")" << endl);
            CPPUNIT_ASSERT(expected_size == granules.size());

            stringstream msg;
            msg << prolog << "In the year " << year << ", month " << month <<  ", day " << day << " the collection '" << collection_name << "' contains "
                    << granules.size() << " granules: ";
            for (size_t i = 0; i < granules.size(); i++) {
                if (i > 0)
                    msg << ", ";
                msg << granules[i];
            }
            BESDEBUG(MODULE, msg.str() << endl);

            for (size_t i = 0; i < granules.size(); i++) {
                msg.str(std::string());
                msg << prolog << "Checking:  expected: " << expected[i]
                        << " received: " << granules[i];
                BESDEBUG(MODULE, msg.str() << endl);
                CPPUNIT_ASSERT(expected[i] == granules[i]);
            }
        }
        catch (BESError &be) {
            string msg = "Caught BESError! Message: " + be.get_message();
            cerr << endl << msg << endl;
            CPPUNIT_ASSERT(!"Caught BESError");
        }
    }

    void get_granule_ids_month_test() {
        string collection_name = "C1276812863-GES_DISC";

        vector<string> expected = {
                "G1277917088-GES_DISC",
                "G1277917126-GES_DISC",
                "G1277917102-GES_DISC",
                "G1277917125-GES_DISC",
                "G1277917121-GES_DISC",
                "G1277917112-GES_DISC",
                "G1277917116-GES_DISC",
                "G1277917161-GES_DISC",
                "G1277917098-GES_DISC",
                "G1277917097-GES_DISC",
                "G1277917105-GES_DISC",
                "G1277917077-GES_DISC",
                "G1277917089-GES_DISC",
                "G1277917109-GES_DISC",
                "G1277917141-GES_DISC",
                "G1277917107-GES_DISC",
                "G1277917114-GES_DISC",
                "G1277917143-GES_DISC",
                "G1277917104-GES_DISC",
                "G1277917093-GES_DISC",
                "G1277917115-GES_DISC",
                "G1277917145-GES_DISC",
                "G1277917059-GES_DISC",
                "G1277917090-GES_DISC",
                "G1277917096-GES_DISC",
                "G1277917110-GES_DISC",
                "G1277917124-GES_DISC",
                "G1277917075-GES_DISC",
                "G1277917094-GES_DISC",
                "G1277917160-GES_DISC",
                "G1277917148-GES_DISC"
        };

        unsigned long  expected_size = 31;
        vector<string> granules;
        try {
            CmrApi cmr;

            string year ="1985";
            string month = "03";

            cmr.get_granule_ids(collection_name, year, month, "", granules);
            BESDEBUG(MODULE, prolog << "Checking expected size ("<< expected_size << ") vs received size (" << granules.size() << ")" << endl);
            CPPUNIT_ASSERT(expected_size == granules.size());

            stringstream msg;
            msg << prolog << "In the year " << year << ", month " << month <<  " the collection '" << collection_name << "' contains "
                    << granules.size() << " granules: ";
            for (size_t i = 0; i < granules.size(); i++) {
                if (i > 0)
                    msg << ", ";
                msg << granules[i];
            }
            BESDEBUG(MODULE, msg.str() << endl);

            for (size_t i = 0; i < granules.size(); i++) {
                msg.str(std::string());
                msg << prolog << "Checking:  expected: " << expected[i]
                        << " received: " << granules[i];
                BESDEBUG(MODULE, msg.str() << endl);
                CPPUNIT_ASSERT(expected[i] == granules[i]);
            }
        }
        catch (BESError &be) {
            string msg = "Caught BESError! Message: " + be.get_message();
            cerr << endl << msg << endl;
            CPPUNIT_ASSERT(!"Caught BESError");
        }
    }

    void get_granules_month_test() {
        string collection_name = "C1276812863-GES_DISC";

        vector<string> expected = {
                "G1277917088-GES_DISC",
                "G1277917126-GES_DISC",
                "G1277917102-GES_DISC",
                "G1277917125-GES_DISC",
                "G1277917121-GES_DISC",
                "G1277917112-GES_DISC",
                "G1277917116-GES_DISC",
                "G1277917161-GES_DISC",
                "G1277917098-GES_DISC",
                "G1277917097-GES_DISC",
                "G1277917105-GES_DISC",
                "G1277917077-GES_DISC",
                "G1277917089-GES_DISC",
                "G1277917109-GES_DISC",
                "G1277917141-GES_DISC",
                "G1277917107-GES_DISC",
                "G1277917114-GES_DISC",
                "G1277917143-GES_DISC",
                "G1277917104-GES_DISC",
                "G1277917093-GES_DISC",
                "G1277917115-GES_DISC",
                "G1277917145-GES_DISC",
                "G1277917059-GES_DISC",
                "G1277917090-GES_DISC",
                "G1277917096-GES_DISC",
                "G1277917110-GES_DISC",
                "G1277917124-GES_DISC",
                "G1277917075-GES_DISC",
                "G1277917094-GES_DISC",
                "G1277917160-GES_DISC",
                "G1277917148-GES_DISC"
        };

        unsigned long  expected_size = 31;
        std::vector<unique_ptr<Granule>> granules;
        try {
            CmrApi cmr;
            string year ="1985";
            string month = "03";

            cmr.get_granules(collection_name, year, month, "",  granules);
            BESDEBUG(MODULE, prolog << "Checking expected size ("<< expected_size << ") vs received size (" << granules.size() << ")" << endl);
            CPPUNIT_ASSERT(expected_size == granules.size());

            stringstream msg;
            msg << prolog << "In the year " << year << ", month " << month <<  " the collection '" << collection_name << "' contains "
                    << granules.size() << " granules: " << endl;
            for (size_t i = 0; i < granules.size(); i++) {
                auto &granule = granules[i];
                msg << granule->getName() << endl
                    << "        size:  " << granule->getSizeStr() << endl
                    << "         lmt:    " << granule->getLastModifiedStr() << endl
                    << " granule_url: " << granule->getDataGranuleUrl() << endl;
            }
            BESDEBUG(MODULE, msg.str());

            for (size_t i = 0; i < granules.size(); i++) {
                auto &granule = granules[i];
                string pgi = granule->getId();
                msg.str(std::string());
                msg << prolog << "Checking:  expected: " << expected[i]
                        << " received: " << pgi;
                BESDEBUG(MODULE, msg.str() << endl);
                CPPUNIT_ASSERT(expected[i] == pgi);
            }

        }
        catch (BESError &be) {
            string msg = "Caught BESError! Message: " + be.get_message();
            cerr << endl << msg << endl;
            CPPUNIT_ASSERT(!"Caught BESError");
        }
    }

    void get_granules_data_access_urls_month_test() {
        string collection_name = "C1276812863-GES_DISC";

        vector<string> expected = {
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850301.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850302.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850303.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850304.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850305.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850306.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850307.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850308.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850309.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850310.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850311.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850312.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850313.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850314.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850315.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850316.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850317.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850318.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850319.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850320.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850321.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850322.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850323.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850324.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850325.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850326.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850327.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850328.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850329.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850330.nc4"),
                string("https://goldsmr4.gesdisc.eosdis.nasa.gov/data/MERRA2/M2T1NXSLV.5.12.4/1985/03/MERRA2_100.tavg1_2d_slv_Nx.19850331.nc4")
        };

        unsigned long  expected_size = 31;
        try {
            CmrApi cmr;
            std::vector<unique_ptr<Granule>> granules;
            string year ="1985";
            string month = "03";

            cmr.get_granules(collection_name, year, month, "",  granules);
            BESDEBUG(MODULE, prolog << "Checking expected size ("<< expected_size << ") vs received size (" << granules.size() << ")" << endl);
            CPPUNIT_ASSERT(expected_size == granules.size());

            stringstream msg;
            msg << prolog << "In the year " << year << ", month " << month <<  " the collection '" << collection_name << "' contains "
                    << granules.size() << " granules. Data Access URLs: " << endl;
            for (size_t i = 0; i < granules.size(); i++) {
                auto &granule = granules[i];
                msg << granule->getName() << endl
                    << "        size: " << granule->getSizeStr() << endl
                    << "         lmt: " << granule->getLastModifiedStr() << endl
                    << " granule_url: " << granule->getDataGranuleUrl() << endl;
            }
            BESDEBUG(MODULE, msg.str());

            for (size_t i = 0; i < granules.size(); i++) {
                auto &granule = granules[i];
                string granule_url = granule->getDataGranuleUrl();
                msg.str(std::string());
                msg << prolog << "Checking:  expected: " << expected[i]
                        << " received: " << granule_url;
                BESDEBUG(MODULE, msg.str() << endl);
                // CPPUNIT_ASSERT(expected[i] == url);
            }
        }
        catch (BESError &be) {
            string msg = "Caught BESError! Message: " + be.get_message();
            cerr << endl << msg << endl;
            CPPUNIT_ASSERT(!"Caught BESError");
        }
    }

    unsigned long gct_helper(string collection, string year, string month, string day){
        stringstream msg;
        CmrApi cmr;
        msg << prolog << "Checking granule count for " << collection << "[" << year << " / " << month << "/"  << day << "]";
        unsigned long granule_count = cmr.granule_count(collection,year,month,day);
        msg << "  granule_count:  " << granule_count << endl;
        BESDEBUG(MODULE, msg.str());
        return granule_count;
    }

    void granule_count_test() {
        stringstream msg;

        string collection;
        string year;
        string month;
        string day;

        unsigned int expected_granule_count;
        unsigned int granules_found;

        collection = "C1276812863-GES_DISC";
        year = "1985";
        month = "03";
        day = "";
        expected_granule_count = 31;
        granules_found = gct_helper(collection, year, month, day);
        if(debug) cerr << prolog << collection << "/" << year << (month.empty()?"":"/") << month << (day.empty()?"":"/") << day
                       << " returned: " << granules_found << " expected: " << expected_granule_count << endl;
        CPPUNIT_ASSERT(granules_found ==  expected_granule_count);

        collection = "C1276812863-GES_DISC";
        year = "1985";
        month = "";
        day = "";
        expected_granule_count = 365;
        granules_found = gct_helper(collection, year, month, day);
        if(debug) cerr << prolog << collection << "/" << year << (month.empty()?"":"/") << month << (day.empty()?"":"/") << day
                       << " returned: " << granules_found << " expected: " << expected_granule_count << endl;
        CPPUNIT_ASSERT(granules_found ==  expected_granule_count);

        collection = "C1276812822-GES_DISC";
        year = "2000";
        month = "";
        day = "";
        expected_granule_count = 366; // 2000 is a leap year
        granules_found = gct_helper(collection, year, month, day);
        if(debug) cerr << prolog << collection << "/" << year << (month.empty()?"":"/") << month << (day.empty()?"":"/") << day
                       << " returned: " << granules_found << " expected: " << expected_granule_count << endl;
        CPPUNIT_ASSERT(granules_found ==  expected_granule_count);
    }

    void get_providers_test() {
        stringstream msg;
        CmrApi cmr;

        vector<unique_ptr<Provider>> providers;
        cmr.get_providers( providers);
        DBG(cerr << "Found " << providers.size() << " Provider records." << endl);
        for(const auto &provider:providers){
            DBG(cerr << prolog << "          ProviderId: " << provider->id() << endl);
        }
    }

    void get_opendap_providers_test() {
        stringstream msg;
        CmrApi cmr;
        std::map<string, std::unique_ptr<cmr::Provider>> providers;

        cmr.get_opendap_providers(providers);

        if(debug){
            for (auto itr = providers.begin(); itr != providers.end(); itr++){
                cerr << prolog << "# ProviderId: " << itr->second->id() << " ";
                cerr << "(OPeNDAP Collections: " << itr->second->get_opendap_collection_count() << ")" << endl;
            }
        }
    }

    void get_opendap_collections_test() {
        stringstream msg;
        CmrApi cmr;
        string provider_id("GES_DISC");
        std::map<string, std::unique_ptr<cmr::Collection>> collections;

        cmr.get_opendap_collections(provider_id, collections);

        DBG(cerr << prolog << "Got " << collections.size() << " Collections" << endl);
        CPPUNIT_ASSERT_MESSAGE("Should get at least one collection", !collections.empty());

        if (debug2) {
            for (auto &collection: collections) cerr << collection.second->to_string() << endl;
        }
    }

    CPPUNIT_TEST_SUITE( CmrApiTest );
    CPPUNIT_TEST(get_providers_test);
    CPPUNIT_TEST(get_opendap_providers_test);
    CPPUNIT_TEST(get_opendap_collections_test);
    CPPUNIT_TEST(get_years_test);
    CPPUNIT_TEST(get_months_test);
    CPPUNIT_TEST(get_days_test);
    CPPUNIT_TEST(get_granule_ids_day_test);
    CPPUNIT_TEST(get_granule_ids_month_test);
    CPPUNIT_TEST(get_granules_month_test);
    CPPUNIT_TEST(get_granules_data_access_urls_month_test);
    CPPUNIT_TEST(granule_count_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CmrApiTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    return bes_run_tests<cmr::CmrApiTest>(argc, argv, "cerr,cmr,timing") ? EXIT_SUCCESS : EXIT_FAILURE;
}
