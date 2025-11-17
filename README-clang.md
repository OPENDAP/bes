Good question — no, clang‑tidy is *not* installed by default on macOS (even if you have Xcode / the command line tools). ([Stack Overflow][1])

Here’s how to install it and get it working:

---

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

If you like, I can *walk you through* installation on **your specific macOS version** (with the exact commands for Apple Silicon vs Intel), and then show you a “quick test” that you have it working with your codebase. Would you like me to do that?

[1]: https://stackoverflow.com/questions/53111082/how-to-install-clang-tidy-on-macos?utm_source=chatgpt.com "How to install clang-tidy on macOS? - Stack Overflow"
[2]: https://gist.github.com/sleepdefic1t/e9bdb1a66b05aa043ab9a2ab6c929509?utm_source=chatgpt.com "brew clang-tidy on macOS · GitHub"
[3]: https://www.cnblogs.com/tengzijian/p/17763811.html?utm_source=chatgpt.com "macOS 安装 clang-tidy - Zijian/TENG - 博客园"
[4]: https://markaicode.com/clang-tidy-18-install-security-checklist/?utm_source=chatgpt.com "Installing Clang-Tidy 18 for C++ Static Analysis: A 2025 Security Audit ..."
[5]: https://github.com/microsoft/vscode-cpptools/issues/10282?utm_source=chatgpt.com "Clang-format/tidy failing on macOS 11 arm64 - GitHub"
[6]: https://pypi.org/project/clang-tools/?utm_source=chatgpt.com "clang-tools · PyPI"

# Here's more...

You’re hitting the classic macOS/Xcode + Homebrew LLVM mismatch: **clang-tidy can’t see the SDK/stdlib include paths** that your real build uses, so it can’t find `<iostream>`. Your code compiles because your compiler gets `-isysroot …` and `-stdlib=libc++` implicitly; clang-tidy doesn’t—unless those flags are in `compile_commands.json`.

Here’s how to get it working fast.

# Quick fix (don’t rebuild your DB)

Try adding the macOS SDK + libc++ include hints directly to clang-tidy:

```bash
SDK=$(xcrun --show-sdk-path)

run-clang-tidy -p . \
  -checks='-*,modernize-use-override' -fix \
  --extra-arg=-std=c++17 \
  --extra-arg=-stdlib=libc++ \
  --extra-arg=-isysroot --extra-arg="$SDK" \
  --extra-arg=-I"$SDK/usr/include/c++/v1"
```

If you don’t have `run-clang-tidy`, do the same with a single file first:

```bash
FILE=modules/dmrpp_module/DmrppMetadataStore.cc
SDK=$(xcrun --show-sdk-path)

clang-tidy -p . -checks='-*,modernize-use-override' -fix -v \
  --extra-arg=-std=c++17 \
  --extra-arg=-stdlib=libc++ \
  --extra-arg=-isysroot --extra-arg="$SDK" \
  --extra-arg=-I"$SDK/usr/include/c++/v1" \
  "$FILE"
```

If that runs without “`'iostream' file not found`”, expand to your whole tree.

# Better fix (regenerate a correct compile DB)

Ideally, **bake those flags into `compile_commands.json`** so you don’t pass extras every time. With Autotools:

1. Ensure the Xcode SDK path is visible to your build:

```bash
export SDKROOT="$(xcrun --show-sdk-path)"
export CXXFLAGS="-std=c++17 -stdlib=libc++"
```

2. Recreate the DB from a clean build:

```bash
make distclean || true   # if available
./configure              # your normal configure

# Capture the build exactly as it compiles (best: bear)
bear -- make -j          # or: intercept-build make -j
```

3. Verify the DB has the right flags:

```bash
jq '.[] | select(.file|endswith("DmrppMetadataStore.cc"))' compile_commands.json
# Look for -isysroot ...MacOSX.sdk and possibly -stdlib=libc++
```

4. Now this should work without extra args:

```bash
run-clang-tidy -p . -checks='-*,modernize-use-override' -fix
```

# Why this happens

* Homebrew `clang-tidy` runs outside the Xcode toolchain. Unless your **compile DB** includes the SDK/stdlib paths, clang-tidy’s internal clang can’t locate `<iostream>` (which lives under `$SDK/usr/include/c++/v1` on macOS).
* Autotools builds often rely on environment-provided defaults; `bear` may not capture implicit SDK info unless it’s present as flags in the actual compile commands.

# Troubleshooting checklist

* **Does clang-tidy see your DB?**
  `clang-tidy -p . -v path/to/some.cc` → if you see “Error while trying to load a compilation database”, your `-p` is wrong.
* **Is that file in the DB?**
  `jq -r '.[].file' compile_commands.json | grep DmrppMetadataStore.cc`
* **Still errors but you want fixes anyway?**
  Some versions support `-fix-errors` to apply fixes even with diagnostics (use cautiously).
* **Headers not being fixed?**
  Add a header filter to limit to your project path:
  `-header-filter='^/Users/jimg/src/opendap/hyrax_git/.*'`

If you paste the `compile_commands.json` entry for `DmrppMetadataStore.cc` (paths can be redacted), I can tell you exactly which flag is missing and give you a one-liner tailored to your setup.
