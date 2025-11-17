# How to use clang-tidy and clang-format to update older C++ code

## Reformat all the C++ files in a directory

To reformat only the C++ source files in git, use _git_ with the _ls-files_ 
command and the file globbing patterns for our code. Take that and pipe it into
_clang-format_. Note the use of _xargs_ to limit the number of things (_-n_) passed
to the formater and set the number of parallel invocations (_-P_).

[!TIP] Avoid running this at the top of a repo since it will recurse and edit
thousands of files. Unless that's what you want.

For all of this stuff, try it first on one file. Look at the results, then go
forward with caution. Test and commit after every change. Here's how to run the 
formatter on just one file. Note that we have a _.clang-format_ file in the root
of both _libdap4_ and _bes_.

```bash
lang-format -i --style=file BESVersionResponseHandler.h
```

Here's a more general version that will reformat many files. Be cautious since
mistakes and get out of hand quickly.

```bash
git  ls-files '*.[ch]pp' '*.cc' '*.h' '*.hh' \
  | xargs -n 50 -P "$(sysctl -n hw.ncpu)" clang-format -i --style=file
```

## Updating C++ code to use 'modern' C++ features

Here's a basic one-file fix that will change all the declarations in a header
to use the C++14 _override_ keyword were applicable:

```bash
SDK=$(xcrun --show-sdk-path)

clang-tidy -p . -checks='-*,modernize-use-override' -fix \
  -extra-arg=-std=c++14 \
  -extra-arg=-stdlib=libc++ \
  -extra-arg=-isysroot -extra-arg="$SDK" \
  -extra-arg=-I"$SDK/usr/include/c++/v1" \
  BESVersionResponseHandler.h
```

It's tempting to make up a 'one script to rule them all,' but don't. Plod along
slowly working on groups of files. Test and commit. Also, the above will need 
more _-I<directory>_ options if the code in question references headers that 
clang-tidy cannot find in the CWD. 

Here is an example of the error from a missing header directory:
```bash
/Users/jimg/src/opendap/hyrax/bes/dispatch/unit-tests/BESCatalogListTest.cc:25:10: error: 'cppunit/TextTestRunner.h' file not found [clang-diagnostic-error]
   25 | #include <cppunit/TextTestRunner.h>
      |          ^~~~~~~~~~~~~~~~~~~~~~~~~~
```

[!CAUTION] Avoid the temptation of running clang-tidy in parallel using xargs.
Doing that will result in corrupt files.

