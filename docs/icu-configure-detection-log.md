# ICU configure detection log

## 2026-05-19 14:55:31 MDT

Prompt:

> I want the BES configure script to look fir the ICU library. Look at the configure.ac and icu.m4 and tell me why it's not finding the library on my OSX machine. I want this to work on Linux, too.

Reasoning and findings:

- `conf/icu.m4` was relying on `AX_PATH_GENERIC([icu], ...)`, which searches for `icu-config` on `PATH`.
- On this macOS machine, plain `which icu-config` fails, while Homebrew has ICU under `/opt/homebrew/opt/icu4c@78`.
- The existing fourth `AC_CHECK_ICU` argument, `$ac_bes_dependencies_prefix`, was effectively ignored because the code that used it to prepend `PREFIX/bin` was commented out.
- Homebrew ICU is keg-only style: its `pkg-config` files live under the ICU prefix and are not visible unless `PKG_CONFIG_PATH` includes `PREFIX/lib/pkgconfig`.
- ICU 78.3 headers require C++17. BES configure previously selected at most C++14, so even after finding Homebrew ICU, NCML would not compile against that ICU.

Changes made:

- Made `AC_CHECK_ICU` prefer `pkg-config` modules `icu-i18n` and `icu-uc`, while keeping `icu-config` as a fallback.
- Made `AC_CHECK_ICU` honor `--with-icu-prefix` and the BES dependencies prefix by temporarily adding `PREFIX/bin`, `PREFIX/lib/pkgconfig`, and `PREFIX/lib64/pkgconfig`.
- Added a C++ compile/link probe using `unicode/smpdtfmt.h` and `unicode/timezone.h` so configure rejects unusable ICU flags instead of only discovering files.
- Added `ICU_CXXFLAGS` substitution and used it in the NCML module makefile.
- Updated the top-level C++ standard probe to prefer C++17 when available, while still falling back through C++14 and C++11.

Validation:

- Ran `autoreconf --force --install --verbose`; it completed successfully.
- Ran configure with:

  ```sh
  PKG_CONFIG_PATH=/Users/jhrg/src/opendap/hyrax/build/lib/pkgconfig \
  PATH=/Users/jhrg/src/opendap/hyrax/build/bin:/usr/bin:/bin:/usr/sbin:/sbin:/opt/homebrew/bin \
  ./configure --prefix=/Users/jhrg/src/opendap/hyrax/build \
      --with-dependencies=/Users/jhrg/src/opendap/hyrax/build/deps \
      --enable-developer \
      --with-icu-prefix=/opt/homebrew/opt/icu4c@78
  ```

- Configure selected `--std=c++17`, found ICU with `pkg-config`, verified the ICU C++ compile/link probe, and enabled NCML.
- `make -C modules/ncml_module -j20` was attempted. It applied the ICU include path and C++17 flags, then failed on `libdap/Array.h` and `libdap/AttrTable.h`. The immediate cause in this environment is the libdap pkg-config cflags returning `-I/Users/jhrg/src/opendap/hyrax/build/include/libdap`, while these BES sources include `<libdap/...>` and need the parent include directory. `dap-config --cflags` returns both include paths.
