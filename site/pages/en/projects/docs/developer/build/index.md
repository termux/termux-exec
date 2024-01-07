---
page_ref: "@PROJECTS__VARIANT@/termux/termux-exec/docs/@DOCS__VERSION@/developer/build/index.md"
---

# Build termux-exec

<!-- @DOCS__HEADER_PLACEHOLDER@ -->

The [`termux-exec`](https://github.com/termux/termux-exec) can be built with the [`termux-packages`](https://github.com/termux/termux-packages) build infrastructure, check [Build environment](https://github.com/termux/termux-packages/wiki/Build-environment) and [Building packages](https://github.com/termux/termux-packages/wiki/Building-packages) docs for more info.

```shell
# Update following variables in packages/termux-exec/build.sh
TERMUX_PKG_SRCURL=file:///home/builder/termux-packages/sources/termux-exec.tar
TERMUX_PKG_SHA256=SKIP_CHECKSUM

# Clone/copy termux-exec directory at termux-packages/sources/termux-exec directory
git clone git@github.com:termux/termux-exec.git sources/termux-exec
# Switch to different (pull) branch from origin if required
(cd sources/termux-exec; git checkout <branch_name>)

# Run docker
./scripts/run-docker.sh

# Export build function in bash shell by pasting in terminal
build_local_package() {
local repo="$1"
local package="$2"
local force_build="$3"
shift 3
local source_file source_dir force_build_flags
source_file="$(grep -E '^TERMUX_PKG_SRCURL=' "$repo/$package/build.sh" | cut -d '=' -f2 | tail -n 1 | sed 's|^file://||')";
source_dir="${source_file%.tar}";
test -d "$source_dir" || { echo "Package source directory not found at \"$source_dir\""; return 1; } && \
rm -f "$source_file" && \
(cd "$(dirname "$source_dir")" && tar -cf "$source_file" --exclude=".git" "$(basename "$source_dir")") || return $?
[ "$force_build" = "1" ] && force_build_flags="-f" || force_build_flags=""
[ "$force_build" = "1" ] && { rm -rf "/home/builder/.termux-build/$package/" "/data/data/.built-packages/$package" || return $?; }
[ "$force_build" != "1" ] && { rm -rf "/data/data/.built-packages/$package" || return $?; }
./build-package.sh $force_build_flags "$@" "$package";
}

# Build
build_local_package packages termux-exec 1 -I
```