When clang-tidy finds errors, it will not fix _anything_ in that source file.
There is a work around; use the _-fix-error_ option. But, this may not be what is best.
The output of clang-tidy should not contain errors. Warnings are OK, but errors
mean the command is likely missing some header files. Here's what the output should look like:
```aiignore
[1/10] Processing file /Users/jimg/src/opendap/hyrax/bes/dispatch/unit-tests/BESCatalogListTest.cc.
[2/10] Processing file /Users/jimg/src/opendap/hyrax/bes/dispatch/unit-tests/BESFileLockingCacheTest.cc.
[3/10] Processing file /Users/jimg/src/opendap/hyrax/bes/dispatch/unit-tests/CatalogItemTest.cc.
[4/10] Processing file /Users/jimg/src/opendap/hyrax/bes/dispatch/unit-tests/CatalogNodeTest.cc.
[5/10] Processing file /Users/jimg/src/opendap/hyrax/bes/dispatch/unit-tests/FileCacheTest.cc.
4 warnings generated.
[6/10] Processing file /Users/jimg/src/opendap/hyrax/bes/dispatch/unit-tests/RequestTimerTest.cc.
4 warnings generated.
[7/10] Processing file /Users/jimg/src/opendap/hyrax/bes/dispatch/unit-tests/ServerAdministratorTest.cc.
4 warnings generated.
[8/10] Processing file /Users/jimg/src/opendap/hyrax/bes/dispatch/unit-tests/TestReporter.cc.
4 warnings generated.
[9/10] Processing file /Users/jimg/src/opendap/hyrax/bes/dispatch/unit-tests/TestRequestHandler.cc.
4 warnings generated.
[10/10] Processing file /Users/jimg/src/opendap/hyrax/bes/dispatch/unit-tests/TestResponseHandler.cc.
4 warnings generated.
/Users/jimg/src/opendap/hyrax/bes/dispatch/unit-tests/FileCacheTest.cc:60:5: warning: 'SHA256_Init' is deprecated [clang-diagnostic-deprecated-declarations]
   60 |     SHA256_Init(&sha256);
      |     ^
/usr/local/include/openssl/sha.h:73:1: note: 'SHA256_Init' has been explicitly marked deprecated here
   73 | OSSL_DEPRECATEDIN_3_0 int SHA256_Init(SHA256_CTX *c);
      | ^
/usr/local/include/openssl/macros.h:229:49: note: expanded from macro 'OSSL_DEPRECATEDIN_3_0'
  229 | #   define OSSL_DEPRECATEDIN_3_0                OSSL_DEPRECATED(3.0)
      |                                                 ^
/usr/local/include/openssl/macros.h:62:52: note: expanded from macro 'OSSL_DEPRECATED'
   62 | #     define OSSL_DEPRECATED(since) __attribute__((deprecated))
      |                                                    ^
/Users/jimg/src/opendap/hyrax/bes/dispatch/unit-tests/FileCacheTest.cc:69:5: warning: 'SHA256_Update' is deprecated [clang-diagnostic-deprecated-declarations]
   69 |     SHA256_Update(&sha256, buffer.data(), length);
      |     ^
...
```

There is a way to make a compilation database using the command _bear_, but I
found that was harder to use and limit the cope of changes. There is also a second
command _run-clang-tidy_ but I found there were issues with that.

## What should be changed and how to do that

Here’s a focused, **C++14-friendly** bundle of clang-tidy checks you can run *individually* or in small groups. 
These are grouped them by “theme” and show the check names; you can plug them into the above in place of 
_modernize-use-override_ above.

This assumes:

* You’re compiling as **C++14**
* You’re using the macOS flags you already found work (`--extra-arg=-std=c++14`, `--extra-arg=-isysroot`, etc.)
* You want **safe, low-surprise** modernizations (no big semantic leaps like auto everywhere).

### 1. Overrides, defaults, deletes, `void` args

These are very safe, very C++11/14 core style improvements:

* `modernize-use-override`
* `modernize-use-equals-default`
* `modernize-use-equals-delete`
* `modernize-redundant-void-arg`

## 2. Pointer / null usage

All C++14 friendly:

* `modernize-use-nullptr` – replace `0` / `NULL` with `nullptr`.
* `modernize-make-unique` – turn `new T(...)` into `std::make_unique<T>(...)` where safe.

If you **don’t** use `std::unique_ptr` heavily or can’t rely on `<memory>` availability, 
skip `modernize-make-unique`. Otherwise, it’s a great upgrade.

### 3. Range-for and STL usage (lightweight)

Some `modernize` checks can be a bit aggressive; here are ones that are usually tame:

* `modernize-loop-convert`
  Convert `for (i = 0; i < vec.size(); ++i)` to range-for where safe.
* `modernize-use-emplace`
  Replace `container.push_back(T(args...))` with `emplace_back(args...)`.

Review these changes carefully.

### 4. Literal & basic expression cleanup

These are usually low drama:

* `modernize-use-bool-literals` – replace `0`/`1` used as bool with `false`/`true`.
* `modernize-use-bare-nullptr` (if available in your version) – similar spirit to `use-nullptr`.
* `modernize-deprecated-headers` – replace `<stdio.h>` with `<cstdio>`, etc.

## 5. Things I’d be *careful* with in legacy code

These are *powerful* but more likely to cause friction in an older, widely-used codebase, even though they’re C++14-compatible:

