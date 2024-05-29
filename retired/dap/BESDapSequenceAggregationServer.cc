// BESDapNullAggregationServer.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2015 OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <iostream>
#include <typeinfo>

#include <libdap/DDS.h>
#include <libdap/Structure.h>
#include <libdap/Sequence.h>
#include <libdap/ConstraintEvaluator.h>
#include <libdap/mime_util.h>

#include "BESResponseHandler.h"
#include "BESDataHandlerInterface.h"

#include "BESDASResponse.h"
#include "BESDDSResponse.h"
#include "BESDataDDSResponse.h"

#include "BESDataNames.h"       // for POST_CONSTRAINT
#include "BESSyntaxUserError.h"

#include "BESDebug.h"

#include "BESDapSequenceAggregationServer.h"

using namespace libdap;

/** @brief Look at the DDS; can it be aggregated by the Sequence Aggregation handler?
 *
 * To be aggregated by the BESDapSequenceAggregationServer (aka sequence.aggregation)
 * the dataset must contain N (N >=1) Structures each of which hold one Sequence and
 * each of the Sequences must have the same columns (number, type and name) and all
 * of those columns must be simple types (No arrays, structures or sequences). In short,
 * the contained sequences must be simple tables of values that can be 'stacked' on
 * top of each other.
 *
 * @param response Test the DDS from this BESDDSResponse object
 * @param names_must_match True if the column names have to match (defaults to true)
 * @return True if the DDS can be aggregated, otherwise false
 */
bool
BESDapSequenceAggregationServer::test_dds_for_suitability(DDS *dds, bool names_must_match)
{
    // DDS *dds = response->get_dds();
    Sequence *proto;    // Will hold the first sequence; use to test uniformity
    for (DDS::Vars_iter i = dds->var_begin(), e = dds->var_end(); i != e; ++i) {
        if ((*i)->type() != dods_structure_c)
            return false;
        Structure *container = static_cast<Structure*>(*i);
        if (container->element_count() != 1)
            return false;
        if (container->get_var_index(0)->type() != dods_sequence_c)
            return false;

        // Now test for uniform Sequences
        Sequence *s = static_cast<Sequence*>(container->get_var_index(0));
        if (i == dds->var_begin()) {    // The first sequence is the prototype
            proto = s;
            for (Sequence::Vars_iter pi = proto->var_begin(), pe = proto->var_end(); pi != pe; ++pi) {
                if (!(*pi)->is_simple_type())
                    return false;
            }
        }
        else {
            if (proto->element_count() != s->element_count())
                return false;

            Sequence::Vars_iter si = s->var_begin(), se = s->var_end();
            for (Sequence::Vars_iter pi = proto->var_begin(), pe = proto->var_end(); pi != pe && si != se; ++pi, ++si) {
                if ((*pi)->type() != (*si)->type())
                    return false;
                if (names_must_match && ((*pi)->name() != (*si)->name()))
                    return false;
            }
        }
    }

    return true;
}

/** @brief Read data values into a collection of Sequences
 *
 * Read data values into the Sequence instances in a DDS when those
 * Sequences are to be aggregated. The values are added to the sequence
 * instances (the 'source' parameter is modified).
 *
 * @note This method is not needed for Sequences that already hold their
 * values, but for sequences that must read values, it must be called.
 *
 * @param source The DDS. This can be the DDS with containers.
 * @param evaluator Use this ConstraintEvaluator instance with reading in
 * data. This is needed by DAP2 because it holds the selection part of the
 * DAP2 constraint.
 */
void
BESDapSequenceAggregationServer::intern_all_data(DataDDS *source, ConstraintEvaluator &evaluator)
{
    for (DDS::Vars_iter i = source->var_begin(), e = source->var_end(); i != e; ++i) {
        (*i)->intern_data(evaluator, *source);
    }
}

/** @brief Aggregate sequences and return a new DDS
 * This method assumes that the DDS is suitable for this operation.
 *
 * @todo Since we have to have  a separate DataDDS method, trim this
 * of its unneeded cruft.
 *
 * @param source The DDS with multiple Sequences, suitable for aggregation.
 * @return A new DDS with a single Sequence.
 */
