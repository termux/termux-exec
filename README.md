# termux-exec
A `execve()` wrapper to fix problem with shebangs when running in Termux

# Problem
A lot of Linux software is written with the assumption that `/bin/sh`, `/usr/bin/env`
and similar file exists. This is not the case on Android where neither `/bin/` or `/usr/`
exists.

When building packages for Termux those hard-coded assumptions are patched away, but this
does not help with installing scripts and programs during runtime.

# Solution
Create an `execve()` wrapper that rewrites calls to execute files under `/bin/` and `/usr/bin`
into the matching Termux executables under `$PREFIX/bin/` and injecat that into processes
using `LD_PRELOAD`.

# How to install
1. Install with `pkg install termux-exec`.
2. Exit your current session and start a new one.
3. From now on shebangs such as `/bin/sh` and `/usr/bin/env python` should work.

# Where is LD_PRELOAD set?
The `$PREFIX/bin/login` program which is used to create new Termux sessions checks for
`$PREFIX/lib/libtermux-exec.so` and if so sets up `LD_PRELOAD` before launching the login shell.
