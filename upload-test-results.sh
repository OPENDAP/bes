#!/bin/sh
#
# Upload the results of tests after running a build on Travis

LOG_FILE_TGZ=bes-autotest-${TRAVIS_JOB_NUMBER}-logs.tar.gz
if test -n $BES_BUILD -a ( test $BES_BUILD = main -o test $BES_BUILD = distcheck )
then
	tar -czf /tmp/${LOG_FILE_TGZ} `find . -name timing -prune -o -name '*.log' -print -o -name '*site_map.txt' -print`
	
	# using: 'test -z "$AWS_ACCESS_KEY_ID" || ...' keeps after_script from running
	# the aws cli for forked PRs (where secure env vars are null). I could've used
	# an 'if' to block out the whole script, but I didn't... jhrg 3/21/18
	test -z "$AWS_ACCESS_KEY_ID" || aws s3 cp /tmp/${LOG_FILE_TGZ} s3://opendap.travis.tests/
fi