auto_ptr<DDS>
BESDapSequenceAggregationServer::build_new_dds(DDS *source)
{
    auto_ptr<DDS> dest(new DDS(0));

    Sequence *proto = 0;

    for (DDS::Vars_iter i = source->var_begin(), e = source->var_end(); i != e; ++i) {
        Structure *container = static_cast<Structure*>(*i);

        Sequence *s = static_cast<Sequence*>(container->get_var_index(0));
        if (i == source->var_begin()) {
            // Because this line copies the values of the first sequence,
            // a minor optimization is to append the subsequent sequence's
            // values to it.
            proto = new Sequence(*s);   // copies the SequenceValues object
        }
#if 0
        else {
            SequenceValues &source = s->value_ref();         // vector<BaseTypeRow *>
            SequenceValues &dest = proto->value_ref();

            // Copy the BaseType objects used to hold values.
            for (vector<BaseTypeRow *>::iterator rows_i = source.begin(); rows_i != source.end(); ++rows_i) {
                // Get the current BaseType Row
                BaseTypeRow *src_bt_row_ptr = *rows_i;
                // Create a new row.
                BaseTypeRow *dest_bt_row_ptr = new BaseTypeRow;
                // Copy the BaseType objects from a row to new BaseType objects.
                // Push new BaseType objects onto new row.
                for (BaseTypeRow::iterator bt_row_iter = src_bt_row_ptr->begin(); bt_row_iter != src_bt_row_ptr->end();
                        ++bt_row_iter) {
                    BaseType *src_bt_ptr = *bt_row_iter;
                    BaseType *dest_bt_ptr = src_bt_ptr->ptr_duplicate();
                    dest_bt_row_ptr->push_back(dest_bt_ptr);
                }
                // Push new row onto d_values.
                dest.push_back(dest_bt_row_ptr);
            }
        }
#endif
    }

    dest->add_var_nocopy(proto);

    return dest;
}

auto_ptr<DataDDS>
BESDapSequenceAggregationServer::build_new_dds(DataDDS *source)
{
    auto_ptr<DataDDS> dest(new DataDDS(0));

    Sequence *proto = 0;

    for (DDS::Vars_iter i = source->var_begin(), e = source->var_end(); i != e; ++i) {
        Structure *container = static_cast<Structure*>(*i);

        Sequence *s = static_cast<Sequence*>(container->get_var_index(0));
        if (i == source->var_begin()) {
            // Because this line copies the values of the first sequence,
            // a minor optimization is to append the subsequent sequence's
            // values to it. Note that using ptr_duplicate() means that
            // if 's' points to a specialization of Sequence, we get that
            // and not a plain instance of Sequence. That's important because
            // TabularSequence, in particular, and TestSequence, each define
            // specializations of the code that serializes values to recognize
            // when a Sequence instance _already_ holds data values.
            proto = static_cast<Sequence*>(s->ptr_duplicate());   // copies the SequenceValues object
        }
        else {
            SequenceValues &source = s->value_ref();         // vector<BaseTypeRow *>
            SequenceValues &dest = proto->value_ref();

            // Copy the BaseType objects used to hold values.
            for (vector<BaseTypeRow *>::iterator rows_i = source.begin(); rows_i != source.end(); ++rows_i) {
                // Get the current BaseType Row
                BaseTypeRow *src_bt_row_ptr = *rows_i;
                // Create a new row.
                BaseTypeRow *dest_bt_row_ptr = new BaseTypeRow;
                // Copy the BaseType objects from a row to new BaseType objects.
                // Push new BaseType objects onto new row.
                for (BaseTypeRow::iterator bt_row_iter = src_bt_row_ptr->begin(); bt_row_iter != src_bt_row_ptr->end();
                        ++bt_row_iter) {
                    BaseType *src_bt_ptr = *bt_row_iter;
                    BaseType *dest_bt_ptr = src_bt_ptr->ptr_duplicate();
                    dest_bt_row_ptr->push_back(dest_bt_ptr);
                }
                // Push new row onto d_values.
                dest.push_back(dest_bt_row_ptr);
            }
        }
    }

    dest->add_var_nocopy(proto);

    return dest;
}

/** @brief Aggregate the Sequences in one or more containers
 *
 * Assume this is called as part of a <define> that has a number of containers.
 * Each container is represented as a Structure and within that is a single
 * Sequence. Each 'container's Sequence' has columns with the same number, type
 * and name along with all of its values. Build an 'aggregated sequence' that
 * uses those columns and the concatenation of all of the values.
 * @param dhi
 */
