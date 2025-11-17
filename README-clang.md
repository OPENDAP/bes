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
