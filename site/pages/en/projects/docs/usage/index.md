---
page_ref: "@ARK_PROJECT__VARIANT@/termux/termux-exec-package/docs/@DOCS__VERSION@/usage/index.md"
---

# termux-exec-package Usage Docs

<!-- @ARK_DOCS__HEADER_PLACEHOLDER@ -->

The [`termux-exec`](https://github.com/termux/termux-exec-package) package provides utils and libraries for Termux exec. It also provides a shared library that is meant to be preloaded with [`$LD_PRELOAD`](https://man7.org/linux/man-pages/man8/ld.so.8.html) by exporting [`LD_PRELOAD="$TERMUX__PREFIX/usr/lib/libtermux-exec-ld-preload.so"`](https://man7.org/linux/man-pages/man8/ld.so.8.html) for proper functioning of the Termux execution environment.

Note that if exporting `LD_PRELOAD` or updating the `termux-exec` library, the current shell will not automatically load the updated library as any libraries that are to be loaded are only done when process is started. If unsetting `LD_PRELOAD`, the current shell will not automatically unload the library either. Changes to `LD_PRELOAD` requires at least one nested `exec()` for changes to take effect, i.e a new nested shell needs to be started and then any new calls/executions in the nested shell will be intercepted by the library set it `LD_PRELOAD`.

### Contents

- [Input Environment Variables](#input-environment-variables)
- [Output Environment Variables](#ouput-environment-variables)
- [Processed Environment Variables](#processed-environment-variables)

---

&nbsp;





## Input Environment Variables

The `termux-exec` uses the following environment variables as input if required.

*For variables with type `bool`, the values `1`, `true`, `on`, `yes`, `y` are parsed as `true` and the values `0`, `false`, `off`, `no`, `n` are parsed as `false`, and for any other value the default value will be used.*

- [`TERMUX_APP__DATA_DIR`](#termux_app__data_dir)
- [`TERMUX_APP__LEGACY_DATA_DIR`](#termux_app__legacy_data_dir)
- [`TERMUX__PREFIX`](#termux__prefix)
- [`TERMUX__SE_PROCESS_CONTEXT`](#termux__se_process_context)
- [`TERMUX_EXEC__LOG_LEVEL`](#termux_exec__log_level)
- [`TERMUX_EXEC__EXECVE_CALL__INTERCEPT`](#termux_exec__execve_call__intercept)
- [`TERMUX_EXEC__SYSTEM_LINKER_EXEC__MODE`](#termux_exec__system_linker_exec__mode)
- [`TERMUX_EXEC__TESTS__LOG_LEVEL`](#termux_exec__tests__log_level)

&nbsp;



### TERMUX_APP__DATA_DIR

The non-legacy [Termux app data directory path](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#termux-private-app-data-directory) (`/data/user/<user_id>/<package_name>` or `/mnt/expand/<volume_uuid>/user/0/<package_name>`) that is assigned by Android for all Termux app data returned for the [`ApplicationInfo.dataDir`](https://developer.android.com/reference/android/content/pm/ApplicationInfo#dataDir) call, that contains the [Termux project directory](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#termux-project-directory) (`TERMUX__PROJECT_DIR`), and optionally the [Termux rootfs directory](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#termux-rootfs-directory) (`TERMUX__ROOTFS`). The value is automatically exported by the Termux app for app version `>= 0.119.0`.

**Type:** `string`

**Commits:** [`8b5f4d9a`](https://github.com/termux/termux-core-package/commit/8b5f4d9a)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec-package/releases/tag/v2.0.0)

**Default value:** `/data/user/0/com.termux` if Termux app is running under primary user `0`.

**Assigned values:**

- An absolute path with max length `TERMUX_APP__DATA_DIR___MAX_LEN` (`69`) including the null `\0` terminator.

If `TERMUX_APP__DATA_DIR` environment variable is not set or value is not valid, then the build value set in [`properties.sh`] file for the app data directory with which `termux-exec` package is compiled with is used, which defaults to `/data/data/@TERMUX_APP__PACKAGE_NAME@`.

The `TERMUX_APP__DATA_DIR___MAX_LEN` is defined in [`TermuxFile.h`](https://github.com/termux/termux-core-package/blob/v0.4.0/lib/termux-core_nos_c/tre/include/termux/termux_core__nos__c/v1/termux/file/TermuxFile.h) and it is the internal buffer length used by `termux-exec` for storing Termux app data directory path and the buffer lengths of all other Termux paths under the app data directory is based on it. The value `69` is the maximum value that will fit the requirement for a valid Android app data directory path, check [termux file path limits](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#file-path-limits) docs for more info. **Packages compiled for Termux must ensure that the `TERMUX_APP__DATA_DIR` value used during compilation is `< TERMUX_APP__DATA_DIR___MAX_LEN`.**

## &nbsp;

&nbsp;



### TERMUX_APP__LEGACY_DATA_DIR

The legacy [Termux app data directory path](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#termux-private-app-data-directory) (`/data/data/<package_name>`) assigned by Android for all Termux app data, that contains the [Termux project directory](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#termux-project-directory) (`TERMUX__PROJECT_DIR`), and optionally the [Termux rootfs directory](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#termux-rootfs-directory) (`TERMUX__ROOTFS`). The value is automatically exported by the Termux app for app version `>= 0.119.0`.

**Type:** `string`

**Commits:** [`8b5f4d9a`](https://github.com/termux/termux-core-package/commit/8b5f4d9a)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec-package/releases/tag/v2.0.0)

**Default value:** `/data/data/com.termux` if Termux app is running under primary user `0`.

**Assigned values:**

- An absolute path with max length `TERMUX_APP__DATA_DIR___MAX_LEN` (`69`) including the null `\0` terminator.

If `TERMUX_APP__LEGACY_DATA_DIR` environment variable is not set or value is not valid, then the build value set in [`properties.sh`] file for the app data directory with which `termux-exec` package is compiled with is used, which defaults to `/data/data/@TERMUX_APP__PACKAGE_NAME@`. If the build value is a non-legacy path (`/data/user/<user_id>/<package_name>` or `/mnt/expand/<volume_uuid>/user/0/<package_name>`), then it is automatically converted to a legacy path.

The `TERMUX_APP__DATA_DIR___MAX_LEN` is defined in [`TermuxFile.h`](https://github.com/termux/termux-core-package/blob/v0.4.0/lib/termux-core_nos_c/tre/include/termux/termux_core__nos__c/v1/termux/file/TermuxFile.h) and it is the internal buffer length used by `termux-exec` for storing Termux app data directory path and the buffer lengths of all other Termux paths under the app data directory is based on it. The value `69` is the maximum value that will fit the requirement for a valid Android app data directory path, check [termux file path limits](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#file-path-limits) docs for more info. **Packages compiled for Termux must ensure that the `TERMUX_APP__DATA_DIR` value used during compilation is `< TERMUX_APP__DATA_DIR___MAX_LEN`.**

## &nbsp;

&nbsp;



### TERMUX__PREFIX

The [Termux prefix directory path](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#termux-prefix-directory) under or equal to the [Termux rootfs directory](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#termux-rootfs-directory) (`TERMUX__ROOTFS`) where all Termux packages data is installed. The value is automatically exported by the Termux app for app version `>= 0.119.0`.

**Type:** `string`

**Commits:** [`8b5f4d9a`](https://github.com/termux/termux-core-package/commit/8b5f4d9a)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec-package/releases/tag/v2.0.0)

**Default value:** `/data/data/com.termux/files/usr`

**Assigned values:**

- An absolute path with max length `TERMUX__PREFIX_DIR___MAX_LEN` (`90`) including the null `\0` terminator.

If `TERMUX__PREFIX` environment variable is not set or value is not valid, then the build value set in [`properties.sh`] file for the app data directory with which `termux-exec` package is compiled with is used, which defaults to `/data/data/@TERMUX_APP__PACKAGE_NAME@/files/usr`. If the build value is not accessible `termux-exec` intercepts will return with an error and `errno` should be set that is set by the [`access()`](https://man7.org/linux/man-pages/man2/access.2.html) call.

The `TERMUX__PREFIX_DIR___MAX_LEN` is defined in [`TermuxFile.h`](https://github.com/termux/termux-core-package/blob/v0.4.0/lib/termux-core_nos_c/tre/include/termux/termux_core__nos__c/v1/termux/file/TermuxFile.h) and it is the internal buffer length used by `termux-exec` for storing Termux prefix directory path. The value `90` is the maximum value that will fit the requirement for a valid Termux prefix directory path, check [termux file path limits](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout#file-path-limits) docs for more info. **Packages compiled for Termux must ensure that the `TERMUX__PREFIX` value used during compilation is `< TERMUX__PREFIX_DIR___MAX_LEN`.**

## &nbsp;

&nbsp;



### TERMUX__SE_PROCESS_CONTEXT

The SeLinux process context of the Termux app process and its child processes. The value is automatically exported by the Termux app for app version `>= 0.119.0`.

This is used while deciding whether to use `system_linker_exec` if [`TERMUX_EXEC__SYSTEM_LINKER_EXEC__MODE`](#termux_exec__system_linker_exec__mode) is set to `enabled`.

**Type:** `string`

**Commits:** [`8b5f4d9a`](https://github.com/termux/termux-core-package/commit/8b5f4d9a)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec-package/releases/tag/v2.0.0)

**Default value:** `u:r:untrusted_app_27:s0:cXXX,cXXX,c512,c768` if Termux app is running under primary user `0` and using [`targetSdkVersion`](https://developer.android.com/guide/topics/manifest/uses-sdk-element#target) `= 28` where `XXX` would be for the app uid for the [categories](https://github.com/agnostic-apollo/Android-Docs/blob/master/site/pages/en/projects/docs/os/selinux/security-context.md#categories) component.

**Assigned values:**

- A valid Android SeLinux process context that matches `REGEX__PROCESS_CONTEXT`:
`^u:r:[^\n\t\r :]+:s0(:c[0-9]+,c[0-9]+(,c[0-9]+,c[0-9]+)?)?$`

If `TERMUX__SE_PROCESS_CONTEXT` is not set or value is not valid, then the process context is read from the `/proc/self/attr/current` file.

The `REGEX__PROCESS_CONTEXT` is defined in [`SelinuxUtils.h`](https://github.com/termux/termux-core-package/blob/v0.4.0/lib/termux-core_nos_c/tre/include/termux/termux_core__nos__c/v1/unix/os/selinux/SelinuxUtils.h), check its field docs for more info on the format of the an Android SeLinux process context.

**See Also:**

- [Security Context](https://github.com/agnostic-apollo/Android-Docs/blob/master/site/pages/en/projects/docs/os/selinux/security-context.md)
- [`untrusted_app` process context type](https://github.com/agnostic-apollo/Android-Docs/blob/master/site/pages/en/projects/docs/os/selinux/context-types.md#untrusted_app)

## &nbsp;

&nbsp;



### TERMUX_EXEC__LOG_LEVEL

The log level for `termux-exec`.

Normally, `termux-exec` does not log anything at log level `1` (`NORMAL`) for intercepts even and will require setting log level to `>= 2` (`DEBUG`) to see log messages.

**Type:** `int`

**Commits:** [`1fac1073`](https://github.com/termux/termux-exec-package/commit/1fac1073)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec-package/releases/tag/v2.0.0)

**Default value:** `1`

**Supported values:**

- `0` (`OFF`) - Log nothing.

- `1` (`NORMAL`) - Log error, warn and info messages and stacktraces.

- `2` (`DEBUG`) - Log debug messages.

- `3` (`VERBOSE`) - Log verbose messages.

- `4` (`VVERBOSE`) - Log very verbose messages.

- `5` (`VVVERBOSE`) - Log very very verbose messages.

## &nbsp;

&nbsp;



### TERMUX_EXEC__EXECVE_CALL__INTERCEPT

Whether `termux-exec` should intercept `execve()` wrapper calls [declared in `unistd.h`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/include/unistd.h;l=95) and [implemented by `exec.cpp`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/bionic/exec.cpp;l=187) in android [`bionic`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/README.md) `libc` library around the [`execve()`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/SYSCALLS.TXT;l=68) system call listed in [`syscalls(2)`](https://man7.org/linux/man-pages/man2/syscalls.2.html) provided by the [linux kernel](https://cs.android.com/android/kernel/superproject/+/ebe69964:common/include/linux/syscalls.h;l=790).

If enabled, then Termux specific logic will run to solve the issues for exec-ing files in Termux that are listed in [`exec()` technical docs](../technical.md#exec) before calling `execve()` system call. If not enabled, then `execve()` system call will be called directly instead.

The other wrapper functions in the `exec()` family of functions declared in `unistd.h` are always intercepted to solve some other issues on older Android versions, check [`libc/bionic/exec.cpp`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/bionic/exec.cpp) git history.

**Type:** `string`

**Commits:** [`1fac1073`](https://github.com/termux/termux-exec-package/commit/1fac1073)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec-package/releases/tag/v2.0.0)

**Default value:** `enable`

**Supported values:**

- `disable` - Intercept `execve()` will be disabled.

- `enable` - Intercept `execve()` will be enabled.

## &nbsp;

&nbsp;



### TERMUX_EXEC__SYSTEM_LINKER_EXEC__MODE

Whether to use [System Linker Exec Solution](../technical.md#system-linker-exec-solution), like to bypass [App Data File Execute Restrictions](../technical.md#app-data-file-execute-restrictions).

**Type:** `string`

**Commits:** [`db738a11`](https://github.com/termux/termux-exec-package/commit/db738a11), [`89422f43`](https://github.com/termux/termux-exec-package/commit/89422f43), [`f7450d01`](https://github.com/termux/termux-exec-package/commit/f7450d01)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec-package/releases/tag/v2.0.0)

**Default value:** `enable`

**Supported values:**

- `disable` - The `system_linker_exec` will be disabled.

- `enable` - The `system_linker_exec` will be enabled but only if required.

- `force` - The `system_linker_exec` will be force enabled even if not required and effective user id does not equal root (`0`) and shell (`2000`).

- `force_all` - The `system_linker_exec` will be force enabled even if not required and is supported, regardless of effective user id.

This is implemented by `shouldEnableSystemLinkerExec()` function ([1](https://github.com/termux/termux-exec-package/blob/v2.6.0/lib/termux-exec_nos_c/tre/include/termux/termux_exec__nos__c/v1/termux/api/termux_exec/service/ld_preload/TermuxExecLDPreload.h#L20), [2](https://github.com/termux/termux-exec-package/blob/v2.6.0/lib/termux-exec_nos_c/tre/src/termux/api/termux_exec/service/ld_preload/TermuxExecLDPreload.c#L31)) and `shouldEnableSystemLinkerExecForFile()` function ([1](https://github.com/termux/termux-exec-package/blob/v2.6.0/lib/termux-exec_nos_c/tre/include/termux/termux_exec__nos__c/v1/termux/api/termux_exec/service/ld_preload/TermuxExecLDPreload.h#L61), [2](https://github.com/termux/termux-exec-package/blob/v2.6.0/lib/termux-exec_nos_c/tre/src/termux/api/termux_exec/service/ld_preload/TermuxExecLDPreload.c#L170)) in `TermuxExecLDPreload.h` and implemented by `TermuxExecLDPreload.c`, and called by [`ExecIntercept.c`](https://github.com/termux/termux-exec-package/blob/v2.6.0/lib/termux-exec_nos_c/tre/src/termux/api/termux_exec/service/ld_preload/direct/exec/ExecIntercept.c#L296).

If `disable` is set, then `system_linker_exec` will never be used and the default `direct` execution type will be used.

If `enable` is set, then `system_linker_exec` will only be used if:
- `system_linker_exec` is required to bypass [App Data File Execute Restrictions](../technical.md#app-data-file-execute-restrictions), i.e device is running on Android `>= 10`.
- Effective user does not equal root (`0`) and shell (`2000`) user (used for [`adb`](https://developer.android.com/tools/adb)), as exec restrictions do not apply for them.
- [`TERMUX__SE_PROCESS_CONTEXT`](#TERMUX__SE_PROCESS_CONTEXT) does not start with `PROCESS_CONTEXT_PREFIX__UNTRUSTED_APP_25` (`u:r:untrusted_app_25:`), `PROCESS_CONTEXT_PREFIX__UNTRUSTED_APP_27` (`u:r:untrusted_app_27:`) or `PROCESS_CONTEXT_PREFIX__RUNAS_APP` (`u:r:runas_app:`), and does not equal `PROCESS_CONTEXT__AOSP_SU` (`u:r:su:s0`), `PROCESS_CONTEXT__KERNEL_SU` (`u:r:ksu:s0`), `PROCESS_CONTEXT__MAGISK_SU` (`u:r:magisk:s0`) or `PROCESS_CONTEXT__SHELL` (`u:r:shell:s0`), for which restrictions are exempted. For more info on them, check [`SelinuxUtils.h`](https://github.com/termux/termux-core-package/blob/v0.4.0/lib/termux-core_nos_c/tre/include/termux/termux_core__nos__c/v1/unix/os/selinux/SelinuxUtils.h).
- Executable or interpreter path is under [`TERMUX_APP__DATA_DIR`] or [`TERMUX_APP__LEGACY_DATA_DIR`] directory.

If `force` is set, then `system_linker_exec` will only be used if:
- `system_linker_exec` is supported, i.e device is running on Android `>= 10`.
- Effective user does not equal root (`0`) and shell (`2000`) user (used for [`adb`](https://developer.android.com/tools/adb)), as exec restrictions do not apply for them.
- Executable or interpreter path is under [`TERMUX_APP__DATA_DIR`] or [`TERMUX_APP__LEGACY_DATA_DIR`] directory.
This can be used if running in an untrusted app with `targetSdkVersion` `<= 28`.

If `force_all` is set, then `system_linker_exec` will only be used if:
- `system_linker_exec` is supported, i.e device is running on Android `>= 10`.
- Effective user is not checked like it is for `force` mode and can equal root (`0`) and shell (`2000`) user.
- Executable or interpreter path is under [`TERMUX_APP__DATA_DIR`] or [`TERMUX_APP__LEGACY_DATA_DIR`] directory.
This can be used if running in an untrusted app with `targetSdkVersion` `<= 28`.

The executable or interpreter paths are checked under [`TERMUX_APP__DATA_DIR`]/[`TERMUX_APP__LEGACY_DATA_DIR`] instead of `TERMUX__ROOTFS` as files could be executed from `TERMUX__APPS_DIR` and `TERMUX__CACHE_DIR`, which are not under the Termux rootfs. Additionally, Termux rootfs may not exist under app data directory at all and could be under another directory under Android rootfs `/`, like if compiling packages for `shell` user for the `com.android.shell` package with the Termux rootfs under `/data/local/tmp` instead of `/data/data/com.android.shell` (and using `force` mode) or compiling packages for `/system` directory.

The Termux app should export `force` mode instead of `force_all` mode in `TERMUX_EXEC__SYSTEM_LINKER_EXEC__MODE` if using `targetSdkVersion` `> 28` so that system linker exec should be forcefully engaged. Exporting `force_all` is not recommended as users running root and shell commands will get a performance hit.

To get whether `system_linker_exec` is currently enabled in Termux based on current process environment if a Termux app data file were to be executed, run the `termux-exec-system-linker-exec is-enabled` command.
To get whether `system_linker_exec` should be enabled in Termux based on current process environment regardless of if a Termux app data file were to be executed, run the `termux-exec-system-linker-exec should-enable` command.

## &nbsp;

&nbsp;



### TERMUX_EXEC__TESTS__LOG_LEVEL

The log level for `termux-exec-tests`.

**Type:** `int`

**Commits:** [`ad67e020`](https://github.com/termux/termux-exec-package/commit/ad67e020)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec-package/releases/tag/v2.0.0)

**Default value:** `1`

**Supported values:**

- `0` (`OFF`) - Log nothing.

- `1` (`NORMAL`) - Log error, warn and info messages and stacktraces.

- `2` (`DEBUG`) - Log debug messages.

- `3` (`VERBOSE`) - Log verbose messages.

- `4` (`VVERBOSE`) - Log very verbose messages.

- `5` (`VVVERBOSE`) - Log very very verbose messages.

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

Note that if `termux-exec` is set in `LD_PRELOAD`, and it sets `TERMUX_EXEC__PROC_SELF_EXE` for the current process/shell, and then `LD_PRELOAD` is unset, then new processes after second nested `exec()` will get old and wrong value of `TERMUX_EXEC__PROC_SELF_EXE` belonging to the first nested process since `termux-exec` will not get called for the second nested process to set the updated value. The `termux-exec` will be called for the first nested process, because just unsetting `LD_PRELOAD` in current process will not unload the `termux-exec` library and it requires at least one nested `exec()`. The `termux-exec` library could unset `TERMUX_EXEC__PROC_SELF_EXE` if `LD_PRELOAD` isn't already set, but then if the first nested process is under [`TERMUX_APP__DATA_DIR`]/[`TERMUX_APP__LEGACY_DATA_DIR`], it will not have access to `TERMUX_EXEC__PROC_SELF_EXE` to read the actual value of the execution command. This would normally not be an issue if `LD_PRELOAD` being set to the `termux-exec` library is mandatory so that it can `system_linker_exec` commands if running with `targetSdkVersion` `>= 29` on an android `>= 10` device, as otherwise permission denied errors would trigger for any command under [`TERMUX_APP__DATA_DIR`]/[`TERMUX_APP__LEGACY_DATA_DIR`] anyways, unless user manually wraps second nested process with `/system/bin/linker64`. This will still be an issue if `system_linker_exec` is optional due to running with an older `targetSdkVersion` or on an older android device and `TERMUX_EXEC__SYSTEM_LINKER_EXEC__MODE` is set to `force`/`force_all`, since then `TERMUX_EXEC__PROC_SELF_EXE` would get exported and will be used by termux packages.

**To prevent issues, if `LD_PRELOAD` is unset in current process, then `TERMUX_EXEC__PROC_SELF_EXE` must also be unset in the first nested process by the user themselves.** For example, running following will echo `<TERMUX__PREFIX>/bin/sh` value twice instead of `<TERMUX__PREFIX>/bin/sh` first and `<TERMUX__PREFIX>/bin/dash` second if `LD_PRELOAD` were to be set.

- `system_linker_exec` optional: `LD_PRELOAD= $TERMUX__PREFIX/bin/sh -c 'echo $TERMUX_EXEC__PROC_SELF_EXE'; $TERMUX__PREFIX/bin/dash -c "echo \$TERMUX_EXEC__PROC_SELF_EXE"'`
- `system_linker_exec` mandatory: `LD_PRELOAD= $TERMUX__PREFIX/bin/sh -c 'echo $TERMUX_EXEC__PROC_SELF_EXE'; /system/bin/linker64 $TERMUX__PREFIX/bin/dash -c "echo \$TERMUX_EXEC__PROC_SELF_EXE"'`

**See Also:**

- https://man7.org/linux/man-pages/man5/procfs.5.html

**Type:** `string`

**Commits:** [`db738a11`](https://github.com/termux/termux-exec-package/commit/db738a11)

**Version:** [`>= 2.0.0`](https://github.com/termux/termux-exec-package/releases/tag/v2.0.0)

**Assigned values:**

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

**See Also:**

- https://manpages.debian.org/testing/manpages/ld.so.8.en.html#LD_LIBRARY_PATH

**Type:** `string`

**Commits:** [`13831552`](https://github.com/termux/termux-exec-package/commit/13831552), [`8b5f4d9a`](https://github.com/termux/termux-core-package/commit/8b5f4d9a)

**Version:** [`>= 0.9`](https://github.com/termux/termux-exec-package/releases/tag/v0.9)

**Processing:**

- If `execve` is intercepted, then `LD_LIBRARY_PATH` will be unset if executing a non native ELF file (like 32-bit binary on a 64-bit host) and executable path starts with `/system/`, but does not equal `/system/bin/sh`, `system/bin/linker` or `/system/bin/linker64`.

## &nbsp;

&nbsp;



### LD_PRELOAD

The list of ELF shared object paths separated with colons ":" to be loaded before all others. This feature can be used to selectively override functions in other shared objects.

**See Also:**

- https://manpages.debian.org/testing/manpages/ld.so.8.en.html#LD_PRELOAD

**Type:** `string`

**Commits:** [`13831552`](https://github.com/termux/termux-exec-package/commit/13831552), [`8b5f4d9a`](https://github.com/termux/termux-core-package/commit/8b5f4d9a)

**Version:** [`>= 0.9`](https://github.com/termux/termux-exec-package/releases/tag/v0.9)

**Processing:**

- If `execve` is intercepted, then `LD_PRELOAD` will be unset if executing a non native ELF file (like 32-bit binary on a 64-bit host) and executable path starts with `/system/`, but does not equal `/system/bin/sh`, `system/bin/linker` or `/system/bin/linker64`.

## &nbsp;

&nbsp;

---

&nbsp;





[`properties.sh`]: https://github.com/termux/termux-packages/blob/master/scripts/properties.sh
[`TERMUX_APP__DATA_DIR`]: #termux_app__data_dir
[`TERMUX_APP__LEGACY_DATA_DIR`]: #termux_app__legacy_data_dir
