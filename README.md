# termux-exec
A `execve()` wrapper to fix two problems with exec-ing files in Termux.

# Problem 1: Cannot execute files not part of the APK
Android 10 started blocking executing files under the app data directory, as
that is a [W^X](https://en.wikipedia.org/wiki/W%5EX) violation - files should be either
writeable or executable, but not both. Resources:

- [Google Android issue](https://issuetracker.google.com/issues/128554619)
- [Termux: No more exec from data folder on targetAPI >= Android Q](https://github.com/termux/termux-app/issues/1072)
- [Termux: Revisit the Android W^X problem](https://github.com/termux/termux-app/issues/2155)

While there is merit in that general principle, this prevents using Termux and Android
as a general computing device, where it should be possible for users to create executable
scripts and binaries.

# Solution 1: Cannot execute files not part of the APK
Create an `exec` interceptor using [LD_PRELOAD](https://en.wikipedia.org/wiki/DLL_injection#Approaches_on_Unix-like_systems),
that instead of executing an ELF file directly, executes `/system/bin/linker64 /path/to/elf`.
Explanation follows below.

On Linux, the kernel is normally responsible for loading both the executable and the
[dynamic linker](https://en.wikipedia.org/wiki/Dynamic_linker). The executable is invoked
by file path with the [execve system call](https://en.wikipedia.org/wiki/Exec_(system_call)).
The kernel loads the executable into the process, and looks for a `PT_INTERP` entry in
its [ELF program header table](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format#Program_header)
of the file - this specifies the path to the dynamic linker (`/system/bin/linker64` for 64-bit Android).

There is another way to load the two ELF objects:
[since 2018](https://android.googlesource.com/platform/bionic/+/8f639a40966c630c64166d2657da3ee641303194)
the dynamic linker can be invoked directly with `exec`.
If passed the filename of an executable, the dynamic linker will load and run the executable itself.
So, instead of executing `path/to/mybinary`, it's possible to execute
`/system/bin/linker64 /absolute/path/to/mybinary` (the linker needs an absolute path).

This is what `termux-exec` does to circumvent the block on executing files in the data
directory - the kernel sees only `/system/bin/linker64` being executed.

This also means that we need to extract [shebangs](https://en.wikipedia.org/wiki/Shebang_(Unix)). So for example, a call to execute:

```sh
./path/to/myscript.sh <args>
```

where the script has a `#!/path/to/interpreter` shebang, is replaced with:

```sh
/system/bin/linker64 /path/to/interpreter ./path/to/myscript.sh <args>
```

Implications:

- It's important that `LD_PRELOAD` is kept - see e.g. [this change in sshd](https://github.com/termux/termux-packages/pull/18069).
We could also consider patching this exec interception into the build process of termux packages, so `LD_PRELOAD` would not be necessary for packages built by the termux-packages repository.

- The executable will be `/system/bin/linker64`. So some programs that inspects the executable name (on itself or other programs) using `/proc/${PID}/exec` or `/proc/${PID}/comm` (where `$(PID}` could be `self`, for the current process) needs to be changed to instead inspect `argv0` of the process instead. See [this llvm driver change](https://github.com/termux/termux-packages/pull/18074) and [this pgrep/pkill change](https://github.com/termux/termux-packages/pull/18075).

- Statically linked binaries will not work. These are rare in Android and Termux, but zig currently produces statically linked binaries against musl libc.

- The interception using `LD_PRELOAD` will only work for programs using the [C library wrappers](https://linux.die.net/man/3/execve) for executing a new process, not when using the `execve` system call directly. Luckily most programs do use this. Programs using raw system calls needs to be patched or be run under [proot](https://wiki.termux.com/wiki/PRoot).

**NOTE**: The above example used `/system/bin/linker64` - on 32-bit systems, the corresponding
path is `/system/bin/linker`.

**NOTE**: While this circumvents the technical restriction, it still might be considered
violating [Google Play policy](https://support.google.com/googleplay/android-developer/answer/9888379).
So this workaround is not guaranteed to enable Play store distribution of Termux - but it's
worth an attempt, and regardless of Play store distribution, updating the targetSdk is necessary.

# Problem 2: Shebang paths
A lot of Linux software is written with the assumption that `/bin/sh`, `/usr/bin/env`
and similar file exists. This is not the case on Android where neither `/bin/` nor `/usr/`
exists.

When building packages for Termux those hard-coded assumptions are patched away - but this
does not help with installing scripts and programs from other sources than Termux packages.

# Solution 2: Shebang paths
Create an `execve()` wrapper that rewrites calls to execute files under `/bin/` and `/usr/bin`
into the matching Termux executables under `$PREFIX/bin/` and inject that into processes
using `LD_PRELOAD`.

# How to install
1. Install with `pkg install termux-exec`.
2. Exit your current session and start a new one.
3. From now on shebangs such as `/bin/sh` and `/usr/bin/env python` should work.

# Where is LD_PRELOAD set?
The `$PREFIX/bin/login` program which is used to create new Termux sessions checks for
`$PREFIX/lib/libtermux-exec.so` and if so sets up `LD_PRELOAD` before launching the login shell.

Soon, when making a switch to target Android 10+, this will be setup by the Termux app even before
launching any process, as `LD_PRELOAD` will be necessary for anything non-system to execute.
