#!/usr/bin/env bash
#
# Reformat all of the files that are part of the git repo that
# match a given pattern using the repo's .clang-format file.

ROOT=/Users/jimg/src/opendap/hyrax/bes/
SDK=$(xcrun --show-sdk-path)

pattern="${1:-'*.[ch]pp *.cc *.h *.hh'}"
test_to_run="${2:-'-*,-modernize-use-override'}"

echo "Files: $pattern"
echo "Tests: $test_to_run"

# xargs -n 50 -P "$(sysctl -n hw.ncpu)"
git  ls-files $pattern \
  | xargs -n 10 \
  clang-tidy -p . -header-filter='.*' -checks="$test_to_run" -fix \
    -extra-arg=-std=c++14 \
    -extra-arg=-stdlib=libc++ \
    -extra-arg=-isysroot -extra-arg="$SDK" \
    -extra-arg=-I"$SDK/usr/include/c++/v1" \
    -extra-arg=-I"$ROOT/dap" \
    -extra-arg=-I"$ROOT/xmlcommand" \
    -extra-arg=-I"/usr/local/include"
