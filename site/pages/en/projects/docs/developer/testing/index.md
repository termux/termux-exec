---
page_ref: "@PROJECTS__VARIANT@/termux/termux-exec/docs/@DOCS__VERSION@/developer/testing/index.md"
---

# Testing termux-exec

<!-- @DOCS__HEADER_PLACEHOLDER@ -->

The [`termux-exec`](https://github.com/termux/termux-exec) can be tested with [`termux-exec-tests`](https://github.com/termux/termux-exec/blob/master/tests/termux-exec-tests.in).

Install the updated `termux-exec` package and then start a new shell so that changes for updated library get loaded.

Update `bash termux-tools termux-am` to the latest version, otherwise tests will fail. Install Termux:API app and latest `termux-api` package to test them as well.

To show help, run `${TERMUX__PREFIX:-$PREFIX}/libexec/installed-tests/termux-exec/termux-exec-tests --help`.

To run all tests, run `TERMUX_EXEC__LOG_LEVEL=1 ${TERMUX__PREFIX:-$PREFIX}/libexec/installed-tests/termux-exec/termux-exec-tests -vvv all`. You can optionally run only `unit` or `runtime` tests as well.
- The `unit` tests test different units/components of the `termux-exec` source code.
- The `runtime` tests test the runtime execution of binaries and scripts and other Termux commands to ensure proper functioning of the Termux execution environment.

Two variants of each test binary is compiled.
- With `fsanitize` enabled with the `-fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer` flags that contain `-fsanitize` in filename
- Without `fsanitize` enabled that contain `-nofsanitize` in filename.

The `runtime-binary-tests` is also additionally compiled for Android API level `28` by `termux-exec` [`build.sh`](https://github.com/termux/termux-packages/blob/master/packages/termux-exec/build.sh) if `TERMUX_PKG_API_LEVEL` is `< 28` to test APIs that are only available on higher Android versions like `fexecve()`. When `termux-exec-tests` is executed, the `run_runtime_binary_tests()` method dynamically calls the `runtime-binary-tests` variant that would be supported by the host device.

This is requires because `fsanitize` does not work on all Android versions/architectures properly and may crash with false positives with the `AddressSantizier: SEGV on unknown address` error, like Android `7` (always crashes) and `x86_64` (requires loop to trigger as occasional crash), even for a source file compiled with an empty `main()` method.

To enable `AddressSantizier` while running `termux-exec-tests`, pass `-f`. To also enable `LeakSanitizer`, pass `-l` as well, but if it is not supported on current device, the `termux-exec-tests` will error out with `AddressSantizier: detect_leaks is not supported on this platform`.

If you get `CANNOT LINK EXECUTABLE *: library "libclang_rt.asan-aarch64.so" not found`, like on Android `7`, you will need to install the `libcompiler-rt` package to get the `libclang_rt.asan-aarch64.so` dynamic library required for `AddressSantizier` if passing `-f`. Export library file path with `export LD_LIBRARY_PATH=${TERMUX__PREFIX:-$PREFIX}/lib/clang/17/lib/linux` before running tests.

---

&nbsp;





## Help

```
termux-exec-tests is a wrapper script for testing termux-exec.

Usage:
    termux-exec-tests [command_options] command

Available commands:
    unit                     Run unit tests.
    runtime                  Run runtime on-device tests.
    all                      Run all tests.

Available command_options:
    [ -h | --help ]          Display this help screen.
    [ -f ]                   Use fsanitize binaries for AddressSanitizer.
    [ -l ]                   Detect memory leaks with LeakSanitizer.
                             Requires '-f` to be passed.
    [ --ld-preload=<path> ]  The path to 'libtermux-exec.so'.
    [ --tests-path=<path> ]  The path to installed-tests directory.
    [ --no-clean ]           Do not clean test files on failure.
    [ --only-ld-tests ]      Only run LD_PRELOAD termux-exec runtime tests.
    [ -q | -quiet ]          Set log level to 'OFF'.
    [ -v | -vv | -vvv | -vvvvv ]  Set log level to 'DEBUG', 'VERBOSE', `VVERBOSE'
                                  and `VVVERBOSE'.
```

---

&nbsp;
