#!/bin/sh
#
# Upload the results of tests after running a build on Travis

LOG_FILE_TGZ=bes-autotest-${TRAVIS_JOB_NUMBER}-logs.tar.gz
S3_BUCKET="s3://opendap.travis.tests"

if test "$BES_BUILD" = main -o "$BES_BUILD" = distcheck -o "$BES_BUILD" = "docker-el8" -o "$BES_BUILD" = "docker-el9"
then
  echo "Packaging log files for '$BES_BUILD'"

  # Create a tarball for non-docker builds from local files - kln 6/23/26
  if ! test -f /tmp/${LOG_FILE_TGZ}; then
    echo "No tarball found, creating a new tarball of test logs"
    tar -czf /tmp/${LOG_FILE_TGZ} $(find . \( -name "*.log" -o -name "*site_map.txt" \) -print)
  fi

  # using: 'test -z "$AWS_ACCESS_KEY_ID" || ...' keeps after_script from running
  # the aws cli for forked PRs (where secure env vars are null). I could've used
  # an 'if' to block out the whole script, but I didn't... jhrg 3/21/18
  test -z "$AWS_ACCESS_KEY_ID" || aws s3 cp /tmp/${LOG_FILE_TGZ} ${S3_BUCKET}
fi

# A quick hack to get the gcovr report to S3. jhrg 4/20/23
if test x$BES_BUILD = xsonar-bes-framework
then
	# using: 'test -z "$AWS_ACCESS_KEY_ID" || ...' keeps after_script from running
	# the aws cli for forked PRs (where secure env vars are null). I could've used
	# an 'if' to block out the whole script, but I didn't... jhrg 3/21/18
	test -z "$AWS_ACCESS_KEY_ID" || aws s3 cp ./gcovr_report.txt ${S3_BUCKET}/bes-gcov-${TRAVIS_JOB_NUMBER}.txt
fi
