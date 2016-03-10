

#include "DapFunctionResultPromoter.h"
#include "Structure.h"
#include "BESDebug.h"
#include "BESUtil.h"





#if 0
/**
 * For an Structure in the given DDS, if that Structure's name ends with
 * "_unwrap" take each variable from the structure and 'promote it to the
 * top level of the DDS. This function deletes the DDS passed to it. The
 * caller is responsible for deleting the returned DDS.
 *
 * @note Here we remove top-level Structure variables that have been added by server
 * functions which return multiple values. This will necessarily be a hack since
 * DAP2 was never designed to do this sort of thing - support functions that
 * return computed values.
 *
 * @note The DDS referenced by 'fdds' may have one or more variables because there may
 * have been one or more function calls given in the CE supplied by the CE. For
 * example, "linear_scale(SST),linear_scale(AIRT)" would have two variables. It is
 * possible for a CE that contains function calls to also include a 'regular'
 * projection (i.e., "linear_scale(SST),SST[0][0:179]") but this means run the function(s)
 * and build a new DDS and then apply the remaining constraints to that new DDS. So,
 * this (hack) code can assume that there is one variable per function call and no
 * other variables. Furthermore, lets adopt a simple convention that functions use
 * a Structure named <something>_unwrap when they want the function result to be
 * unwrapped and something else when they want this code to leave the Structure as
 * it is.
 *
 * @param fdds The source DDS - look for Structures here
 * @return A new DDS with new instances such that the Structures named
 * *_unwrap have been removed and their members 'promoted' up to the new
 * DDS's top level scope.
 */
libdap::DDS *DapFunctionResultPromoter::promote_function_output_structures(libdap::DDS *fdds)
{
    // Look in the top level of the DDS for a promotable member - i.e. a member
    // variable that is a collection and whose name ends with "_unwrap"
    bool found_promotable_member = false;
    for (libdap::DDS::Vars_citer di = fdds->var_begin(), de = fdds->var_end(); di != de && !found_promotable_member; ++di) {
        libdap::Structure *collection = dynamic_cast<libdap::Structure *>(*di);
        if (collection && BESUtil::endsWith(collection->name(), "_unwrap")) {
            found_promotable_member = true;
            BESDEBUG("func", "DapFunctionResultPromoter::promote_function_output_structures() - Found promotable member variable in the DDS: " << collection->name() << endl);
        }
    }

    // If we found one or more promotable member variables, promote them.
    if (found_promotable_member) {

        // Dump pointers to the values here temporarily... If we had methods in libdap
        // that could be used to access the underlying erase() and insert() methods, we
        // could skip the (maybe expensive) copy operations I use below. What we would
        // need are ways to delete a Structure/Constructor without calling delete on its
        // fields and ways to call vector::erase() and vector::insert(). Some of this
        // exists, but it's not quite enough.

        libdap::DDS *temp_dds = new libdap::DDS(fdds->get_factory(), fdds->get_dataset_name(), fdds->get_dap_version());
        for (libdap::DDS::Vars_citer di = fdds->var_begin(), de = fdds->var_end(); di != de; ++di) {
            libdap::Structure *collection = dynamic_cast<libdap::Structure *>(*di);
            if (collection && BESUtil::endsWith(collection->name(), "_unwrap")) {
                BESDEBUG("func", "DapFunctionResultPromoter::promote_function_output_structures() - Promoting members of collection '" << collection->name() << "'" << endl);
               // So we're going to 'flatten this structure' and return its fields
                libdap::Structure::Vars_iter vi;
                for (vi =collection->var_begin(); vi != collection->var_end(); ++vi) {
                    BESDEBUG("func", "DapFunctionResultPromoter::promote_function_output_structures() - Promoting variable '" << (*vi)->name() << "' ptr: " << *vi << endl);
                    temp_dds->add_var(*vi); // better to use add_var_nocopy(*vi); need to modify libdap?
                }
            }
            else {
                temp_dds->add_var(*di);
            }
        }
        delete fdds;
        return temp_dds;
    }
    else {
        BESDEBUG("func", "DapFunctionResultPromoter::promote_function_output_structures() - Nothing in DDS to promote." << endl);
        // Otherwise do nothing to alter the DDS
        return fdds;
    }
}
#endif


#if 1
/**
 * For an Structure in the given DDS, if that Structure's name ends with
 * "_unwrap" take each variable from the structure and 'promote it to the
 * top level of the DDS. !!This function deletes the DDS passed to it. The
 * caller is responsible for deleting the returned DDS.!!
 *
 * @note Here we remove top-level Structure variables that have been added by server
 * functions which return multiple values. This will necessarily be a hack since
 * DAP2 was never designed to do this sort of thing - support functions that
 * return computed values.
 *
 * @note The DDS referenced by 'fdds' may have one or more variables because there may
 * have been one or more function calls given in the CE supplied by the CE. For
 * example, "linear_scale(SST),linear_scale(AIRT)" would have two variables. It is
 * possible for a CE that contains function calls to also include a 'regular'
 * projection (i.e., "linear_scale(SST),SST[0][0:179]") but this means run the function(s)
 * and build a new DDS and then apply the remaining constraints to that new DDS. So,
 * this (hack) code can assume that there is one variable per function call and no
 * other variables. Furthermore, lets adopt a simple convention that functions use
 * a Structure named <something>_unwrap when they want the function result to be
 * unwrapped and something else when they want this code to leave the Structure as
 * it is.
 *
 * @param fdds The source DDS - look for Structures here
 * @return A new DDS with new instances such that the Structures named
 * *_unwrap have been removed and their members 'promoted' up to the new
 * DDS's top level scope.
 */
