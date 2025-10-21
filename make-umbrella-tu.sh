#!/usr/bin/env bash
#
# Generate a TU that includes every project header

mkdir -p tools
OUT=tools/tidy_headers_umbrella.cpp
echo "// generated for clang-tidy" > "$OUT"
git -C "$ROOT" ls-files '*.h' '*.hpp' '*.hh' '*.hxx' \
  | sed 's/^/#include "/; s/$/"/' >> "$OUT"

# Append a compile entry for that TU to your compile_commands.json
# (intercept-build or bear with --append so you keep the existing DB)
SDK=$(xcrun --show-sdk-path)
intercept-build --append \
  c++ -std=c++17 -stdlib=libc++ -isysroot "$SDK" -I"$SDK/usr/include/c++/v1" \
     -I"$ROOT/include" -I"$ROOT" \
     -c "$OUT" -o /tmp/tidy_headers_umbrella.o
