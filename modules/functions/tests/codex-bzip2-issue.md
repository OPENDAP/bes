## What I asked:
When I compile and link the functions module in the @modules/funtions directory, the operation appears to work. However, when I run tests on the module in the @modules/functions/tests directory using, for example './testsuite -v 1' I get an error that the bzip2 library cannot be found. The bzip2 library is present on this computer in '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib'. I expect that the compiler will search this location by default. How can I fix this issue. Here is the full text of the run-time error I get when running the test: functions/tests % ./testsuite -v 1 ## ------------------------------------------------------------- ## ## bes 3.21.1 test suite: bes/modules/functions/tests testsuite. ## ## ------------------------------------------------------------- ## 1. testsuite.at:7: testing bescmd/tabular_0.dods.bescmd ... COMMAND: besstandalone -c bes.conf -i bescmd/tabular_0.dods.bescmd; skip if not ./testsuite.at:7: besstandalone -c $abs_builddir/$bes_conf -i $input | getdap -Ms - --- /dev/null 2026-05-05 08:36:21 +++ /Users/jhrg/src/opendap/hyrax/bes/modules/functions/tests/testsuite.dir/at-groups/1/stderr 2026-05-05 08:36:21 @@ -0,0 +1,5 @@ +Caught plugin exception during initialization of functions module: + dlopen(/Users/jhrg/src/opendap/hyrax/bes/modules/functions/.libs/libfunctions_module.so, 0x0009): Library not loaded: @rpath/libbz2.dylib + Referenced from: <9FEC2897-E7A7-4490-8303-5C87ACDE2081> /Users/jhrg/src/opendap/hyrax/bes/modules/functions/.libs/libfunctions_module.so + Reason: tried: '/Users/jhrg/src/opendap/hyrax/bes/dispatch/.libs/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/bes/xmlcommand/.libs/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/build/deps/lib/libbz2.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OS/Users/jhrg/src/opendap/hyrax/build/deps/lib/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/bes/modules/functions/.libs/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/build/deps/proj/lib/libbz2.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OS/Users/jhrg/src/opendap/hyrax/build/deps/proj/lib/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/build/deps/lib/libbz2.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OS/Users/jhrg/src/opendap/hyrax/build/deps/lib/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/bes/modules/functions/.libs/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/build/deps/proj/lib/libbz2.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OS/Users/jhrg/src/opendap/hyrax/build/deps/proj/lib/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/build/deps/lib/libbz2.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OS/Users/jhrg/src/opendap/hyrax/build/deps/lib/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/bes/standalone/.libs/libbz2.dylib' (no such file) +Error reading response type information: Found EOF stdout: 1. testsuite.at:7: FAILED (testsuite.at:7)
No tasks in progress


When I compile and link the functions module in the @modules/funtions directory, the operation appears to work. However, when I run tests on the module in the @modules/functions/tests directory using, for example './testsuite -v 1' I get an error that the bzip2 library cannot be found. The bzip2 library is present on this computer in '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib'. I expect that the compiler will search this location by default. How can I fix this issue. 

Here is the full text of the run-time error I get when running the test:

functions/tests % ./testsuite -v 1
## ------------------------------------------------------------- ##
## bes 3.21.1 test suite: bes/modules/functions/tests testsuite. ##
## ------------------------------------------------------------- ##
1. testsuite.at:7: testing bescmd/tabular_0.dods.bescmd ...
COMMAND: besstandalone -c bes.conf -i bescmd/tabular_0.dods.bescmd; skip if not 
./testsuite.at:7: besstandalone -c $abs_builddir/$bes_conf -i $input | getdap -Ms -
--- /dev/null	2026-05-05 08:36:21
+++ /Users/jhrg/src/opendap/hyrax/bes/modules/functions/tests/testsuite.dir/at-groups/1/stderr	2026-05-05 08:36:21
@@ -0,0 +1,5 @@
+Caught plugin exception during initialization of functions module:
+    dlopen(/Users/jhrg/src/opendap/hyrax/bes/modules/functions/.libs/libfunctions_module.so, 0x0009): Library not loaded: libbz2.dylib
+  Referenced from: <9FEC2897-E7A7-4490-8303-5C87ACDE2081> /Users/jhrg/src/opendap/hyrax/bes/modules/functions/.libs/libfunctions_module.so
+  Reason: tried: '/Users/jhrg/src/opendap/hyrax/bes/dispatch/.libs/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/bes/xmlcommand/.libs/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/build/deps/lib/libbz2.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OS/Users/jhrg/src/opendap/hyrax/build/deps/lib/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/bes/modules/functions/.libs/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/build/deps/proj/lib/libbz2.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OS/Users/jhrg/src/opendap/hyrax/build/deps/proj/lib/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/build/deps/lib/libbz2.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OS/Users/jhrg/src/opendap/hyrax/build/deps/lib/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/bes/modules/functions/.libs/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/build/deps/proj/lib/libbz2.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OS/Users/jhrg/src/opendap/hyrax/build/deps/proj/lib/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/build/deps/lib/libbz2.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OS/Users/jhrg/src/opendap/hyrax/build/deps/lib/libbz2.dylib' (no such file), '/Users/jhrg/src/opendap/hyrax/bes/standalone/.libs/libbz2.dylib' (no such file)
+Error reading response type information: Found EOF
stdout:
1. testsuite.at:7:  FAILED (testsuite.at:7)

## Codex's response:

This is a runtime loader problem, not a compile-time search-path problem.

Your built module currently contains:

```sh
@rpath/libbz2.dylib
```

but its embedded rpaths are only things like:

```sh
/Users/jhrg/src/opendap/hyrax/build/deps/lib
/Users/jhrg/src/opendap/hyrax/build/deps/proj/lib
@loader_path
```

There is no `libbz2.dylib` in those locations. The SDK path you mentioned contains `.tbd` linker stubs, not runtime dylibs. The relevant stub says the system runtime install name is:

```sh
/usr/lib/libbz2.1.0.dylib
```

The suspicious bit is that your configured GDAL dependency flags include:

```sh
-L/opt/homebrew/anaconda3/lib
```

and that library has this install name:

```sh
@rpath/libbz2.dylib
```

So the functions module appears to have picked up Anaconda’s `libbz2`, but you did not embed an rpath to Anaconda’s lib directory.

Best fixes, in order:

1. Reconfigure/rebuild so the BES/GDAL dependency chain does not inject `/opt/homebrew/anaconda3/lib` unless you really intend to depend on it. That is the clean fix.

2. If you intentionally want the Anaconda dylibs, add the matching rpath at configure time:

```sh
./configure \
  --prefix="$prefix" \
  --with-dependencies="$prefix/deps" \
  --enable-developer \
  LDFLAGS="-L/opt/homebrew/anaconda3/lib -Wl,-rpath,/opt/homebrew/anaconda3/lib"
```

Then rebuild the module.

3. For a quick local confirmation only, run:

```sh
DYLD_LIBRARY_PATH=/opt/homebrew/anaconda3/lib ./testsuite -v 1
```

4. As a temporary binary patch, you can change the module to use the system bzip2 install name:

```sh
install_name_tool \
  -change @rpath/libbz2.dylib /usr/lib/libbz2.1.0.dylib \
  ../.libs/libfunctions_module.so
```

That will be overwritten by the next rebuild, so I would not treat it as the real fix.

I would fix the contaminated dependency flags first. In this build, `gdal-config --dep-libs` is pulling in `/opt/homebrew/anaconda3/lib`; that is likely the root cause.