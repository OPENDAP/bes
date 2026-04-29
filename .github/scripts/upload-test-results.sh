#!/bin/sh
#
# Upload the results of failed GitHub Actions tests to S3.

set -e

RUN_ID=${GITHUB_RUN_ID:-unknown-run}
RUN_ATTEMPT=${GITHUB_RUN_ATTEMPT:-1}
JOB_NAME=${GITHUB_JOB:-unknown-job}
RUNNER_LABEL=${GHA_RUNNER_LABEL:-unknown-runner}

LOG_FILE_TGZ=bes-autotest-gha-${RUN_ID}-${RUN_ATTEMPT}-${JOB_NAME}-${RUNNER_LABEL}-logs.tar.gz
LOG_FILE_TGZ=$(printf '%s' "$LOG_FILE_TGZ" | tr '/ ' '__')
LOG_FILE_PATH=/tmp/${LOG_FILE_TGZ}
S3_URI=s3://opendap.github.actions.test/${LOG_FILE_TGZ}

if test -z "$AWS_ACCESS_KEY_ID"
then
    echo "AWS_ACCESS_KEY_ID is not set; skipping test log upload."
    exit 0
fi

if ! find . -name timing -prune -o -name '*.log' -print -o -name '*site_map.txt' -print | grep -q .
then
    echo "No test log files found to upload."
    exit 0
fi

echo "Packaging GitHub Actions test logs into ${LOG_FILE_PATH}"
tar -czf "${LOG_FILE_PATH}" `find . -name timing -prune -o -name '*.log' -print -o -name '*site_map.txt' -print`

echo "Uploading ${LOG_FILE_PATH} to ${S3_URI}"
aws s3 cp "${LOG_FILE_PATH}" "${S3_URI}"
