#!/bin/bash
#
# Checks to see if the AWS CLI is present and configured with credentials.
#
aws configure list | grep "access_key" | grep -v "<not set>" > /dev/null
ret=$?
if test $ret != 0 ; then
    echo "ERROR - The AWS_ACCESS_KEY_ID is not set."
    exit $ret;
fi
aws configure list | grep "secret_key" | grep -v "<not set>" > /dev/null
ret=$?
if test $ret != 0 ; then
    echo "ERROR - The AWS_SECRET_ACCESS_KEY is not set."
    exit $ret;
fi
aws configure list | grep "region" | grep -v "<not set>" > /dev/null
ret=$?
if test $ret != 0 ; then
    echo "WARNING - The AWS_DEFAULT_REGION is not set."
    exit 0;
fi

