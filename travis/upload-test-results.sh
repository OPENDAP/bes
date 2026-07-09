#!/bin/bash
#
# Upload the results of tests after running a build on Travis
function loggy() {
    echo  "$@" | awk '{ print "# upload-test-result() - "$0;}'  >&2
}
HR="^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
HR1="--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---"

LOG_FILE_TGZ="/tmp/bes-autotest-${TRAVIS_JOB_NUMBER}-logs.tar.gz"
S3_BUCKET="s3://opendap.travis.tests"
TEST_LOGS_DIR="/home/bes_user/bes-test-logs"

loggy "$HR"
loggy "BEGIN"

if test "$BES_BUILD" = main -o "$BES_BUILD" = distcheck -o "$BES_BUILD" = "docker-el8" -o "$BES_BUILD" = "docker-el9"
then
  loggy "$HR1"
  loggy "Packaging log files for '$BES_BUILD'"
  loggy ""
  loggy "Checking test logs on host."
  loggy "$(ls -l /tmp)"
  loggy "$HR1"
  loggy "Checking test logs in Docker image: $SNAPSHOT_IMAGE_TAG, $TEST_LOGS_DIR"
  loggy "$(docker run --rm "${SNAPSHOT_IMAGE_TAG}" -c "ls -l $TEST_LOGS_DIR")"
  loggy "$HR1"

  # Create a tarball for non-docker builds from local files - kln 6/23/26
  if test -f "$LOG_FILE_TGZ"; then
    # using: 'test -z "$AWS_ACCESS_KEY_ID" || ...' keeps after_script from running
    # the aws cli for forked PRs (where secure env vars are null). I could've used
    # an 'if' to block out the whole script, but I didn't... jhrg 3/21/18
    # test -z "$AWS_ACCESS_KEY_ID" || aws s3 cp "${LOG_FILE_TGZ}" "${S3_BUCKET}"
    if [ -n "$AWS_ACCESS_KEY_ID" ]; then
        aws s3 cp "${LOG_FILE_TGZ}" "${S3_BUCKET}"
    else
      loggy "Skipping upload to S3, no AWS credentials were found."
    fi
  else
    loggy "ERROR - The test logs tarball ($LOG_FILE_TGZ) was not found."
  fi
else
  loggy "No logs will uploaded for build: '$BES_BUILD'"
fi

# A quick hack to get the gcovr report to S3. jhrg 4/20/23
if test "$BES_BUILD" = "xsonar-bes-framework"
then
	# using: 'test -z "$AWS_ACCESS_KEY_ID" || ...' keeps after_script from running
	# the aws cli for forked PRs (where secure env vars are null). I could've used
	# an 'if' to block out the whole script, but I didn't... jhrg 3/21/18
	test -z "$AWS_ACCESS_KEY_ID" || aws s3 cp ./gcovr_report.txt "${S3_BUCKET}/bes-gcov-${TRAVIS_JOB_NUMBER}.txt"
fi

loggy "END"
loggy "$HR"