void BESDapSequenceAggregationServer::aggregate(BESDataHandlerInterface &dhi)
{
    BESDEBUG("sequence.aggregation", "In BESDapSequenceAggregationServer::aggregate" << endl);

    // Print out the dhi info
    BESDEBUG("sa3", "DHI: " << endl);
    if (BESDebug::IsSet("sa3")) dhi.dump(*(BESDebug::GetStrm()));

    // Get the response object
    BESResponseObject *response = dhi.response_handler->get_response_object() ;

    BESDEBUG("sequence.aggregation", "typeid(response).name(): " << typeid(response).name() << endl);

    // This handler can be called for any type of response object
    // TODO Add DAP4 support!
    if (dynamic_cast<BESDASResponse*>(response)) { // typeid(response) == typeid(BESDASResponse*)) {
        // BESDASResponse *bdas = static_cast<BESDASResponse*>(response);
        throw BESInternalError("BESDapSequenceAggregationServer does not know how to aggregate DAS objects.", __FILE__,
                __LINE__);
    }
    else if (dynamic_cast<BESDDSResponse*>(response)) { // typeid(response) == typeid(BESDDSResponse*)) {
        BESDDSResponse *bdds = static_cast<BESDDSResponse*>(response);

        // Test that the structure of the DDS is valid for this AggregationServerHandler.
        if (!test_dds_for_suitability(bdds->get_dds()))
            throw BESSyntaxUserError("BESDapSequenceAggregationServer: The containers were not suitable for aggregation.", __FILE__, __LINE__);

        // Evaluate the CE here? FIXME jhrg 2/13/15

        auto_ptr<DDS> agg_dds = build_new_dds(bdds->get_dds());
        agg_dds->filename("none");
        string c1 = name_path((*dhi.containers.begin())->get_real_name());
        string c2 = name_path((*dhi.containers.rbegin())->get_real_name());
        agg_dds->set_dataset_name(c1 + "_to_" + c2);

        bdds->set_dds(agg_dds.get());
        agg_dds.release();

         dhi.data[POST_CONSTRAINT] = ""; // FIXME Hack, but this makes it work...
    }
    else if (dynamic_cast<BESDataDDSResponse*>(response)) { // typeid(response) == typeid(BESDataDDSResponse*)) {
        BESDataDDSResponse *bes_data_dds = static_cast<BESDataDDSResponse*>(response);
        // Test that the structure of the DDS is valid for this AggregationServerHandler.
        if (!test_dds_for_suitability(bes_data_dds->get_dds()))
            throw BESSyntaxUserError("BESDapSequenceAggregationServer: The containers were not suitable for aggregation.", __FILE__, __LINE__);

        bes_data_dds->get_dds()->mark_all(true);        // FIXME jhrg 2/13/15

        intern_all_data(bes_data_dds->get_dds(), bes_data_dds->get_ce());

        BESDEBUG("sa2", "DDS (before aggregation): " << endl);
        if (BESDebug::IsSet("sa2")) bes_data_dds->get_dds()->print_constrained(*(BESDebug::GetStrm()));

        auto_ptr<DataDDS> agg_data_dds = build_new_dds(bes_data_dds->get_dds());
        agg_data_dds->filename("none");
        string c1 = name_path((*dhi.containers.begin())->get_real_name());
        string c2 = name_path((*dhi.containers.rbegin())->get_real_name());
        agg_data_dds->set_dataset_name(c1 + "_to_" + c2);

        BESDEBUG("sa2", "DDS (after aggregation): " << endl);
        if (BESDebug::IsSet("sa2")) agg_data_dds->print_constrained(*(BESDebug::GetStrm()));

        BESDEBUG("sa2", "...and it's Sequence: " << endl);
        if (BESDebug::IsSet("sa2")) dynamic_cast<Sequence&>(*(agg_data_dds->get_var_index(0))).print_val_by_rows(*(BESDebug::GetStrm()));

        bes_data_dds->set_dds(agg_data_dds.get());
        agg_data_dds.release();

        dhi.data[POST_CONSTRAINT] = ""; // FIXME Hack
    }
    else {
        throw BESInternalError("BESDapSequenceAggregationServer only works for DAP2 response objects.", __FILE__, __LINE__);
    }
}

void BESDapSequenceAggregationServer::dump(ostream &strm) const
{
    BESAggregationServer::dump(strm);
}