* `modernize-use-auto` – can be stylistically controversial and occasionally hurt readability in generic-heavy code.
* `modernize-pass-by-value` – changes function signatures; can cause ABI or overload selection changes.
* `modernize-use-using` – rewrites `typedef` to `using`; safe, but touches a ton of code and can be noisy in diffs.
* `modernize-use-noexcept` – can change exception specifications, sometimes interacting badly with older exception-heavy code.

I’d either:

* run these **very late**, once everything else is clean,
* or hand-pick them for specific directories / modules via `HeaderFilterRegex` or limiting the file list.

## Suggested order of passes

If you want a practical sequence that’s unlikely to blow anything up:

1. **Overrides & special members & `void` args:**

   ```text
   modernize-use-override
   modernize-use-equals-default
   modernize-use-equals-delete
   modernize-redundant-void-arg
   ```

2. **Null & optional smart pointer upgrade:**

   ```text
   modernize-use-nullptr
   [maybe] modernize-make-unique
   ```

3. **Literal + header cleanup:**

   ```text
   modernize-use-bool-literals
   modernize-deprecated-headers
   ```

4. **Loop + emplace (one at a time if you prefer):**

   ```text
   modernize-loop-convert
   modernize-use-emplace
   ```

## Getting _clang-tidy_ on OSX

[!NOTE]
clang‑tidy is *not* installed by default on macOS (even if you have Xcode / the command line tools). ([Stack Overflow][1])

Here’s how to install it and get it working:

### ✅ Installation steps

1. Install LLVM via Homebrew

   ```bash
   brew install llvm
   ```

   This will install newer LLVM tools (including clang-tidy) but they will be keg-only (not automatically in your PATH). ([Gist][2])

2. Add the LLVM tool-bin directory to your PATH (or create symlinks)
   For example on Apple Silicon (homebrew default `/opt/homebrew`) you might do:

   ```bash
   export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
   ```

   Or symlink:

   ```bash
   ln -s "$(brew --prefix llvm)/bin/clang-tidy" /usr/local/bin/clang-tidy
   ```

   ([cnblogs.com][3])

3. Verify installation:

   ```bash
   clang-tidy --version
   ```

   It should print something like `LLVM (clang-tidy) version X.Y.Z`. If it doesn’t exist, your PATH/symlink isn’t set up. ([Markaicode][4])

4. (Optional) Create a `compile_commands.json` for your project so clang-tidy can use the correct include paths, macros, etc.

---

### ⚠️ Notes & caveats

* Because this installs a newer LLVM than the system’s default clang, you may end up using a different compiler version than Xcode’s default — that’s usually fine but something to keep aware of.
* On macOS with M1/ARM or newer SDKs, sometimes you’ll hit compatibility issues if the tooling was built for a different architecture or SDK. (There are issues reported for arm64 builds of clang-tidy on macOS) ([GitHub][5])
* If you only want a lightweight install (just clang-tidy) you may look at alternative binaries (there is a “clang-tools” package on PyPI, etc) but the Homebrew llvm route is the most reliable. ([PyPI][6])

---

[1]: https://stackoverflow.com/questions/53111082/how-to-install-clang-tidy-on-macos?utm_source=chatgpt.com "How to install clang-tidy on macOS? - Stack Overflow"
[2]: https://gist.github.com/sleepdefic1t/e9bdb1a66b05aa043ab9a2ab6c929509?utm_source=chatgpt.com "brew clang-tidy on macOS · GitHub"
[3]: https://www.cnblogs.com/tengzijian/p/17763811.html?utm_source=chatgpt.com "macOS 安装 clang-tidy - Zijian/TENG - 博客园"
[4]: https://markaicode.com/clang-tidy-18-install-security-checklist/?utm_source=chatgpt.com "Installing Clang-Tidy 18 for C++ Static Analysis: A 2025 Security Audit ..."
[5]: https://github.com/microsoft/vscode-cpptools/issues/10282?utm_source=chatgpt.com "Clang-format/tidy failing on macOS 11 arm64 - GitHub"
[6]: https://pypi.org/project/clang-tools/?utm_source=chatgpt.com "clang-tools · PyPI"
