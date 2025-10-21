#!/usr/bin/env bash
#
# Fairly raw script to update code for files listed in a compilation_commands.json
# file. Look for all the things that use 'virtual ... method();' which _should_
# use '... method() override;' and edit them in place. This is tuned for OSX and
# its needs and assumes: 1. that you have built the json file, 2. you have the
# llvm package from brew that has clang-tidy, 3. you edit this to make the paths
# correct as needed. jhrg 10/21/25

ROOT="/Users/jimg/src/opendap/hyrax_git"
SDK=$(xcrun --show-sdk-path)

run-clang-tidy -p . \
  -checks='-*,modernize-use-override' -fix \
  -header-filter="^${ROOT}/.*" \
  --extra-arg=-std=c++17 \
  --extra-arg=-stdlib=libc++ \
  --extra-arg=-isysroot --extra-arg="$SDK" \
  --extra-arg=-I"$SDK/usr/include/c++/v1"
