#!/usr/bin/env bash
#
# Reformat all of the files that are part of the git repo that
# match a given pattern using the repo's .clang-format file.

git  ls-files '*.[ch]pp' '*.cc' '*.h' '*.hh' \
  | xargs -n 50 -P "$(sysctl -n hw.ncpu)" clang-format -i --style=file
