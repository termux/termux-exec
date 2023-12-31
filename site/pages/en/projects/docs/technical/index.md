---
page_ref: "@PROJECTS__VARIANT@/termux/termux-exec/docs/@DOCS__VERSION@/technical/index.md"
---

# termux-exec Technical Docs

<!-- @DOCS__HEADER_PLACEHOLDER@ -->

The [`termux-exec`](https://github.com/termux/termux-exec) is a shared library that is meant to be preloaded with [`$LD_PRELOAD`](https://man7.org/linux/man-pages/man8/ld.so.8.html) for proper functioning of the Termux execution environment.

The following functions are overridden.

- [`exec()`](#exec)

---

&nbsp;





## `exec()`

The `exec()` family of functions are [declared in `unistd.h`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/include/unistd.h;l=92-100) and [implemented by `exec.cpp`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/bionic/exec.cpp) in android [`bionic`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/README.md) `libc` library. The `exec()` functions are wrappers around the [`execve()`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/SYSCALLS.TXT;l=68) system call listed in [`syscalls(2)`](https://man7.org/linux/man-pages/man2/syscalls.2.html) provided by the [linux kernel](https://cs.android.com/android/kernel/superproject/+/ebe69964:common/include/linux/syscalls.h;l=790), which can also be directly called with the [`syscall(2)`](https://man7.org/linux/man-pages/man2/syscall.2.html) library function [declared in `unistd.h`](https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:bionic/libc/include/unistd.h;l=308). Note that there is also a `execve()` wrapper in `unistd.h` around the `execve()` system call. The `termux-exec` overrides the entire `exec()` family of functions, but will not override direct calls to the `execve()` system call via `syscall(2)`, which is usually not done by programs.

##### See also

- [exec (system call) wiki](https://en.wikipedia.org/wiki/Exec_(system_call))
- [`unistd.h` POSIX spec](https://pubs.opengroup.org/onlinepubs/7908799/xsh/unistd.h.html)
- [`execve` POSIX spec](https://pubs.opengroup.org/onlinepubs/7908799/xsh/execve.html)
- [`unistd.h` POSIX spec](https://pubs.opengroup.org/onlinepubs/7908799/xsh/unistd.h.html)
- [`execve` POSIX spec](https://pubs.opengroup.org/onlinepubs/7908799/xsh/execve.html)
- [`execve(2)` linux man](https://man7.org/linux/man-pages/man2/execve.2.html)
- [`exec(3)` linux man](https://man7.org/linux/man-pages/man3/exec.3.html)

&nbsp;



The `termux-exec` overrides the `exec()` family of functions to solve the following issues when exec-ing files in Termux.

- [App Data File Execute Restrictions](#app-data-file-execute-restrictions)
- [Linux vs Termux `bin` paths](#linux-vs-termux-bin-paths)

&nbsp;



### App Data File Execute Restrictions

Android `>= 10` as part of `W^X` restrictions with the [`0dd738d8`](https://cs.android.com/android/_/android/platform/system/sepolicy/+/0dd738d810532eb41ad8d90520156212ce756648) commit via [SeLinux](https://source.android.com/docs/security/features/selinux) policies removed the `untrusted_app*` domains/process context type assigned to untrusted third party app processes that use `targetSdkVersion` `>= 29` to `exec()` their app data files that are assigned the `app_data_file` file context type, like under the `/data/data/<package_name>` (for user `0`) directory. Two backward compatibility domains were also added for which `exec()` was still allowed, the `untrusted_app_25` domain for apps that use `targetSdkVersion` `<= 25` and `untrusted_app_27` that use `targetSdkVersion` `26-28`. For all `untrusted_app*` domains, `dlopen()` on app data files is still allowed.

Check [`App Data File Execute Restrictions` android docs](https://github.com/agnostic-apollo/Android-Docs/blob/master/site/pages/en/projects/docs/apps/processes/app-data-file-execute-restrictions.md) for more information on the `W^X` restrictions, including that apply to other app domains.

While there is merit to prevent execution of untrusted code in writable storage as a general principle, this prevents using Termux and Android as a general purpose computing device, where it should be possible for users to download executable binaries and scripts from trusted locations or compile it locally and then execute it, after user explicitly grants the app the permission to do so with some kind of runtime/development permission provided by Android.

##### See also

- [`issuetracker#128554619`](https://issuetracker.google.com/issues/128554619)
- [`termux/termux-app#1072`: No more exec from data folder on targetAPI >= Android Q](https://github.com/termux/termux-app/issues/1072)
- [`termux/termux-app#2155`: Revisit the Android W^X problem](https://github.com/termux/termux-app/issues/2155)

&nbsp;

#### System Linker Exec Solution

Check [`System Linker Exec` android docs](https://github.com/agnostic-apollo/Android-Docs/blob/master/site/pages/en/projects/docs/apps/processes/app-data-file-execute-restrictions.md#system-linker-exec) for detailed info for the solution to bypass `W^X` restrictions.

The [dynamic linker](https://en.wikipedia.org/wiki/Dynamic_linker) is the part of an operating system that loads and links the shared libraries needed by an executable when it is executed. The kernel is normally responsible for loading both the executable and the dynamic linker. When a [`execve()` system call](https://en.wikipedia.org/wiki/Exec_(system_call)) is made for an [`ELF`](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format#Program_header) executable, the kernel loads the executable file, then reads the path to the dynamic linker from the `PT_INTERP` entry in [`ELF` program header table](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format#Program_header) and then attempts to load and execute this other executable binary for the dynamic linker, which then loads the initial executable image and all the dynamically-linked libraries on which it depends and starts the executable. For binaries built for Android, `PT_INTERP` specifies the path to `/system/bin/linker64` for 64-bit binaries and `/system/bin/linker` for 32-bit binaries. You can check `ELF` file headers with the [`readelf`](https://www.man7.org/linux/man-pages/man1/readelf.1.html) command, like `readelf --program-headers --dynamic /path/to/executable`.

The system provided `linker` at `/system/bin/linker64` on 64-bit Android and `/system/bin/linker` on 32-bit Android can also be passed an absolute path to an executable file on Android `>= 10` for it to execute, even if the executable file itself cannot be executed directly from the app data directory. Note that some 64-devices have 32-bit Android.

An ELF file at `/data/data/com.foo/executable` can be executed with:

```shell
/system/bin/linker64 /data/data/com.foo/executable [args]
```

A script file at `/data/data/com.foo/script.sh` that has the `#!/path/to/interpreter` [shebang](https://en.wikipedia.org/wiki/Shebang_(Unix)) can be executed with:

```sh
/system/bin/linker64 /path/to/interpreter /data/data/com.foo/script.sh [args]
```

This is possible because when a file is executed with the system linker, the kernel/SeLinux only sees the `linker` binary assigned the `system_linker_exec` file context type being executed by the app process and not the `*app_data_file` being executed by the linker.

Support in Android `linker` to execute files was added in Android `10`, so this method cannot be used on older Android versions, like for some system app domains. For `untrusted_app*` domains, this is not an issue since they can execute files directly on Android `< 10`.

- https://cs.android.com/android/_/android/platform/bionic/+/8f639a40966c630c64166d2657da3ee641303194
- https://cs.android.com/android/_/android/platform/bionic/+/refs/tags/android-10.0.0_r1:linker/linker_main.cpp

#### System Linker Exec Issues

1. The `$LD_PRELOAD` environmental variable must be exported and must contain the path to the `termux-exec` library before any app data file is executed, but not for android system binaries under `/system/bin`. It needs to be set for all entry points into termux, including when connecting to a `sshd` server, see [`termux/termux-packages#18069`](https://github.com/termux/termux-packages/pull/18069). We can also consider patching exec interception into the build process of termux packages, so `$LD_PRELOAD` would not be necessary for packages built by the `termux-packages` repository.

2. The executable file will be `/system/bin/linker64` and not the ELF file meant to be executed, so some programs that inspect the executable name with [`proc`](https://man7.org/linux/man-pages/man5/proc.5.html) files like `/proc/<pid>/exe` or `/proc/<pid>/comm` will see the wrong name. For packages that read `/proc/self/exe` file of their own process, the `termux-exec` exports the [`$TERMUX_EXEC__PROC_SELF_EXE`](../usage/index.md#termux_exec__proc_self_exe) environment variable with the absolute path to ELF file being executed so that such packages can be patched to read it first instead, however, this will not work if reading `exe` file of other processes as `TERMUX_EXEC__PROC_SELF_EXE` will only be set for the current process, and not others. Programs like `pgrep`/`pkill` that match process name from its command, will have to be patched to either do full matching (`-f`/`--full`) OR preferably as the linker binary is normally not executed,  so during matching to skip the first argument if its for the system linker and second arg is for an app data file under `TERMUX__ROOTFS` and `TERMUX_EXEC__PROC_SELF_EXE` is set for itself to indicate its running in a `system_linker_exec` environment. See also [`termux/termux-packages#18069`](https://github.com/termux/termux-packages/pull/18075).

3. Statically linked binaries will not work. These are rare in Android and Termux, but `zig` currently produces statically linked binaries against `musl` `libc`.

4. Packages that call the `execve()` system call directly will need to be patched to use one of the `exec()` family wrappers or they should be run under [`proot`](https://wiki.termux.com/wiki/PRoot).

5. This solution is not compliant with Google PlayStore policies as executing code downloaded (or compiled) at runtime from sources outside the Google Play Store, like not packed inside the apk is not allowed. However, if even apps using this like Termux are not approved to be uploaded to PlayStore, it at least allows using currently latest `targetSdkVersion` `= 34` to target Android `14`.

    > An app distributed via Google Play may not modify, replace, or update itself using any method other than Google Play's update mechanism. Likewise, an app may not download executable code (such as dex, JAR, .so files) from a source other than Google Play. This restriction does not apply to code that runs in a virtual machine or an interpreter where either provides indirect access to Android APIs (such as JavaScript in a webview or browser).
    > Apps or third-party code, like SDKs, with interpreted languages (JavaScript, Python, Lua, etc.) loaded at run time (for example, not packaged with the app) must not allow potential violations of Google Play policies.

    - https://support.google.com/googleplay/android-developer/answer/9888379?hl=en

## &nbsp;

&nbsp;



### Linux vs Termux `bin` paths

A lot of Linux software is written with the assumption that rootfs is at `/` and system binaries under the `/bin` or `/usr/bin` directories and the system binaries for script [shebangs](https://en.wikipedia.org/wiki/Shebang_(Unix)) interpreter paths exist at paths like `/bin/sh`, `/usr/bin/env`, etc.

However, on Android the `/bin` path does not exist on Android `<= 8.1`. On Android `>= 9`, it is a [symlink to `/system/bin` directory](https://cs.android.com/android/_/android/platform/system/core/+/refs/tags/android-9.0.0_r1:rootdir/init.rc;l=48) added via [`ff1ef9f2`](https://cs.android.com/android/_/android/platform/system/core/+/ff1ef9f2b10d98131ea8945c642dd8388d9b0250). The `/usr` path does not exist on any Android version.

But the `/system/bin` directory is for the path to android system binaries, not the ones provided by Termux. The rootfs for the linux environment provided by apps like Termux is under their app data directory instead, like under the `/data/data/<package_name>` (for user `0`) directory or `/data/user/<user_id>/<package_name>` for other secondary users. For packages compiled for the main Termux app, it is at `/data/data/com.termux/files` and its `bin` path is at `/data/data/com.termux/files/usr/bin`. Check [Termux filesystem layout](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout) for more info.

When packages are built for Termux with the [`termux-packages`](https://github.com/termux/termux-packages) build infrastructure, the harcoded paths for `/` rootfs in package sources are patched and replaced with the rootfs for Termux, but if `/bin/*` or `/usr/bin/*` paths are executed, like via the shell or if they are set in scripts or programs downloaded from outside the Termux packages repos or written by users themselves, they will either fail to execute with `No such file or directory` errors or will execute the android system binaries under `/system/bin/*` if the same filename exists, instead of executing binaries under the Termux `bin` path.

&nbsp;

#### Solution

The `termux-exec` if set in `$LD_PRELOAD`  environmental variable overrides the `exec()` family of functions so that if the executable path is under the `/bin/*` or `/usr/bin/*` directories or if a script that is being executed has the interpreter path set to a path under `/bin/*` or `/usr/bin/*` directories, then the `*/bin/` prefix in the path is replaced with the termux `$TERMUX__ROOTFS/usr/bin/` prefix, where `$TERMUX__ROOTFS` is the environmental variable exported by the Termux app. If `$TERMUX__ROOTFS` is not exported or is not a valid absolute path, then the default `TERMUX__ROOTFS` set by the `Makefile` during `termux-exec` build time is used instead as the Termux rootfs path.

---

&nbsp;