libdap::DDS *DapFunctionResultPromoter::promote_function_output_structures(libdap::DDS *fdds)
{
    // Look in the top level of the DDS for a promotable member - i.e. a member
    // variable that is a collection and whose name ends with "_unwrap"
    bool found_promotable_member = false;
    for (libdap::DDS::Vars_citer di = fdds->var_begin(), de = fdds->var_end(); di != de && !found_promotable_member; ++di) {
        libdap::Structure *collection = dynamic_cast<libdap::Structure *>(*di);
        if (collection && BESUtil::endsWith(collection->name(), "_unwrap")) {
            found_promotable_member = true;
            BESDEBUG("func", "DapFunctionResultPromoter::promote_function_output_structures() - Found promotable member variable in the DDS: " << collection->name() << endl);
        }
    }

    // If we found one or more promotable member variables, promote them.
    if (found_promotable_member) {

        // Dump pointers to the values here temporarily... If we had methods in libdap
        // that could be used to access the underlying erase() and insert() methods, we
        // could skip the (maybe expensive) copy operations I use below. What we would
        // need are ways to delete a Structure/Constructor without calling delete on its
        // fields and ways to call vector::erase() and vector::insert(). Some of this
        // exists, but it's not quite enough.
        //
        // Proposal: Make it so that the Structure/Constructor has a release() method
        // that allows you take the members out of the instance with the contract
        // that "The released members will be deleted deleted so that the
        // Structure/Constructor object only need to drop it's reference to them".
        // With that in place we might then be able to manipulate the DAP objects
        // without excessive copying. We can use add_var_nocopy() to put things in,
        // and we can use release() to pull things out.
        //
        // Assumption: add_var_nocopy() has the contract that the container to which
        // the var is added will eventually delete the var.


        libdap::DDS *temp_dds = new libdap::DDS(fdds->get_factory(), fdds->get_dataset_name(), fdds->get_dap_version());

        std::vector<libdap::BaseType *> upVars;
        std::vector<libdap::BaseType *> droppedContainers;
        for (libdap::DDS::Vars_citer di = fdds->var_begin(), de = fdds->var_end(); di != de; ++di) {

            libdap::Structure *collection = dynamic_cast<libdap::Structure *>(*di);

            if (collection && BESUtil::endsWith(collection->name(), "_unwrap")) {

                // Once we promote the members we need to drop the parent collection
                // but we can't do that while we're iterating over its contents
                // so we'll cache the reference for later.
                droppedContainers.push_back(collection);

                BESDEBUG("func", "DapFunctionResultPromoter::promote_function_output_structures() - Promoting members of collection '" << collection->name() << "'" << endl);

                // We're going to 'flatten this structure' and return its fields
                libdap::Structure::Vars_iter vi;
                for (vi =collection->var_begin(); vi != collection->var_end(); ++vi) {
                    BESDEBUG("func", "DapFunctionResultPromoter::promote_function_output_structures() - Promoting variable '" << (*vi)->name() << "' ptr: " << *vi << endl);

                    libdap::BaseType *origVar = *vi;

                    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    // This performs a deep copy on origVar (ouch!), and we do it because in the current
                    // libdap API, when we delete parent structure the variable will be deleted too.
                    // Because we can't pluck a variable out of a DAP object without deleting it.
                    // @TODO Fix the libdap API to allow this operation without the copy/delete bits.
                    libdap::BaseType *newVar = origVar->ptr_duplicate();
                    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                    // This ensures that the variable's semantics are consistent
                    // with a top level variable.
                    newVar->set_parent(0);

                    // Add the new variable to the list of stuff to add back to the dataset.
                    upVars.push_back(newVar);
                }
            }
        }
        // Drop Promoted Containers

        for(std::vector<libdap::BaseType *>::iterator it=droppedContainers.begin(); it != droppedContainers.end(); ++it) {
            libdap::BaseType *bt = *it;
            BESDEBUG("func", "DapFunctionResultPromoter::promote_function_output_structures() - Deleting Promoted Collection '" << bt->name() << "' ptr: " << bt << endl);

            // Delete the Container variable and ALL of it's children.
            // @TODO Wouldn't it be nice if at this point it had no children? I think so too.
            fdds->del_var(bt->name());
        }

        // Add (copied) promoted variables to top-level of DDS
        for( std::vector<libdap::BaseType *>::iterator it = upVars.begin(); it != upVars.end(); it ++) {
            libdap::BaseType *bt = *it;
            BESDEBUG("func", "DapFunctionResultPromoter::promote_function_output_structures() - Adding Promoted Variable '" << bt->name() << "' to DDS. ptr: " << bt << endl);
            fdds->add_var(bt);
        }
    }
    else {
        BESDEBUG("func", "DapFunctionResultPromoter::promote_function_output_structures() - Nothing in DDS to promote." << endl);
        // Otherwise do nothing to alter the DDS
    }


    return fdds;
}
#endif
