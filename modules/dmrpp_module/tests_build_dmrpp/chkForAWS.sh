#!/bin/bash
#
# Checks to see if the AWS CLI is present and configured with credentials.
#
aws configure list > /dev/null
ret=$?
if test $ret != 0 ; then
    echo "The AWS CLI is not available. AWS tests will not be run."
    exit $ret;
fi
aws configure list | grep "access_key" | grep -v "<not set>" > /dev/null
ret=$?
if test $ret != 0 ; then
    echo "The AWS_ACCESS_KEY_ID is not set. AWS tests will not be run."
    exit $ret;
fi
aws configure list | grep "secret_key" | grep -v "<not set>" > /dev/null
ret=$?
if test $ret != 0 ; then
    echo "The AWS_SECRET_ACCESS_KEY is not set. AWS tests will not be run."
    exit $ret;
fi
aws configure list | grep "region" | grep -v "<not set>" > /dev/null
ret=$?
if test $ret != 0 ; then
    echo "The AWS_DEFAULT_REGION is not set."
    exit 0;
fi

