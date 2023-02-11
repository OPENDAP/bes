#!/bin/bash

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#
# Figure out the new ChangeLog content (if any)
#
function get_change_log_update_text() {
    echo "#--------------------------------------------------" >&2
    echo "# BEGIN: get_change_log_update_text()" >&2

    local bes_version_numbers=${1:-"NO_VERSION_FOUND"}
    echo "# bes_version_numbers: ${bes_version_numbers}" >&2

    #local hyrax_version_numbers=${2:-"1.16.8-0"}
    #echo "# bes_version_numbers: ${bes_version_numbers}" >&2

    local most_recent=$(grep -e "[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]" ChangeLog | head -1 | awk '{print $1;}')
    echo "# most_recent: ${most_recent}" >&2


    local update_text=$(./travis/gitlog-to-changelog --since="${most_recent}" )

    # Make the log lines 72 chars max.
    update_text=$(echo "${update_text}" | fold -s -w 72 | awk '{if(!match($0,"^[0-9]|^[\\t]")){printf("\t%s\n",$0);}else{print $0;}}' - )
    echo "#--------------------------------------------------" >&2
    echo "# Checking for duplicates" >&2
    local dups=$( echo "${update_text}" | grep -n "${most_recent}";)
    if test -n "${dups}"
    then
        echo "# Processing duplicates." >&2
        # Find most recent dup.
        local last_entry=$( echo "${dups}" | head -n 1 | awk '{split($1,a,":");print a[1]-1;}' ;)
        echo "# last_entry: ${last_entry}" >&2
        if test ${last_entry} -ne 0;
        then
            # Trim the update so that it ends before the most recent duplicate.
            update_text=$(echo "${update_text}" | head -n ${last_entry})
        else
            update_text=
        fi
    else
        echo "# No duplicates found." >&2
    fi
    echo "#--------------------------------------------------" >&2
    echo "# update_text: " >&2
    echo "${update_text}" | awk '{print "##    "$0;}' >&2

    if test -n "${update_text}"
    then
        echo "## -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
        echo "##"
        echo "##  bes-${bes_version_numbers}"
        #echo "##  hyrax-${hyrax_version_numbers}"
        echo "##"
        echo ""
        echo "${update_text}"
        echo ""
    fi
}

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#  main()
#
#  * Gets the ChangeLog update from "get_change_log_update_text BES_VERSION"
#  * If there is a non empty update then the ChangeLog file is updated.
#  * Commits the changes to git on the current branch with a [skip ci] message.
#  * Pushes the result to GitHub.
#
function main() {
    echo "##################################################################################################" >&2
    echo "#" >&2
    echo "# BEGIN: $0" >&2
    echo "#" >&2
    #
    export TRAVIS_BRANCH=${TRAVIS_BRANCH:-$(git rev-parse --abbrev-ref HEAD)}
    echo "#    TRAVIS_BRANCH: ${TRAVIS_BRANCH}"
    #
    export BES_BUILD_NUMBER=${BES_BUILD_NUMBER:-"<TEST>"}
    echo "# BES_BUILD_NUMBER: ${BES_BUILD_NUMBER}" >&2
    #
    export BES_VERSION=$(cat ./bes_VERSION)
    BES_VERSION="${BES_VERSION}-${BES_BUILD_NUMBER}"
    echo "#      BES_VERSION: ${BES_VERSION}"
    echo "#" >&2


    change_log_update=$(get_change_log_update_text "${BES_VERSION}";)
    if test -n "${change_log_update}"
    then
        tmp_file=$(mktemp)
        echo "${change_log_update}" > "${tmp_file}"
        cat ChangeLog >> "${tmp_file}"
        mv "${tmp_file}" ChangeLog
        echo "# ChangeLog:"
        echo ChangeLog | awk '{print "##    "$0;}'
        echo "#"
        echo "# Committing and pushing the new ChangeLog."
        echo "#"
        git checkout "${TRAVIS_BRANCH}"
        git commit -m "TheRobotTravis updated ChangeLog [skip ci]" ChangeLog
        git push
    fi
    echo "##################################################################################################" >&2
#    echo "# Committing and pushing the new ChangeLog."

}

main