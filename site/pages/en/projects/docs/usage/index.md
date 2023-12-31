---
page_ref: "@PROJECTS__VARIANT@/termux/termux-exec/docs/@DOCS__VERSION@/usage/index.md"
---

# termux-exec Usage

<!-- @DOCS__HEADER_PLACEHOLDER@ -->

The [`termux-exec`](https://github.com/termux/termux-exec) is a shared library that is meant to be preloaded by exporting [`LD_PRELOAD=/data/data/com.termux/files/usr/lib/libtermux-exec.so`](https://man7.org/linux/man-pages/man8/ld.so.8.html) in shells for proper functioning of the Termux execution environment. This is automatically done by Termux in the [`login`](https://github.com/termux/termux-tools/blob/v1.40.1/scripts/login.in#L42-L45) script, but currently not for shells started by plugins, check [here](https://github.com/termux/termux-tasker#termux-environment) for more info.

Note that if exporting `LD_PRELOAD` or updating the `termux-exec` library, the current shell will not automatically load the updated library as any libraries that are to be loaded are only done when process is started. If unsetting `LD_PRELOAD`, the current shell will not automatically unload the library either. Changes to `LD_PRELOAD` requires at least one nested `exec()` for changes to take effect, i.e a new nested shell needs to be started and then any new calls/executions in the nested shell will be hooked by the library set it `LD_PRELOAD`.

### Contents

- [Input Environment Variables](#input-environment-variables)
- [Output Environment Variables](#ouput-environment-variables)
- [Processed Environment Variables](#processed-environment-variables)

---

&nbsp;





## Input Environment Variables

The `termux-exec` uses the following environment variables as input if required.

*For variables with type `bool`, the values `1`, `true`, `on`, `yes`, `y` are parsed as `true` and the values `0`, `false`, `off`, `no`, `n` are parsed as `false`, and for any other value the default value will be used.*

- [`TERMUX__ROOTFS`](#termux__rootfs)
- [`TERMUX__SE_PROCESS_CONTEXT`](#termux__se_process_context)
- [`TERMUX_EXEC__LOG_LEVEL`](#termux_exec__log_level)
- [`TERMUX_EXEC__INTERCEPT_EXECVE`](#termux_exec__intercept_execve)
- [`TERMUX_EXEC__SYSTEM_LINKER_EXEC`](#termux_exec__system_linker_exec)
- [`TERMUX_EXEC_TESTS__LOG_LEVEL`](#termux_exec_tests__log_level)

&nbsp;



### TERMUX__ROOTFS

The Termux rootfs directory path.

This is where all termux installed packages and user files exist under.

**Type:** `string`

**Commits:** [`b5a42a15`](https://github.com/termux/termux-exec/commit/b5a42a15)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec/releases/tag/v2.0.0)

**Default:** null

**Values:**

- An absolute path with max length `TERMUX__ROOTFS_DIR_MAX_LEN` (`86`) including the null `\0` terminator.

If `TERMUX__ROOTFS` is not set or value is not valid, then the default value `/data/data/@TERMUX_APP__PACKAGE_NAME@/files` for rootfs with which `termux-exec` package is compiled with is used if the path exists and is executable, otherwise `termux-exec` hooks will return with an error and `errno` should be set that is set by the [`access()`](https://man7.org/linux/man-pages/man2/access.2.html) call.

The `TERMUX__ROOTFS_DIR_MAX_LEN` is defined in [`termux_files.h`](https://github.com/termux/termux-exec/blob/b5a42a15/src/termux/termux_files.h) and it is the internal buffer length used by `termux-exec` for storing Termux rootfs directory path and the buffer lengths of all other Termux paths under the rootfs is based on it. The value `86` is the maximum value that will fit the requirement for a valid Termux rootfs directory path under android app data directory path, check [termux file path limits](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#file-path-limits) docs for more info. **Packages compiled for Termux must ensure that the `TERMUX__ROOTFS` value used during compilation is `< TERMUX__ROOTFS_DIR_MAX_LEN`.**

## &nbsp;

&nbsp;



### TERMUX__SE_PROCESS_CONTEXT

The SeLinux process context of the Termux app process and its child processes.

This is used while deciding whether to use `system_linker_exec` if [`TERMUX_EXEC__SYSTEM_LINKER_EXEC`](#termux_exec__system_linker_exec) is set to `enabled`.

**Type:** `string`

**Commits:** [`b5a42a15`](https://github.com/termux/termux-exec/commit/b5a42a15)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec/releases/tag/v2.0.0)

**Default:** null

**Values:**

- A valid Android SeLinux process context that matches `REGEX__PROCESS_CONTEXT`:
`^u:r:[^\n\t\r :]+:s0(:c[0-9]+,c[0-9]+(,c[0-9]+,c[0-9]+)?)?$`

If `TERMUX__SE_PROCESS_CONTEXT` is not set or value is not valid, then the process context is read from the `/proc/self/attr/current` file.

The `REGEX__PROCESS_CONTEXT` is defined in [`selinux_utils.h`](https://github.com/termux/termux-exec/blob/b5a42a15/src/os/selinux_utils.h), check its field docs for more info on the format of the an Android SeLinux process context.

## &nbsp;

&nbsp;



### TERMUX_EXEC__LOG_LEVEL

The log level for `termux-exec`.

Normally, `termux-exec` does not log anything at log level `1` (`NORMAL`) for hooks even and will require setting log level to `>= 2` (`DEBUG`) to see log messages.

**Type:** `int`

**Commits:** [`b5a42a15`](https://github.com/termux/termux-exec/commit/b5a42a15)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec/releases/tag/v2.0.0)

**Default:** `1`

**Values:**

- `0` (`OFF`) - Log nothing.

- `1` (`NORMAL`) - Log error, warn and info messages and stacktraces.

- `2` (`DEBUG`) - Log debug messages.

- `3` (`VERBOSE`) - Log verbose messages.

- `4` (`VVERBOSE`) - Log very verbose messages.

## &nbsp;

&nbsp;



### TERMUX_EXEC__INTERCEPT_EXECVE

Whether `termux-exec` should intercept `execve()` wrapper [declared in `unistd.h`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/include/unistd.h;l=95) and [implemented by `exec.cpp`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/bionic/exec.cpp;l=187) in android [`bionic`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/README.md) `libc` library around the [`execve()`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/SYSCALLS.TXT;l=68) system call listed in [`syscalls(2)`](https://man7.org/linux/man-pages/man2/syscalls.2.html) provided by the [linux kernel](https://cs.android.com/android/kernel/superproject/+/ebe69964:common/include/linux/syscalls.h;l=790).

If enabled, then Termux specific logic will run to solve the issues for exec-ing files in Termux that are listed in [`exec()` technical docs](../technical.md#exec) before calling `execve()` system call. If not enabled, then `execve()` system call will be called directly instead.

The other wrapper methods in the `exec()` family of functions declared in `unistd.h` are always intercepted to solve some other issues on older Android versions, check [`libc/bionic/exec.cpp`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/bionic/exec.cpp) git history.

**Type:** `bool`

**Commits:** [`b5a42a15`](https://github.com/termux/termux-exec/commit/b5a42a15)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec/releases/tag/v2.0.0)

**Default:** `true`

**Values:**

- `true` - Intercept `execve()` enabled.

- `false` - Intercept `execve()` disabled.

## &nbsp;

&nbsp;



### TERMUX_EXEC__SYSTEM_LINKER_EXEC

Whether to use [System Linker Exec Solution](../technical.md#system-linker-exec-solution), like to bypass [App Data File Execute Restrictions](../technical.md#app-data-file-execute-restrictions).

**Type:** `string`

**Commits:** [`b5a42a15`](https://github.com/termux/termux-exec/commit/b5a42a15)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec/releases/tag/v2.0.0)

**Default:** null

**Values:**

- `disable` - The `system_linker_exec` will be disabled.

- `enable` - The `system_linker_exec` will be enabled but only if required.

- `force` - The `system_linker_exec` will be force enabled even if not required and is supported.

This is implemented by `should_system_linker_exec()` in [`exec.c`](https://github.com/termux/termux-exec/blob/b5a42a15/src/exec/exec.c).

If `disable` is set, then `system_linker_exec` will never be used.

If `enable` is set, then `system_linker_exec` will only be used if:
- `system_linker_exec` is required to bypass [App Data File Execute Restrictions](../technical.md#app-data-file-execute-restrictions), i.e device is running on Android `>= 10`.
- Effective user does not equal root (`0`) and shell (`2000`) user.
- [`TERMUX__SE_PROCESS_CONTEXT`](#TERMUX__SE_PROCESS_CONTEXT) does not start with `PROCESS_CONTEXT_PREFIX__UNTRUSTED_APP_25` (`u:r:untrusted_app_25:`) and `PROCESS_CONTEXT_PREFIX__UNTRUSTED_APP_27` (`u:r:untrusted_app_27:`) for which restrictions are exempted. For more info on them, check [`selinux_utils.h`](https://github.com/termux/termux-exec/blob/b5a42a15/src/os/selinux_utils.h).
- Executable or interpreter path is under [`TERMUX__ROOTFS`] directory.

If `force` is set, then `system_linker_exec` will only be used if:
- `system_linker_exec` is supported, i.e device is running on Android `>= 10`.
- Executable or interpreter path is under [`TERMUX__ROOTFS`] directory.
This can be used if running in an untrusted app with `targetSdkVersion` `<= 28`.

## &nbsp;

&nbsp;



### TERMUX_EXEC_TESTS__LOG_LEVEL

The log level for `termux-exec-tests`.

**Type:** `int`

**Commits:** [`b5a42a15`](https://github.com/termux/termux-exec/commit/b5a42a15)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec/releases/tag/v2.0.0)

**Default:** `1`

**Values:**

- `0` (`OFF`) - Log nothing.

- `1` (`NORMAL`) - Log error, warn and info messages and stacktraces.

- `2` (`DEBUG`) - Log debug messages.

- `3` (`VERBOSE`) - Log verbose messages.

- `4` (`VVERBOSE`) - Log very verbose messages.

---

&nbsp;





## Output Environment Variables

The `termux-exec` sets the following environment variables if required.

- [`TERMUX_EXEC__PROC_SELF_EXE`](#termux_exec__proc_self_exe)

&nbsp;



### TERMUX_EXEC__PROC_SELF_EXE

If `system_linker_exec` is being used, then to execute the `/path/to/executable` file, we will be executing `/system/bin/linker64 /path/to/executable [args]`.

 The `/proc/pid/exe` file is a symbolic link containing the actual path of the executable being executed. However, if using `system_linker_exec`, then `/proc/pid/exe` will contain `/system/bin/linker64` instead of `/path/to/executable`.

So `termux-exec` sets the `TERMUX_EXEC__PROC_SELF_EXE` env variable when `execve` is intercepted to the processed (normalized, absolutized and prefixed) path for the executable file that is to be executed with the linker. This allows patching software that reads `/proc/self/exe` in `termux-packages` build infrastructure to instead use `getenv("TERMUX_EXEC__PROC_SELF_EXE")`.

&nbsp;

Note that if `termux-exec` is set in `LD_PRELOAD`, and it sets `TERMUX_EXEC__PROC_SELF_EXE` for the current process/shell, and then `LD_PRELOAD` is unset, then new processes after second nested `exec()` will get old and wrong value of `TERMUX_EXEC__PROC_SELF_EXE` belonging to the first nested process since `termux-exec` will not get called for the second nested process to set the updated value. The `termux-exec` will be called for the first nested process, because just unsetting `LD_PRELOAD` in current process will not unload the `termux-exec` library and it requires at least one nested `exec()`. The `termux-exec` library could unset `TERMUX_EXEC__PROC_SELF_EXE` if `LD_PRELOAD` isn't already set, but then if the first nested process is under [`TERMUX__ROOTFS`], it will not have access to `TERMUX_EXEC__PROC_SELF_EXE` to read the actual value of the execution command. This would normally not be an issue if `LD_PRELOAD` being set to the `termux-exec` library is mandatory so that it can `system_linker_exec` commands if running with `targetSdkVersion` `>= 29` on an android `>= 10` device, as otherwise permission denied errors would trigger for any command under [`TERMUX__ROOTFS`] anyways, unless user manually wraps second nested process with `/system/bin/linker64`. This will still be an issue if `system_linker_exec` is optional due to running with an older `targetSdkVersion` or on an older android device and `TERMUX_EXEC__SYSTEM_LINKER_EXEC` is set to `force`, since then `TERMUX_EXEC__PROC_SELF_EXE` would get exported and will be used by termux packages.

**To prevent issues, if `LD_PRELOAD` is unset in current process, then `TERMUX_EXEC__PROC_SELF_EXE` must also be unset in the first nested process by the user themselves.** For example, running following will echo `<TERMUX__PREFIX>/bin/sh` value twice instead of `<TERMUX__PREFIX>/bin/sh` first and `<TERMUX__PREFIX>/bin/dash` second if `LD_PRELOAD` were to be set.

- `system_linker_exec` optional: `LD_PRELOAD= $TERMUX__PREFIX/bin/sh -c 'echo $TERMUX_EXEC__PROC_SELF_EXE'; $TERMUX__PREFIX/bin/dash -c "echo \$TERMUX_EXEC__PROC_SELF_EXE"'`
- `system_linker_exec` mandatory: `LD_PRELOAD= $TERMUX__PREFIX/bin/sh -c 'echo $TERMUX_EXEC__PROC_SELF_EXE'; /system/bin/linker64 $TERMUX__PREFIX/bin/dash -c "echo \$TERMUX_EXEC__PROC_SELF_EXE"'`

**See also:**

- https://man7.org/linux/man-pages/man5/procfs.5.html

**Type:** `string`

**Commits:** [`b5a42a15`](https://github.com/termux/termux-exec/commit/b5a42a15)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec/releases/tag/v2.0.0)

**Values:**

- The normalized, absolutized and prefixed path to the executable file is being executed by `execve()` if `system_linker_exec` is being used.

---

&nbsp;





## Processed Environment Variables

The `termux-exec` processes the following environment variables if required.

- [`LD_LIBRARY_PATH`](#ld_library_path)
- [`LD_PRELOAD`](#ld_preload)

&nbsp;



### LD_LIBRARY_PATH

The list of directory paths separated with colons `:` that should be searched in for dynamic shared libraries to link programs against.

**See also:**

- https://manpages.debian.org/testing/manpages/ld.so.8.en.html#LD_LIBRARY_PATH

**Type:** `string`

**Commits:** [`13831552`](https://github.com/termux/termux-exec/commit/13831552), [`b5a42a15`](https://github.com/termux/termux-exec/commit/b5a42a15)

**Version:** [`>= 0.9`](https://github.com/termux/termux-exec/releases/tag/v0.9)

**Processing:**

- If `execve` is intercepted, then `LD_LIBRARY_PATH` will be unset if executing a non native ELF file (like 32-bit binary on a 64-bit host) and executable path starts with `/system/`, but does not equal `/system/bin/sh`, `system/bin/linker` or `/system/bin/linker64`.

## &nbsp;

&nbsp;



### LD_PRELOAD

The list of ELF shared object paths separated with colons ":" to be loaded before all others. This feature can be used to selectively override functions in other shared objects.

**See also:**

- https://manpages.debian.org/testing/manpages/ld.so.8.en.html#LD_PRELOAD

**Type:** `string`

**Commits:** [`13831552`](https://github.com/termux/termux-exec/commit/13831552), [`b5a42a15`](https://github.com/termux/termux-exec/commit/b5a42a15)

**Version:** [`>= 0.9`](https://github.com/termux/termux-exec/releases/tag/v0.9)

**Processing:**

- If `execve` is intercepted, then `LD_PRELOAD` will be unset if executing a non native ELF file (like 32-bit binary on a 64-bit host) and executable path starts with `/system/`, but does not equal `/system/bin/sh`, `system/bin/linker` or `/system/bin/linker64`.

## &nbsp;

&nbsp;

---

&nbsp;





[`TERMUX__ROOTFS`]: #termux__rootfs
