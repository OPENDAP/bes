#!/bin/sh
#
# Upload the results of tests after running a build on Travis

LOG_FILE_TGZ=bes-autotest-${TRAVIS_JOB_NUMBER}-logs.tar.gz
if [[ "$BES_BUILD" = "main" || "$BES_BUILD" = "distcheck" || "$BES_BUILD" = "docker"* ]]
then
	tar -czf /tmp/${LOG_FILE_TGZ} `find . -name timing -prune -o -name '*.log' -print -o -name '*site_map.txt' -print`

	# using: 'test -z "$AWS_ACCESS_KEY_ID" || ...' keeps after_script from running
	# the aws cli for forked PRs (where secure env vars are null). I could've used
	# an 'if' to block out the whole script, but I didn't... jhrg 3/21/18

	test -z "$AWS_ACCESS_KEY_ID" || aws s3 cp /tmp/${LOG_FILE_TGZ} s3://opendap.travis.tests/
fi

# A quick hack to get the gcovr report to S3. jhrg 4/20/23
if test x$BES_BUILD = xsonar-bes-framework
then
	# using: 'test -z "$AWS_ACCESS_KEY_ID" || ...' keeps after_script from running
	# the aws cli for forked PRs (where secure env vars are null). I could've used
	# an 'if' to block out the whole script, but I didn't... jhrg 3/21/18
	test -z "$AWS_ACCESS_KEY_ID" || aws s3 cp ./gcovr_report.txt s3://opendap.travis.tests/bes-gcov-${TRAVIS_JOB_NUMBER}.txt
fi
