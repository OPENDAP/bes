# VS Code Include Squiggles Diagnosis

## 2026-05-19 08:52:37 MDT

Prompt:

> Can you diagnose 'squiggles' in the vscode editor when an include file cannot be found?

Assumptions and environment:

- Repository: `/Users/jhrg/src/opendap/hyrax/bes`
- Active source file: `dap/unit-tests/TemporaryFileTest.cc`
- VS Code workspace settings are available through `.vscode/settings.json`, which is a symlink to `.vscode/jhrg/settings.json`.
- Configured `prefix`: `/Users/jhrg/src/opendap/hyrax/build`
- Configured Homebrew path: `/opt/homebrew`
- I ignored `retired/` per repository instructions.

Findings:

- There is no top-level `compile_commands.json`, even though `.vscode/settings.json` sets `C_Cpp.default.compileCommands` to `${workspaceFolder}/compile_commands.json`.
- There is no `.vscode/c_cpp_properties.json`.
- `.vscode/settings.json` sets `C_Cpp.default.configurationProvider` to `ms-vscode.makefile-tools`. When that provider is active, the C/C++ extension relies on Makefile Tools for per-file compiler configuration.
- The autotools dry-run compile command for `TemporaryFileTest.cc` includes the include paths needed by the source:
  - `../../dap`
  - `../../dispatch`
  - `../../modules/dmrpp_module`
  - `/Users/jhrg/src/opendap/hyrax/build/include`
  - `/Users/jhrg/src/opendap/hyrax/build/include/libdap`
  - `/opt/homebrew/Cellar/cppunit/1.15.1/include`
  - `/opt/homebrew/include`
  - `/Users/jhrg/src/opendap/hyrax/build/deps/include`
- A direct `g++ -fsyntax-only` using the autotools dry-run include flags succeeds for `dap/unit-tests/TemporaryFileTest.cc`.

Diagnosis:

The include squiggles are most likely an IntelliSense configuration problem, not a source-level missing include. The build system knows the required include paths, but VS Code currently depends on Makefile Tools or a missing `compile_commands.json` to communicate those paths to the C/C++ extension.

Most likely causes:

1. Makefile Tools has not been configured/built in VS Code for this workspace, so it is not providing a real per-file configuration.
2. The configured `compile_commands.json` path does not exist, so the fallback compile database path cannot help.
3. If a header such as `test_config.h` or `config.h` is squiggled after a clean checkout, generated headers may not exist until `./configure` and the relevant make target have run.

Validation run:

- `make -n -C dap/unit-tests TemporaryFileTest`
- `g++ -fsyntax-only` for `TemporaryFileTest.cc` using the include flags from the autotools dry-run command

Validation not run:

- I did not run `make check`.
- I did not run or reconfigure VS Code/Makefile Tools from inside the editor.
- I did not regenerate autotools files.

## 2026-05-19 09:12:33 MDT

Prompt:

> I changed the settings.json but the squiggles remain

New findings:

- `C_Cpp.default.compileCommands` is now commented out, so VS Code is no longer pointed at the missing `${workspaceFolder}/compile_commands.json`.
- `C_Cpp.default.configurationProvider` was still set to `ms-vscode.makefile-tools`.
- With a configuration provider active, the Microsoft C/C++ extension generally asks the provider for the per-file compiler configuration instead of relying on `C_Cpp.default.includePath`.
- The fallback include paths in `.vscode/jhrg/settings.json` should be sufficient for `TemporaryFileTest.cc` if cpptools is allowed to use them:
  - `${workspaceFolder}/**`
  - `${config:prefix}/include/**`
  - `${config:prefix}/deps/include/**`
  - `${config:HOMEBREW}/include/**`
- The headers checked directly exist:
  - `/opt/homebrew/include/cppunit/TextTestRunner.h`
  - `/Users/jhrg/src/opendap/hyrax/build/include/libdap/debug.h`
  - `./config.h`
  - `./dap/unit-tests/test_config.h`
  - `./modules/common/run_tests_cppunit.h`
- `.vscode/jhrg/tasks.json` still contains a stale `/Users/jimg/src/opendap/hyrax/build` prefix in the task environment. That may affect VS Code-launched configure/build tasks, even though `.vscode/jhrg/settings.json` uses `/Users/jhrg/src/opendap/hyrax/build`.

Action taken:

- I commented out `C_Cpp.default.configurationProvider` in `.vscode/jhrg/settings.json` as a reversible diagnostic step, so cpptools can use the explicit fallback `includePath`.

Next editor-side checks:

- Run `C/C++: Reset IntelliSense Database`.
- Run `Developer: Reload Window`.
- Open `C/C++: Log Diagnostics` for `dap/unit-tests/TemporaryFileTest.cc` and verify that the active configuration includes the fallback paths above.
- If squiggles remain, note the exact include file being squiggled; the next diagnosis depends on which header is failing.

Validation run:

- Re-read `.vscode/jhrg/settings.json`.
- Checked existence of representative cppunit, libdap, generated, and local BES headers.

Validation not run:

- I did not run `make check`.
- I did not regenerate a compile database.
- I did not run VS Code commands from inside the editor.
